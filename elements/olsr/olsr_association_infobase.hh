//TED 190404: Created

#ifndef OLSR_ASSOCIATION_INFOBASE_HH
#define OLSR_ASSOCIATION_INFOBASE_HH

#include <click/element.hh>
#include <click/ipaddress.hh>
#include <click/bighashmap.hh>
#include <click/timer.hh>
#include "ippair.hh"
#include "olsr_rtable.hh"
#include "click_olsr.hh"
#include "olsr_compact_association_info_base.hh"

CLICK_DECLS

class OLSRRoutingTable;

class OLSRAssociationInfoBase: public Element{
public:

  OLSRAssociationInfoBase();
  ~OLSRAssociationInfoBase();

  const char* class_name() const { return "OLSRAssociationInfoBase"; }
  OLSRAssociationInfoBase *clone() const { return new OLSRAssociationInfoBase(); }
  const char *port_count() const  { return "0/0"; }

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void uninitialize();

  struct association_data *add_tuple(IPAddress gateway_addr, IPAddress network_addr, IPAddress netmask, timeval time);
  struct association_data *find_tuple(IPAddress gateway_addr, IPAddress network_addr, IPAddress netmask);
  void remove_tuple(IPAddress gateway_addr, IPAddress network_addr, IPAddress netmask);
  HashMap<IPPair, void*> *get_association_set();
  Vector<IPPair> *get_associations();
  void redundancy_check();
  void print_association_set();
  void clear();

  
private:
  typedef HashMap <IPPair, void *> AssociationSet;

  AssociationSet *_associationSet;
  
  Vector<IPPair> *_associations; 
  Vector<IPPair> *_noRedundants; 
  
  OLSRCompactAssociationInfoBase *_compactSet;
  OLSRCompactAssociationInfoBase * _compactSet2;
   
  Timer _timer;
  OLSRRoutingTable *_routingTable;
  bool _useTimer;
  bool _redundancyCheck;
  bool _compact;
  
  IPAddress _home_network;
  IPAddress _home_netmask;
 
  void add_handlers();   
  static int set_home_network_write_handler(const String &conf, Element *e, void *, ErrorHandler * errh);

  void run_timer(Timer *);
};


CLICK_ENDDECLS
#endif
