#ifndef OLSR_CHECKPKTLENGTH_HH
#define OLSR_CHECKPKTLENGTH_HH

#include <click/element.hh>
#include "olsr_duplicate_set.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRCheckPacketLength: public Element{
public:
  OLSRCheckPacketLength();
  ~OLSRCheckPacketLength();

  const char* class_name() const { return "OLSRCheckPacketLength"; }
  const char* processing() const { return PUSH; }
  OLSRCheckPacketLength *clone() const { return new OLSRCheckPacketLength(); }
  const char *port_count() const  { return "1/2"; }

  void push(int, Packet *packet);

private:
  OLSRDuplicateSet *_duplicateSet;
};

CLICK_ENDDECLS
#endif
