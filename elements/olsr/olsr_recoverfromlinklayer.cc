
#include <click/config.h>
#include <click/confparse.hh>
#include "olsr_recoverfromlinklayer.hh"
#include "click_olsr.hh"
#include <clicknet/ether.h>
#include <click/etheraddress.hh>

CLICK_DECLS


OLSRRecoverFromLinkLayer::OLSRRecoverFromLinkLayer()
{
}


OLSRRecoverFromLinkLayer::~OLSRRecoverFromLinkLayer()
{
}


int
OLSRRecoverFromLinkLayer::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_parse(conf, this, errh,
	                cpElement, "NeighborInfoBase Element", &_neighborInfoBase,
	                cpElement, "LinkInfoBase Element", &_linkInfoBase,
			cpElement, "InterfaceInfoBase Element", &_interfaceInfoBase,
			cpElement, "TCGenerator Element", &_tcGenerator,
			cpElement, "RoutingTable Element", &_routingTable,
	                cpElement, "OLSR ARP Querier Element", &_arpQuerier,
	                cpIPAddress, "Nodes main IP address", &_myMainIP,
	                0) < 0)
		return -1;
	return 0;
}


int
OLSRRecoverFromLinkLayer::initialize(ErrorHandler *)
{
	return 0;
}

void
OLSRRecoverFromLinkLayer::push(int, Packet *packet)
{
	timeval now;
	click_gettimeofday(&now);

	EtherAddress ether_addr;
	memcpy(ether_addr.data(), packet->data(), 6);

	// do reverse ARP
	IPAddress next_hop_IP = _arpQuerier->lookup_mac(ether_addr);

	if (next_hop_IP == IPAddress("0.0.0.0"))
	{
		output(1).push(packet);		//this case should not occur that much
		return;
	}

	click_chatter("%f | %s | %s | reverse lookup succeeded: the original next hop was %s", Timestamp(now).doubleval(), _myMainIP.unparse().c_str(), __FUNCTION__, next_hop_IP.s().c_str());
	// set the gw as dst (for logging purposes)
	packet->set_dst_ip_anno(next_hop_IP);
	// remove the matching link from the link info base
	IPAddress next_hop_main_IP = _interfaceInfoBase->get_main_address(next_hop_IP);
	_linkInfoBase->remove_link(_myMainIP, next_hop_IP);
	// remove the matching neighbor from the neighbor info base if there are no more links
	bool other_interfaces_left = false;
	bool mpr_selector_removed = true;
	for (OLSRLinkInfoBase::LinkSet::iterator iter = _linkInfoBase->get_link_set()->begin(); iter != _linkInfoBase->get_link_set()->end(); iter++)
	{
		link_data *data = (link_data *)iter.value();
		if (_interfaceInfoBase->get_main_address(data->L_neigh_iface_addr) == next_hop_main_IP)
		{
			other_interfaces_left = true;
			break;

		}
	}
	if (!other_interfaces_left)
	{
		_neighborInfoBase->remove_neighbor(next_hop_main_IP);
		if (_neighborInfoBase->find_mpr_selector (next_hop_main_IP))
		{
			_neighborInfoBase->remove_mpr_selector(next_hop_main_IP);
			mpr_selector_removed=true;
		}
	}
	// make sure that the MPRs and Route Table are updated
	if (mpr_selector_removed) _tcGenerator->notify_mpr_selector_changed();
	_neighborInfoBase->compute_mprset();
	_routingTable->compute_routing_table();
	// now push the packet out through output 0
	output(0).push(packet);
}


CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRRecoverFromLinkLayer);

