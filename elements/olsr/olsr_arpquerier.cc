/*
 * arpquerier.{cc,hh} -- ARP resolver element
 * Robert Morris, Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2005 Regents of the University of California
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "olsr_arpquerier.hh"
#include <clicknet/ether.h>
#include <click/etheraddress.hh>
#include <click/ipaddress.hh>
#include <click/confparse.hh>
#include <click/bitvector.hh>
#include <click/router.hh>
#include <click/error.hh>
#include <click/glue.hh>
CLICK_DECLS

OLSRARPQuerier::OLSRARPQuerier()
    : _age_head(0), _age_tail(0), _expire_timer(expire_hook, this)
{
    // input 0: IP packets
    // input 1: ARP responses
    // output 0: ether/IP and ether/ARP queries
  for (int i = 0; i < NMAP; i++)
    _map[i] = 0;
}

OLSRARPQuerier::~OLSRARPQuerier()
{
}

int
OLSRARPQuerier::configure(Vector<String> &conf, ErrorHandler *errh)
{
    _capacity = 2048;
    _bcast_addr = IPAddress();
    IPAddress bcast_mask;
    bool confirm_bcast = false;
    if (cp_va_parse_remove_keywords(conf, 1, this, errh,
				    "CAPACITY", cpUnsigned, "packet capacity", &_capacity,
				    cpConfirmKeywords,
				    "BROADCAST", cpIPAddress, "IP local broadcast address", &confirm_bcast, &_bcast_addr,
				    cpEnd) < 0)
	return -1;
    if (conf.size() == 1)
	conf.push_back(conf[0]);
    if (cp_va_parse(conf, this, errh,
		    cpIPAddressOrPrefix, "IP address", &_my_ip, &bcast_mask,
		    cpEthernetAddress, "Ethernet address", &_my_en,
		    cpEnd) < 0)
	return -1;
    if (!_bcast_addr)
	_bcast_addr = _my_ip | ~bcast_mask;
    if (_bcast_addr == _my_ip)
	_bcast_addr = 0xFFFFFFFFU;
    return 0;
}

int
OLSRARPQuerier::live_reconfigure(Vector<String> &conf, ErrorHandler *errh)
{
    if (configure(conf, errh) < 0) {
	// if the configuration failed do nothing and return with a
	// failure indication
	return -1;
    }
  
    // if the new configuration succeeded then wipe out the old arp
    // table and reset the queries and pkts_killed counters
    clear_map();
    _arp_queries = 0;
    _drops = 0;
    _arp_responses = 0;
    return 0;
}

int
OLSRARPQuerier::initialize(ErrorHandler *)
{
  _expire_timer.initialize(this);
  _expire_timer.schedule_after_msec(EXPIRE_TIMEOUT_MS);
  _arp_queries = 0;
  _drops = 0;
  _arp_responses = 0;
  _cache_size = 0;
  return 0;
}

void
OLSRARPQuerier::cleanup(CleanupStage)
{
  clear_map();
}

void
OLSRARPQuerier::clear_map()
{
  // Walk the arp cache table and free 
  // any stored packets and arp entries.
  for (int i = 0; i < NMAP; i++) {
    for (ARPEntry *ae = _map[i]; ae; ) {
      ARPEntry *n = ae->next;
      while (Packet *p = ae->head) {
	  ae->head = p->next();
	  p->kill();
	  _drops++;
      }
      delete ae;
      ae = n;
    }
    _map[i] = 0;
  }
  _cache_size = 0;
}

void
OLSRARPQuerier::take_state(Element *e, ErrorHandler *errh)
{
    OLSRARPQuerier *arpq = (OLSRARPQuerier *)e->cast("OLSRARPQuerier");
    if (!arpq || _my_ip != arpq->_my_ip || _my_en != arpq->_my_en)
	return;
    if (_arp_queries > 0) {
	errh->error("late take_state");
	return;
    }

    memcpy(_map, arpq->_map, sizeof(ARPEntry *) * NMAP);
    memset(arpq->_map, 0, sizeof(ARPEntry *) * NMAP);

    _age_head = arpq->_age_head;
    _age_tail = arpq->_age_tail;
    _cache_size = arpq->_cache_size;
    _arp_queries = arpq->_arp_queries;
    _drops = arpq->_drops;
    _arp_responses = arpq->_arp_responses;

    // Need to change some pprev entries.
    for (int i = 0; i < NMAP; i++)
	if (_map[i])
	    _map[i]->pprev = &_map[i];
    if (_age_head)
	_age_head->age_pprev = &_age_head;
    
    arpq->_age_head = arpq->_age_tail = 0;
    arpq->_cache_size = 0;
}


void
OLSRARPQuerier::expire_hook(Timer *timer, void *thunk)
{
    // Expire any old entries, and make sure there's room for at least one
    // packet.
    OLSRARPQuerier *arpq = (OLSRARPQuerier *)thunk;
    arpq->_lock.acquire_write();
    int jiff = click_jiffies();
    ARPEntry *ae;

    // Delete old entries.
    while ((ae = arpq->_age_head) && (jiff - ae->last_response_jiffies) > 300*CLICK_HZ) {
	if ((*ae->pprev = ae->next))
	    ae->next->pprev = ae->pprev;

	if ((arpq->_age_head = ae->age_next))
	    arpq->_age_head->age_pprev = &arpq->_age_head;
	else
	    arpq->_age_tail = 0;
	
	while (Packet *p = ae->head) {
	    ae->head = p->next();
	    p->kill();
	    arpq->_cache_size--;
	    arpq->_drops++;
	}

	delete ae;
    }

    // Mark entries for polling, and delete packets to make space.
    while (ae) {
	// Only set polling on timer calls.
	if (jiff - ae->last_response_jiffies > 60*CLICK_HZ && timer)
	    ae->polling = 1;
	else if (arpq->_cache_size < arpq->_capacity)
	    break;
	while (arpq->_cache_size >= arpq->_capacity && ae->head) {
	    Packet *p = ae->head;
	    if (!(ae->head = p->next()))
		ae->tail = 0;
	    p->kill();
	    arpq->_cache_size--;
	    arpq->_drops++;
	}
	ae = ae->age_next;
    }

    if (timer)
	timer->schedule_after_msec(EXPIRE_TIMEOUT_MS);
    arpq->_lock.release_write();
}

void
OLSRARPQuerier::send_query_for(IPAddress want_ip)
{
  WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
  if (!q) {
    click_chatter("in arp querier: cannot make packet!");
    return;
  }
  memset(q->data(), '\0', q->length());
  
  click_ether *e = (click_ether *) q->data();
  q->set_ether_header(e);
  memcpy(e->ether_dhost, "\xff\xff\xff\xff\xff\xff", 6);
  memcpy(e->ether_shost, _my_en.data(), 6);
  e->ether_type = htons(ETHERTYPE_ARP);

  click_ether_arp *ea = (click_ether_arp *) (e + 1);
  ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
  ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
  ea->ea_hdr.ar_hln = 6;
  ea->ea_hdr.ar_pln = 4;
  ea->ea_hdr.ar_op = htons(ARPOP_REQUEST);
  memcpy(ea->arp_tpa, want_ip.data(), 4);
  memcpy(ea->arp_sha, _my_en.data(), 6);
  memcpy(ea->arp_spa, _my_ip.data(), 4);

  _arp_queries++;
  output(noutputs()-1).push(q);
}

/*
 * If the packet's IP address is in the table, add an ethernet header
 * and push it out.
 * Otherwise push out a query packet.
 * May save the packet in the ARP table for later sending.
 * May call p->kill().
 */
