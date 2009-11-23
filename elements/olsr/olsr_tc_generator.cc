//TED 070504: Created

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/udp.h>
#include <clicknet/ether.h>
#include <click/ipaddress.hh>
#include <click/router.hh>
#include <click/vector.hh>
#include <click/bighashmap.hh>
#include "olsr_tc_generator.hh"
#include "olsr_neighbor_infobase.hh"
#include "click_olsr.hh"

CLICK_DECLS

OLSRTCGenerator::OLSRTCGenerator()
		: _timer(this)
{
}


OLSRTCGenerator::~OLSRTCGenerator()
{
}


int
OLSRTCGenerator::configure(Vector<String> &conf, ErrorHandler *errh)
{
	bool add_tc_msg=false;
	bool mpr_full_link_state=false;
	bool full_link_state=false;
	int res = cp_va_parse(conf, this, errh,
	                      cpInteger, "TC sending interval (msec)", &_period,
	                      cpInteger, "Topology Holding Time (msec)",&_top_hold_time,
	                      cpElement, "Neighbor InfoBase element", &_neighborInfo,
	                      cpIPAddress, "my main IPAddress", &_myIP,
	                      cpOptional, cpKeywords,
	                      "ADDITIONAL_TC",cpBool,"send addtional TC messages?",&add_tc_msg,
	                      "MPR_FULL_LINK_STATE", cpBool, "send full link state information", &mpr_full_link_state,
	                      "FULL_LINK_STATE", cpBool, "enable sending TC packets even when a node is not an MPR", &full_link_state,
	                      cpEnd);
	_additional_TC_msg=add_tc_msg;
	_mpr_full_link_state=mpr_full_link_state;
	_full_link_state=full_link_state;
	if ( res < 0 )
		return res;
	if ( _period <= 0 )
		return errh->error("period must be greater than 0");
	return res;
}


int
OLSRTCGenerator::initialize(ErrorHandler *)
{

	_timer.initialize(this);

	_vtime = compute_vtime();
	_end_of_validity_time = make_timeval(0,0);
	_node_is_mpr = false;
	_ansn = 1;
	_last_msg_sent_at = make_timeval(0,0);
	return 0;
}

void
OLSRTCGenerator::run_timer(Timer *)
{
	if (_node_is_mpr)
	{
		output(0).push(generate_tc());
		int period=(int)(_period*.95+(random() % (_period/10)));
		//click_chatter ("emitting other tc after %d ms\n",period);
		_timer.reschedule_after_msec(period);
	}
	else
	{
		Packet *p = generate_tc_when_not_mpr();
		if (p != 0)
			output(0).push(p);
	}
}

void
OLSRTCGenerator::set_node_is_mpr(bool value)
{
	bool node_was_mpr = _node_is_mpr;
	_node_is_mpr = value;
	if (_node_is_mpr)
	{
		_end_of_validity_time = make_timeval(0,0);
		int delay=(int) (random() % (_period/20));
		_timer.schedule_after_msec(delay);
		click_chatter ("node %s has become MPR, emitting tc after %d ms\n",_myIP.unparse().c_str(),delay);
	}
	else if (node_was_mpr && !_node_is_mpr)
	{
		struct timeval now;
		click_gettimeofday(&now);
		_end_of_validity_time.tv_sec=_period/1000; //period in msec
		_end_of_validity_time.tv_usec=_period*1000;
		_end_of_validity_time += now;
		int delay=(int) (random() % (_period/20));
		_timer.schedule_after_msec(delay);
		click_chatter ("is not mpr emitting tc_not_mpr after %d ms\n",_myIP.unparse().c_str(),delay);
	}

}


