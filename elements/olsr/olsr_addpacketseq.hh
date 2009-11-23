//TED 130504: Created
/*
  =c
  OLSR specific element, sets packet sequence number field of the OLSR packet header

  =s
  OLSRAddPacketSeq()

  =io
  One input, one output
  
  =processing
  PUSH

  =d
  Gets OLSR packets on its input. Sets the packet sequence number in the OLSR packet header. One is needed for each network interface.

  =a
  OLSRForward
*/
#ifndef OLSR_ADDPACKETSEQ_HH
#define OLSR_ADDPACKETSEQ_HH

#include <click/element.hh>

CLICK_DECLS

class OLSRAddPacketSeq: public Element{
public:

  OLSRAddPacketSeq();
  ~OLSRAddPacketSeq();

  const char* class_name() const { return "OLSRAddPacketSeq"; }
  const char* processing() const { return PUSH; }
  OLSRAddPacketSeq *clone() const { return new OLSRAddPacketSeq(); }
  const char *port_count() const  { return "1/1"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);
  int initialize(ErrorHandler *);

  void push(int, Packet *packet);

private:
  IPAddress _interfaceAddress;
  uint16_t _seq_num;

  uint16_t get_seq_num();
};
CLICK_ENDDECLS
#endif
