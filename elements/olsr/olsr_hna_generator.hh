
//TED 070504 :Created
/*
  =c
  OLSR specific element, generates OLSR HNA messages

  =s
  OLSRHNAGenerator(INTERVAL(msecs), Holding Time (msec), IPAddress, NETWORK IPAddress/netmask, ASSOCIATION_INFO AssociationInfoBase Element)
  
  =io
  One output

  =processing
  PUSH

  =d
  Generates a HNA message every INTERVAL msecs if there are associations to be advertised. Using the click script one can configure 1 network address/netmask pair that should be advertised. In this case NETWORK should be set. One can also configure ASSOCIATION_INFO. In this case the HNA generator will advertise all the entries that are in the AssociationInfoBase, meaning that the list of advertised associations is dynamic.
  Additional associations can be added from an ns2 TCL script using the writehandler add_association. (up to now these associations can not be removed).
    [$node_($i) set classifier_] writehandler "hna_generator_name" "add_association" "143.129.73.0/24"
  "hna_generator_name" should be replaced with the name of the OLSRHNAGenerator Element in the click script.
  MODE 5 is similar to mode 4, but it requires ASSOCIATION_INFO to be set.
  @NOTE: the interface for configuring hna_generator could be improved upon: instead of having 1 pair, a list of pairs to be advertised should be offered. A possibility is also to allow associations to be removed dynamically.

  =a
  OLSRHelloGenerator, OLSRForward

*/

#ifndef OLSR_HNA_GENERATOR_HH
#define OLSR_HNA_GENERATOR_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/ipaddress.hh>
#include "olsr_association_infobase.hh"
#include "click_olsr.hh"
#include <click/vector.hh>

CLICK_DECLS

class OLSRNeighborInfoBase;

#define HNA_MODE_FIXED			0
#define HNA_MODE_ASSOCIATION		1
#define HNA_MODE_CONFIG			2
#define HNA_MODE_SIM			3
#define HNA_MODE_SIM_AND_ASSOCIATION	4

class OLSRHNAGenerator : public Element{
public:

  OLSRHNAGenerator();
  ~OLSRHNAGenerator();
  
  const char *class_name() const { return "OLSRHNAGenerator"; }
  const char *processing() const { return PUSH; }
  OLSRHNAGenerator *clone() const {return new OLSRHNAGenerator(); }
  const char *port_count() const  { return "0/1"; }

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);

  void generate_hna();
  void run_timer(Timer *);

private:
  struct _Association {
  	IPAddress network_addr;
  	IPAddress netmask;
  };  
  
  int _period;
  uint8_t _vtime;
  struct timeval _end_of_validity_time;
  Timer _timer;
  OLSRAssociationInfoBase *_association_info;
  IPAddress _my_ip;

  Vector<_Association> _fixedAssociations;
  
  timeval _last_msg_sent_at;
  int _hna_hold_time;

  uint8_t compute_vtime();
  void add_handlers();
  static int add_association_write_handler(const String &association, Element *e, void *, ErrorHandler *);
};

CLICK_ENDDECLS
#endif
  


