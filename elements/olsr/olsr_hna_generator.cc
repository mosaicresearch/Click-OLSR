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
#include <math.h>
#include <clicknet/udp.h>
#include <clicknet/ether.h>
#include <click/ipaddress.hh>
#include <click/router.hh>
#include <click/vector.hh>
#include <click/bighashmap.hh>
#include "olsr_hna_generator.hh"
#include "olsr_neighbor_infobase.hh"
#include "click_olsr.hh"

CLICK_DECLS

OLSRHNAGenerator::OLSRHNAGenerator()
		: _timer(this), _association_info(0)
{
}


OLSRHNAGenerator::~OLSRHNAGenerator()
{
}


int
OLSRHNAGenerator::configure(Vector<String> &conf, ErrorHandler *errh)
{
	IPAddress network_addr = IPAddress();
	IPAddress netmask = IPAddress();
	int res = cp_va_parse(conf, this, errh,
	                      cpInteger, "HNA sending interval (msec)", &_period,
	                      cpInteger, "HNA Holding Time (msec)",&_hna_hold_time,
	                      cpIPAddress, "my main IPAddress", &_my_ip,
	                      cpKeywords,
	                      "NETWORK", cpIPPrefix, "the network that HNA should advertise e.g. 10.0.0.0/24", &network_addr, &netmask,
	                      "ASSOCIATION_INFO", cpElement, "AssociationInfoBase element: contains the networks to be advertised", &_association_info,
	                      0);

	if ( res < 0 )
		return res;
	if ( _period <= 0 )
		return errh->error("period must be greater than 0");
	if (!(network_addr == IPAddress() || netmask == IPAddress()))
	{
		_Association association;
		association.network_addr = network_addr;
		association.netmask = netmask;
		_fixedAssociations.push_back(association);
	}
	return res;
}


int
OLSRHNAGenerator::initialize(ErrorHandler *)
{
	_timer.initialize(this);
	_timer.schedule_after_msec(_period); // Send OLSR HELLO periodically
	_vtime = compute_vtime();
	_end_of_validity_time = make_timeval(0,0);
	_last_msg_sent_at = make_timeval(0,0);
	return 0;
}


void
OLSRHNAGenerator::run_timer(Timer *)
{
	generate_hna();
	_timer.schedule_after_msec(_period);
}