void
OLSRARPQuerier::handle_ip(Packet *p)
{
    // delete packet if we are not configured
    if (!_my_ip) {
	p->kill();
	_drops++;
	return;
    }

    IPAddress ipa = p->dst_ip_anno();
    int bucket = ip_bucket(ipa);
    ARPEntry *ae;

    // Easy case: requires only read lock
  retry_read_lock:
    _lock.acquire_read();
    ae = _map[bucket];
    while (ae && ae->ip != ipa)
	ae = ae->next;
    if (ae && ae->ok) {
	int was_polling = ae->polling;
	ae->polling = 0;
	if (WritablePacket *q = p->push_mac_header(sizeof(click_ether))) {
	    click_ether *e = q->ether_header();
	    memcpy(e->ether_shost, _my_en.data(), 6);
	    memcpy(e->ether_dhost, ae->en.data(), 6);
	    e->ether_type = htons(ETHERTYPE_IP);
	    _lock.release_read();
	    output(0).push(q);
	} else {
	    _drops++;
	    _lock.release_read();
	}
	if (was_polling)
	    send_query_for(ipa);
	return;
    }
    _lock.release_read();

    // Check special IP addresses
    if (!ipa) {
	static bool zero_warned = false;  
	if (!zero_warned) {
	    click_chatter("%s: would query for 0.0.0.0; missing dest IP addr annotation?", declaration().c_str());
	    zero_warned = true;
	}
	_drops++;
	p->kill();
	return;
    } else if (ipa.addr() == 0xFFFFFFFFU || ipa == _bcast_addr) {
	if (WritablePacket *q = p->push_mac_header(sizeof(click_ether))) {
	    click_ether *e = q->ether_header();
	    memcpy(e->ether_shost, _my_en.data(), 6);
	    memset(e->ether_dhost, 0xFF, 6);
	    e->ether_type = htons(ETHERTYPE_IP);
	    output(0).push(q);
	} else
	    _drops++;
	return;
    }

    // Hard case: requires write lock
    // 18.May.2005 -- must expire BEFORE we grab the ae pointer!!
    // because expiring might in fact DELETE the ae pointer.
    if (_cache_size >= _capacity)	// get some space if necessary
	expire_hook(0, this);
    
    _lock.acquire_write();
    ae = _map[bucket];
    while (ae && ae->ip != ipa)
	ae = ae->next;
    if (ae && ae->ok) {
	_lock.release_write();
	goto retry_read_lock;
    } else if (ae) {
	if (ae->tail)
	    ae->tail->set_next(p);
	else
	    ae->head = p;
	ae->tail = p;
	p->set_next(0);
	_cache_size++;
    } else if ((ae = new ARPEntry)) {
	ae->ip = ipa;
	ae->ok = ae->polling = 0;
	ae->last_response_jiffies = click_jiffies() - CLICK_HZ;
	
	ae->head = ae->tail = p;
	p->set_next(0);
	
	ae->pprev = &_map[bucket];
	if ((ae->next = _map[bucket]))
	    ae->next->pprev = &ae->next;
	_map[bucket] = ae;
	
	if (_age_tail)
	    ae->age_pprev = &_age_tail->age_next;
	else
	    ae->age_pprev = &_age_head;
	_age_tail = *ae->age_pprev = ae;
	ae->age_next = 0;
	
	_cache_size++;
    } else {
	p->kill();
	_drops++;
	_lock.release_write();
	return;
    }
    
    // Send a query for any given address at most 10 times a second.
    int jiff = click_jiffies();
    if ((int) (jiff - ae->last_response_jiffies) >= CLICK_HZ / 10) {
	ae->last_response_jiffies = jiff;
	_lock.release_write();
	send_query_for(ipa);
    } else
	_lock.release_write();
}

