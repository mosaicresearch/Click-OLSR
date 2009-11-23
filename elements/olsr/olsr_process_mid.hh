//TED 260404: Created
/*
  =c
  Processes OLSR MID messages, storing information in the OLSRInterfaceInfoBase, triggering routing table update if necessary

  =s
  OLSRProcessMID(OLSRInterfaceInfoBase Element, OLSRRoutingTable Element)

  =io
  One input, two outputs, discarded messages are output on port 1, all other on port 0

  =processing
  PUSH

  =d
  Gets OLSR MID messages on input port. Packets are parsed, and information is stored in the OLSRInterfaceInfoBase element given as argument. If the processing of the packet leads to a change in the OLSRInterfacInfoBase, a routing table update is triggered in the OLSRRoutingTable element.

  =a
  OLSRProcessTC, OLSRProcessHello, OLSRClassifier, OLSRForward

*/

#ifndef OLSR_PROCESS_MID_HH
#define OLSR_PROCESS_MID_HH

#include <click/element.hh>
#include "olsr_interface_infobase.hh"
#include "olsr_packethandle.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRProcessMID : public Element {
public:

  OLSRProcessMID();
  ~OLSRProcessMID();

  const char *class_name() const   { return "OLSRProcessMID"; }
  const char *processing() const   { return PUSH; }
  OLSRProcessMID *clone() const  { return new OLSRProcessMID(); }
  const char *port_count() const  	{ return "1/2"; }

  int configure(Vector<String> &, ErrorHandler *);
  
  //  int initialize(ErrorHandler *);  
  //  void uninitialize();
  void push(int, Packet *);

private:

  OLSRInterfaceInfoBase *_interfaceInfo;
  OLSRRoutingTable *_routingTable;

};

CLICK_ENDDECLS
#endif