void
OLSRHNAGenerator::generate_hna()
{
	//click_chatter ("generate_hna \n");
	Vector<IPPair> *association_set = 0;
	int packet_size;

	// compute the HNA packet size
	// HNAGenerator stores a list of fixed associations.
	packet_size = sizeof(olsr_pkt_hdr) + sizeof(olsr_msg_hdr) + sizeof(in_addr)*2*_fixedAssociations.size();
	// HNAGenerator can also advertise the information in an _association_info element
	if (_association_info != 0)
	{
		association_set = _association_info->get_associations();
		if (_fixedAssociations.size() == 0 && association_set->size() == 0)
		{
			return;
		}
		packet_size += association_set->size()*sizeof(in_addr)*2;
	}


	int headroom = sizeof(click_ether) + sizeof(click_ip) + sizeof(click_udp);
	int tailroom = 5 * sizeof(in_addr); //enough room for 5 advertised neighbors
	WritablePacket *packet = Packet::make(headroom,0,packet_size, tailroom);
	if ( packet == 0 )
	{
		click_chatter( "in %s: cannot make packet!", name().c_str());
	}
	memset(packet->data(), 0, packet->length());

	olsr_pkt_hdr *pkt_hdr = (olsr_pkt_hdr *) packet->data();
	pkt_hdr->pkt_length = 0; //added in OLSRForward
	pkt_hdr->pkt_seq = 0; //added in OLSRForward

	olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) (pkt_hdr + 1);
	msg_hdr->msg_type = OLSR_HNA_MESSAGE;
	msg_hdr->vtime = _vtime;
	msg_hdr->msg_size = htons(sizeof(olsr_msg_hdr));
	msg_hdr->originator_address = _my_ip.in_addr();
	msg_hdr->ttl = 255;  //HNA messages should diffuse into entire network
	msg_hdr->hop_count = 0;
	msg_hdr->msg_seq = 0; //added in OLSRForward element


	bool first_iter = true;
	in_addr *address = 0;
	address = (in_addr *) (msg_hdr + 1);
	if (! _fixedAssociations.empty())
	{
		//click_chatter("%s | generating a hna for the subnet\n");
		//click_chatter("%s | I am advertising the following subnets that can be reached\n");
		for (Vector<_Association>::iterator iter = _fixedAssociations.begin(); iter != _fixedAssociations.end(); iter++)
		{

			if (first_iter)	first_iter = false;
			else address = (in_addr *) (address + 1);

			_Association* association = iter;
			*address = association->network_addr.in_addr();
			//	click_chatter("%s | network_addr = %s\n", _my_ip.unparse().c_str(), association->network_addr.unparse().c_str());
			address = (in_addr *) (address + 1);
			*address = association->netmask.in_addr();
			//	click_chatter("%s | netmask = %s\n", _my_ip.unparse().c_str(), association->netmask.unparse().c_str());
			msg_hdr->msg_size = htons( ntohs(msg_hdr->msg_size) + 2 * sizeof(in_addr) );

		}
	}
	if (_association_info != 0 && association_set != 0 && ! association_set->empty())
	{
		click_chatter("%s | generating a hna for the subnet\n", name().c_str());
		click_chatter("%s | I am advertising the following subnets that can be reached\n", name().c_str());
		for (Vector<IPPair>::iterator iter = association_set->begin(); iter != association_set->end(); iter++)
		{
			/* //--mvhaen-- changes due to MORHE
			 // a slight hack. If we were set to advertise a subnet, don't advertise nodes that we already advertised
			 for (Vector<_Association>::iterator iter2 = _fixedAssociations.begin(); iter2 != _fixedAssociations.end(); iter2++)
			 {
			   if (iter->_from.matches_prefix(iter->_network_addr, iter2->netmask_))
			   {
			     continue;
			   }
			 }
			 //--mvhaen--*/
			if (first_iter)	first_iter = false;
			else address = (in_addr *) (address + 1);

			*address = iter->_from.in_addr();
			click_chatter("%s | A_network_addr = %s\n", _my_ip.unparse().c_str(), iter->_from.unparse().c_str());
			address = (in_addr *) (address + 1);
			*address = iter->_to.in_addr();
			click_chatter("%s | A_netmask = %s\n", _my_ip.unparse().c_str(), iter->_to.unparse().c_str());
			msg_hdr->msg_size = htons( ntohs(msg_hdr->msg_size) + 2 * sizeof(in_addr) );

		}
	}
	//  pkt_hdr->pkt_length = htons ( ntohs(pkt_hdr->pkt_length) + ntohs(msg_hdr->msg_size) );
	//click_chatter ("end hna_generate\n");
	output(0).push(packet);

}

uint8_t
OLSRHNAGenerator::compute_vtime()
{
	uint8_t return_value = 0;
	int t = _hna_hold_time*1000; //top_hold_time in msec -> t in µsec
	//_top_hold_time.tv_usec+_top_hold_time.tv_sec*1000000; //fixpoint -> calculation in µsec

	int b=0;
	while ((t / OLSR_C_us) >= (1<<(b+1)))
		b++;
	int a=(((16*t/OLSR_C_us)>>b)-16);
	int value=(OLSR_C_us *(16+a)*(1<<b))>>4;
	if (value<t) a++;
	if (a==16)
	{
		b++; a=0;
	}

	if ( (a <= 15 && a >= 0) && (b <= 15 && b >= 0) )
		return_value = ((a << 4) | b );
	return return_value;
}

int
OLSRHNAGenerator::add_association_write_handler(const String &conf, Element *e, void *, ErrorHandler * errh)
{
	OLSRHNAGenerator* me = (OLSRHNAGenerator *) e;
	IPAddress network_addr;
	IPAddress netmask;
	int res = cp_va_parse(conf, me, errh,
	                      cpIPPrefix, "the network address that HNA should advertise", &network_addr, &netmask,
	                      0);
	if ( res < 0 )
		return res;
	_Association association;
	association.network_addr = network_addr;
	association.netmask = netmask;
	me->_fixedAssociations.push_back(association);

	return 0;
}


void
OLSRHNAGenerator::add_handlers()
{
	add_write_handler("add_association", add_association_write_handler, (void *)0);
}

#include <click/vector.cc>

CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRHNAGenerator);

