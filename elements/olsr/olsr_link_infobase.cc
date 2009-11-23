//TED 160404: Created
//Partially based on grid/linktable - implementation

#include <click/config.h>
#include <click/confparse.hh>
#include "olsr_link_infobase.hh"
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/bighashmap.hh>
#include "ippair.hh"
#include "click_olsr.hh"

CLICK_DECLS

OLSRLinkInfoBase::OLSRLinkInfoBase()
		: _timer(this)
{
}


OLSRLinkInfoBase::~OLSRLinkInfoBase()
{
}


int
OLSRLinkInfoBase::configure (Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_parse(conf, this, errh,
	                cpElement, "NeighborInfoBase element", &_neighborInfo,
	                cpElement, "InterfaceInfoBase element", &_interfaceInfo,
	                cpElement, "Duplicate Set element", &_duplicateSet,
	                cpElement, "Routing Table Element", &_routingTable,
	                cpElement, "TC generator element", &_tcGenerator, 0) < 0)
		return -1;
	return 0;
}


int
OLSRLinkInfoBase::initialize(ErrorHandler *)
{
	_timer.initialize(this);
	_linkSet = new LinkSet();		//ok new
	return 0;
}

void OLSRLinkInfoBase::uninitialize()
{
	delete _linkSet;
}

void
OLSRLinkInfoBase::run_timer(Timer *)
{
	struct timeval now, next_timeout, time;
	bool neighbor_removed = false;
	bool mpr_selector_removed=false;
	bool neighbor_downgraded = false;
	click_gettimeofday(&now);
	next_timeout = make_timeval(0,0);
	IPAddress neighbor;
//	neighbor_data *neighbor_entry;
	HashMap<IPAddress, IPAddress> links_removed_obj;
	HashMap<IPAddress, IPAddress> *links_removed = &links_removed_obj;
	HashMap<IPAddress, IPAddress> links_downgraded_obj;
	HashMap<IPAddress, IPAddress> *links_downgraded = &links_downgraded_obj;

	//find expired links and remove them
	if (! _linkSet->empty() )
	{
		for (LinkSet::iterator iter = _linkSet->begin(); iter != _linkSet->end(); iter++)
		{
			link_data *data = (link_data *)iter.value();
			//store the main address of the node to which this was a link
			neighbor = data->_main_addr; //_interfaceInfo->get_main_address(data->L_neigh_iface_addr);	
			if (data->L_time <= now)
			{
				links_removed->insert(data->L_neigh_iface_addr, neighbor);
				click_chatter("link %s <--> %s expired | %d %d\n", data->L_local_iface_addr.unparse().c_str(), data->L_neigh_iface_addr.unparse().c_str(), data->L_time.tv_sec, data->L_time.tv_usec);
				remove_link(data->L_local_iface_addr, data->L_neigh_iface_addr);
			}
			else if (data->L_SYM_time <= now)
			{
				links_downgraded->insert(data->L_neigh_iface_addr, neighbor);
// 				neighbor_data* nbr_entry = _neighborInfo->find_neighbor(neighbor);
// 				if (nbr_entry->N_status == OLSR_SYM_NEIGH || nbr_entry->N_status == OLSR_MPR_NEIGH)
// 				{
				click_chatter("link %s <--> %s no longer symmetric | %d %d\n", data->L_local_iface_addr.unparse().c_str(), data->L_neigh_iface_addr.unparse().c_str(), data->L_SYM_time.tv_sec, data->L_SYM_time.tv_usec);
//					neighbor = _interfaceInfo->get_main_address(data->L_neigh_iface_addr);
// 					nbr_entry->N_status=OLSR_NOT_NEIGH;
// 					neighbor_downgraded = true;
// 				}
			}
		}
	}

	//remove neighbors that have no more links
	if (! links_removed->empty())
	{
		neighbor_removed = true;
		for( HashMap<IPAddress, IPAddress>::iterator iter = links_removed->begin(); iter != links_removed->end(); iter++)
		{
			bool other_links_left = false;
			for (LinkSet::iterator link = _linkSet->begin(); link != _linkSet->end(); link++)
			{
				link_data *data = (link_data *)link.value();
				if (_interfaceInfo->get_main_address(data->L_neigh_iface_addr) == iter.value() && data->L_SYM_time > now)
				{
					other_links_left = true;
				}
			}
			if (!other_links_left) {
				_neighborInfo->remove_neighbor(iter.value());
				neighbor_removed = true;
				if (_neighborInfo->find_mpr_selector (iter.value()))
				{
					_neighborInfo->remove_mpr_selector(iter.value());
					mpr_selector_removed=true;
				}
			}
		}
	}

	//downgrade neighbors that have no more symmetric links
	if (! links_downgraded->empty() ) {
		neighbor_downgraded = true;
		for( HashMap<IPAddress, IPAddress>::iterator iter = links_downgraded->begin(); iter != links_downgraded->end(); iter++)
		{
			bool sym_link_left = false;
			for (LinkSet::iterator link = _linkSet->begin(); link != _linkSet->end(); link++)
			{
				link_data *data = (link_data *)link.value();
				if (_interfaceInfo->get_main_address(data->L_neigh_iface_addr) == iter.value() && data->L_SYM_time > now)
				{
					sym_link_left = true;
				}
			}
			if (!sym_link_left) {
				neighbor_data* nbr_entry = _neighborInfo->find_neighbor(iter.value());
				nbr_entry->N_status=OLSR_NOT_NEIGH;
				neighbor_downgraded = true;
				if (_neighborInfo->find_mpr_selector (iter.value()))
				{
					_neighborInfo->remove_mpr_selector(iter.value());
					mpr_selector_removed=true;
				}
			}
		}
	}

	//find next link to expire
	if (! _linkSet->empty() )
	{
		for (LinkSet::iterator iter = _linkSet->begin(); iter != _linkSet->end(); iter++)
		{
			link_data *data = (link_data *)iter.value();

			neighbor = data->_main_addr;

			if (_neighborInfo->find_neighbor(neighbor)->N_status == OLSR_SYM_NEIGH)
			{
				time = data->L_SYM_time;
			}
			else
			{
				time = data->L_time;
			}
			if (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0)
			{
				next_timeout = time;
			}
			if ( time < next_timeout )
			{
				next_timeout = time;
			}
		}
	}

	if (mpr_selector_removed) _tcGenerator->notify_mpr_selector_changed();
	if (! (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0) )
		_timer.schedule_at(next_timeout);    //set timer
	if (neighbor_removed || neighbor_downgraded)
	{
		_neighborInfo->compute_mprset();
		_routingTable->compute_routing_table();
	}
}


