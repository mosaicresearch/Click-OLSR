//TED 070504 :Created
/*
  =c
  OLSR specific element

  =s
  OLSRReceiveChecker(INTERVAL(msecs), OLSRNeighborInfoBase element, IPAddress)
  
  =io
  One output

  =processing
  PUSH

  =d
  @TODO

  =a
  OLSRHelloGenerator, OLSRForward
  
*/
#ifndef OLSR_RECEIVE_CHECKER_HH
#define OLSR_RECEIVE_CHECKER_HH


#include <click/element.hh>
#include <click/timer.hh>
#include <click/ipaddress.hh>
#include "olsr_neighbor_infobase.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRReceiveChecker : public Element {
public:

  OLSRReceiveChecker();
  ~OLSRReceiveChecker();
  
  const char *class_name() const { return "OLSRReceiveChecker"; }  
  const char *processing() const { return PUSH; }
  OLSRReceiveChecker *clone() const {return new OLSRReceiveChecker(); }
  const char *port_count() const  { return "1/1"; }


  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);

  void run_timer(Timer *);
  void push(int port, Packet *packet);

private:
  Timer _timer;
  OLSRNeighborInfoBase *_neighborInfo;
  int _period;
  IPAddress _myIP;
  
};

CLICK_ENDDECLS
#endif
  
