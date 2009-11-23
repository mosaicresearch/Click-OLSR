// Copyright (c) 2004 by the University of Antwerp
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the Polytechnic University of Catalonia, Computer Networking
// Group.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/ipaddress.hh>
#include "click_olsr.hh"
#include "olsr_neighbor_infobase.hh"
#include "olsr_process_hna.hh"
#include "olsr_packethandle.hh"

CLICK_DECLS

OLSRProcessHNA::OLSRProcessHNA()
{
}

OLSRProcessHNA::~OLSRProcessHNA()
{
}


int
OLSRProcessHNA::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (cp_va_parse(conf, this, errh,
		  cpElement, "AssociationInfoBase Element", &_associationInfo,
		  cpElement, "NeighborInfoBase Element", &_neighborInfo,
		  cpElement, "Routing Table Element", &_routingTable,
		  cpIPAddress, "My main address", &_my_ip, 0) < 0)
    return -1;
  return 0;
}


void
OLSRProcessHNA::push(int, Packet *packet){
  //click_chatter ("OLSR_process_HNA::push \n");
  msg_hdr_info msg_info;
  neighbor_data *neighbor_tuple;
  association_data *association_tuple;

  bool update_hna = false;
  bool new_hna_added = false;
  struct timeval now;
  IPAddress network_address, originator_address, source_address, netmask;
  in_addr * addr;

  click_gettimeofday(&now);

  msg_info = OLSRPacketHandle::get_msg_hdr_info(packet, 0);
  originator_address = msg_info.originator_address;

  //dst_ip_anno must be set, must be source address of ippacket
  source_address = packet->dst_ip_anno();

  /// 12.5.1 if sender interface not in symmetric 1-hop neighborhood the message is discarded
  neighbor_tuple = _neighborInfo->find_neighbor(source_address);
  if (neighbor_tuple == 0 || neighbor_tuple->N_status == OLSR_NOT_NEIGH) {
	packet->kill();
	return;
  }

  int msg_bytes_left = msg_info.msg_size - sizeof(olsr_msg_hdr);
  int msg_offset = sizeof(olsr_msg_hdr);

  click_chatter("%f | %s | received a HNA message from %s", Timestamp(now).doubleval(), _my_ip.unparse().c_str(), originator_address.unparse().c_str());


  if (msg_bytes_left > 0){ //there are advertised networks in the hna-message
    do {
      addr = (in_addr*)(packet->data() + msg_offset);
      network_address = IPAddress(*addr);
      addr = (in_addr*)(packet->data() + msg_offset) + 1;
      netmask = IPAddress(*addr);

      click_chatter("%f | %s | network_address %s | netmask %s", Timestamp(now).doubleval(), _my_ip.unparse().c_str(), network_address.unparse().c_str(), netmask.unparse().c_str());

      association_tuple = _associationInfo->find_tuple(originator_address, network_address, netmask);
      if (association_tuple != 0) {
        association_tuple->A_time = now + msg_info.validity_time;
	update_hna = true;
      } else {
	_associationInfo->add_tuple(originator_address, network_address, netmask, (now + msg_info.validity_time));
	new_hna_added = true;
      }

      msg_bytes_left -= sizeof(in_addr) * 2;
      msg_offset += sizeof(in_addr) * 2;
    } while (  msg_bytes_left > 0  );
  }

  _associationInfo->print_association_set();

  if (new_hna_added || update_hna){
    //click_chatter("Recomputing Routing Table\n");
    _routingTable->compute_routing_table();
    //click_chatter("Routing table recomputed");
    click_chatter("%f | %s | Routing Table:\n",Timestamp(now).doubleval(), _my_ip.unparse().c_str());
    _routingTable->print_routing_table();
  }
  output(0).push(packet);
}
 

CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRProcessHNA)

