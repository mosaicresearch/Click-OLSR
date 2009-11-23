//TED 260404 :Created
//Based on grid/hello

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>
#include "olsr_hello_generator.hh"
#include <click/vector.hh>
#include <click/bighashmap.hh>
#include "ippair.hh"
#include "olsr_neighbor_infobase.hh"
#include "olsr_link_infobase.hh"
#include "click_olsr.hh"
#include "olsr_packethandle.hh"


CLICK_DECLS

OLSRHelloGenerator::OLSRHelloGenerator()
		: _timer( this ), _node_willingness( OLSR_WILLINGNESS )
{
}

OLSRHelloGenerator::~OLSRHelloGenerator()
{
}

int
OLSRHelloGenerator::configure( Vector<String> &conf, ErrorHandler *errh )
{
	int res = cp_va_parse( conf, this, errh,
	                       cpInteger, "period (msec)", &_period,
	                       cpInteger, "Neighbor Hold time", &_neighbor_hold_time,
	                       cpElement, "Element LinkInfoBase", &_linkInfoBase,
	                       cpElement, "Element NeighborInfoBase", &_neighborInfoBase,
	                       cpElement, "Element InterfaceInfoBase", &_interfaceInfoBase,
	                       cpElement, "Element OLSRForward", &_forward,
	                       cpIPAddress, "Interface IPAddress", &_local_iface_addr,
	                       cpIPAddress, "Main IPAddress of node", &_myMainIP,
	                       cpKeywords,
	                       "WILLINGNESS", cpInteger, "Willingness of the node", &_node_willingness
	                       , 0 );
	if ( res < 0 )
		return res;
	if ( _period <= 0 )
		return errh->error( "period must be greater than 0" );
	return res;
}

int
OLSRHelloGenerator::initialize( ErrorHandler * )
{
	_timer.initialize( this );
	uint32_t start = ( random() % _period );
	click_chatter ( "hello start %d\n", start );
	_timer.schedule_after_msec( start ); // Send OLSR HELLO periodically
	_htime = compute_htime();
	_vtime = compute_vtime();
	click_chatter ( "_neighbor_hold_time = %d | _vtime = %d\n", _neighbor_hold_time, _vtime );
	return 0;
}

void
OLSRHelloGenerator::run_timer(Timer *)
{
	output( 0 ).push( generate_hello() );
	int period = ( int ) ( _period * .95 + ( random() % ( _period / 10 ) ) );
	//click_chatter ("emitting other hello after %d ms\n",period);
	_timer.reschedule_after_msec( period );
}

