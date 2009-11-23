//TED 190404: Created

#ifndef OLSR_RTABLE_HH
#define OLSR_RTABLE_HH

#include <click/element.hh>
#include <click/bighashmap.hh>
#include "olsr_link_infobase.hh"
#include "olsr_neighbor_infobase.hh"
#include "olsr_topology_infobase.hh"
#include "olsr_interface_infobase.hh"
#include "olsr_association_infobase.hh"
#include "click_olsr.hh"
#include "olsr_lineariplookup.hh"

#define logging

CLICK_DECLS

class OLSRLinkInfoBase;
class OLSRNeighborInfoBase;
class OLSRTopologyInfoBase;
class OLSRInterfaceInfoBase;
class OLSRAssociationInfoBase;


class OLSRRoutingTable: public Element {
public:

  OLSRRoutingTable();
  ~OLSRRoutingTable();

  const char *class_name() const       { return "OLSRRoutingTable"; }
  OLSRRoutingTable *clone() const      { return new OLSRRoutingTable; }
  const char *port_count() const       { return "0/0"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);
  int initialize(ErrorHandler *);
  void uninitialize();

  //IPAddress get_next_hop(const IPAddress& destination);
  //IPAddress get_output_interface(const IPAddress& destination);
  void print_routing_table();
  void compute_routing_table();

private:

  //typedef HashMap<IPAddress, void *> RTable;
  //class RTable *_routingTable;
  IPAddress _myIP;
  IPAddress _myMask;
  OLSRLinkInfoBase *_linkInfo;
  OLSRNeighborInfoBase *_neighborInfo;
  OLSRInterfaceInfoBase *_interfaceInfo;
  OLSRTopologyInfoBase *_topologyInfo;
  OLSRLocalIfInfoBase *_localIfaces;
  OLSRAssociationInfoBase *_associationInfo;
  OLSRLinearIPLookup *_linearIPlookup;
  OLSRAssociationInfoBase *_visitorInfo;
  ErrorHandler *_errh;

};


CLICK_ENDDECLS

#endif
