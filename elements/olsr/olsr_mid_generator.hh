//TED 070504 :Created
/*
  =c
  OLSR specific element, generates OLSR MID messages

  =s
  OLSRMIDGenerator(INTERVAL(msecs), OLSRNeighborInfoBase element, IPAddress)
  
  =io
  One output

  =processing
  PUSH

  =d
  Generates a MID message every INTERVAL msecs if the node has been selected as MPR by another node. The message is based on the info in the node's MPR Selector Set stored in the OLSRNeighborInfobase element given as argument.

  =a
  OLSRHelloGenerator, OLSRForward
  
*/
#ifndef OLSR_MID_GENERATOR_HH
#define OLSR_MID_GENERATOR_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/ipaddress.hh>
#include "olsr_local_if_infobase.hh"
#include "click_olsr.hh"


CLICK_DECLS

class OLSRLocalIfInfoBase;

class OLSRMIDGenerator : public Element{
public:

  OLSRMIDGenerator();
  ~OLSRMIDGenerator();
  
  const char *class_name() const { return "OLSRMIDGenerator"; }  
  const char *processing() const { return PUSH; }
  OLSRMIDGenerator *clone() const {return new OLSRMIDGenerator(); }
  const char *port_count() const  	{ return "0/1"; }


  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);

  Packet *generate_mid();
  void run_timer(Timer *);

private:
  OLSRLocalIfInfoBase *_localIfInfoBase;
  int _period;
  uint8_t _vtime;
  Timer _timer;
  IPAddress _myIP;
  int _mid_hold_time;
  uint8_t compute_vtime();
};

CLICK_ENDDECLS
#endif
  
