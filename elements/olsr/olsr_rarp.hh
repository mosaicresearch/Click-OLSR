//TED 300304: Created
#ifndef OLSR_RARP_HH
#define OLSR_RARP_HH

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
 * OLSRRARP(OLSRDuplicateSet element, ip_address)
 *
 * =d
 * The OLSRRARP element  
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


class OLSRRARP : public Element
{
public:
	OLSRRARP();
	~OLSRRARP();

	const char *class_name() const  { return "OLSRRARP"; }
	const char *processing() const  { return PUSH; }
	OLSRRARP *clone() const   { return new OLSRRARP; }
  	const char *port_count() const  { return "1/2"; }

	int configure(Vector<String> &conf, ErrorHandler *errh);
	int initialize(ErrorHandler *);
	void push(int, Packet*);

private:
	OLSRARPQuerier		*_arpQuerier;
	IPAddress		_myMainIP;
	int			_offset;
};

CLICK_ENDDECLS

#endif

