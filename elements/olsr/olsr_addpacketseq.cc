//TED 130504: Created
#include <click/config.h>
#include <click/confparse.hh>
#include "olsr_packethandle.hh"
#include "olsr_addpacketseq.hh"
#include "click_olsr.hh"

CLICK_DECLS

OLSRAddPacketSeq::OLSRAddPacketSeq()
{
}


OLSRAddPacketSeq::~OLSRAddPacketSeq()
{
}


int
OLSRAddPacketSeq::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if ( cp_va_parse(conf, this, errh,
		   cpIPAddress, "Output Interface Address", &_interfaceAddress,
		   0) < 0 )
    return -1;
  return 0;
}


int 
OLSRAddPacketSeq::initialize(ErrorHandler *)
{
  _seq_num = 0;
  return 0;
}


void 
OLSRAddPacketSeq::push(int, Packet *packet)
{
   olsr_pkt_hdr *pkt_hdr = (olsr_pkt_hdr *) packet->data();
  pkt_hdr->pkt_seq = htons( get_seq_num() );

  output(0).push(packet);
 }


uint16_t 
OLSRAddPacketSeq::get_seq_num()
{
  _seq_num++;
  if (_seq_num >= OLSR_MAX_SEQNUM)
    _seq_num = 1;
  return _seq_num;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRAddPacketSeq);

