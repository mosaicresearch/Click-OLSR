//TED 300304: Created
#ifndef OLSR_CLASSIFIER_HH
#define OLSR_CLASSIFIER_HH

#include <click/element.hh>
#include "click_olsr.hh"
#include "olsr_duplicate_set.hh"
#include "olsr_local_if_infobase.hh"

CLICK_DECLS
/* =c
 * OLSR specific element, splits up OSLR packets and classifies the OLSR messages within 
 *
 * =s
 * OLSRClassifier(OLSRDuplicateSet element, ip_address)
 *
 * =d
 * The OLSRClassifier element takes OLSR packets on its input. The packets must
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

class OLSRPacketHandle;

class OLSRClassifier : public Element{
public:
  OLSRClassifier();
  ~OLSRClassifier();

  const char *class_name() const  { return "OLSRClassifier"; }
  const char *processing() const  { return PUSH; }
  OLSRClassifier *clone() const   { return new OLSRClassifier; }
  const char *port_count() const  { return "1/6"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);
  int initialize(ErrorHandler *);
  void push(int, Packet*);

private:
  OLSRLocalIfInfoBase *_localIfInfoBase;
  OLSRDuplicateSet *_duplicateSet;
  IPAddress _myMainIP;
};

CLICK_ENDDECLS

#endif
  
