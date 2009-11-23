//TED 160404: Created
//Partially based on grid/linktable - implementation

#ifndef OLSR_LINK_INFOBASE_HH
#define OLSR_LINK_INFOBASE_HH 

#include <click/ipaddress.hh>
#include <click/timer.hh>
#include <click/element.hh>
#include <click/bighashmap.hh>
#include "olsr_neighbor_infobase.hh"
#include "olsr_interface_infobase.hh"
#include "olsr_duplicate_set.hh"
#include "olsr_tc_generator.hh"
#include "olsr_rtable.hh"
#include "ippair.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRRoutingTable;
class OLSRNeighborInfoBase;
class OLSRInterfaceInfoBase;
class OLSRDuplicateSet;
class OLSRTCGenerator;

class OLSRLinkInfoBase: public Element{
public:

  OLSRLinkInfoBase();
  ~OLSRLinkInfoBase();

  const char* class_name() const { return "OLSRLinkInfoBase"; }
  OLSRLinkInfoBase *clone() const { return new OLSRLinkInfoBase(); }
  const char *port_count() const  { return "0/0"; }


  int configure(Vector<String> &conf, ErrorHandler *errh);
  int initialize(ErrorHandler *);
  void uninitialize();

  struct link_data *add_link(IPAddress local_addr, IPAddress neigh_addr, struct timeval time);
  struct link_data *find_link(IPAddress local_addr, IPAddress neigh_addr);
  bool update_link(IPAddress local_addr, IPAddress neigh_addr, struct timeval sym_time, struct timeval asym_time, struct timeval time);
  void remove_link(IPAddress local_addr, IPAddress neigh_addr);
  HashMap<IPPair, void *> *get_link_set();
  void print_link_set();
  typedef HashMap<IPPair, void *> LinkSet;
  
private:
  
  LinkSet *_linkSet;

  OLSRNeighborInfoBase *_neighborInfo;
  OLSRInterfaceInfoBase *_interfaceInfo;
  OLSRRoutingTable *_routingTable;
  OLSRDuplicateSet *_duplicateSet;
  OLSRTCGenerator *_tcGenerator;
 

  Timer _timer;
  

  void run_timer(Timer *);

};

CLICK_ENDDECLS
#endif