Packet *
OLSRHelloGenerator::generate_hello()
{
	//  uint64_t cycles=click_get_cycles();
	struct timeval now;
	click_gettimeofday( &now );
	int number_link_codes = 0;
	int number_addresses = 0;
	HashMap<uint8_t, Vector<IPAddress> > neighbor_interfaces;
	HashMap<IPAddress, bool> neighbor_included;
	HashMap<IPPair, void *> *link_set = _linkInfoBase->get_link_set();
	Vector <IPAddress> adv_link_addr;
	if ( ! link_set->empty() )
	{
		for ( HashMap<IPPair, void*>::iterator iter = link_set->begin(); iter != link_set->end(); iter++ )
		{
			struct link_data *data;
			data = ( link_data * ) iter.value();
			if ( ( data->L_local_iface_addr == _local_iface_addr ) && ( data->L_time >= now ) )
			{
				uint8_t link_code = get_link_code( data, now );
				//click_chatter ("link with link_code: %d\n",link_code);
				IPAddress main_address = _interfaceInfoBase->get_main_address( data->L_neigh_iface_addr ); //get main address of links remote interface
				neighbor_included.insert ( main_address, true );
				Vector <IPAddress> * neighbor_iface = neighbor_interfaces.findp( link_code );
				if ( neighbor_iface == 0 )
				{
					adv_link_addr.clear();
					adv_link_addr.push_back( data->L_neigh_iface_addr );
					neighbor_interfaces.insert( link_code, adv_link_addr );
					number_link_codes++;
					number_addresses++;
				}
				else
				{
					neighbor_iface->push_back( data->L_neigh_iface_addr );
					number_addresses++;
				}
			}
		}
	}

	HashMap<IPAddress, void *> *neighborSet = _neighborInfoBase->get_neighbor_set();

	for ( HashMap<IPAddress, void *> ::iterator iter = neighborSet->begin(); iter != neighborSet->end(); iter++ )
	{
		neighbor_data *neighbor = ( neighbor_data * ) iter.value();
		if ( !neighbor_included.findp( neighbor->N_neigh_main_addr ) )
		{
			uint8_t link_code = OLSR_UNSPEC_LINK;
			if ( _neighborInfoBase->find_mpr( neighbor->N_neigh_main_addr ) != 0 )
				link_code = link_code | ( OLSR_MPR_NEIGH << 2 );
			else
			{
				if ( neighbor->N_status == OLSR_SYM_NEIGH )
					link_code = link_code | ( OLSR_SYM_NEIGH << 2 );
				else
					link_code = link_code | ( OLSR_NOT_NEIGH << 2 );
			}

			Vector <IPAddress> * neighbor_iface = neighbor_interfaces.findp( link_code );
			if ( neighbor_iface == 0 )
			{
				adv_link_addr.clear();
				adv_link_addr.push_back( neighbor->N_neigh_main_addr );
				neighbor_interfaces.insert( link_code, adv_link_addr );
				number_link_codes++;
				number_addresses++;
			}
			else
			{
				neighbor_iface->push_back( neighbor->N_neigh_main_addr );
				number_addresses++;
			}
		}
	}

	int packet_size = sizeof( olsr_pkt_hdr ) + sizeof( olsr_msg_hdr ) + sizeof( olsr_hello_hdr ) + number_link_codes * sizeof ( olsr_link_hdr ) + number_addresses * sizeof ( in_addr );
	int headroom = sizeof( click_ether ) + sizeof( click_ip ) + sizeof( click_udp );
	int tailroom = 0;
	WritablePacket *packet = Packet::make( headroom, 0, packet_size, tailroom );
	if ( packet == 0 )
	{
		click_chatter( "in %s: cannot make packet!", name().c_str() );
	}
	memset( packet->data(), 0, packet->length() );
	//packet->set_perfctr_anno(cycles);
	struct timeval tv;
	click_gettimeofday( &tv );
	packet->set_timestamp_anno( tv );

	olsr_pkt_hdr *pkt_hdr = ( olsr_pkt_hdr * ) packet->data();
	pkt_hdr->pkt_length = 0;
	pkt_hdr->pkt_seq = 0; //added in OLSRAddPaqSeq

	olsr_msg_hdr *msg_hdr = ( olsr_msg_hdr * ) ( pkt_hdr + 1 );
	msg_hdr->msg_type = OLSR_HELLO_MESSAGE;
	msg_hdr->vtime = _vtime;
	msg_hdr->msg_size = htons( sizeof( olsr_msg_hdr ) + sizeof( olsr_hello_hdr ) );
	msg_hdr->originator_address = _myMainIP.in_addr();
	msg_hdr->ttl = 1;  //hello packets MUST NOT be forwarded
	msg_hdr->hop_count = 0;


	olsr_hello_hdr *hello_hdr = ( olsr_hello_hdr * ) ( msg_hdr + 1 );
	hello_hdr->reserved = 0;
	hello_hdr->htime = _htime;
	hello_hdr->willingness = _node_willingness;

	if ( neighbor_interfaces.empty() )
	{ //if no neighbors, broadcast willingness
		click_chatter( "OLSRHelloGenerator, no neighbors, broadcasting willingness anyway\n" );
	}

	else
	{ // there are neighbors, generate link messages
		in_addr *address = 0;
		olsr_link_hdr *link_hdr;
		int number_in_neighbor_interfaces = 0; //rather unelegant solution to problem with pointers,
		//avoids overwriting addresses in hello message

		for ( HashMap<uint8_t, Vector <IPAddress> >::iterator iter = neighbor_interfaces.begin(); iter != neighbor_interfaces.end(); iter++ )
		{
			//if (packet->put(sizeof(olsr_link_hdr))==0) click_chatter ("put 1 resulted in 0\n");

			if ( number_in_neighbor_interfaces == 0 )
			{
				link_hdr = ( olsr_link_hdr * ) ( hello_hdr + 1 );
			}
			else
				link_hdr = ( olsr_link_hdr * ) ( address + 1 );

			link_hdr->link_code = iter.key();
			link_hdr->reserved = 0;
			link_hdr->link_msg_size = htons( sizeof( olsr_link_hdr ) );
			Vector<IPAddress> addr_vector = iter.value();
			int size = addr_vector.size();
			//click_chatter ("adding %d addresses with linkcode %d \n",size, iter.key());
			for ( int i = 0; i < size; i++ )
			{
				// click_chatter ("\t i=%d: %s\n",i,addr_vector[i].unparse().c_str());
				if ( i == 0 )
					address = ( in_addr * ) ( link_hdr + 1 );
				else
					address = ( in_addr * ) ( address + 1 );

				*address = addr_vector[ i ].in_addr();
				link_hdr->link_msg_size = htons( ntohs( link_hdr->link_msg_size ) + sizeof( in_addr ) );
			}

			msg_hdr->msg_size = htons( ntohs( msg_hdr->msg_size ) + ntohs( link_hdr->link_msg_size ) );
			number_in_neighbor_interfaces++;
		}
	}
	msg_hdr_info msg_info = OLSRPacketHandle::get_msg_hdr_info( packet, sizeof( olsr_pkt_hdr ) );
	pkt_hdr->pkt_length = htons( msg_info.msg_size + sizeof( olsr_pkt_hdr ) );
	pkt_hdr->pkt_seq = 0; //added in AddPacketSeq (for each interface)
	msg_hdr->msg_seq = htons ( _forward->get_msg_seq() );	//this also increases the sequence number;
	return packet;
}


