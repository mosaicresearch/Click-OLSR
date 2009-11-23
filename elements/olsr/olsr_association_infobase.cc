//TED 220404: Created
#include <click/config.h>
#include <click/confparse.hh>
#include "olsr_association_infobase.hh"
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include "ippair.hh"
#include "click_olsr.hh"

CLICK_DECLS

OLSRAssociationInfoBase::OLSRAssociationInfoBase()
  : _timer(this), _useTimer(true), _redundancyCheck(false), _compact(false)
{
}


OLSRAssociationInfoBase::~OLSRAssociationInfoBase()
{
}


int
OLSRAssociationInfoBase::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if ( cp_va_parse( conf, this, errh,
		    cpElement, "Routing Table Element", &_routingTable,
		    cpKeywords,
		    "USE_TIMER", cpBool, "use a timer", &_useTimer,
			"REDUNDANCY_CHECK", cpBool, "remove redundant association entries", &_redundancyCheck,
			"HNA_COMPACTING", cpBool, "compact HNA messages", &_compact,
			"HOME_NETWORK", cpIPAddress, "home network", &_home_network,
			"HOME_NETWORK_NETMASK", cpIPAddress, "home network netmask", &_home_netmask,
		    0) < 0 )
    return -1;
	


/*  redundancy_check();
  	
    OLSRCompactAssociationInfoBase compact;
	compact.add(IPAddress("10.0.0.0"), IPAddress("255.255.255.255"));
	compact.print_compact_set();	
	compact.add(IPAddress("10.0.0.1"), IPAddress("255.255.255.255"));
	compact.print_compact_set();	
	compact.add(IPAddress("10.0.0.2"), IPAddress("255.255.255.255"));
	compact.print_compact_set();	
	compact.add(IPAddress("10.0.0.3"), IPAddress("255.255.255.255"));
	compact.print_compact_set();	
//	compact.add(IPAddress("10.0.0.1"), IPAddress("255.255.255.255"));
//	compact.print_compact_set();
//	compact.remove(IPAddress("10.1.0.0"), IPAddress("255.255.255.0"));	
//	compact.print_compact_set();	
//	compact.remove(IPAddress("10.0.0.1"), IPAddress("255.255.255.255"));	
//	compact.print_compact_set();
	compact.remove(IPAddress("10.0.0.1"), IPAddress("255.255.255.255"));
	compact.print_compact_set();
	compact.remove(IPAddress("10.0.0.2"), IPAddress("255.255.255.255"));
	compact.print_compact_set();
  exit(0);	*/	  
  return 0;
}


int
OLSRAssociationInfoBase::initialize(ErrorHandler *)
{
	if (_useTimer) _timer.initialize(this);
	_associationSet = new AssociationSet;
	_associations = new Vector<IPPair>;
	if (_redundancyCheck) _noRedundants = new Vector<IPPair>;
	if (_compact) {
		_compactSet = new OLSRCompactAssociationInfoBase;
		_compactSet2 = new OLSRCompactAssociationInfoBase;
	}
	return 0;
}

void OLSRAssociationInfoBase::uninitialize()
{
 delete _associationSet;
 if (_redundancyCheck) delete _noRedundants;
 if (_compact) {
 	delete _compactSet;
 	delete _compactSet2;
}
}

association_data *
OLSRAssociationInfoBase::add_tuple(IPAddress gateway_addr, IPAddress network_addr, IPAddress netmask, timeval time)
{
  IPPair ippair= IPPair(gateway_addr, network_addr, netmask);
  struct association_data *data;
  data = new struct association_data;		//released in remove

  data->A_gateway_addr = gateway_addr;
  data->A_network_addr = network_addr;
  data->A_netmask = netmask;
  data->A_time = time;
  
  if ( _associationSet->empty() ) {
    if (_useTimer) _timer.schedule_at(time);
  }
  if ( _associationSet->insert(ippair, data) ) {
  	if (_compact) {
		_compactSet->add(network_addr, netmask);
	}
    return data;
  }
  return 0;
}


association_data *
OLSRAssociationInfoBase::find_tuple(IPAddress gateway_addr, IPAddress network_addr, IPAddress netmask)
{
  if (! _associationSet->empty() ) {
    IPPair ippair = IPPair(gateway_addr, network_addr, netmask);
    struct HashMap<IPPair, void*>::Pair *pair;
    pair = _associationSet->find_pair(ippair);

    if (! pair == 0 ){
      association_data *data = (association_data *)pair->value;
      return data;
    }
  }

  return NULL;
}

void
OLSRAssociationInfoBase::remove_tuple(IPAddress gateway_addr, IPAddress network_addr, IPAddress netmask)
{

	IPPair ippair = IPPair(gateway_addr, network_addr, netmask);
	association_data *ptr=(association_data *)_associationSet->find(ippair);
	_associationSet->remove(ippair);
	delete ptr;
	
	if (_compact) {
		_compactSet->remove(network_addr, netmask);
	}  
}


HashMap<IPPair, void*> *
OLSRAssociationInfoBase::get_association_set()
{
  return _associationSet;
}

void
OLSRAssociationInfoBase::clear()
{
	for ( AssociationSet::iterator iter = _associationSet->begin(); iter != _associationSet->end(); iter++){
		IPPair *entry = (IPPair *) iter.value();
		delete entry;			//free memory of entries
	}
	delete _associationSet;
	_associationSet = new AssociationSet;
	if (_compactSet) _compactSet->get_compact_set()->clear();
}

