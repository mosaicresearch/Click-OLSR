//TED 220404: Created
#include <click/config.h>
#include <click/confparse.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include "ippair.hh"
#include "click_olsr.hh"
#include "olsr_compact_association_info_base.hh"

CLICK_DECLS

OLSRCompactAssociationInfoBase::OLSRCompactAssociationInfoBase()
{
  _compactSet = new CompactSet;
}


OLSRCompactAssociationInfoBase::~OLSRCompactAssociationInfoBase()
{
  delete _compactSet;
}


int
OLSRCompactAssociationInfoBase::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if ( cp_va_parse( conf, this, errh,
		    0) < 0 )
    return -1;
	  
  return 0;
}


int
OLSRCompactAssociationInfoBase::initialize(ErrorHandler *)
{
  return 0;
}

void OLSRCompactAssociationInfoBase::uninitialize()
{
}

void
OLSRCompactAssociationInfoBase::add(IPAddress network_addr, IPAddress netmask)
{

	// make sure that the network_addr ends with zeros
	IPPair ippair(network_addr & netmask, netmask);

	printf("inserting %s\n", ippair._from.unparse_with_mask(ippair._to).c_str());	
	
	// find where we can insert our new entry. 
	CompactSet::iterator iter = _compactSet->begin();
	while ((iter != _compactSet->end()) && ( htonl(ippair._from.addr()) > htonl(iter->_from.addr()) )) iter++;

	// insert the entry
	CompactSet::iterator curr = _compactSet->insert(iter, ippair);
	CompactSet::iterator prev = curr-1;
	CompactSet::iterator next = curr+1;
	
	
	
	// check wether the previous or next entry can be used to compact.
	
	bool madeChange = false;
	
	do {

		if ( curr != _compactSet->begin() && curr->_to == prev->_to && curr->_from.matches_prefix(prev->_from, IPAddress::make_prefix(prev->_to.mask_to_prefix_len()-1)) ) {
//			printf("inserting %s: first if branch: matches %s\n", ippair._from.unparse_with_mask(ippair._to).c_str(), prev->_from.unparse_with_mask(prev->_to).c_str());
			curr = _compactSet->insert(prev, IPPair(curr->_from & IPAddress::make_prefix(prev->_to.mask_to_prefix_len()-1), IPAddress::make_prefix(prev->_to.mask_to_prefix_len()-1)));
			next = curr+1;	
			next = _compactSet->erase(next);				
			next = curr+1;			
			next = _compactSet->erase(next);					
			curr = next-1;
			prev = curr-1;
			madeChange = true;
		} else if ( curr != _compactSet->end() && next != _compactSet->end() && curr->_to == next->_to && curr->_from.matches_prefix(next->_from, IPAddress::make_prefix(next->_to.mask_to_prefix_len()-1)) ) {
//			printf("inserting %s: second if branch: matches %s\n", ippair._from.unparse_with_mask(ippair._to).c_str(), next->_from.unparse_with_mask(next->_to).c_str());
			curr = _compactSet->insert(curr, IPPair(curr->_from & IPAddress::make_prefix(next->_to.mask_to_prefix_len()-1), IPAddress::make_prefix(next->_to.mask_to_prefix_len()-1)));
			next = curr+1;		
			next = _compactSet->erase(next);	
			next = curr+1;	
			next = _compactSet->erase(next);			
			curr = next-1;
			prev = curr-1;
			madeChange = true;
		} else {
			madeChange = false;
		} 
	} while(madeChange && _compactSet->size() > 1);	

}


void
OLSRCompactAssociationInfoBase::remove(IPAddress network_addr, IPAddress netmask)
{

	// make sure that the network_addr ends with zeros
	IPPair ippair(network_addr & netmask, netmask);

	// find where we can insert our new entry. 
	CompactSet::iterator iter = _compactSet->begin();
	while ( (iter != _compactSet->end()) && !(ippair._from.matches_prefix(iter->_from , iter->_to)) ) iter++;
	
	IPPair left;
	IPPair right;
	IPAddress new_mask;
	uint32_t bit;
	
  	printf("erasing %s %s\n", iter->_from.unparse_with_mask(iter->_to).c_str(), network_addr.unparse_with_mask(netmask).c_str());
  	if (*iter == ippair) _compactSet->erase(iter);
	else {
		while (true) {
			new_mask = IPAddress::make_prefix(iter->_to.mask_to_prefix_len()+1);
			bit = 1 << (32 - (iter->_to.mask_to_prefix_len()+1));
			left._from = iter->_from;
			left._to = new_mask;
			right._from = IPAddress(ntohl(htonl(iter->_from.addr()) | bit));
			right._to = new_mask;

  			printf("splitting into %s %s\n", left._from.unparse_with_mask(left._to).c_str(), right._from.unparse_with_mask(right._to).c_str());									
			if (right == ippair) {
				iter = _compactSet->insert(iter, left);
				_compactSet->erase(iter+1);
				break;
			} else if (left == ippair) {
				iter = _compactSet->insert(iter, right);
				_compactSet->erase(iter+1);
				break;
			} else {
				iter = _compactSet->insert(iter, right);
				iter = _compactSet->insert(iter, left);
				iter = _compactSet->erase(iter+2);			
			}
			
			if (ippair._from.matches_prefix(right._from, right._to)) {
				iter = iter-1;
				printf("chose %s\n", iter->_from.unparse_with_mask(iter->_to).c_str());	
			} else if (ippair._from.matches_prefix(left._from, left._to)) {
				iter = iter-2;
				printf("chose %s\n", iter->_from.unparse_with_mask(iter->_to).c_str());	
			} else {
				printf("error int olsr_compact_association_info_base.cc\n");
				exit(0);
			}
			
		}
	}

}

CompactSet *
OLSRCompactAssociationInfoBase::get_compact_set() {
	return _compactSet;
}

void
OLSRCompactAssociationInfoBase::print_compact_set()
{
  if (! _compactSet->empty() ){
    click_chatter("Compact Set:\n");
    for ( CompactSet::iterator iter = _compactSet->begin(); iter != _compactSet->end(); iter++){
      click_chatter("\tNetwork: %s\n", iter->_from.unparse_with_mask(iter->_to).c_str());
    }
  }
  else
    click_chatter("Compact set: empty\n");
}

#include <click/vector.cc>
#include <click/bighashmap.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<IPPair, void *>;
template class Vector<IPPair>;
#endif

CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRCompactAssociationInfoBase);

