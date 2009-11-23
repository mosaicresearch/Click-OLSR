//TED 070504: Created

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/udp.h>
#include <clicknet/ether.h>
#include <click/ipaddress.hh>
#include <click/router.hh>
#include <click/vector.hh>

#include "olsr_mid_generator.hh"

#include "click_olsr.hh" 
#include "olsr_local_if_infobase.hh"
CLICK_DECLS

OLSRMIDGenerator::OLSRMIDGenerator()
  : _timer(this)
{
}


OLSRMIDGenerator::~OLSRMIDGenerator()
{
}


int
OLSRMIDGenerator::configure(Vector<String> &conf, ErrorHandler *errh)
{
  int res = cp_va_parse(conf, this, errh,
			cpInteger, "MID sending interval (msec)", &_period,
			cpInteger, "MID Holding Time", &_mid_hold_time,
			cpElement, "local Interface Information base", &_localIfInfoBase
			, 0);
  if ( res < 0 )
    return res;
  if ( _period <= 0 )
    return errh->error("period must be greater than 0");
  
  return res;
}


int
OLSRMIDGenerator::initialize(ErrorHandler *)
{
if (_localIfInfoBase->get_number_ifaces() > 1) //if just one interface mid messages must not be send
{
  _timer.initialize(this);
  _timer.schedule_now(); // Send OLSR MID messages periodically
  _vtime = compute_vtime();
  }
    return 0;
}


void
OLSRMIDGenerator::run_timer(Timer *)
{
    output(0).push(generate_mid());
    _timer.reschedule_after_msec(_period);
}



Packet *
OLSRMIDGenerator::generate_mid()
{
  Vector <IPAddress> * localInterfaceList = _localIfInfoBase->get_local_ifaces_addr();
  int packet_size = sizeof(olsr_pkt_hdr) + sizeof(olsr_msg_hdr) + (_localIfInfoBase->get_number_ifaces()-1)*sizeof(in_addr);
  int headroom = sizeof(click_ether) + sizeof(click_ip) + sizeof(click_udp);
  int tailroom = 0; 
  WritablePacket *packet = Packet::make(headroom,0,packet_size, tailroom);
  if ( packet == 0 ){
      click_chatter( "in %s: cannot make packet!", name().c_str());
  }
  memset(packet->data(), 0, packet->length());
   struct timeval tv;
  click_gettimeofday(&tv);
  packet->set_timestamp_anno(tv);
  
  olsr_pkt_hdr *pkt_hdr = (olsr_pkt_hdr *) packet->data();
  pkt_hdr->pkt_length = 0; //added in OLSRForward 
  pkt_hdr->pkt_seq = 0; //added in OLSRForward
  olsr_msg_hdr *msg_hdr = (olsr_msg_hdr *) (pkt_hdr + 1);
  msg_hdr->msg_type = OLSR_MID_MESSAGE;
  msg_hdr->vtime = _vtime; 
 
  msg_hdr->msg_size = htons(sizeof(olsr_msg_hdr) + (_localIfInfoBase->get_number_ifaces()-1)*sizeof(in_addr));
  msg_hdr->originator_address = (*localInterfaceList)[0].in_addr();			//first element is Main IP Address=Originator Address
  msg_hdr->ttl = 255;  //MID messages should diffuse into entire network
  msg_hdr->hop_count = 0; 
  msg_hdr->msg_seq = 0; //added in OLSRForward element   
  in_addr *address = (in_addr *) (msg_hdr + 1);    
  for (int i=1;i<localInterfaceList->size();i++) //i=0 -> mainAddress, not to be advertised!
   { 
     *address = (*localInterfaceList)[i].in_addr();
      address++;
    }
  return packet;
}

uint8_t 
OLSRMIDGenerator::compute_vtime()
{
  uint8_t return_value = 0;
  int t = _mid_hold_time*1000; //_mid_hold_time in msec, t in µsec
  //_mid_hold_time.tv_usec+_mid_hold_time.tv_sec*1000000; //fixpoint -> calculation in µsec
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

EXPORT_ELEMENT(OLSRMIDGenerator);

