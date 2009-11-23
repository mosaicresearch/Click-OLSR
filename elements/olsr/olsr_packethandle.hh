//TED 150404 : Created
/*
  A static class to handle reading of OLSR packets in the ClickOLSR router 
  implenetation. Based on code used in a Click implementation of AODV by
  The University of Colorado at Boulder, thus the copyright statement below.
*/
/*****************************************************************************
 *  Copyright 2002, Univerity of Colorado at Boulder.                        *
 *                                                                           *
 *                        All Rights Reserved                                *
 *                                                                           *
 *  Permission to use, copy, modify, and distribute this software and its    *
 *  documentation for any purpose other than its incorporation into a        *
 *  commercial product is hereby granted without fee, provided that the      *
 *  above copyright notice appear in all copies and that both that           *
 *  copyright notice and this permission notice appear in supporting         *
 *  documentation, and that the name of the University not be used in        *
 *  advertising or publicity pertaining to distribution of the software      *
 *  without specific, written prior permission.                              *
 *                                                                           *
 *  UNIVERSITY OF COLORADO DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS      *
 *  SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND        *
 *  FITNESS FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL THE UNIVERSITY    *
 *  OF COLORADO BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL         *
 *  DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA       *
 *  OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER        *
 *  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR         *
 *  PERFORMANCE OF THIS SOFTWARE.                                            *
 *                                                                           *
 ****************************************************************************/

#ifndef OLSR_PACKETHANDLE_HH
#define OLSR_PACKETHANDLE_HH

#include <click/element.hh>
#include <clicknet/ip.h>
#include <click/ipaddress.hh>
//#include <math.h>
#include "click_olsr.hh"

CLICK_DECLS


/* OLSR packet handler...
*/
class OLSRPacketHandle
{
public :
        OLSRPacketHandle();
        ~OLSRPacketHandle();

        static struct pkt_hdr_info get_pkt_hdr_info(Packet *);
        static struct msg_hdr_info get_msg_hdr_info(Packet *, int);
        static struct hello_hdr_info get_hello_hdr_info(Packet *, int);
        static struct link_hdr_info get_link_hdr_info(Packet *, int);
        static struct tc_hdr_info get_tc_hdr_info(Packet *, int);
        static void print_packet(Packet *);

private:
        static struct timeval calculate_validity_time(int vtime_a, int vtime_b);

};




inline pkt_hdr_info
OLSRPacketHandle::get_pkt_hdr_info(Packet *_packet)
{
        pkt_hdr_info hdr_info;
        olsr_pkt_hdr *pkt_hdr;

        pkt_hdr = (olsr_pkt_hdr *) _packet->data();

        hdr_info.pkt_length = (int) ntohs(pkt_hdr->pkt_length);
        hdr_info.pkt_seq =  (int) ntohs(pkt_hdr->pkt_seq);

        return hdr_info;
}


inline msg_hdr_info
OLSRPacketHandle::get_msg_hdr_info(Packet *_packet, int offset)
{
        msg_hdr_info hdr_info;
        olsr_msg_hdr *msg_hdr;

        msg_hdr = (olsr_msg_hdr *)( _packet->data() + offset );

        hdr_info.msg_type = (int) msg_hdr->msg_type;
        hdr_info.vtime_a = (int) (msg_hdr->vtime) >> 4;
        hdr_info.vtime_b = (int) (msg_hdr->vtime) & 0x0f;
        hdr_info.validity_time = calculate_validity_time(hdr_info.vtime_a, hdr_info.vtime_b);
        hdr_info.msg_size = (int) ntohs(msg_hdr->msg_size);
        hdr_info.originator_address = IPAddress(msg_hdr->originator_address);
        hdr_info.ttl = (int) msg_hdr->ttl;
        hdr_info.hop_count = (int) msg_hdr->hop_count;
        hdr_info.msg_seq = (int) ntohs(msg_hdr->msg_seq);

        return hdr_info;
}


