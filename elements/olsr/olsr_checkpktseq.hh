/*
  =c
  OLSR specific element, checks the sequence number of packets, discards packets received out of order

  =s
  OLSRCheckPacketSeq(OSLRDuplicateSet element)

  =io
  One input, two outputs
  Output port 1: packets with ok sequence number
  Output port 2: packets received out of order
  
  =processing
  PUSH

  =d
  Takes OLSR packet on its input. Packets must have their source address in the destination address annotation. The packet's sequence number is then checked in the OLSRDuplicateSet given as argument.

  =a
  OLSRCheckPacketLength, OLSRClassifier
*/
#ifndef OLSR_CHECKPKTSEQ_HH
#define OLSR_CHECKPKTSEQ_HH

#include <click/element.hh>
#include "olsr_duplicate_set.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRCheckPacketSeq: public Element{
public:
  OLSRCheckPacketSeq();
  ~OLSRCheckPacketSeq();

  const char* class_name() const { return "OLSRCheckPacketSeq"; }
  const char* processing() const { return PUSH; }
  OLSRCheckPacketSeq *clone() const { return new OLSRCheckPacketSeq(); }
  const char *port_count() const  { return "1/2"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);
  void push(int, Packet *packet);

private:
  OLSRDuplicateSet *_duplicateSet;

  bool pkt_seq_old(IPAddress source_addr, int pkt_seq);
};

CLICK_ENDDECLS
#endif

