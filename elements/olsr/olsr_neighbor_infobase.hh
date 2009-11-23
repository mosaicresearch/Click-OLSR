//TED 170404: Created

#ifndef OLSR_NEIGHBOR_INFOBASE_HH
#define OLSR_NEIGHBOR_INFOBASE_HH

#include <click/element.hh>
#include <click/ipaddress.hh>
#include <click/bighashmap.hh>
#include <click/vector.hh>
#include <click/timer.hh>
#include "olsr_rtable.hh"
#include "olsr_tc_generator.hh"
#include "ippair.hh"
#include "click_olsr.hh"
#include "olsr_link_infobase.hh"
#include "olsr_hello_generator.hh"
#include "olsr_interface_infobase.hh"


//#define profiling_kernel
#define do_it


CLICK_DECLS



class OLSRRoutingTable;
class OLSRTCGenerator;
class OLSRLinkInfoBase;
class OLSRInterfaceInfoBase;
class OLSRHelloGenerator;

class OLSRNeighborInfoBase: public Element
{
public:

	OLSRNeighborInfoBase();
	~OLSRNeighborInfoBase();

	const char* class_name() const { return "OLSRNeighborInfoBase"; }
	OLSRNeighborInfoBase *clone() const { return new OLSRNeighborInfoBase(); }
	int initialize(ErrorHandler *);
	void uninitialize();
	int configure(Vector<String>&, ErrorHandler *errh);

	struct neighbor_data *add_neighbor(IPAddress neigh_addr);
	struct neighbor_data *find_neighbor(IPAddress neigh_addr);
	bool update_neighbor(IPAddress neigh_addr, int status, int willingness);
	void remove_neighbor(IPAddress neigh_addr);
	void print_neighbor_set();
	HashMap<IPAddress, void *> *get_neighbor_set();

	bool add_twohop_neighbor(IPAddress neigh_addr, IPAddress twohop_neigh_addr, struct timeval time );
	struct twohop_data *find_twohop_neighbor(IPAddress neigh_addr, IPAddress twohop_neigh_addr);
	void remove_twohop_neighbor(IPAddress neigh_addr, IPAddress twohop_neigh_addr);
	void print_twohop_set();
	HashMap<IPPair, void*> *get_twohop_set();

	struct mpr_selector_data *add_mpr_selector(IPAddress ms_addr, struct timeval time);
	struct mpr_selector_data *find_mpr_selector(IPAddress ms_addr);
	bool is_mpr_selector(IPAddress ms_addr);
	void remove_mpr_selector(IPAddress ms_addr);
	void print_mpr_selector_set();
	HashMap<IPAddress, void *> *get_mpr_selector_set();

	void compute_mprset();
	IPAddress *find_mpr(const IPAddress &address);
	void print_mpr_set();

	void add_handlers();
	
	void  additional_mprs_is_enabled(bool in);
#ifdef profiling_kernel
	static String read_handler(Element *e, void *thunk);
#endif


private:
	typedef HashMap<IPAddress, void *> NeighborSet;
	typedef HashMap<IPPair, void *> TwoHopSet;
	typedef HashMap<IPAddress, void *> MPRSelectorSet;
	typedef HashMap<IPAddress, IPAddress> MPRSet;
	typedef HashMap<IPAddress, Vector <IPAddress> > N2Set ;

	NeighborSet *_neighborSet;
	TwoHopSet *_twohopSet;
	MPRSelectorSet *_mprSelectorSet;
	MPRSet *_mprSet;
	OLSRRoutingTable *_routingTable;
	OLSRTCGenerator *_tcGenerator;
	OLSRHelloGenerator *_helloGenerator;
	OLSRLinkInfoBase *_linkInfoBase;
	OLSRInterfaceInfoBase *_interfaceInfoBase;
	//OLSRLocalIfInfoBase *_localIfInfoBase;
	Timer _twohop_timer;
	Timer _mpr_selector_timer;
	bool _additional_hello_message;
	IPAddress _myMainIP;
/// == mvhaen ====================================================================================================
	bool _additional_mprs;
	static int additional_mprs_is_enabled_handler(const String &conf, Element *e, void *, ErrorHandler * errh);
/// == !mvhaen ===================================================================================================

#ifdef profiling_kernel
	uint64_t _cyclesaccum;
	long _count;

#endif

	static void mpr_selector_expiry_hook(Timer *timer, void *thunk);
	static void twohop_expiry_hook(Timer *timer, void *thunk);
};

CLICK_ENDDECLS
#endif

