/*
  =c
  Processes OLSR Hello messages, storing information in the OLSRNeighborInfoBase, OLSRLinkInfoBase, triggering MPR computation and Routing Table update if necessary

  =s
  OLSRProcessHNA(OLSRLinkInfoBase element, OLSRNeighborInfoBase element, OLSRInterfaceInfoBase Element, OLSRRoutingTable Element, OLSRTCGenerator element, IPAddress)

  =io
  One input, one output

  =processing
  PUSH

  =d
  Gets OLSR Hello messages on input port. The incoming packets need to have their destionation address annotation set to the 1-hop source address of the message. Packets are parsed, and information is stored in the OLSRLinkInfoBase and OLSRNeighborInfoBase elements given as arguments. If the processing of the packet leads to the adding of an MPR Selector in the OLSRNeighborInfoBase element, the Advertise Neighbor Sequence Number (ANSN) in the OLSRTCGenerator element is updated. If a neighbor or 2-hop neighbor node is added in the OLSRNeighborInfoBase element, an MPR calculation is triggered in the OLSRNeighborInfobase element, and a routing table update is triggered in the OLSRRoutingTable element.

  =a
  OLSRProcessTC, OLSRProcessMID, OLSRClassifier, OLSRForward

*/
#ifndef OLSR_PROCESS_HNA_HH
#define OLSR_PROCESS_HNA_HH

#include <click/element.hh>
#include "olsr_association_infobase.hh"
#include "olsr_rtable.hh"
#include "olsr_packethandle.hh"
#include "click_olsr.hh"


CLICK_DECLS

/*
=c
OLSRProcessHNA

=s
Processes OLSR Hello messages

=io
one input, one output

=d
Processes OSLR Hello messages, updating appropriate datastructures

=a

*/

class OLSRTCGenerator;
class OLSRRoutingTable;
class OLSRLinkInfoBase;
class OLSRNeighborInfoBase;
class OLSRInterfaceInfoBase;

class OLSRProcessHNA : public Element {
public:

  OLSRProcessHNA();
  ~OLSRProcessHNA();

  const char *class_name() const   { return "OLSRProcessHNA"; }
  const char *processing() const   { return PUSH; }
  OLSRProcessHNA *clone() const  { return new OLSRProcessHNA(); }  	
  const char *port_count() const  	{ return "1/2"; }
  //Packet *simple_action(Packet *);

  int configure(Vector<String> &, ErrorHandler *);

  //  int initialize(ErrorHandler *);
  //  void uninitialize();
  void push(int, Packet *);

private:
  OLSRAssociationInfoBase *_associationInfo;
  OLSRNeighborInfoBase *_neighborInfo;
  OLSRRoutingTable *_routingTable;
  IPAddress _my_ip;
};

CLICK_ENDDECLS
#endif
