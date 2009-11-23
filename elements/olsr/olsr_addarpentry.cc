// -*- c-basic-offset: 4 -*-
/*
 * setetherheader.{cc,hh} 
 * Robert Morris
 *
 * Copyright (c) 2005 University of Antwerp
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
#include "olsr_addarpentry.hh"
#include <click/etheraddress.hh>
#include <clicknet/ether.h>
#include <click/confparse.hh>

CLICK_DECLS

AddARPEntry::AddARPEntry()
{}

AddARPEntry::~AddARPEntry()
{}

int
AddARPEntry::configure(Vector<String> &conf, ErrorHandler *errh)
{
	return cp_va_parse(conf, this, errh,
	                   cpElement, "ARP Querier/Table", &_arpQuerier,
	                   cpEnd);
}

Packet *
AddARPEntry::simple_action(Packet *p)
{
	if (p->ether_header())
	{
		EtherAddress src;
		memcpy(src.data(), p->ether_header()->ether_shost, 6);
		_arpQuerier->insert_entry(p->dst_ip_anno(), src);

	}
	else
	{
		click_chatter("%s | %s | ethernet header not set\n", name().c_str(), __FUNCTION__);
	}
	return(p);
}

EXPORT_ELEMENT(AddARPEntry)

CLICK_ENDDECLS
