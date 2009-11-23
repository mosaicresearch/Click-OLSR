
#include <click/config.h>
#include <click/confparse.hh>
#include <click/package.hh>
#include "olsr_packethandle.hh"
#include "olsr_packetanalyzer.hh"
#include "click_olsr.hh"
#include <click/packet_anno.hh>

CLICK_DECLS


OLSRPacketAnalyzer::OLSRPacketAnalyzer()
{
}


OLSRPacketAnalyzer::~OLSRPacketAnalyzer()
{
}

// Output ports
// 0 - Discard
// 1 - Hello Messages
// 2 - TC Messages
// 3 - MID Messages
// 4 - HNA Messages 
// 5 - Other


String
OLSRPacketAnalyzer::analyze(Packet *packet, int offset)
{
	//pkt_hdr_info pkt_info = OLSRPacketHandle::get_pkt_hdr_info(packet);
	msg_hdr_info msg_info = OLSRPacketHandle::get_msg_hdr_info(packet, offset);
	
	String res = "";
	
	switch(msg_info.msg_type){
	case OLSR_HELLO_MESSAGE:
		res = "[HELLO]";
		break;
	case OLSR_TC_MESSAGE:
		res = "[TC]";
		break;
	case OLSR_MID_MESSAGE:
		res = "[MID]";	
		break;
	case OLSR_HNA_MESSAGE:
		res = "[HNA]";
		break;
	default:
		res = "[UNKNOWN PACKET TYPE]";
	}
	
	
	return res;
}


CLICK_ENDDECLS
ELEMENT_REQUIRES(SimPacketAnalyzer)
EXPORT_ELEMENT(OLSRPacketAnalyzer);

