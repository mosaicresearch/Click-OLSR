//TED 110504: Created

#include <click/config.h>
#include <click/confparse.hh>
#include "click_olsr.hh"
#include "olsr_forward.hh"
#include "olsr_packethandle.hh"
#include <click/packet_anno.hh>

CLICK_DECLS

OLSRForward::OLSRForward()
{
}

OLSRForward::~OLSRForward()
{
}

int 
OLSRForward::configure(Vector<String> &conf, ErrorHandler *errh)
{
 int dup_hold_time;
  if (cp_va_parse(conf, this, errh,
  		  cpInteger, "Duplicate Holding Time (sec)",&dup_hold_time,
		  cpElement, "DuplicateSet Element", &_duplicateSet, 
		  cpElement, "Neighbor InfoBase Element", &_neighborInfo,
		  cpElement, "InterfaceInfoBase Element", &_interfaceInfo,
		  cpElement, "localIfInfoBase Element", &_localIfInfoBase,
		  cpIPAddress, "main IP address", &_myMainIP,
		  0) < 0)
    return -1;
 _dup_hold_time=make_timeval (dup_hold_time,0);
  return 0;
}

int 
OLSRForward::initialize(ErrorHandler *)
{
  _msg_seq = 0;
  return 0;
}

void
OLSRForward::push(int port, Packet *packet)
{
    struct timeval now;
  click_gettimeofday(&now);

  int paint=static_cast<int>(PAINT_ANNO(packet));//packets get marked with paint 0..N depending on Interface they arrive on
  IPAddress receiving_If_IP=_localIfInfoBase->get_iface_addr(paint); //gets IP of Interface N
  if (port == 0){
      bool retransmit=false;
    msg_hdr_info msg_info = OLSRPacketHandle::get_msg_hdr_info(packet, 0);
    
    if (msg_info.originator_address != _myMainIP){
      //step 2
      duplicate_data *duplicate_tuple = _duplicateSet->find_duplicate_entry(msg_info.originator_address, msg_info.msg_seq);
      if (duplicate_tuple != 0){
	
	if (duplicate_tuple->D_retransmitted){
	  //olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) packet->data();
	 //click_chatter ("FORWARD (node %s): DROPPING message from %s received on interface %s messagetype  --  ALREADY RETRANSMITTED: %d\n",_myMainIP.unparse().cc(),msg_info.originator_address.unparse().cc(), receiving_If_IP.unparse().cc(),msg_hdr->msg_type);
	  output(1).push(packet); //discard
	  return;
	}
	//check if interface msg was received on is in D_iface_list
	for (int i = 0; i < duplicate_tuple->D_iface_list.size(); i++){
	  IPAddress iface_address = duplicate_tuple->D_iface_list.at(i);
	  if (iface_address == receiving_If_IP){
	    //olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) packet->data();
	 //click_chatter ("FORWARD (node %s): DROPPING message from %s received on interface %s messagetype  --  receiving INTERFACE on D_iface_list: %d\n",_myMainIP.unparse().cc(),msg_info.originator_address.unparse().cc(), receiving_If_IP.unparse().cc(),msg_hdr->msg_type);
	    output(1).push(packet); //discard
	    return;
	  }
	}
      }
        //step 4
      
      IPAddress source_addr = packet->dst_ip_anno();
      IPAddress source_addr_main_IP =  _interfaceInfo->get_main_address(source_addr);
      if (_neighborInfo->is_mpr_selector(source_addr_main_IP)){
  	if (msg_info.ttl > 1)//message must be retransmitted
	retransmit=true;
	 //else click_chatter ("DISCARDING message because of TTL\n");
	  if (duplicate_tuple == 0)
	    duplicate_tuple = _duplicateSet->add_duplicate_entry(msg_info.originator_address, msg_info.msg_seq);
	  duplicate_tuple->D_time = now + _dup_hold_time;
	  duplicate_tuple->D_iface_list.push_back(receiving_If_IP);
	  duplicate_tuple->D_retransmitted = retransmit;
	  }
	  //else click_chatter ("DISCARDING message because main Address %s of source Address %s is NOT an MPR Selector\n",source_addr.unparse().cc(),source_addr_main_IP.unparse().cc());
	 }
	  if (retransmit)
	  {
	  olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) packet->data();
	  msg_hdr->ttl = msg_hdr->ttl - 1;
	  msg_hdr->hop_count = msg_hdr->hop_count + 1;
	//adding olsr packet header
	packet=packet->push(sizeof(olsr_pkt_hdr));
	olsr_pkt_hdr *pkt_hdr = (olsr_pkt_hdr *) packet->data();
	pkt_hdr->pkt_length = htons(msg_info.msg_size + sizeof(olsr_pkt_hdr));
	pkt_hdr->pkt_seq = 0;
	output(0).push(packet); //forward packet
	//click_chatter ("FORWARD (node %s): relaying message from %s received on interface %s messagetype: %d\n",_myMainIP.unparse().cc(),msg_info.originator_address.unparse().cc(), receiving_If_IP.unparse().cc(),msg_hdr->msg_type);
	return;
	}
	else
	 {
	 //olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) packet->data();
	 //click_chatter ("FORWARD (node %s): DROPPING message from %s received on interface %s messagetype: %d\n",_myMainIP.unparse().cc(),msg_info.originator_address.unparse().cc(), receiving_If_IP.unparse().cc(),msg_hdr->msg_type);
	 output(1).push(packet); //discard
	 
         return;
      }
    }
  else if ( port == 1 ) { //packets from self, and msg_seq
      msg_hdr_info msg_info = OLSRPacketHandle::get_msg_hdr_info(packet, sizeof(olsr_pkt_hdr));
    olsr_pkt_hdr *pkt_hdr = (olsr_pkt_hdr *) packet->data();
    pkt_hdr->pkt_length = htons(msg_info.msg_size + sizeof(olsr_pkt_hdr));
    pkt_hdr->pkt_seq = 0;
    olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) (pkt_hdr + 1);
    msg_hdr->msg_seq = htons ( get_msg_seq() );
    //click_chatter ("FORWARD (node %s) broadcasting message from me: messagetype: %d\n",_myMainIP.unparse().cc(),msg_hdr->msg_type);
    output(0).push(packet); //forward packet
    return;
  }
  else{
    click_chatter("reached end of OLSRForward::push, discarding packet"); //should never get here
    output(1).push(packet);//discard packet
  }
}


uint16_t
OLSRForward::get_msg_seq()
{
  _msg_seq++;
  if (_msg_seq >= OLSR_MAX_SEQNUM)
    _msg_seq = 1;
  return _msg_seq;
}


CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRForward);

