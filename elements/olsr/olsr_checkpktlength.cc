#include <click/config.h>
#include <click/confparse.hh>
#include <click/package.hh>
#include "olsr_packethandle.hh"
#include "olsr_checkpktlength.hh"
#include "click_olsr.hh"


CLICK_DECLS

OLSRCheckPacketLength::OLSRCheckPacketLength()
{
}


OLSRCheckPacketLength::~OLSRCheckPacketLength()
{
}


void
OLSRCheckPacketLength::push(int, Packet *packet)
{
  pkt_hdr_info pkt_info = OLSRPacketHandle::get_pkt_hdr_info(packet);

  if ( pkt_info.pkt_length < 16 ){
    //discard if packet length is less than a packet hdr + message hdr
    click_chatter("Dropping packet, pkt_length too short");
    output(0).push(packet); 
    return;
  }
  output(1).push(packet);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRCheckPacketLength);

