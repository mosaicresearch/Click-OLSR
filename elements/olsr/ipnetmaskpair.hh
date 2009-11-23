//TED 190404: Created
//'Stolen' from grid/linktable - implementation 

#ifndef IPPAIR_HH
#define IPPAIR_HH

#include <click/ipaddress.hh>
#include <click/bighashmap.hh>

CLICK_DECLS

//class IPAddress;

class IPNetmaskPair {
public:

  IPAddress _to;
  IPAddress _to_netmask;
  IPAddress _from;
  IPAddress _from_netmask;

  IPPair() : _to(), _to_netmask("255.255.255.255"), _from(), _from_netmask( { }

  IPPair(IPAddress from, IPAddress to) {
      _to = to;
      _from = from;
  }

  bool contains(IPAddress foo) {
    return ((foo == _to) || (foo == _from));
  }
  bool other(IPAddress foo) { return ((_to == foo) ? _from : _to); }

};

inline bool
operator==(IPPair one, IPPair other)
{
  return (other._to == one._to && other._from == one._from);
}

inline unsigned
hashcode(IPPair p) 
{
  return hashcode(p._to) + hashcode(p._from);
}


CLICK_ENDDECLS
#endif
