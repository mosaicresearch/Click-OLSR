//TED 190404: Created

#ifndef OLSR_LOCAL_IF_INFOBASE_HH
#define OLSR_LOCAL_IF_INFOBASE_HH

#include <click/element.hh>
#include <click/etheraddress.hh>
#include <click/vector.hh>

CLICK_DECLS

class OLSRLocalIfInfoBase: public Element
{
public:

        OLSRLocalIfInfoBase();
        ~OLSRLocalIfInfoBase();

    	const char *class_name() const		{ return "OLSRLocalIfInfoBase"; }
        OLSRLocalIfInfoBase *clone() const 	{ return new OLSRLocalIfInfoBase(); }
  	const char *port_count() const  	{ return "0/0"; }

        int configure(Vector<String> &conf, ErrorHandler *errh);

        //  EtherAddress get_main_addressEther();
        //  int get_sizeEther();
        //  EtherAddress get_interfaceEther(int i);
        //  Vector <EtherAddress> get_all_local_interfacesEther();

        IPAddress get_main_IPaddress();
        int get_number_ifaces();
        IPAddress get_iface_addr(int i);
        int get_index (IPAddress local_iface_addr);
        Vector <IPAddress> * get_local_ifaces_addr();

private:
        //  typedef Vector <EtherAddress> LocalInterfaceEtherSet;
        typedef Vector <IPAddress> LocalInterfaceIPSet;

        //  class LocalInterfaceEtherSet _localinterfaceEtherSet;
        LocalInterfaceIPSet _localinterfaceIPSet;

};


CLICK_ENDDECLS
#endif
