//TED 190404: Created

#ifndef OLSR_COMPACT_ASSOCIATION_INFOBASE_HH
#define OLSR_COMPACT_ASSOCIATION_INFOBASE_HH

#include <click/element.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/timer.hh>
#include "ippair.hh"
#include "click_olsr.hh"

CLICK_DECLS

class OLSRRoutingTable;

typedef Vector <IPPair> CompactSet;
typedef CompactSet::iterator CompactEntry;

class OLSRCompactAssociationInfoBase: public Element{
public:



  OLSRCompactAssociationInfoBase();
  ~OLSRCompactAssociationInfoBase();

  const char* class_name() const { return "OLSRCompactAssociationInfoBase"; }
  OLSRCompactAssociationInfoBase *clone() const { return new OLSRCompactAssociationInfoBase(); }
  const char *port_count() const  { return "0/0"; }
  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void uninitialize();

  void add(IPAddress network_addr, IPAddress netmask);
  void remove(IPAddress network_addr, IPAddress netmask);
  void print_compact_set();
  CompactSet *get_compact_set();


private:
  
  CompactSet *_compactSet;
};


CLICK_ENDDECLS
#endif
