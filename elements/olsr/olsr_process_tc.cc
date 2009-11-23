//TED 140504: created

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/ipaddress.hh>
#include "click_olsr.hh"
#include "olsr_process_tc.hh"
#include "olsr_topology_infobase.hh"


CLICK_DECLS

OLSRProcessTC::OLSRProcessTC()
{
}

OLSRProcessTC::~OLSRProcessTC()
{
}


int
OLSRProcessTC::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (cp_va_parse(conf, this, errh, 
		  cpElement, "TopologyInfoBase Element", &_topologyInfo,
		  cpElement, "Neighbor InfoBase Element", &_neighborInfo,
		  cpElement, "InterfaceInfoBase Element", &_interfaceInfo,
		  cpElement, "Routing Table Element", &_routingTable,
		  cpIPAddress, "my main IP", &_myMainIP,
		  0) < 0)
    return -1;
  return 0;
}

//output 0 - Packets that are to be forwarded
//output 1 - Discard

void
OLSRProcessTC::push(int, Packet *packet)
{

  bool topology_tuple_added = false;
  bool topology_tuple_removed = false;
  msg_hdr_info msg_info;
  tc_hdr_info tc_info;
  int ansn;
  topology_data *topology_tuple;
  IPAddress originator_address;
  struct timeval now;
  click_gettimeofday(&now);
  
  msg_info = OLSRPacketHandle::get_msg_hdr_info(packet, 0);
  tc_info = OLSRPacketHandle::get_tc_hdr_info(packet, (int) sizeof(olsr_msg_hdr));
  originator_address = msg_info.originator_address;
  ansn = tc_info.ansn;
  //click_chatter ("node %s Processing TC message with ansn %d from originator address %s\n",_myMainIP.unparse().cc(),ansn,originator_address.unparse().cc());
  //_topologyInfo->print_topology();
  //_neighborInfo->print_neighbor_set();
  //RFC ch 9.5 - TC Msg processing  
  //step 1 - discard message if not from symmetric 1-hop neighbor
  IPAddress src_addr = packet->dst_ip_anno();
  if ( _neighborInfo->find_neighbor(_interfaceInfo->get_main_address( src_addr)) == 0 ){
    //click_chatter("Discarded TC message, not from 1-hop neighbor (src address: %s)\n",src_addr.unparse().cc());
    output(1).push(packet);
    return;
  }
  //RFC 9.5 step 2 - discard message received out of order
  if ( _topologyInfo->newer_tuple_exists(originator_address, (int) ansn) ){
   //click_chatter("Discarded TC message, received out of order\n");
    output(1).push(packet);
    return;
  }
  //step 3 - delete outdated topology tuples
  if ( _topologyInfo->remove_outdated_tuples(originator_address, (int) ansn) ){
    //click_chatter("topology_tuple_removed\n");
    topology_tuple_removed = true;
  }  

  //step 4 - record topology tuple
  int remaining_neigh_bytes = msg_info.msg_size - (int)sizeof(olsr_msg_hdr) - (int)sizeof(olsr_tc_hdr);
  int neigh_addr_offset = sizeof(olsr_msg_hdr) + sizeof(olsr_tc_hdr);
  
  while ( remaining_neigh_bytes >= (int) sizeof(in_addr) ){
    in_addr *address = (in_addr *) (packet->data() + neigh_addr_offset);
    IPAddress dest_addr = IPAddress(*address);
    if (dest_addr != _myMainIP && _neighborInfo->find_neighbor(dest_addr) == 0){
      //dont record entries for myself or my neighbors
      topology_tuple = _topologyInfo->find_tuple(dest_addr, originator_address);
      if ( topology_tuple == 0 ){
	topology_tuple = _topologyInfo->add_tuple(dest_addr, originator_address, (now+msg_info.validity_time));
	topology_tuple->T_seq = ansn;
	//click_chatter("topology tuple added\n");
	topology_tuple_added = true;
      }
      else{
	topology_tuple->T_time = now + msg_info.validity_time;
	//click_chatter ("topology tuple updates\n");
      }
    }
    remaining_neigh_bytes -= sizeof(in_addr);
    neigh_addr_offset += sizeof(in_addr);
  }
  if ( topology_tuple_added || topology_tuple_removed ){
    //click_chatter("recomputing routing table");
    _routingTable->compute_routing_table();
    //_routingTable->print_routing_table();
  }

  output(0).push(packet);
  //_topologyInfo->print_topology();
}


CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRProcessTC);

