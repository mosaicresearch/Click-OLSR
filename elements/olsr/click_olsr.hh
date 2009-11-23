/*
TED 040304: Created
Loosely based on click_aodv.hh from Colorado AODV Click implementation
*/

#ifndef CLICK_OLSR_HH
#define CLICK_OLSR_HH

#include <click/ipaddress.hh>
#include <click/glue.hh>
#include <click/vector.hh>
#include <click/string.hh>
#include <click/ipaddress.hh>
//#include <netinet/in.h>

//#define debug

// OLSR specific constants 
// as suggested by RFC3626  

CLICK_DECLS

//Number of allowed interfaces
#define OLSR_NO_OF_INTERFACES 1

//Scaling factor for validity time calculation
//const float OLSR_C = 0.0625;
const int OLSR_C_us = 62500;

#define OLSR_HELLO_INTV 2
#define OLSR_REF_INTV 2
#define OLSR_TC_INTV 5
#define OLSR_MID_INTV 5
#define OLSR_HNA_INTV 5

//Message Types
#define OLSR_HELLO_MESSAGE 1
#define OLSR_TC_MESSAGE    2
#define OLSR_MID_MESSAGE   3
#define OLSR_HNA_MESSAGE   4

//Link Types
#define OLSR_UNSPEC_LINK 0
#define OLSR_ASYM_LINK   1
#define OLSR_SYM_LINK    2
#define OLSR_LOST_LINK   3

//Neighbor Types
#define OLSR_NOT_NEIGH 0
#define OLSR_SYM_NEIGH 1
#define OLSR_MPR_NEIGH 2

//Link Hysteresis
#define OLSR_HYST_THRESHOLD_HIGH 0.8
#define OLSR_HYST_THRESHOLD_LOW  0.3
#define OLSR_HYST_SCALING        0.5

//Willingness
#define OLSR_WILL_NEVER   0
#define OLSR_WILL_LOW     1
#define OLSR_WILL_DEFAULT 3
#define OLSR_WILL_HIGH    6
#define OLSR_WILL_ALWAYS  7

//Misc Constants
#define OLSR_TC_REDUNDANCY 0
#define OLSR_MPR_COVERAGE  1
// const struct timeval OSLR_MAXJITTER =  OLSR_HELLO_INTERVAL / 4;               //never used

#define OLSR_PORT 698
const uint8_t OLSR_WILLINGNESS = OLSR_WILL_DEFAULT;
const uint16_t OLSR_MAX_SEQNUM = 65535;
//IPv4
#define OLSR_MINIMUM_PACKET_LENGTH 16 

/// == mvhaen ====================================================================================================
#define MIN_MPR 2
/// == !mvhaen ===================================================================================================


//OLSR Packet Header
struct olsr_pkt_hdr{
  uint16_t pkt_length;
  uint16_t pkt_seq;
};

//OLSR Message Header
struct olsr_msg_hdr{
  uint8_t msg_type;
  uint8_t vtime;
  uint16_t msg_size;
  in_addr originator_address;
  uint8_t ttl;
  uint8_t hop_count;
  uint16_t msg_seq;
};

//Hello message header
struct olsr_hello_hdr{
  uint16_t reserved;
  uint8_t htime;
  uint8_t willingness;
};

//Link message header
struct olsr_link_hdr{
  uint8_t link_code;
  uint8_t reserved;
  uint16_t link_msg_size;
};

//TC message header
struct olsr_tc_hdr{
  uint16_t ansn;
  uint16_t reserved;
};


//Wrappers
struct pkt_hdr_info{
  int pkt_length;
  int pkt_seq;
};

struct msg_hdr_info{
  int msg_type;
  int vtime_a;  //highest four bits in the vtime field
  int vtime_b;  //lowest four bits in the vitme field
  struct timeval validity_time;
  int msg_size;
  IPAddress originator_address;
  int ttl;
  int hop_count;
  int msg_seq;
};

struct hello_hdr_info{
  int htime_a; //highest four bits in htime field
  int htime_b; //lowest four bits in htime field
  int willingness;
};

struct link_hdr_info{
  int neigh_type;
  int link_type;
  int link_msg_size;
};

struct tc_hdr_info{
  int ansn;
};


//Data structure tuples
struct duplicate_data{
  IPAddress D_addr;
  int D_seq_num;
  int D_retransmitted;
  Vector<IPAddress> D_iface_list;
  struct timeval D_time;
};

struct link_data{
  IPAddress L_local_iface_addr;
  IPAddress L_neigh_iface_addr;
  IPAddress _main_addr;
  struct timeval L_SYM_time;
  struct timeval L_ASYM_time;
  struct timeval L_time;
};

struct neighbor_data{
  IPAddress N_neigh_main_addr;
  int N_status;
  int N_willingness;
};

struct twohop_data{
  IPAddress N_neigh_main_addr;
  IPAddress N_twohop_addr;
  struct timeval N_time;
};

struct mpr_selector_data{
  IPAddress MS_main_addr;
  struct timeval MS_time;
};

struct topology_data{
  IPAddress T_dest_addr;
  IPAddress T_last_addr;
  int T_seq;
  struct timeval T_time;
};

struct interface_data{
  IPAddress I_iface_addr;
  IPAddress I_main_addr;
  struct timeval I_time;
};

struct association_data{
  IPAddress A_gateway_addr;
  IPAddress A_network_addr;
  IPAddress A_netmask;
  struct timeval A_time;
};

//Routing table entry

struct rtable_entry{
  IPAddress R_dest_addr;
  IPAddress R_next_addr;
  int R_dist;
  IPAddress R_iface_addr;
};


CLICK_ENDDECLS

#endif
