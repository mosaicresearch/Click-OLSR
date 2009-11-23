#ifndef JOIN_HH
#define JOIN_HH
#include <click/element.hh>

CLICK_DECLS

class Join : public Element {
  
 public:
  
  Join();
  ~Join();
  
  const char *class_name() const		{ return "Join"; }
  const char *processing() const		{ return AGNOSTIC; }
  const char *port_count() const  		{ return "1-/1"; }  

  Packet * simple_action(Packet *);
  
};


CLICK_ENDDECLS

#endif
