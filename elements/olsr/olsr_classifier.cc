
#include <click/config.h>
#include <click/confparse.hh>
#include <click/package.hh>
#include "olsr_packethandle.hh"
#include "olsr_classifier.hh"
#include "click_olsr.hh"
#include <click/packet_anno.hh>

CLICK_DECLS


OLSRClassifier::OLSRClassifier()
{
}


OLSRClassifier::~OLSRClassifier()
{
}


int
OLSRClassifier::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (cp_va_parse(conf, this, errh,
		  cpElement, "Duplicate Set Element", &_duplicateSet,
		  cpElement, "localIfInfoBase Element", &_localIfInfoBase,
		  cpIPAddress, "Nodes main IP address", &_myMainIP,
		  0) < 0)
    return -1;
  return 0;
}


int
OLSRClassifier::initialize(ErrorHandler *)
{
  return 0;
}


// Output ports
// 0 - Discard
// 1 - Hello Messages
// 2 - TC Messages
// 3 - MID Messages
// 4 - HNA Messages 
// 5 - Other


void
OLSRClassifier::push(int, Packet *packet)
{
  pkt_hdr_info pkt_info = OLSRPacketHandle::get_pkt_hdr_info(packet);
  msg_hdr_info msg_info = OLSRPacketHandle::get_msg_hdr_info(packet, sizeof(olsr_pkt_hdr));
#ifdef debug
  click_chatter ("\nsoy el node con IP %s \t OLSR_Classifier\n",_myMainIP.unparse().cc() );
#endif
  //  click_chatter("\nReceived message:\nmsg_type: %d, originator: %s, seq_num: %d \n", msg_info.msg_type, msg_info.originator_address.unparse().cc(), msg_info.msg_seq);
  //click_chatter("pkt_length: %d, msg_size: %d\n", pkt_info.pkt_length, msg_info.msg_size); 
  int extra_message = pkt_info.pkt_length - msg_info.msg_size - sizeof(olsr_pkt_hdr);
  packet->pull(sizeof(olsr_pkt_hdr));
 
  do{
    Packet *p = packet->clone();  

    if (msg_info.ttl <= 0 ){
//     click_chatter("Discarding message, ttl == 0");
      output(0).push(p); //discard messages with ttl = 0
    }
    else if ( msg_info.originator_address == _myMainIP ){
//      click_chatter("Discarding message sent by me (%s)", _myMainIP.unparse().cc() );
      output(0).push(p); //discard messages from self
    }
    else{
      duplicate_data *duplicate = _duplicateSet->find_duplicate_entry(msg_info.originator_address, msg_info.msg_seq);
      if ( duplicate != 0 ){
	bool considered_for_forward = false;
	int paint=static_cast<int>(PAINT_ANNO(packet));//packets get marked with paint 0..N depending on Interface they arrive on
  IPAddress receiving_ip=_localIfInfoBase->get_iface_addr(paint); //gets IP of Interface N
	
	for ( int i = 0; i < duplicate->D_iface_list.size(); i++ ){
	  IPAddress iface_addr = duplicate->D_iface_list.at(i);
	  if ( receiving_ip == iface_addr ) 
	    considered_for_forward = true;
	}
	if ( considered_for_forward ){
// 	  click_chatter ("OLSRCLASSIFIER (node %s): DROPPING message from %s (org_add) received on interface %s \n",_myMainIP.unparse().cc(), msg_info.originator_address.unparse().cc(), receiving_ip.unparse().cc());
// 	  click_chatter("Discarding message, already considered for forward"); 
	  output(0).push(p); //discard messages already considered for forward
	}
	else
	  output(5).push(p); //consider message for forward without processing
      }
      else{ //process message
	//	click_chatter("extra_message: %d, msg_info.msg_size: %d\n", extra_message, msg_info.msg_size);
	if (extra_message >= (int)sizeof(olsr_msg_hdr)){
	  p->take(extra_message);
	}
	switch(msg_info.msg_type){
	case OLSR_HELLO_MESSAGE:
	  output(1).push(p);
	  break;
	case OLSR_TC_MESSAGE:
	  output(2).push(p);
	  break;
	case OLSR_MID_MESSAGE:
	  output(3).push(p);
	  break;
	case OLSR_HNA_MESSAGE:
	  output(4).push(p);
	  break;
	default:
	  output(5).push(p); //not a known message type, consider for forward anyway
	}
      }
    }

    int offset = msg_info.msg_size;

    //remove message just considered from packet
    if (extra_message >= (int)sizeof(olsr_msg_hdr)){
      msg_info = OLSRPacketHandle::get_msg_hdr_info(packet, offset);
      packet->pull( offset );
      extra_message = extra_message - msg_info.msg_size;
    }
    else
      extra_message = -1;

  }while ( extra_message >= 0 );  //there is more than one message in packet
  
  packet->kill(); //all messages considered, kill original packet
}


CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRClassifier);

