//TED 190404: Created

#ifndef OLSR_TOPOLOGY_INFOBASE_HH
#define OLSR_TOPOLOGY_INFOBASE_HH

#include <click/element.hh>
#include <click/ipaddress.hh>
#include <click/bighashmap.hh>
#include <click/timer.hh>
#include "ippair.hh"
#include "olsr_rtable.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRRoutingTable;

class OLSRTopologyInfoBase: public Element{
public:

  OLSRTopologyInfoBase();
  ~OLSRTopologyInfoBase();

  const char* class_name() const { return "OLSRTopologyInfoBase"; }
  OLSRTopologyInfoBase *clone() const { return new OLSRTopologyInfoBase(); } 
  const char *port_count() const { return "0/0"; }

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void uninitialize();

  struct topology_data *add_tuple(IPAddress dest_addr, IPAddress last_addr, timeval time);
  struct topology_data *find_tuple(IPAddress dest_addr, IPAddress last_addr);
  bool newer_tuple_exists(IPAddress last_addr, int ansn);
  bool remove_outdated_tuples(IPAddress last_addr, int ansn);
  void remove_tuple(IPAddress dest_addr, IPAddress last_addr);
  HashMap<IPPair, void*> *get_topology_set();
 void print_topology();
private:
  typedef HashMap <IPPair, void *> TopologySet;

  TopologySet *_topologySet;
  Timer _timer;
  OLSRRoutingTable *_routingTable;

  void run_timer(Timer *);
};

CLICK_ENDDECLS
#endif
