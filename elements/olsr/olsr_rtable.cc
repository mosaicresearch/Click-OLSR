//TED 220404: Created

#include <click/config.h>
#include <click/error.hh>
#include <click/router.hh>
#include <click/confparse.hh>
#include <click/ipaddress.hh>
#include "ippair.hh"
#include "olsr_rtable.hh"
#include "click_olsr.hh"

// #define profiling

CLICK_DECLS

OLSRRoutingTable::OLSRRoutingTable()
{
	_visitorInfo = 0;
}

OLSRRoutingTable::~OLSRRoutingTable()
{}


int
OLSRRoutingTable::configure( Vector<String> &conf, ErrorHandler *errh )
{
	if ( cp_va_parse( conf, this, errh,
	                  cpElement, "Neighbor InfoBase Element", &_neighborInfo,
	                  cpElement, "Link InfoBase Element", &_linkInfo,
	                  cpElement, "Topology InfoBase Element", &_topologyInfo,
	                  cpElement, "Interface InfoBase", &_interfaceInfo,
	                  cpElement, "local Interface Infobase", &_localIfaces,
	                  cpElement, "association Infobase", &_associationInfo,
	                  cpElement, "Routing table", &_linearIPlookup,
	                  cpIPAddress, "own IPAddress", &_myIP,
	                  cpKeywords,
	                  "SUBNET_MASK", cpIPAddress, "subnet mask", &_myMask,
	                  "VISITOR_INFO", cpElement, "visitor Infobase", &_visitorInfo,
	                  0 ) < 0 )
		return -1;

	_errh = errh;
	return 0;
}


int
OLSRRoutingTable::initialize( ErrorHandler *errh )
{
	if ( !_neighborInfo )
		return errh->error( "Could not find Neighbor InfoBase" );
	else if ( !_linkInfo )
		return errh->error( "Could not find Link InfoBase" );
	else if ( !_topologyInfo )
		return errh->error( "Could not find Topology InfoBase" );
	if ( !_interfaceInfo )
		return errh->error( "Could not find Interface InfoBase" );

	return 0;
}


void
OLSRRoutingTable::uninitialize()
{}


void
OLSRRoutingTable::print_routing_table()
{
	struct timeval now;
	click_gettimeofday( &now );
	click_chatter( "%f | %s | %s\n", Timestamp( now ).doubleval(), _myIP.unparse().c_str(), _linearIPlookup->dump_routes().c_str() );
}


