#ifndef CLICK_ADDARPENTRY_HH
#define CLICK_ADDARPENTRY_HH
#include <click/element.hh>
#include "olsr_arpquerier.hh"
CLICK_DECLS

/*
 * =c
 *
 * AddARPEntry()
 *
 * =s Ethernet
 *
 * =d
 *
 * =n
 *
 * =e
 *
 */

class AddARPEntry : public Element
{
public:

	AddARPEntry();
	~AddARPEntry();

	const char *class_name() const		{ return "AddARPEntry"; }
	const char *processing() const		{ return AGNOSTIC; }
	const char *port_count() const		{ return "1/1"; }

	int configure(Vector<String> &, ErrorHandler *);

	Packet *simple_action(Packet *);
private:
	OLSRARPQuerier	*_arpQuerier;
};

CLICK_ENDDECLS
#endif
