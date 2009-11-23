//TED 190404: Created

#ifndef OLSR_INTERFACE_INFOBASE_HH
#define OLSR_INTERFACE_INFOBASE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/ipaddress.hh>
#include <click/bighashmap.hh>
#include "olsr_rtable.hh"
#include "olsr_local_if_infobase.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRRoutingTable;
class OLSRLocalIfInfoBase;

class OLSRInterfaceInfoBase: public Element{
public:

  OLSRInterfaceInfoBase();
  ~OLSRInterfaceInfoBase();

  const char* class_name() const { return "OLSRInterfaceInfoBase"; }
  OLSRInterfaceInfoBase *clone() const { return new OLSRInterfaceInfoBase(); } 
  const char *port_count() const  { return "0/0"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);
  int initialize(ErrorHandler *);
  void uninitialize();

  bool add_interface(IPAddress iface_addr, IPAddress main_addr, struct timeval time);
  struct interface_data *find_interface(IPAddress iface_addr);
  void remove_interface(IPAddress iface_addr);
  void remove_interfaces_from(IPAddress neigh_addr);
  IPAddress get_main_address(IPAddress iface_addr);
  bool update_interface(IPAddress iface_addr, struct timeval time);
  HashMap<IPAddress, void*> *get_interface_set();
  void print_interfaces();

private:
  typedef HashMap <IPAddress, void *> InterfaceSet;

  InterfaceSet *_interfaceSet;
  OLSRLocalIfInfoBase *_localIfInfoBase;
  OLSRRoutingTable *_routingTable;
  Timer _timer;
  
  void run_timer(Timer *);

};


CLICK_ENDDECLS
#endif