void
OLSRAssociationInfoBase::run_timer(Timer *)
{
  struct timeval now, next_timeout;
  bool association_tuple_removed = false;
  click_gettimeofday(&now);
  next_timeout = make_timeval(0, 0);

  //find expired topology tuple and delete them
  if (! _associationSet->empty()){
    for (AssociationSet::iterator iter = _associationSet->begin(); iter != _associationSet->end(); iter++){
      association_data *tuple = (association_data *) iter.value();
      if (tuple->A_time <= now){
	remove_tuple(tuple->A_gateway_addr, tuple->A_network_addr, tuple->A_netmask);
	//click_chatter("Association tuple expired");
	association_tuple_removed = true;
      }
    }
  }

  //find next topology tuple to expire
  if (! _associationSet->empty()){
    for (AssociationSet::iterator iter = _associationSet->begin(); iter != _associationSet->end(); iter++){
      association_data *tuple = (association_data *) iter.value();
      if (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0)
	next_timeout = tuple->A_time;
      if ( tuple->A_time < next_timeout )
	next_timeout = tuple->A_time;
    }
  }

  if (! (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0) )
    _timer.schedule_at(next_timeout);    //set timer
  if (association_tuple_removed){
    //click_chatter("recomputing routing table");
    _routingTable->compute_routing_table();
    //_routingTable->print_routing_table();
  }
}


Vector<IPPair> *
OLSRAssociationInfoBase::get_associations()
{
	print_association_set();
	_associations->clear();
	for ( AssociationSet::iterator iter = _associationSet->begin(); iter != _associationSet->end(); iter++){
      association_data *entry = (association_data *) iter.value();
	  _associations->push_back(IPPair(entry->A_network_addr, entry->A_netmask));
    }	

 	if (_compact && _redundancyCheck) {
		click_chatter("%s | returning associations with redundant entries removed\n", name().c_str());
		_associations->push_back(IPPair(_home_network , _home_netmask));
		redundancy_check();
		_compactSet2->get_compact_set()->clear();		
		for (Vector<IPPair>::iterator iter = _noRedundants->begin(); iter != _noRedundants->end(); iter++) {
			_compactSet2->add(iter->_from, iter->_to);
		}
		return _compactSet2->get_compact_set();
	} else if (_compact) {
		_compactSet->print_compact_set();
		click_chatter("%s | returning compacted association entries\n", name().c_str());
		return _compactSet->get_compact_set();
	} else if (_redundancyCheck) {
		click_chatter("%s | returning associations with redundant entries removed\n", name().c_str());
		_associations->push_back(IPPair(_home_network , _home_netmask));
		redundancy_check();
		return _noRedundants;
	} else {
		click_chatter("%s | returning associations\n", name().c_str());
		return _associations;
	}
}

void
OLSRAssociationInfoBase::redundancy_check()
{
	Vector<IPPair> bucket[2];
	Vector<IPPair>::iterator previous;	
	
	_noRedundants->clear();
	
	// copy all IPPairs into a vector and perform a radix sort on the IPAddresses
	
	uint32_t bit = 0;
	for (uint32_t i = 0; i < 32; i++) {
		bit = 1 << i; 
		for (Vector<IPPair>::iterator iter = _associations->begin(); iter != _associations->end(); iter++) {
			uint32_t addr = htonl(iter->_from.addr());
			uint32_t res = (addr & bit) >> i;
			bucket[res].push_back(*iter);
		}
		_associations->clear();
		for (Vector<IPPair>::iterator iter = bucket[0].begin(); iter != bucket[0].end(); iter++) {
			_associations->push_back(*iter);
		}		
		bucket[0].clear();
		for (Vector<IPPair>::iterator iter = bucket[1].begin(); iter != bucket[1].end(); iter++) {
			_associations->push_back(*iter);
		}
		bucket[1].clear();
	}
	
	previous = 0;
	for (Vector<IPPair>::iterator iter = _associations->begin(); iter != _associations->end(); iter++) {
		if (!previous) {
			_noRedundants->push_back(*iter);
			previous = iter;
			continue;
		}
		if (iter->_from.matches_prefix(previous->_from, previous->_to)) continue;
		else {
			_noRedundants->push_back(*iter);
			previous = iter;
		}
	}
}

int
OLSRAssociationInfoBase::set_home_network_write_handler(const String &conf, Element *e, void *, ErrorHandler * errh)
{
  OLSRAssociationInfoBase* me = (OLSRAssociationInfoBase *) e;
  int res = cp_va_parse(conf, me, errh,
			cpIPPrefix, "the network address that HNA should advertise", &me->_home_network, &me->_home_netmask,
			0);
  if ( res < 0 )
    return res;  
  return 0;
}


void
OLSRAssociationInfoBase::add_handlers()
{
  this->add_write_handler("set_home_network", set_home_network_write_handler, (void *)0);
}

void
OLSRAssociationInfoBase::print_association_set()
{
  if (! _associationSet->empty() ){
    click_chatter("Association Set:\n");
    for ( AssociationSet::iterator iter = _associationSet->begin(); iter != _associationSet->end(); iter++){
      association_data *entry = (association_data *) iter.value();
      click_chatter("Gateway: %s\n", entry->A_gateway_addr.unparse().c_str());
      click_chatter("\tNetwork: %s\n", entry->A_network_addr.unparse().c_str());
      click_chatter("\tNetmask: %s\n", entry->A_netmask.unparse().c_str());
    }
  }
  else
    click_chatter("Association set: empty\n");
}

#include <click/vector.cc>
#include <click/bighashmap.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<IPPair, void *>;
template class Vector<IPPair>;
#endif

CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRAssociationInfoBase);