inline hello_hdr_info
OLSRPacketHandle::get_hello_hdr_info(Packet *_packet, int offset)
{
        hello_hdr_info hdr_info;
        olsr_hello_hdr *hello_header;

        hello_header = (olsr_hello_hdr *)(_packet->data() + offset);

        hdr_info.htime_a = (int) (hello_header->htime) >> 4;
        hdr_info.htime_b = (int) (hello_header->htime) & 0x0f;
        hdr_info.willingness = (int) hello_header->willingness;

        return hdr_info;
}


inline link_hdr_info
OLSRPacketHandle::get_link_hdr_info(Packet *_packet, int offset)
{
        link_hdr_info hdr_info;
        olsr_link_hdr *link_header;

        link_header = (olsr_link_hdr *)(_packet->data() + offset);

        hdr_info.neigh_type = (link_header->link_code) >> 2;
        hdr_info.link_type = (link_header->link_code) & 0x03;
        hdr_info.link_msg_size = ntohs(link_header->link_msg_size);

        return hdr_info;
}


inline tc_hdr_info
OLSRPacketHandle::get_tc_hdr_info(Packet *_packet, int offset)
{
        tc_hdr_info hdr_info;
        olsr_tc_hdr *tc_hdr;

        tc_hdr = (olsr_tc_hdr *)(_packet->data() + offset);

        hdr_info.ansn = (int) ntohs(tc_hdr->ansn);

        return hdr_info;
}


inline void
OLSRPacketHandle::print_packet(Packet *_packet)
{
        pkt_hdr_info hdr_info;
        msg_hdr_info msg_info;

        hdr_info = get_pkt_hdr_info(_packet);
        msg_info = get_msg_hdr_info(_packet, sizeof(olsr_pkt_hdr));

        click_chatter("pkt length: %d", hdr_info.pkt_length);
        click_chatter("pkt seq_no: %d", hdr_info.pkt_seq);

        click_chatter("OLSR type: %d\n", msg_info.msg_type);
        click_chatter("Vtime: %d%d\n", msg_info.vtime_a, msg_info.vtime_b);
        click_chatter("msg size: %d\n", msg_info.msg_size);
        click_chatter("Orig addr: %s\n", msg_info.originator_address.unparse().c_str());
        click_chatter("TTL: %d\n", msg_info.ttl);
        click_chatter("hop_count: %d\n", msg_info.hop_count);
        click_chatter("msg_seq: %d\n", msg_info.msg_seq);

}


/*inline timeval
OLSRPacketHandle::calculate_validity_time(int vtime_a, int vtime_b){
 //calculates validity time as described in OLSR RFC3626 section 3.3.2 
 double part_a, part_b, result;
 struct timeval validity_time;
 
 part_a =  1 + (double) (vtime_a)/16;
 part_b = pow(2, vtime_b);
 result = OLSR_C * part_a * part_b;
 validity_time = double2timeval(result);
 
 return validity_time;
}*/


inline timeval
OLSRPacketHandle::calculate_validity_time(int vtime_a, int vtime_b)
{
        //calculates validity time as described in OLSR RFC3626 section 3.3.2
        int t;
        struct timeval validity_time;

        t=(OLSR_C_us *(16+vtime_a)*(1<<vtime_b))>>4;

        validity_time.tv_sec=(t / 1000000);
        validity_time.tv_usec=(t % 1000000);

        /*  //______________________________
          
            double part_a, part_b, result;
          struct timeval validity_t;
         
          part_a =  1 + (double) (vtime_a)/16;
          part_b = pow(2, vtime_b);
          result = OLSR_C * part_a * part_b;
          validity_t = double2timeval(result);
          
          
          if (validity_time!=validity_t)
          {
          click_chatter ("calculate_validity_time new: vtime_a=%d \t vtime_b=%d \t t=%d\n",vtime_a,vtime_b,t);
          click_chatter ("calculate_validity_time old: vtime_a=%d \t vtime_b=%d \t t=%f\n",vtime_a,vtime_b,result);
           click_chatter ("calc_val_time: valor nuevo: %d %d \nvalor viejo: %d %d\n",validity_time.tv_sec,validity_time.tv_usec,validity_t.tv_sec,validity_t.tv_usec);
          } 
        */

        return validity_time;
}


CLICK_ENDDECLS

#endif
