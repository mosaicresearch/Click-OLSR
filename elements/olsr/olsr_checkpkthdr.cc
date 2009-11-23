
#include <click/config.h>
#include <click/confparse.hh>
#include <click/package.hh>
#include "olsr_packethandle.hh"
#include "olsr_checkpkthdr.hh"
#include "click_olsr.hh"

CLICK_DECLS


OLSRCheckPacketHeader::OLSRCheckPacketHeader()
{
}


OLSRCheckPacketHeader::~OLSRCheckPacketHeader()
{
}


int
OLSRCheckPacketHeader::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (cp_va_parse(conf, this, errh,
		  cpElement, "Duplicate Set Element", &_duplicateSet,
		  0) < 0)
    return -1;
  return 0;
}


void
OLSRCheckPacketHeader::push(int, Packet *packet)
{
  pkt_hdr_info pkt_info = OLSRPacketHandle::get_pkt_hdr_info(packet);
  IPAddress source_addr = packet->dst_ip_anno();

  if (pkt_length_too_short(pkt_info.pkt_length)) {
    //discard if packet length is less than a packet hdr + message hdr
    click_chatter("Dropping packet, pkt_length too short");
    output(1).push(packet); 
    return;
  }
  else if ( pkt_seq_old(source_addr, pkt_info.pkt_seq) ){ 
    click_chatter("Dropping packet, pkt seq old");
    output(1).push(packet); //discard if packet sequence number is old
    return;
  }
  output(0).push(packet);
}


bool
OLSRCheckPacketHeader::pkt_seq_old(IPAddress source_addr, int pkt_seq)
{
  int prev_pkt_seq = 0;
  prev_pkt_seq = _duplicateSet->find_packet_seq(source_addr);
 
  if (prev_pkt_seq == 0){
    _duplicateSet->add_packet_seq(source_addr, pkt_seq);
    return false;
  }
  else{
    // check if new pkt seq is bigger than that allready stored according to RFC ch 19
    if (   ( pkt_seq > prev_pkt_seq && (pkt_seq - prev_pkt_seq) <= (OLSR_MAX_SEQNUM/2) )
	   ||  ( prev_pkt_seq > pkt_seq && (prev_pkt_seq - pkt_seq) > (OLSR_MAX_SEQNUM/2) )   ){    
      _duplicateSet->update_packet_seq(source_addr, pkt_seq);
      return false;
    }
  }
  return true;
}

bool 
OLSRCheckPacketHeader::pkt_length_too_short(int pkt_length)
{
  if ( pkt_length < OLSR_MINIMUM_PACKET_LENGTH ){
    return true;
  }
  return false;
}



CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRCheckPacketHeader);

