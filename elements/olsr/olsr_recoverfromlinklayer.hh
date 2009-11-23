//TED 300304: Created
#ifndef OLSR_RECOVERFROMLINKLAYER_HH
#define OLSR_RECOVERFROMLINKLAYER_HH

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
 * OLSRRecoverFromLinkLayer(OLSRDuplicateSet element, ip_address)
 *
 * =d
 * The OLSRRecoverFromLinkLayer element  
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


class OLSRRecoverFromLinkLayer : public Element
{
public:
	OLSRRecoverFromLinkLayer();
	~OLSRRecoverFromLinkLayer();

	const char *class_name() const  { return "OLSRRecoverFromLinkLayer"; }
	const char *processing() const  { return PUSH; }
	OLSRRecoverFromLinkLayer *clone() const   { return new OLSRRecoverFromLinkLayer; }
  	const char *port_count() const { return "1/2"; }

	int configure(Vector<String> &conf, ErrorHandler *errh);
	int initialize(ErrorHandler *);
	void push(int, Packet*);

private:
	OLSRNeighborInfoBase	*_neighborInfoBase;
	OLSRLinkInfoBase 	*_linkInfoBase;
	OLSRARPQuerier		*_arpQuerier;
	OLSRInterfaceInfoBase	*_interfaceInfoBase;
	OLSRRoutingTable	*_routingTable;
	OLSRTCGenerator		*_tcGenerator;
	IPAddress		_myMainIP;
};

CLICK_ENDDECLS

#endif

