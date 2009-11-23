//TED 110504: Created

#include <click/config.h>
#include <click/bighashmap.hh>
#include "click_olsr.hh"
#include "olsr_duplicate_set.hh"

CLICK_DECLS

OLSRDuplicateSet::OLSRDuplicateSet()
  : _timer(this)
{
}


OLSRDuplicateSet::~OLSRDuplicateSet()
{
}


int 
OLSRDuplicateSet::configure(Vector<String> &, ErrorHandler *)
{
  return 0;
}


int 
OLSRDuplicateSet::initialize(ErrorHandler *)
{
  _timer.initialize(this);
  _duplicateSet = new HashMap<DuplicatePair, void *>;	//ok new
  _packetSeqList = new HashMap<IPAddress, int>;		//ok new
  return 0;
}

void OLSRDuplicateSet::uninitialize()
{
 delete _duplicateSet;
 delete _packetSeqList;
}


duplicate_data *
OLSRDuplicateSet::find_duplicate_entry(IPAddress address, int seq_num)
{
  if (! _duplicateSet->empty()){
    DuplicatePair pair = DuplicatePair(address, seq_num);
    duplicate_data *data = (duplicate_data *) _duplicateSet->find(pair);
    return data;
  }
  return 0;
}  


duplicate_data *
OLSRDuplicateSet::add_duplicate_entry(IPAddress address, int seq_num)
{
  DuplicatePair pair = DuplicatePair(address, seq_num);
  duplicate_data *data = new duplicate_data;		//new ok, freed en remove
  data->D_addr = address;
  data->D_seq_num = seq_num;

  if (_duplicateSet->insert(pair, data))
    return data;

  return 0;
}


void
OLSRDuplicateSet::remove_duplicate_entry(IPAddress address, int seq_num)
{
  DuplicatePair pair = DuplicatePair(address, seq_num);
  DuplicatePair *ptr = (DuplicatePair*) _duplicateSet->find(pair); //getting stored generic pointer, to release allocated memory 
  _duplicateSet->remove(pair);
  delete ptr;
}


void
OLSRDuplicateSet::add_packet_seq(IPAddress iface_addr, int seq_num){
  _packetSeqList->insert(iface_addr, seq_num);
}

void
OLSRDuplicateSet::remove_packet_seq(IPAddress iface_addr){
  _packetSeqList->remove(iface_addr);
}

void
OLSRDuplicateSet::update_packet_seq(IPAddress iface_addr, int seq_num){
  HashMap<IPAddress, int>::Pair *pair =_packetSeqList->find_pair(iface_addr);
  pair->value = seq_num;
}

int
OLSRDuplicateSet::find_packet_seq(IPAddress iface_addr){
  int pkt_seq = _packetSeqList->find(iface_addr);
  return pkt_seq;
}


void
OLSRDuplicateSet::run_timer(Timer *)
{
  struct timeval now, next_timeout;
  click_gettimeofday(&now);
  next_timeout = make_timeval(0, 0);

  //find expired duplicate entries and delete them
  if (! _duplicateSet->empty()){
    for (DuplicateSet::iterator iter = _duplicateSet->begin(); iter != _duplicateSet->end(); iter++){
      duplicate_data *entry = (duplicate_data *) iter.value();
      if (entry->D_time <= now){
	remove_duplicate_entry(entry->D_addr, entry->D_seq_num);
      }
    }
  }

  //find next duplicate entry to expire
  if (! _duplicateSet->empty()){
    for (DuplicateSet::iterator iter = _duplicateSet->begin(); iter != _duplicateSet->end(); iter++){
      duplicate_data *entry = (duplicate_data *) iter.value();
      if (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0)
	next_timeout = entry->D_time;
      if ( entry->D_time < next_timeout )
	next_timeout = entry->D_time;
    }
  }

  if (! (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0) )
    _timer.schedule_at(next_timeout);    //set timer

}


#include <click/bighashmap.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<DuplicatePair, void *>;
template HashMap<IPAddress, int>;
#endif

CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRDuplicateSet);

