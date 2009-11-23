//TED 110504: Created
#ifndef OLSR_DUPLICATE_SET_HH
#define OLSR_DUPLICATE_SET_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/bighashmap.hh>

CLICK_DECLS

class DuplicatePair {
public:

  IPAddress _address;
  int _seq_num;

  DuplicatePair() : _address(), _seq_num() { }

  DuplicatePair(IPAddress address, int seq_num) {
      _address = address;
      _seq_num = seq_num;
  }


};

inline bool
operator==(DuplicatePair one, DuplicatePair other)
{
  return (other._address == one._address && other._seq_num == one._seq_num);
}

inline unsigned
hashcode(DuplicatePair p) 
{
  return hashcode(p._address) + hashcode(p._seq_num);
}


class OLSRDuplicateSet: public Element{
public:
  OLSRDuplicateSet();
  ~OLSRDuplicateSet();

  const char* class_name() const { return "OLSRDuplicateSet"; }
  OLSRDuplicateSet *clone() const { return new OLSRDuplicateSet(); }
  const char *port_count() const  { return "0/0"; }
  
  int configure(Vector<String> &conf, ErrorHandler *errh);
  int initialize(ErrorHandler *);
  void uninitialize();

  struct duplicate_data *find_duplicate_entry(IPAddress address, int seq_num);
  struct duplicate_data *add_duplicate_entry(IPAddress address, int seq_num);
  void remove_duplicate_entry(IPAddress address, int seq_num);

  void add_packet_seq(IPAddress iface_addr, int seq_num);
  void remove_packet_seq(IPAddress iface_addr);
  void update_packet_seq(IPAddress iface_addr, int seq_num);
  int find_packet_seq(IPAddress iface_addr);

private:
  typedef HashMap<DuplicatePair, void *> DuplicateSet;
  DuplicateSet *_duplicateSet;
  Timer _timer;
  HashMap<IPAddress, int> *_packetSeqList;

  void run_timer(Timer *);
};

CLICK_ENDDECLS
#endif