/*
 * Got an ARP response.
 * Update our ARP table.
 * If there was a packet waiting to be sent, return it.
 */
void
OLSRARPQuerier::handle_response(Packet *p)
{
  if (p->length() < sizeof(click_ether) + sizeof(click_ether_arp))
    return;

  _arp_responses++;
  
  click_ether *ethh = (click_ether *) p->data();
  click_ether_arp *arph = (click_ether_arp *) (ethh + 1);
  IPAddress ipa = IPAddress(arph->arp_spa);
  EtherAddress ena = EtherAddress(arph->arp_sha);
  if (ntohs(ethh->ether_type) == ETHERTYPE_ARP
      && ntohs(arph->ea_hdr.ar_hrd) == ARPHRD_ETHER
      && ntohs(arph->ea_hdr.ar_pro) == ETHERTYPE_IP
      && ntohs(arph->ea_hdr.ar_op) == ARPOP_REPLY
      && !ena.is_group()) {
    int bucket = ip_bucket(ipa);

    _lock.acquire_write();
    ARPEntry *ae = _map[bucket];
    while (ae && ae->ip != ipa)
      ae = ae->next;
    if (!ae) {
	// XXX would be nice to store an entry for this preemptive response
	_lock.release_write();
	return;
    }
    
    if (ae->ok && ae->en != ena)
	click_chatter("OLSRARPQuerier overwriting an entry");
    ae->en = ena;
    ae->ok = 1;
    ae->polling = 0;
    ae->last_response_jiffies = click_jiffies();
    Packet *cached_packet = ae->head;
    ae->head = ae->tail = 0;
    if (_age_tail != ae) {
	*ae->age_pprev = ae->age_next;
	ae->age_next->age_pprev = ae->age_pprev;
	ae->age_pprev = &_age_tail->age_next;
	ae->age_next = 0;
	_age_tail = *ae->age_pprev = ae;
    }
    _lock.release_write();

    // Send out packets in the order in which they arrived
    while (cached_packet) {
	Packet *next = cached_packet->next();
	handle_ip(cached_packet);
	cached_packet = next;
	_cache_size--;
    }
  }
}