link_data *
OLSRLinkInfoBase::add_link(IPAddress local_addr, IPAddress neigh_addr, timeval time)
{
	IPPair ippair=IPPair(local_addr, neigh_addr);;
	struct link_data *data;
	data = new struct link_data;		//memory freed in remove

	data->L_local_iface_addr = local_addr;
	data->L_neigh_iface_addr = neigh_addr;
	data->L_time = time;
	click_chatter("link %s <--> %s insert | %d %d\n", data->L_local_iface_addr.unparse().c_str(), data->L_neigh_iface_addr.unparse().c_str(), data->L_time.tv_sec, data->L_time.tv_usec);

	if (_linkSet->empty())
		_timer.schedule_at(time);
	if (_linkSet->insert(ippair, data) ) {
		return data;
	}
	return 0;
}


link_data*
OLSRLinkInfoBase::find_link(IPAddress local_addr, IPAddress neigh_addr)
{
	if (! _linkSet->empty() )
	{
		IPPair ippair = IPPair(local_addr, neigh_addr);
		link_data *data = (link_data *) _linkSet->find(ippair);

		if (! data == 0 )
			return data;
	}
	return 0;
}


bool
OLSRLinkInfoBase::update_link(IPAddress local_addr, IPAddress neigh_addr, struct timeval sym_time, struct timeval asym_time, struct timeval time)
{
	link_data *data;
	data = find_link(local_addr, neigh_addr);
	if (! data == 0 )
	{
		data->L_SYM_time = sym_time;
		data->L_ASYM_time = asym_time;
		data->L_time = time;

		click_chatter("link %s <--> %s updating| %d %d\n", data->L_local_iface_addr.unparse().c_str(), data->L_neigh_iface_addr.unparse().c_str(), data->L_time.tv_sec, data->L_time.tv_usec);

		_timer.schedule_now();
		return true;
	}
	return false;
}


void
OLSRLinkInfoBase::remove_link(IPAddress local_addr, IPAddress neigh_addr)
{
	IPPair ippair = IPPair(local_addr, neigh_addr);
	link_data *ptr=(link_data*) _linkSet->find(ippair);
	
	click_chatter("link %s <--> %s removing| %d %d\n", ptr->L_local_iface_addr.unparse().c_str(), ptr->L_neigh_iface_addr.unparse().c_str(), ptr->L_time.tv_sec, ptr->L_time.tv_usec);
	
// 	_interfaceInfo->remove_interfaces_from(neigh_addr);

	_linkSet->remove(ippair);
	delete ptr;

	//reset the packet seq num from this interface, node might be down
	_duplicateSet->remove_packet_seq(neigh_addr);

}

HashMap<IPPair, void*> *
OLSRLinkInfoBase::get_link_set()
{
	return _linkSet;
}


void
OLSRLinkInfoBase::print_link_set()
{
	if (! _linkSet->empty() )
	{
		for (LinkSet::iterator iter = _linkSet->begin(); iter != _linkSet->end(); iter++)
		{
			link_data *data = (link_data *) iter.value();
			click_chatter("link:\n");
			click_chatter("\tlocal_iface: %s\n", data->L_local_iface_addr.unparse().c_str());
			click_chatter("\tneigh_iface: %s\n", data->L_neigh_iface_addr.unparse().c_str());
			click_chatter("\tL_SYM_time: %d\n", data->L_SYM_time.tv_sec);
			click_chatter("\tL_ASYM_time: %d\n", data->L_ASYM_time.tv_sec);
			click_chatter("\tL_time: %d\n", data->L_time.tv_sec);
		}
	}
	else
	{
		click_chatter("LinkSet is empty\n");
	}
}



#include <click/bighashmap.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<IPPair, void *>;
template class HashMap<IPPair, void *>::iterator;
#endif

CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRLinkInfoBase);

