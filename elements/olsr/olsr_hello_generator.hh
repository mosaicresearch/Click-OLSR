// TED 260404 : Created
/*
  =c
  OLSR specific element, generates OLSR Hello messages
 
  =s
  OLSRHelloGenerator(INTERVAL(msecs), OLSRLinkInfoBase element, OLSRNeighborInfoBase element, OLSRInterfaceInfobase element, MY_IPADDRESS)
  
  =io
  One output
 
  =processing
  PUSH
 
  =d
  Generates a Hello message every INTERVAL msecs based on the information stored in the OLSRLinkInfoBase given as argument. If the node has no neighbors, a Hello message is made, containing only this node's willingness to forward traffic.
 
  =a
  OLSRTCGenerator, OLSRForward
  
*/
#ifndef OLSR_HELLO_GENERATOR_HH
#define OLSR_HELLO_GENERATOR_HH

//#define profiling

#include <click/element.hh>
#include <click/timer.hh>
#include <click/ipaddress.hh>
#include "olsr_link_infobase.hh"
#include "olsr_neighbor_infobase.hh"
#include "olsr_interface_infobase.hh"
#include "click_olsr.hh"
#include "olsr_forward.hh"

CLICK_DECLS

class OLSRForward;

class OLSRHelloGenerator : public Element
{
public:

	OLSRHelloGenerator();
	~OLSRHelloGenerator();

	const char *class_name() const		{ return "OLSRHelloGenerator"; }
	const char *processing() const		{ return PUSH; }
	OLSRHelloGenerator *clone() const	{ return new OLSRHelloGenerator(); }
  	const char *port_count() const  	{ return "0/1"; }

	int configure(Vector<String> &, ErrorHandler *);
	int initialize(ErrorHandler *);

	Packet *generate_hello();//IPAddress local_iface_addr);
	void notify_mpr_change();
	void run_timer(Timer *);

	void add_handlers();	
	
private:

	uint8_t get_link_code(struct link_data *data, timeval now);
	uint8_t compute_htime();
	uint8_t compute_vtime();	
	
	void set_period(int period);
	void set_neighbor_hold_time(int neighbor_hold_time);

	static int set_period_handler(const String &conf, Element *e, void *, ErrorHandler * errh);
	static int set_neighbor_hold_time_handler(const String &conf, Element *e, void *, ErrorHandler * errh);
	
	int _period;
	uint8_t _htime, _vtime;
	Timer _timer;
	OLSRLinkInfoBase *_linkInfoBase;
	OLSRNeighborInfoBase *_neighborInfoBase;
	OLSRInterfaceInfoBase *_interfaceInfoBase;
	OLSRForward *_forward;
	IPAddress _local_iface_addr;
	IPAddress _myMainIP;
	int _neighbor_hold_time;
	int _node_willingness;
};



CLICK_ENDDECLS
#endif
