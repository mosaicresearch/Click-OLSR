//TED 300304: Created
#ifndef OLSR_IDEALRECOVERFROMLINKLAYER_HH
#define OLSR_IDEALRECOVERFROMLINKLAYER_HH

#include <click/element.hh>
#include "olsr_neighbor_infobase.hh"
#include "olsr_link_infobase.hh"
#include "olsr_arpquerier.hh"
#include "olsr_rtable.hh"
#include "olsr_interface_infobase.hh"
#include "olsr_tc_generator.hh"

CLICK_DECLS
/* =c
 * OLSR specific element, splits up OSLR packets and classifies the OLSR messages within 
 *
 * =s
 * OLSRIdealRecoverFromLinkLayer(OLSRDuplicateSet element, ip_address)
 *
 * =d
 * The OLSRIdealRecoverFromLinkLayer element  
 *
 * =io
 * One input, five outputs
 * Output port 0: Discarded messages
 *
 * =processing
 * PUSH
 *
 * =a
 * OLSRProcessHello, OLSRProcessTC, OLSRProcessMID, OLSRForward
 */


class OLSRIdealRecoverFromLinkLayer : public Element
{
public:
	OLSRIdealRecoverFromLinkLayer();
	~OLSRIdealRecoverFromLinkLayer();

	const char *class_name() const  { return "OLSRIdealRecoverFromLinkLayer"; }
	const char *processing() const  { return PUSH; }
	OLSRIdealRecoverFromLinkLayer *clone() const   { return new OLSRIdealRecoverFromLinkLayer; }
	const char *port_count() const  { return "0/1"; }


	int configure(Vector<String> &conf, ErrorHandler *errh);
	int initialize(ErrorHandler *);

	void notify(const IPAddress &next_hop_IP);

	void add_handlers();
	
private:
	static int notify_handler(const String &conf, Element *e, void *, ErrorHandler * errh);	
	
	OLSRNeighborInfoBase	*_neighborInfoBase;
	OLSRLinkInfoBase 	*_linkInfoBase;
	OLSRInterfaceInfoBase	*_interfaceInfoBase;
	OLSRRoutingTable	*_routingTable;
	OLSRTCGenerator		*_tcGenerator;
	IPAddress		_myMainIP;
};

CLICK_ENDDECLS

#endif