void
OLSRARPQuerier::push(int port, Packet *p)
{
  if (port == 0)
    handle_ip(p);
  else {
    handle_response(p);
    p->kill();
  }
}

String
OLSRARPQuerier::read_table(Element *e, void *)
{
  OLSRARPQuerier *q = (OLSRARPQuerier *)e;
  String s;
  for (int i = 0; i < NMAP; i++)
    for (ARPEntry *e = q->_map[i]; e; e = e->next) {
      s += e->ip.s() + " " + (e->ok ? "1" : "0") + " " + e->en.s() + "\n";
    }
  return s;
}

String
OLSRARPQuerier::read_stats(Element *e, void *thunk)
{
  OLSRARPQuerier *q = (OLSRARPQuerier *)e;

  switch ((uintptr_t) thunk) {
    case 0:
      return
        String(q->_drops.value()) + " packets killed\n" +
        String(q->_arp_queries.value()) + " ARP queries sent\n";
    case 1:
      return String(q->_arp_queries.value());
    case 2:
      return String(q->_arp_responses.value());
    case 3:
      return String(q->_drops.value());
    default:
      return String();
  }
}

void
OLSRARPQuerier::add_handlers()
{
    add_read_handler("table", read_table, (void *)0);
    add_read_handler("stats", read_stats, (void *)0);
    add_read_handler("queries", read_stats, (void *)1);
    add_read_handler("responses", read_stats, (void *)2);
    add_read_handler("drops", read_stats, (void *)3);
}

/**
 * mvhaen -- addition for RARP lookup in OLSR
 */
IPAddress
OLSRARPQuerier::lookup_mac(const EtherAddress &ether)
{
	_lock.acquire_read();

	IPAddress ret_val("0.0.0.0");

	ARPEntry *ae = 0;
	for (int bucket=0; bucket < NMAP; bucket++)
	{
		ae = _map[bucket];
		while (ae && ae->en != ether)
		{
			ae = ae->next;
		}
		if (ae && ae->en == ether)
		{
			break;
		}
	}

	if (ae && ae->ok)
	{
		ret_val = ae->ip;
	}
	_lock.release_read();
	return ret_val;
}

/**
 * mvhaen -- addition for adding mac address. OLSR uses this to add 
 * IP - mac address tuples gathered from HELLO messages.
 */
void OLSRARPQuerier::insert_entry(const IPAddress &ip, const EtherAddress &ether)
{
	int bucket = ip_bucket(ip);
		
	_lock.acquire_read();

	ARPEntry *ae = _map[bucket];

	while (ae && ae->ip != ip)
		ae = ae->next;

	if (ae)
	{
		if (ae->ok && ae->en != ether)
			click_chatter("OLSRARPQuerier overwriting an entry");		
		ae->en = ether;
		ae->last_response_jiffies = click_jiffies();
		_lock.release_read();
	}
	else
	{
		_lock.release_read();
		_lock.acquire_write();
		if (_cache_size >= _capacity)
			expire_hook(0, this);
		if (ARPEntry *ae = new ARPEntry)
		{
			ae->ip = ip;
			ae->en = ether;
			ae->ok = 1;				// the entry is automatically ok, since we are setting the mac address
			ae->polling = 0;			// don't know what polling does ..
			ae->head = ae->tail = 0;		// no buffered packets
			ae->last_response_jiffies = click_jiffies();
			ae->pprev = &_map[bucket];
			if ((ae->next = _map[bucket]))
				ae->next->pprev = &ae->next;
			_map[bucket] = ae;

			if (_age_tail)
				ae->age_pprev = &_age_tail->age_next;
			else
				ae->age_pprev = &_age_head;
			_age_tail = *ae->age_pprev = ae;
			ae->age_next = 0;
		}
		_lock.release_write();
	}
}


CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRARPQuerier)
ELEMENT_MT_SAFE(OLSRARPQuerier)
