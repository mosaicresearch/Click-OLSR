/*
  =c
  OLSR specific element, checks the header of OLSR packets

  =s
  OLSRCheckPacketHeader(OSLRDuplicateSet element)

  =io
  One input, two outputs
  Output port 1: packets with ok sequence number and length
  Output port 2: packets that are too short or received out of order
  
  =processing
  PUSH

  =d
  Takes OLSR packet on its input. Packets must have their source address in the destination address annotation. Checks that the packets' length is greater than 15 (the size of an olsr packet header + an olsr message header) The packet's sequence number is then checked against the OLSRDuplicateSet given as argument.

  =a
  OLSRClassifier

*/
#ifndef OLSR_CHECKPKTHDR_HH
#define OLSR_CHECKPKTHDR_HH

#include <click/element.hh>
#include "olsr_duplicate_set.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRCheckPacketHeader: public Element{
public:
  OLSRCheckPacketHeader();
  ~OLSRCheckPacketHeader();

  const char* class_name() const { return "OLSRCheckPacketHeader"; }
  const char* processing() const { return PUSH; }
  OLSRCheckPacketHeader *clone() const { return new OLSRCheckPacketHeader(); }
  const char *port_count() const  { return "1/2"; }
  int configure(Vector<String> &conf, ErrorHandler *errh);
  void push(int, Packet *packet);

private:
  OLSRDuplicateSet *_duplicateSet;

  bool pkt_seq_old(IPAddress source_addr, int pkt_seq);
  bool pkt_length_too_short(int pkt_length);
};

CLICK_ENDDECLS
#endif

