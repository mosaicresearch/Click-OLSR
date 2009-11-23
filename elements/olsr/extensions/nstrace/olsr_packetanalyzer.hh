//TED 300304: Created
#ifndef OLSR_PACKETANALYZER_HH
#define OLSR_PACKETANALYZER_HH

#include <click/element.hh>
#include "click_olsr.hh"
#include "../ns/simpacketanalyzer.hh"

CLICK_DECLS
/* =c
 * OLSR specific element, splits up OSLR packets and classifies the OLSR messages within 
 *
 * =s
 * OLSRPacketAnalyzer(OLSRDuplicateSet element, ip_address)
 *
 * =d
 * The OLSRPacketAnalyzer element takes OLSR packets on its input. The packets must
 * have their destination address annotation set, preferably with the source address
 * of the packet. The packets are split into their OLSR message components and sent 
 * to an output port according to their message type. Messages that are received out 
 * of order or are from the address in argument 2 are output on port 0. Messages 
 * already considered for forward, as indicated in the duplicate set element in 
 * argument 1, are also output on port 0.  
 *
 * =io
 * One input, five outputs
 * Output port 0: Discarded messages - received out of order, from the address given 
 * as argument 2 or already considered for forward (as indicated by duplicate set)
 * Output port 1: Hello messages
 * Output port 2: TC messages
 * Output port 3: MID messages
 * Output port 4: HNA messages
 * Output port 5: Messages with unknown message type
 *
 * =processing
 * PUSH
 *
 * =a
 * OLSRProcessHello, OLSRProcessTC, OLSRProcessMID, OLSRForward
 */

class OLSRPacketAnalyzer : public SimPacketAnalyzer {
public:
  OLSRPacketAnalyzer();
  ~OLSRPacketAnalyzer();

  const char *class_name() const  { return "OLSRPacketAnalyzer"; }
  const char *processing() const  { return PUSH; }
  OLSRPacketAnalyzer *clone() const   { return new OLSRPacketAnalyzer; }

  virtual String analyze(Packet*, int offset);

private:

};

CLICK_ENDDECLS

#endif
  