void
OLSRRoutingTable::compute_routing_table()
{
	HashMap<IPAddress, void *> *neighbor_set = _neighborInfo->get_neighbor_set();
	HashMap<IPPair, void *> *link_set = _linkInfo->get_link_set();
	HashMap<IPPair, void*> *twohop_set = _neighborInfo->get_twohop_set();
	HashMap<IPPair, void*> *topology_set = _topologyInfo->get_topology_set();
	HashMap<IPAddress, void*> *interface_set = _interfaceInfo->get_interface_set();
	HashMap<IPPair, void*> *association_set = _associationInfo->get_association_set();
	IPAddress netmask32( "255.255.255.255" );
	IPRoute newiproute;
	const IPRoute *iproute = 0;

#ifdef profiling

	click_chatter ( "%f | %s | CALCULATING ROUTINGTABLE\n", timeval2double( now ), _myIP.unparse().c_str() );
	click_chatter ( "%f | %s | Neighbors: %d\ttwohopset: %d\ttopology_set: %d\tlink_set: %d\t interface_set %d\t\n", timeval2double( now ), _myIP.unparse().c_str(), neighbor_set->size(), twohop_set->size(), topology_set->size(), link_set->size(), interface_set->size() );
	struct timeval start;
	struct timeval now;
	double time_dif;
	click_gettimeofday( &start );
#endif



	/*
	_neighborInfo->print_neighbor_set();
	_neighborInfo->print_mpr_set();
	_neighborInfo->print_mpr_selector_set();
	_neighborInfo->print_twohop_set();
	_topologyInfo->print_topology();
	*/


	//RFC ch 10 'Routing table calculation' - step 1 - delete all entries
	_linearIPlookup->clear();
#ifdef profiling

	click_gettimeofday( &now );
	time_dif = ( now.tv_sec - start.tv_sec ) + ( now.tv_usec - start.tv_usec ) / 1E6;
	click_chatter ( "%f | %s | time part 1 %f", timeval2double( now ), _myIP.unparse().c_str(), time_dif );
	click_gettimeofday( &start );
#endif


	//step 2 - adding routes to symmetric neighbors
	for ( HashMap<IPAddress, void*>::iterator iter = neighbor_set->begin(); iter != neighbor_set->end(); iter++ ) {
		neighbor_data *neighbor = ( neighbor_data * ) iter.value();
		if ( neighbor->N_status == OLSR_SYM_NEIGH ) {
			link_data * link = 0;
			link_data *lastlinktoneighbor = 0;
			bool neigh_main_addr_added = false;
			for ( HashMap<IPPair, void *>::iterator i = link_set->begin(); i != link_set->end(); i++ ) {
				link = ( link_data * ) i.value();
				IPAddress neigh_main_addr = _interfaceInfo->get_main_address( link->L_neigh_iface_addr );
				if ( neigh_main_addr == neighbor->N_neigh_main_addr ) {
					lastlinktoneighbor = link;
					newiproute.addr = link->L_neigh_iface_addr;
					newiproute.mask = netmask32;
					newiproute.gw = link->L_neigh_iface_addr;
					newiproute.port = _localIfaces->get_index( link->L_local_iface_addr );
					newiproute.extra = 1;
					_linearIPlookup->add_route( newiproute, false, 0, _errh );
					// 					_linearIPlookup->add_route( link->L_neigh_iface_addr,
					// 					                            netmask32,
					// 					                            link->L_neigh_iface_addr,
					// 					                            _localIfaces->get_index( link->L_local_iface_addr ),
					// 					                            1,
					// 					                            _errh );

					if ( neigh_main_addr == link->L_neigh_iface_addr )
						neigh_main_addr_added = true;
					//	  click_chatter ("adding neighbor %s\n",link->L_neigh_iface_addr.unparse().c_str());
				}
			}
			if ( ! neigh_main_addr_added && lastlinktoneighbor != 0 ) { //(lastlinktoneighbor != 0) should never fail
				newiproute.addr = _interfaceInfo->get_main_address( lastlinktoneighbor->L_neigh_iface_addr );
				newiproute.mask = netmask32;
				newiproute.gw = link->L_neigh_iface_addr;
				newiproute.port = _localIfaces->get_index( link->L_local_iface_addr );
				newiproute.extra = 1;
				_linearIPlookup->add_route( newiproute, false, 0, _errh );
				// 				_linearIPlookup->add_route( _interfaceInfo->get_main_address( lastlinktoneighbor->L_neigh_iface_addr ),
				// 				                            netmask32,
				// 				                            lastlinktoneighbor->L_neigh_iface_addr,
				// 				                            _localIfaces->get_index( lastlinktoneighbor->L_local_iface_addr ),
				// 				                            1,
				// 				                            _errh );

				// 	click_chatter ("adding MAIN neighbor %s\n",lastlinktoneighbor->L_neigh_iface_addr.unparse().c_str());
			}
		}
	}

#ifdef profiling

	click_gettimeofday( &now );
	time_dif = ( now.tv_sec - start.tv_sec ) + ( now.tv_usec - start.tv_usec ) / 1E6;
	click_chatter ( "%f | %s | time part 2 %f", timeval2double( now ), _myIP.unparse().c_str(), time_dif );
	click_gettimeofday( &start );
#endif

	//step 3 - adding routes to twohop neighbors
	for ( HashMap<IPPair, void*>::iterator iter = twohop_set->begin(); iter != twohop_set->end(); iter++ ) {
		twohop_data *twohop = ( twohop_data * ) iter.value();
		if ( twohop->N_twohop_addr != _myIP ) {
			//do not add neighbors, do not add twohop neighbors that have already been added
			if ( !_linearIPlookup->lookup_iproute( twohop->N_twohop_addr ) ) {
				if ( (iproute = _linearIPlookup->lookup_iproute( twohop->N_neigh_main_addr )) ) {
					neighbor_data *neighbor = ( neighbor_data * ) neighbor_set->find( twohop->N_neigh_main_addr );
					if ( neighbor->N_willingness > OLSR_WILL_NEVER ) {
						newiproute.addr = twohop->N_twohop_addr;
						newiproute.mask = netmask32;
						newiproute.gw = iproute->gw;
						newiproute.port = iproute->port;
						newiproute.extra = 2;
						_linearIPlookup->add_route( newiproute, false, 0, _errh );
// 						_linearIPlookup->add_route( twohop->N_twohop_addr,
// 						                            netmask32,
// 						                            entry.gw,
// 						                            entry.output,
// 						                            2,
// 						                            _errh );
						// 	  click_chatter ("adding twohopneighbor %s\n",new_entry->R_dest_addr.unparse().c_str());
					}
				}
			}
		}
	}
#ifdef profiling

	click_gettimeofday( &now );
	time_dif = ( now.tv_sec - start.tv_sec ) + ( now.tv_usec - start.tv_usec ) / 1E6;
	click_chatter ( "%f | %s | time part 3 %f", timeval2double( now ), _myIP.unparse().c_str(), time_dif );
	click_gettimeofday( &start );
#endif
	//step 4 - adding nodes with distance greater than 2
	for ( int h = 2; h >= 2; h++ ) {
		bool route_added = false;
		for ( HashMap<IPPair, void*>::iterator iter = topology_set->begin(); iter != topology_set->end(); iter++ ) {
			topology_data *topology = ( topology_data * ) iter.value();
			//if (_routingTable->find(topology->T_dest_addr) == 0 ){
			if ( ! _linearIPlookup->lookup_iproute( topology->T_dest_addr ) ) {
				//there is no entry in the routing table for this address
				if ( (iproute = _linearIPlookup->lookup_iproute( topology->T_last_addr )) ) {
					if ( iproute->extra == h ) {
						// there is an entry for the last but one address in the route
						newiproute.addr = topology->T_dest_addr;
						newiproute.mask = netmask32;
						newiproute.gw = iproute->gw;
						newiproute.port = iproute->port;
						newiproute.extra = h + 1;
						_linearIPlookup->add_route( newiproute, false, 0, _errh );
// 						_linearIPlookup->add_route( topology->T_dest_addr,
// 						                            IPAddress( "255.255.255.255" ),
// 						                            entry.gw,
// 						                            entry.output,
// 						                            h + 1,
// 						                            _errh );
						// click_chatter("route added to %s, distance: %d\n", new_entry->R_dest_addr.unparse().c_str(),  new_entry->R_dist);
						route_added = true;
					}
				}
			}
		}
		if ( ! route_added )
			break; //if no new nodes are added in an iteration, stop looking for new routes
	}

#ifdef profiling

	click_gettimeofday( &now );
	time_dif = ( now.tv_sec - start.tv_sec ) + ( now.tv_usec - start.tv_usec ) / 1E6;
	click_chatter ( "time part 4 %f", time_dif );
	click_gettimeofday( &start );
#endif

	//step 5 - add routes to other nodes' interfaces that have not already been added
	for ( HashMap<IPAddress, void *>::iterator iter = interface_set->begin(); iter != interface_set->end(); iter++ ) {
		interface_data *interface = ( interface_data * ) iter.value();
		//rtable_entry *entry = (rtable_entry *) _routingTable->find(interface->I_main_addr);
		if ( _linearIPlookup->lookup_iproute( interface->I_main_addr )  ) {
			if (! (iproute = _linearIPlookup->lookup_iproute( interface->I_iface_addr ) )) {

				newiproute.addr = interface->I_iface_addr;
				newiproute.mask = netmask32;
				newiproute.gw = iproute->gw;
				newiproute.port = iproute->port;
				newiproute.extra = iproute->extra;
				_linearIPlookup->add_route( newiproute, false, 0, _errh );
// 				_linearIPlookup->add_route( interface->I_iface_addr,
// 				                            IPAddress( "255.255.255.255" ),
// 				                            entry.gw,
// 				                            entry.output,
// 				                            entry.dist,
// 				                            _errh );
				// 	click_chatter ("adding other ifaces %s\n",new_entry->R_dest_addr.unparse().c_str());
			}
		}
	}
	if ( _visitorInfo ) {
		_visitorInfo->clear();
		//	click_chatter ("%s | checking for new visitors!!!!\n",_myIP.unparse().c_str());
		for ( OLSRLinearIPLookup::IPRouteTableIterator iter = _linearIPlookup->begin() ; iter != _linearIPlookup->end(); iter++ ) {
			// click_chatter ("%s | CHECKING if node %s is visiting my network\n",_myIP.unparse().c_str(), _linearIPlookup->_t[index].addr.unparse().c_str());
			if ( iter->mask == netmask32 && !iter->addr.matches_prefix( _myIP, _myMask ) ) { // check if this node is on my subnet
				_visitorInfo->add_tuple( iter->gw, iter->addr, iter->mask, make_timeval( 0, 0 ) );
				timeval now;
				click_gettimeofday( &now );
				click_chatter ( "%f | %s | node %s is visiting my network\n", Timestamp( now ).doubleval(), _myIP.unparse().c_str(), iter->addr.unparse().c_str() );
			}
		}
	}

	//step 6 - add routes to entries in the association table
	/// @TODO I don't feel that this code is 100% compliant, this definitely needs to be looked at and possibly rewritten (see p. 54 of the RFC)
	for ( HashMap<IPPair, void *>::iterator iter = association_set->begin(); iter != association_set->end(); iter++ ) {
		association_data *association = ( association_data * ) iter.value();
		if ( (iproute = _linearIPlookup->lookup_iproute( association->A_gateway_addr )) ) {
			if ( !_linearIPlookup->lookup_iproute( association->A_network_addr ) /** @TODO This seems like a weird or. have to check this again!! || _linearIPlookup->lookup_iproute( association->A_network_addr )->gw != iproute->gw */) {
				newiproute.addr = association->A_network_addr;
				newiproute.mask = association->A_netmask;
				newiproute.gw = iproute->gw;
				newiproute.port = iproute->port;
				newiproute.extra = iproute->extra;
				_linearIPlookup->add_route( newiproute, false, 0, _errh );
/*				_linearIPlookup->add_route( association->A_network_addr,
				                            association->A_netmask,
				                            gw_entry.gw,
				                            gw_entry.output,
				                            gw_entry.dist,
				                            _errh );*/
			} else {
				if ( _linearIPlookup->lookup_iproute( association->A_network_addr )->extra > iproute->extra ) {
					_linearIPlookup->update(association->A_network_addr, iproute->gw, iproute->port, iproute->extra);
				}
			}
		}
	}


#ifdef profiling

	click_gettimeofday( &now );
	time_dif = ( now.tv_sec - start.tv_sec ) + ( now.tv_usec - start.tv_usec ) / 1E6;
	click_chatter ( "%f | %s | time part 5 %f", timeval2double( now ), _myIP.unparse().c_str(), time_dif );
	click_chatter ( "%f | %s | Routingtable size: %d\n", timeval2double( now ), _myIP.unparse().c_str(), _routingTable->size() );

#endif

	// #ifdef logging
	//   timeval now;
	//   click_gettimeofday(&now);
	//   click_chatter ("%f | %s | ROUTINGTABLE has been computed \n",timeval2double(now), _myIP.unparse().c_str());
	//   print_routing_table();
	//   //  _neighborInfo->print_mpr_set();
	//   _neighborInfo->print_mpr_selector_set();
	//   _topologyInfo->print_topology();
	//   click_chatter ("\n");
	// #endif

}




#include <click/bighashmap.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<IPAddress, void *>
;
#endif

CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRRoutingTable);

