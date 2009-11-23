//TED 20404: Created

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "olsr_local_if_infobase.hh"
#include <click/vector.hh>

CLICK_DECLS

OLSRLocalIfInfoBase::OLSRLocalIfInfoBase()
{
}


OLSRLocalIfInfoBase::~OLSRLocalIfInfoBase()
{
}


int
OLSRLocalIfInfoBase::configure(Vector<String> &conf, ErrorHandler *errh)
{
        EtherAddress eth;
        IPAddress ip;
        int before = errh->nerrors();
        for (int i=0; i<conf.size(); i++) {
                // if (!cp_ethernet_address (conf[i],&eth,this)) errh->error("argument %d should be EthnernetAddress", i+1);
                //  else _localinterfaceEtherSet.push_back (eth);i++;
                if (!cp_ip_address (conf[i],&ip,this))
                        errh->error("argument %d should be IPAddress", i+1);
                else
                        _localinterfaceIPSet.push_back (ip);
        }
        // for (int i=0; i<_localinterfaceEtherSet.size();i++) click_chatter ("eth%d: %s",i,_localinterfaceEtherSet[i].unparse().c_str());
        // for (int i=0; i<_localinterfaceIPSet.size();i++) click_chatter ("IP%d: %s",i,_localinterfaceIPSet[i].unparse().c_str());

        return (errh->nerrors() != before ? -1 : 0);
}

IPAddress
OLSRLocalIfInfoBase::get_iface_addr(int i)
{
        if (i>_localinterfaceIPSet.size())
                return IPAddress();
        else
                return (_localinterfaceIPSet[i]);
}


IPAddress
OLSRLocalIfInfoBase::get_main_IPaddress()
{
        if (! _localinterfaceIPSet.empty() )
                return (_localinterfaceIPSet[0]);
        return IPAddress();
}


int
OLSRLocalIfInfoBase::get_number_ifaces()
{
        if (! _localinterfaceIPSet.empty() )
                return _localinterfaceIPSet.size();
        return 0;
}

int
OLSRLocalIfInfoBase::get_index(IPAddress local_iface_addr)
{
        int index=-1;
        for (int i=0; i<_localinterfaceIPSet.size(); i++)
                if (_localinterfaceIPSet[i]==local_iface_addr) {
                        index=i;
                        break;
                }
        return index;
}

Vector<IPAddress> *
OLSRLocalIfInfoBase::get_local_ifaces_addr()
{
        return (&_localinterfaceIPSet);
}

/*EtherAddress
OLSRLocalIfInfoBase::get_interfaceEther(int i)
{
 if (i>_localinterfaceEtherSet.size()) return EtherAddress();
  else return (_localinterfaceEtherSet[i]);
}
 
 
EtherAddress
OLSRLocalIfInfoBase::get_main_addressEther()
{
  if (! _localinterfaceEtherSet.empty() ) return (_localinterfaceEtherSet[0]);
  return EtherAddress();
}
 
 
int
OLSRLocalIfInfoBase::get_sizeEther(){
  if (! _localinterfaceEtherSet.empty() ) return _localinterfaceEtherSet.size();
  return 0;
}
 
 
Vector<EtherAddress> 
OLSRLocalIfInfoBase::get_all_local_interfacesEther()
{
  return (_localinterfaceEtherSet);
}
*/
// generate Vector template instance
#include <click/vector.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class Vector<IPAddress::push_back>
;
#endif


CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRLocalIfInfoBase);