void OLSRHelloGenerator::notify_mpr_change()
{
	_timer.schedule_now();
}

uint8_t
OLSRHelloGenerator::get_link_code( struct link_data *data, timeval now )
{
	uint8_t link_code;
	if ( data->L_SYM_time >= now )
		link_code = OLSR_SYM_LINK;
	else if ( data->L_ASYM_time >= now && data->L_SYM_time < now )
		link_code = OLSR_ASYM_LINK;
	else
		link_code = OLSR_LOST_LINK;

	IPAddress neigh_iface_addr = data->L_neigh_iface_addr.addr();
	IPAddress neigh_main_addr = _interfaceInfoBase->get_main_address( neigh_iface_addr );

	if ( _neighborInfoBase->find_mpr( neigh_main_addr ) != 0 )
		link_code = link_code | ( OLSR_MPR_NEIGH << 2 );
	else
	{
		neighbor_data *neigh_data = _neighborInfoBase->find_neighbor( neigh_main_addr );
		if ( ! neigh_data == 0 )
		{
			if ( neigh_data->N_status == OLSR_SYM_NEIGH )
				link_code = link_code | ( OLSR_SYM_NEIGH << 2 );
			else
				link_code = link_code | ( OLSR_NOT_NEIGH << 2 );
		}
		else
			click_chatter( "neighbor_type not set myIP: %s\t neighbor main %s\t neigh iface %s\tlinkcode=%d\t\n", _myMainIP.unparse().c_str(), neigh_main_addr.unparse().c_str(), neigh_iface_addr.unparse().c_str(), link_code );
	}
	return link_code;
}


