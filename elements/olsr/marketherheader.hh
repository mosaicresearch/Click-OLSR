#ifndef CLICK_MARKETHERHEADER_HH
#define CLICK_MARKETHERHEADER_HH
#include <click/element.hh>
CLICK_DECLS

/*
 * =c
 *
 * MarkEtherHeader()
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

class MarkEtherHeader : public Element
{
public:

	MarkEtherHeader();
	~MarkEtherHeader();

	const char *class_name() const		{ return "MarkEtherHeader"; }
	const char *processing() const		{ return AGNOSTIC; }

	int configure(Vector<String> &, ErrorHandler *);

	Packet *simple_action(Packet *);

};

CLICK_ENDDECLS
#endif
