//TED 20404: Created

#include <click/config.h>
#include <click/confparse.hh>
#include "olsr_interface_infobase.hh"
#include <click/ipaddress.hh>
//#include "ippair.hh"
#include "click_olsr.hh"

CLICK_DECLS

OLSRInterfaceInfoBase::OLSRInterfaceInfoBase()
  : _timer(this)
{
}


OLSRInterfaceInfoBase::~OLSRInterfaceInfoBase()
{
}


int
OLSRInterfaceInfoBase::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if ( cp_va_parse(conf, this, errh, 
		   cpElement, "Routing Table Element", &_routingTable,
		   cpElement, "local Interfaces Information Base", &_localIfInfoBase,
		   0) < 0 )
    return -1;
  return 0;
}


int
OLSRInterfaceInfoBase::initialize(ErrorHandler *)
{
  _interfaceSet = new InterfaceSet();	//ok new
  _timer.initialize(this);
  return 0;
}

void OLSRInterfaceInfoBase::uninitialize()
{
 delete _interfaceSet;
}

bool
OLSRInterfaceInfoBase::add_interface(IPAddress iface_addr, IPAddress main_addr, struct timeval time)
{
  struct interface_data *data;
  data = new struct interface_data;		//new memory released in remove

  data->I_iface_addr = iface_addr;
  data->I_main_addr = main_addr;
  data->I_time = time;

  if (_interfaceSet->empty()) {
    _timer.schedule_at(time);
  }
  
//    click_chatter("Inserted (%s, (iface addr = %s, main addr = %s, timeval=%u)) in interfaceSet", iface_addr.unparse().c_str(), iface_addr.unparse().c_str(), main_addr.unparse().c_str(), time.tv_sec);
  return ( _interfaceSet->insert(iface_addr, data) );
}


interface_data *
OLSRInterfaceInfoBase::find_interface(IPAddress iface_addr)
{
  if (! _interfaceSet->empty() ) {
    interface_data *data;
    data = (interface_data*) _interfaceSet->find(iface_addr);

    if (! data == 0 )
      return data;
  }
  return 0;
}


void
OLSRInterfaceInfoBase::remove_interface(IPAddress iface_addr)
{
	interface_data *ptr=(interface_data *) _interfaceSet->find (iface_addr);
  _interfaceSet->remove(iface_addr);
 delete ptr;
}

void
OLSRInterfaceInfoBase::remove_interfaces_from(IPAddress neigh_addr)
{
	click_chatter("Removing interfaces from neighbor %s", neigh_addr.unparse().c_str());
	bool interface_removed = false;
	
	if (!_interfaceSet->empty()) {
		IPAddress main_addr = get_main_address(neigh_addr);
		
		for (InterfaceSet::iterator iter = _interfaceSet->begin(); iter != _interfaceSet->end(); iter++) {
			interface_data *tuple = (interface_data *) iter.value();
			if (tuple->I_main_addr == main_addr) {
				remove_interface(tuple->I_iface_addr);
				interface_removed = true;
			}
		}
	}
	
	if (interface_removed) {
		_routingTable->compute_routing_table();
	}
}


IPAddress
OLSRInterfaceInfoBase::get_main_address(IPAddress iface_addr)
{
 //print_interfaces();
 
  if (! _interfaceSet->empty() ){
    interface_data *data;
    data = find_interface(iface_addr);
    if ( ! data == 0 ){
      return data->I_main_addr;
    }
  }
  if (_localIfInfoBase->get_index(iface_addr)>=0) return _localIfInfoBase->get_main_IPaddress();
  //could be node with just 1 interface
  return iface_addr;
}


bool
OLSRInterfaceInfoBase::update_interface(IPAddress iface_addr, struct timeval time){
  interface_data *data;
  data = find_interface(iface_addr);
  if ( ! data == 0 ) {
    data->I_time = time;
    return true;
  }
  return false;
}


HashMap<IPAddress, void*> *
OLSRInterfaceInfoBase::get_interface_set()
{
  return _interfaceSet;
}


void
OLSRInterfaceInfoBase::run_timer(Timer *)
{
  struct timeval now, next_timeout;
  bool interface_removed = false;
  click_gettimeofday(&now);
  next_timeout = make_timeval(0,0);
  
  //find expired interface tuples
  if (!_interfaceSet->empty()){
    for (InterfaceSet::iterator iter = _interfaceSet->begin(); iter != _interfaceSet->end(); iter++){
      interface_data *tuple = (interface_data *) iter.value();
//       if (tuple->I_time <= now) {
      if (tuple->I_time <= now) {
	remove_interface(tuple->I_iface_addr);
	interface_removed = true;
      }
    }
  }

  //find next interface to expire
  if (! _interfaceSet->empty()){
    for (InterfaceSet::iterator iter = _interfaceSet->begin(); iter != _interfaceSet->end(); iter++){
      interface_data *tuple = (interface_data *) iter.value();
      if (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0)
	next_timeout = tuple->I_time;
      if ( tuple->I_time < next_timeout )
	next_timeout = tuple->I_time;
    }
  }
  if (! (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0) )
    _timer.schedule_at(next_timeout);    //set timer
  if (interface_removed)
    _routingTable->compute_routing_table();
}

void OLSRInterfaceInfoBase::print_interfaces()
{
 click_chatter ("Interface Infobase: #%d\n",_interfaceSet->size());
 for (InterfaceSet::iterator iter = _interfaceSet->begin(); iter != _interfaceSet->end(); iter++){
      interface_data *tuple = (interface_data *) iter.value();
      click_chatter ("\tIface_addr=%s\tMain_addr=%s\t\n",tuple->I_iface_addr.unparse().c_str(),tuple->I_main_addr.unparse().c_str());
      }
 }
 


#include <click/bighashmap.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<IPPair, void *>;
#endif

CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRInterfaceInfoBase);