uint8_t
OLSRHelloGenerator::compute_htime()
{
	//_period in milliseconds
	//OLSR_C_us in µsec
	uint8_t return_value = 0;
	int t = _period * 1000;
	int b = 0;
	while ( ( t / OLSR_C_us ) >= ( 1 << ( b + 1 ) ) )
		b++;
	int a = ( ( ( 16 * t / OLSR_C_us ) >> b ) - 16 );
	int value = ( OLSR_C_us * ( 16 + a ) * ( 1 << b ) ) >> 4;
	if ( value < t ) a++;
if ( a == 16 ) {b++; a = 0;}
	if ( ( a <= 15 && a >= 0 ) && ( b <= 15 && b >= 0 ) )
		return_value = ( ( a << 4 ) | b );
	return return_value;
}

uint8_t
OLSRHelloGenerator::compute_vtime()
{
	uint8_t return_value = 0;
	int t = _neighbor_hold_time * 1000; //msec->µsec
	//_neighbor_hold_time.tv_usec+_neighbor_hold_time.tv_sec*1000000; //fixpoint -> calculation in µsec
	int b = 0;
	while ( ( t / OLSR_C_us ) >= ( 1 << ( b + 1 ) ) )
		b++;
	int a = ( ( ( 16 * t / OLSR_C_us ) >> b ) - 16 );
	int value = ( OLSR_C_us * ( 16 + a ) * ( 1 << b ) ) >> 4;
	if ( value < t ) a++;
if ( a == 16 ) {b++; a = 0;}
	if ( ( a <= 15 && a >= 0 ) && ( b <= 15 && b >= 0 ) )
		return_value = ( ( a << 4 ) | b );
	return return_value;
}

/// == mvhaen ====================================================================================================
void
OLSRHelloGenerator::set_period(int period)
{
	_period = period;
	_htime = compute_htime();
	click_chatter ( "_period = %d | _htime = %d\n", _period, _htime );
}

void
OLSRHelloGenerator::set_neighbor_hold_time(int neighbor_hold_time)
{
	_neighbor_hold_time = neighbor_hold_time;
	_vtime = compute_vtime();
	click_chatter ( "_neighbor_hold_time = %d | _vtime = %d\n", _neighbor_hold_time, _vtime );
}

int
OLSRHelloGenerator::set_period_handler(const String &conf, Element *e, void *, ErrorHandler * errh)
{
	OLSRHelloGenerator* me = (OLSRHelloGenerator *) e;
	int new_period;
	int res = cp_va_parse( conf, me, errh,cpInteger, "period (msec)", &new_period, 0 );
	if ( res < 0 )
		return res;
	if ( new_period <= 0 )
		return errh->error( "period must be greater than 0" );
	me->set_period(new_period);
	return res;
}

int
OLSRHelloGenerator::set_neighbor_hold_time_handler(const String &conf, Element *e, void *, ErrorHandler * errh)
{
	OLSRHelloGenerator* me = (OLSRHelloGenerator *) e;
	int new_nbr_hold_time;
	int res = cp_va_parse( conf, me, errh, cpInteger, "Neighbor Hold time", &new_nbr_hold_time, 0 );	
	if ( res < 0 )
		return res;
	me->set_neighbor_hold_time(new_nbr_hold_time);
	return res;
}

void
OLSRHelloGenerator::add_handlers()
{
	add_write_handler("set_period", set_period_handler, (void *)0);
	add_write_handler("set_neighbor_hold_time", set_neighbor_hold_time_handler, (void *)0);
}
/// == !mvhaen ===================================================================================================

#include <click/bighashmap.cc>
#include <click/vector.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class Vector<IPAddress>;
template class HashMap<IPPair, void *>;
template class HashMap<IPPair, void *>::iterator;
#endif


CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRHelloGenerator);

