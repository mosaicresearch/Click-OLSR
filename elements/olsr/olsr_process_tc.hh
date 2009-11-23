//TED 140504: Created
/*
  =c
  Processes OLSR TC messages, storing information in the OLSRInterfaceInfoBase, triggering routing table update if necessary

  =s
  OLSRProcessTC(OLSRTopologyInfoBase Element, OLSRNeighborInfoBase Element,  OLSRRoutingTable Element)

  =io
  One input, two outputs, discarded messages are output on port 1, all other on port 0

  =processing
  PUSH

  =d
  Gets OLSR TC messages on input port. Packets are parsed, and information is stored in the OLSRTopologyInfoBase element given as argument. If the processing of the packet leads to a change in the OLSRTopologyInfoBase, a routing table update is triggered in the OLSRRoutingTable element.

  =a
  OLSRProcessMID, OLSRProcessHello, OLSRClassifier, OLSRForward

*/

#ifndef OLSR_PROCESS_TC_HH
#define OLSR_PROCESS_TC_HH

#include <click/element.hh>
#include "olsr_topology_infobase.hh"
#include "olsr_neighbor_infobase.hh"
#include "olsr_packethandle.hh"
#include "olsr_rtable.hh"
#include "olsr_interface_infobase.hh"
#include "click_olsr.hh"


CLICK_DECLS

class OLSRTopologyInfoBase;
class OLSRRoutingTable;
class OLSRNeighborInfoBase;
class OLSRInterfaceInfoBase;

class OLSRProcessTC : public Element {
public:

  OLSRProcessTC();
  ~OLSRProcessTC();

  const char *class_name() const   { return "OLSRProcessTC"; }
  const char *processing() const   { return PUSH; }
  OLSRProcessTC *clone() const  { return new OLSRProcessTC(); }
  const char *port_count() const  { return "1/2"; }
  
  int configure(Vector<String> &, ErrorHandler *);
  void push(int, Packet *);
  
private:
  
  OLSRTopologyInfoBase *_topologyInfo;
  OLSRNeighborInfoBase *_neighborInfo;
  OLSRRoutingTable *_routingTable;
  OLSRInterfaceInfoBase *_interfaceInfo;
  IPAddress _myMainIP;
};

CLICK_ENDDECLS
#endif