Packet *
OLSRTCGenerator::generate_tc()
{
	//   uint64_t cycles=click_get_cycles();
	HashMap<IPAddress, void *> *advertise_set = 0;
	int num_to_advertise = 0;

	if (_mpr_full_link_state)
	{
		HashMap<IPAddress, void *> *neighbor_set = _neighborInfo->get_neighbor_set();
		if (! neighbor_set->empty())
		{
			for (HashMap<IPAddress, void *>::iterator iter = neighbor_set->begin(); iter != neighbor_set->end(); iter++)
			{
				neighbor_data *nbr = (neighbor_data *) iter.value();
				if (nbr->N_status == OLSR_SYM_NEIGH || nbr->N_status == OLSR_MPR_NEIGH)
				{
					num_to_advertise++;
				}

			}
		}
	}
	else
	{
		advertise_set = _neighborInfo->get_mpr_selector_set();
		num_to_advertise = advertise_set->size();
	}


	int packet_size = sizeof(olsr_pkt_hdr) + sizeof(olsr_msg_hdr) + sizeof(olsr_tc_hdr) + num_to_advertise*sizeof(in_addr);
	int headroom = sizeof(click_ether) + sizeof(click_ip) + sizeof(click_udp);
	int tailroom = 0;
	WritablePacket *packet = Packet::make(headroom,0,packet_size, tailroom);
	if ( packet == 0 )
	{
		click_chatter( "in %s: cannot make packet!", name().c_str());
	}
	memset(packet->data(), 0, packet->length());
	//   packet->set_perfctr_anno(cycles);
	olsr_pkt_hdr *pkt_hdr = (olsr_pkt_hdr *) packet->data();
	pkt_hdr->pkt_length = 0; //added in OLSRForward
	pkt_hdr->pkt_seq = 0; //added in OLSRForward

	struct timeval now;
	click_gettimeofday(&now);
	packet->set_timestamp_anno(now);


	olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) (pkt_hdr + 1);
	msg_hdr->msg_type = OLSR_TC_MESSAGE;
	msg_hdr->vtime = _vtime;
	msg_hdr->msg_size = htons(sizeof(olsr_msg_hdr) + sizeof(olsr_tc_hdr)+ num_to_advertise*sizeof(in_addr));
	msg_hdr->originator_address = _myIP.in_addr();
	msg_hdr->ttl = 255;  //TC messages should diffuse into entire network
	msg_hdr->hop_count = 0;
	msg_hdr->msg_seq = 0; //added in OLSRForward element

	olsr_tc_hdr *tc_hdr = (olsr_tc_hdr *) (msg_hdr + 1);
	tc_hdr->ansn = htons( get_ansn() );
	tc_hdr->reserved = 0;

	if ( num_to_advertise != 0 )
	{
		if (_mpr_full_link_state)
		{
			in_addr * address = (in_addr *) (tc_hdr + 1);
			for (HashMap<IPAddress, void *>::iterator iter = advertise_set->begin(); iter != advertise_set->end(); iter++)
			{
				neighbor_data *nbr = (neighbor_data *) iter.value();
				if (nbr->N_status == OLSR_SYM_NEIGH || nbr->N_status == OLSR_MPR_NEIGH)
				{
					*address = nbr->N_neigh_main_addr.in_addr();
					address++;
				}
			}
		}
		else
		{
			in_addr *address = (in_addr *) (tc_hdr + 1);
			for (HashMap<IPAddress, void *>::iterator iter = advertise_set->begin(); iter != advertise_set->end(); iter++)
			{
				mpr_selector_data *mpr_selector = (mpr_selector_data *) iter.value();
				*address = mpr_selector->MS_main_addr.in_addr();
				address++;
			}
		}
	}

	/// == mvhaen ====================================================================================================
	// some experimental stuff. maybe finish this later on. Basically has an MPR advertise all its symmetrical neighbors instead of all the MPR selectors
	/*
	num_sym_nbr=0;
	if (! neighbor_set->empty())
	{
		
		for (HashMap<IPAddress, void *>::iterator iter = neighbor_set->begin(); iter; iter++)
		{
			neighbor_data *nbr = (neighbor_data *) iter.value();
			if (nbr->N_status == OLSR_SYM_NEIGH || nbr->N_status == OLSR_MPR_NEIGH) {
				in_addr *address;
				if (num_sym_nbr == 0) {
					address = (in_addr *) (tc_hdr + 1);
				} else {
					address++;
				}
				*address = nbr->N_neigh_main_addr.in_addr();
				num_sym_nbr++;
			}
			
		}
	}
	*/
	/// ==!mvhaen ===================================================================================================



	return packet;
}



Packet *
OLSRTCGenerator::generate_tc_when_not_mpr()
{
	//   uint64_t cycles=click_get_cycles();
	struct timeval now;
	click_gettimeofday(&now);
	if (now <= _end_of_validity_time)
	{
		int packet_size = sizeof(olsr_pkt_hdr) + sizeof(olsr_msg_hdr) + sizeof(olsr_tc_hdr) ;
		int headroom = sizeof(click_ether) + sizeof(click_ip) + sizeof(click_udp);
		WritablePacket *packet = Packet::make(headroom,0,packet_size, 0);
		if ( packet == 0 )
		{
			click_chatter( "in %s: cannot make packet!", name().c_str());
		}
		memset(packet->data(), 0, packet->length());
		//   packet->set_perfctr_anno(cycles);
		olsr_pkt_hdr *pkt_hdr =(olsr_pkt_hdr *) packet->data();
		pkt_hdr->pkt_length = 0; //added in OLSRForward
		pkt_hdr->pkt_seq = 0; //added in OLSRForward

		olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) (pkt_hdr + 1);
		msg_hdr->msg_type = OLSR_TC_MESSAGE;
		msg_hdr->vtime = _vtime;
		msg_hdr->msg_size = htons(sizeof(olsr_msg_hdr) + sizeof(olsr_tc_hdr));
		msg_hdr->originator_address = _myIP.in_addr();
		msg_hdr->ttl = 255;  //TC messages should diffuse into entire network
		msg_hdr->hop_count = 0;
		msg_hdr->msg_seq = 0; //added in OLSRForward element

		olsr_tc_hdr *tc_hdr = (olsr_tc_hdr *) (msg_hdr + 1);
		tc_hdr->ansn = htons( get_ansn() );
		tc_hdr->reserved = 0;

		_timer.reschedule_after_msec(_period);
		return packet;
	}
	return 0;
}


void
OLSRTCGenerator::increment_ansn()
{
	_ansn++;
}

void
OLSRTCGenerator::notify_mpr_selector_changed()
{
	_ansn++;
	if (_additional_TC_msg) _timer.schedule_now();
}


uint16_t
OLSRTCGenerator::get_ansn()
{
	return _ansn;
}


uint8_t
OLSRTCGenerator::compute_vtime()
{
	uint8_t return_value = 0;
	int t = _top_hold_time*1000; //top_hold_time in msec -> t in µsec
	//_top_hold_time.tv_usec+_top_hold_time.tv_sec*1000000; //fixpoint -> calculation in µsec

	int b=0;
	while ((t / OLSR_C_us) >= (1<<(b+1)))
		b++;
	int a=(((16*t/OLSR_C_us)>>b)-16);
	int value=(OLSR_C_us *(16+a)*(1<<b))>>4;
	if (value<t) a++;
if (a==16) {b++; a=0;}

	if ( (a <= 15 && a >= 0) && (b <= 15 && b >= 0) )
		return_value = ((a << 4) | b );
	return return_value;
}




CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRTCGenerator);


