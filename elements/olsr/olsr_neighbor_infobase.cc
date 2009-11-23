//TED 210404: Created

#include <click/config.h>
#include <click/confparse.hh>
#include "olsr_neighbor_infobase.hh"
#include <click/ipaddress.hh>

#include "ippair.hh"
#include "click_olsr.hh"


//#define profiling
//#define debug
CLICK_DECLS

OLSRNeighborInfoBase::OLSRNeighborInfoBase()
		: _twohop_timer(twohop_expiry_hook, this),
		_mpr_selector_timer(mpr_selector_expiry_hook, this)
{
}


OLSRNeighborInfoBase::~OLSRNeighborInfoBase()
{
}


int
OLSRNeighborInfoBase::configure(Vector<String> &conf, ErrorHandler *errh)
{
	bool add_hello_msg=false;
	bool add_mprs = false;
	if ( cp_va_parse(conf, this, errh,
	                 cpElement, "Routing Table Element", &_routingTable,
	                 cpElement, "TC Generator Element", &_tcGenerator,
	                 cpElement, "Hello Generator Element",&_helloGenerator,
	                 cpElement, "Link Information Base", &_linkInfoBase,
	                 cpElement, "Interface Information Base", &_interfaceInfoBase,
	                 cpIPAddress, "Nodes main IP address", &_myMainIP,
	                 cpOptional,
	                 cpKeywords,"ADDITIONAL_HELLO",cpBool,"send additional hello message?",&add_hello_msg,
	                 cpKeywords,"ADDITIONAL_MPRS",cpBool,"choose additional mprs",&add_mprs,
	                 0) < 0 )

		return -1;
	_additional_hello_message=add_hello_msg;
	_additional_mprs=add_mprs;
	return 0;
}


int
OLSRNeighborInfoBase::initialize(ErrorHandler *)
{
	_neighborSet = new NeighborSet;	//ok->freed in uninitialize
	_twohopSet = new TwoHopSet;		//ok->freed in uninitialize
	_mprSelectorSet = new MPRSelectorSet;//ok->freed in uninitialize
	_mprSet = new MPRSet;		//ok->freed in uninitialize
	_twohop_timer.initialize(this);
	_mpr_selector_timer.initialize(this);
#ifdef profiling_kernel
	_cyclesaccum=0;
	_count=0;
#endif

	return 0;
}

void OLSRNeighborInfoBase::uninitialize()
{
	delete _neighborSet;
	delete _twohopSet;
	delete _mprSelectorSet;
	delete _mprSet;
}


void
OLSRNeighborInfoBase::mpr_selector_expiry_hook(Timer *timer, void *thunk)
{
	OLSRNeighborInfoBase *nib = (OLSRNeighborInfoBase *) thunk;
	bool mpr_selector_removed = false;
	struct timeval now, next_timeout;
	click_gettimeofday(&now);
	next_timeout = make_timeval(0, 0);

	//find expired MPR selectors and delete them
	if (! nib->_mprSelectorSet->empty())
	{
		for (MPRSelectorSet::iterator iter = nib->_mprSelectorSet->begin(); iter != nib->_mprSelectorSet->end(); iter++)
		{
			mpr_selector_data *mpr_selector = (mpr_selector_data *) iter.value();
			if (mpr_selector->MS_time <= now)
			{
				//s        click_chatter ("node %s: MPR_Selector %s has expired, about to delete it\n",nib->_myMainIP.unparse().c_str(),mpr_selector->MS_main_addr.unparse().c_str());
				nib->remove_mpr_selector(mpr_selector->MS_main_addr);
				mpr_selector_removed = true;
			}
		}
	}

	//find next MPR selector to expire
	if (! nib->_mprSelectorSet->empty())
	{
		for (MPRSelectorSet::iterator iter = nib->_mprSelectorSet->begin(); iter != nib->_mprSelectorSet->end(); iter++)
		{
			mpr_selector_data *mpr_selector = (mpr_selector_data *) iter.value();
			if (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0)
				next_timeout = mpr_selector->MS_time;
			else if ( mpr_selector->MS_time < next_timeout )
				next_timeout = mpr_selector->MS_time;
		}
	}

	if (mpr_selector_removed)
		nib->_tcGenerator->increment_ansn();

	if (! (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0) )
		timer->schedule_at(next_timeout);    //set timer

}


void
OLSRNeighborInfoBase::twohop_expiry_hook(Timer *timer, void *thunk)
{
	OLSRNeighborInfoBase *nib = (OLSRNeighborInfoBase *) thunk;
	struct timeval now, next_timeout;
	bool twohop_removed = false;;
	click_gettimeofday(&now);
	next_timeout = make_timeval(0,0);

	//find expired twohop neighbors and delete them
	if (! nib->_twohopSet->empty())
	{
		for (TwoHopSet::iterator iter = nib->_twohopSet->begin(); iter != nib->_twohopSet->end(); iter++)
		{
			twohop_data *twohop = (twohop_data *) iter.value();
			if (twohop->N_time <= now)
			{
				nib->remove_twohop_neighbor(twohop->N_neigh_main_addr, twohop->N_twohop_addr);
				twohop_removed = true;
			}
		}
	}

	//find next twohop neighbor to expire
	if (! nib->_twohopSet->empty())
	{
		for (TwoHopSet::iterator iter = nib->_twohopSet->begin(); iter != nib->_twohopSet->end(); iter++)
		{
			twohop_data *twohop = (twohop_data *) iter.value();
			if (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0)
				next_timeout = twohop->N_time;
			else if ( twohop->N_time < next_timeout )
				next_timeout = twohop->N_time;
		}
	}

	if (! (next_timeout.tv_sec == 0 && next_timeout.tv_usec == 0) )
		timer->schedule_at(next_timeout);    //set timer
	if (twohop_removed)
	{
		nib->_routingTable->compute_routing_table();
		nib->compute_mprset();
	}
}


neighbor_data *
OLSRNeighborInfoBase::add_neighbor(IPAddress neigh_addr)
{
	click_chatter("Adding new neighbor: %s", neigh_addr.unparse().c_str());
	struct neighbor_data *data;
	data = new struct neighbor_data;		//freed in remove

	data->N_neigh_main_addr = neigh_addr;
	if (_neighborSet->insert(neigh_addr, data) )
		return data;
	return 0;
}


neighbor_data *
OLSRNeighborInfoBase::find_neighbor(IPAddress neigh_addr)
{
	if (! _neighborSet->empty() )
	{
		struct HashMap<IPAddress, void*>::Pair *pair;
		pair = _neighborSet->find_pair(neigh_addr);

		if (! pair == 0 )
		{
			neighbor_data *data = (neighbor_data *) pair->value;
			return data;
		}
	}

	return 0;
}


bool
OLSRNeighborInfoBase::update_neighbor(IPAddress neigh_addr, int status, int willingness)
{
	struct neighbor_data *data;
	data = find_neighbor(neigh_addr);
	if (! data == 0 )
	{
		data->N_status = status;
		data->N_willingness = willingness;
		return true;
	}
	return false;
}


void
OLSRNeighborInfoBase::remove_neighbor(IPAddress neigh_addr)
{
	_neighborSet->remove(neigh_addr);
	if (! _twohopSet->empty())
	{
		for (TwoHopSet::iterator iter = _twohopSet->begin(); iter != _twohopSet->end(); iter++)
		{
			twohop_data *twohop = (twohop_data *) iter.value();
			if (twohop->N_neigh_main_addr == neigh_addr)
				remove_twohop_neighbor(twohop->N_neigh_main_addr, twohop->N_twohop_addr);
		}
	}
}


void
OLSRNeighborInfoBase::print_neighbor_set()
{
	if (! _neighborSet->empty())
	{
		for (NeighborSet::iterator iter = _neighborSet->begin(); iter != _neighborSet->end(); iter++)
		{
			neighbor_data *data = (neighbor_data *) iter.value();
			click_chatter("neighbor: %s\n", data->N_neigh_main_addr.unparse().c_str());
			click_chatter("\tstatus: %d\n", data->N_status );
			click_chatter("\twillingness: %d\n", data->N_willingness );
		}
	}
	else
	{
		click_chatter("Neighbor Set empty");
	}
}


HashMap<IPAddress, void *> *
OLSRNeighborInfoBase::get_neighbor_set()
{
	return _neighborSet;
}


bool
OLSRNeighborInfoBase::add_twohop_neighbor(IPAddress neigh_addr, IPAddress twohop_neigh_addr, struct timeval time)
{
	IPPair ippair=IPPair(neigh_addr, twohop_neigh_addr);;
	struct twohop_data *data, *data2;
	data = new struct twohop_data;		//ok freed in remove

	data->N_neigh_main_addr = neigh_addr;
	data->N_twohop_addr = twohop_neigh_addr;
	data->N_time = time;


	data2 = find_twohop_neighbor(neigh_addr, twohop_neigh_addr);

	if (data2 != 0)
	{
		_twohopSet->remove(ippair);
	}

	if (_twohopSet->empty())
		_twohop_timer.schedule_at(time);

	return ( _twohopSet->insert(ippair, data) );
}


twohop_data *
OLSRNeighborInfoBase::find_twohop_neighbor(IPAddress neigh_addr, IPAddress twohop_neigh_addr)
{
	if (! _twohopSet->empty() )
	{
		IPPair ippair = IPPair(neigh_addr, twohop_neigh_addr);
		struct HashMap<IPPair, void *>::Pair *pair;
		pair = _twohopSet->find_pair(ippair);

		if (! pair == 0 )
		{
			twohop_data *data = (twohop_data *)pair->value;
			return data;
		}
	}

	return 0;
}

void
OLSRNeighborInfoBase::remove_twohop_neighbor(IPAddress neigh_addr, IPAddress twohop_neigh_addr)
{
	IPPair ippair = IPPair(neigh_addr, twohop_neigh_addr);
	twohop_data *ptr = (twohop_data*) _twohopSet->find(ippair);
	_twohopSet->remove(ippair);
	delete ptr;
}


void
OLSRNeighborInfoBase::print_twohop_set()
{
	if (! _twohopSet->empty() )
	{
		for (TwoHopSet::iterator iter = _twohopSet->begin(); iter != _twohopSet->end(); iter++)
		{
			twohop_data *data = (twohop_data *) iter.value();
			click_chatter("twohop neighbor: %s\n", data->N_twohop_addr.unparse().c_str());
			click_chatter("\tN_neigh_main_addr: %s\n", data->N_neigh_main_addr.unparse().c_str());
			click_chatter("\tN_time: %d\n", data->N_time.tv_sec );
		}
	}
	else
	{
		click_chatter("Twohop Set empty\n");
	}
}


HashMap<IPPair, void*> *
OLSRNeighborInfoBase::get_twohop_set()
{
	return _twohopSet;
}


mpr_selector_data *
OLSRNeighborInfoBase::add_mpr_selector(IPAddress ms_addr, timeval time)
{
	struct mpr_selector_data *data;
	data = new struct mpr_selector_data;		//ok, freed in remove

	data->MS_main_addr = ms_addr;
	data->MS_time = time;

	if ( _mprSelectorSet->empty() )
	{
		_mpr_selector_timer.schedule_at(time);
		_tcGenerator->set_node_is_mpr(true);
	}

	if ( _mprSelectorSet->insert(ms_addr, data) )
		return data;

	return 0;

}


mpr_selector_data *
OLSRNeighborInfoBase::find_mpr_selector(IPAddress ms_addr)
{
	if (! _mprSelectorSet->empty() )
	{
		mpr_selector_data *data = (mpr_selector_data *) _mprSelectorSet->find(ms_addr);

		if (! data == 0 )
			return data;
	}
	return 0;
}


bool
OLSRNeighborInfoBase::is_mpr_selector(IPAddress ms_addr)
{
	if (_mprSelectorSet->find(ms_addr) != 0)
		return true;
	return false;
}


void
OLSRNeighborInfoBase::remove_mpr_selector(IPAddress ms_addr)
{
	mpr_selector_data *ptr=(mpr_selector_data*) _mprSelectorSet->find(ms_addr);
	if (ptr)
	{
		_mprSelectorSet->remove(ms_addr);
		delete ptr;
		if (_mprSelectorSet->empty())
			_tcGenerator->set_node_is_mpr(false);
		//click_chatter ("node %s: removed MPR Selector %s",_myMainIP.unparse().c_str(),ms_addr.unparse().c_str());
	}
	//else click_chatter ("node %s: asked to remove MPR Selector %s which is not in MPR Selector Set",_myMainIP.unparse().c_str(),ms_addr.unparse().c_str());
}


void
OLSRNeighborInfoBase::print_mpr_selector_set()
{
	if (! _mprSelectorSet->empty() )
	{
		for (MPRSelectorSet::iterator iter = _mprSelectorSet->begin(); iter != _mprSelectorSet->end(); iter++)
		{
			mpr_selector_data *data = (mpr_selector_data *) iter.value();
			click_chatter("MPR Selector: %s\n", data->MS_main_addr.unparse().c_str());
		}
	}

	else
	{
		click_chatter("MPR Selector Set empty\n");
	}
}


HashMap<IPAddress, void *> *
OLSRNeighborInfoBase::get_mpr_selector_set()
{
	return _mprSelectorSet;
}


void
OLSRNeighborInfoBase::compute_mprset()
{// as described in the RFC, chapter 8.3.1 MPR Computation
	//node has only one interface

#ifdef do_it
#ifdef profiling_kernel
	uint64_t cycles=click_get_cycles();
	_count++;
#endif

	HashMap<IPPair, void*> *linkSet=_linkInfoBase->get_link_set();
	HashMap <IPAddress, void *> *interfaceSet=_interfaceInfoBase->get_interface_set();
	NeighborSet N_interface;
	TwoHopSet twohopSet_interface;

	NeighborSet *N;
	TwoHopSet *twohopset;
	N2Set *N2;
	HashMap<IPAddress, int> D_y_obj;
	HashMap<IPAddress, int> *D_y = &D_y_obj;
	MPRSet mprset;

	HashMap<IPAddress, Vector <IPAddress> > coverage;	//stores vector of nodes reachable by a 1hop Neighbor


	Vector <IPAddress> * IP_Vector_ptr;
	for (TwoHopSet::iterator iter = _twohopSet->begin(); iter != _twohopSet->end(); iter++)
	{
		twohop_data *twohop = (twohop_data *) iter.value();

		if ((IP_Vector_ptr = coverage.findp(twohop->N_neigh_main_addr)))
			IP_Vector_ptr->push_back (twohop->N_twohop_addr);
		else
		{
			Vector <IPAddress> IP_Vector;
			IP_Vector.push_back(twohop->N_twohop_addr),
			coverage.insert (twohop->N_neigh_main_addr,IP_Vector);
		}
	}
	HashMap <IPAddress, NeighborSet > N_set; //Set of Neighborsets (for all local interfaces)
	NeighborSet *N_of_interface_ptr; 	  //Neighborset pointer for one interface
	HashMap <IPAddress, TwoHopSet> twohop_set;	//Set of TwoHopSets for (for all local interfaces)
	TwoHopSet *twohop_of_interface_ptr;
	HashMap <IPAddress, N2Set> n2_set;
	//N2Set n2_set_of_interface;
	MPRSet old_mprset;
	if (_additional_hello_message) old_mprset=(*_mprSet);

	_mprSet->clear();

	//though this is not performance optimized it is the first shot at multiple interfaces
	if (! linkSet->empty() )
	{
		for (HashMap<IPPair, void*>::iterator iter = linkSet->begin(); iter != linkSet->end(); iter++)
		{ 	//for all links
			link_data *data = (link_data *)iter.value();			//get link data
			IPAddress main_address=_interfaceInfoBase->get_main_address(data->L_neigh_iface_addr); //get main address of other
			neighbor_data *neigh = find_neighbor (main_address);	//side of the link and its neighbor data ptr
			if (neigh)
			{
				if (!(N_of_interface_ptr=N_set.findp(data->L_local_iface_addr)))	//if there doesnt already exist
				{								//a neighborset for this
					NeighborSet N_of_interface; 				//local Interface, create one
					N_of_interface.insert (main_address,neigh);		//include this in the new created hasmap
					N_set.insert (data->L_local_iface_addr,N_of_interface);	//insert it in the list for the corresponding
				}								//local interface address
				else 				//allmost the same as above, but adding the it to an existing list
				{
					N_of_interface_ptr->insert (main_address,neigh);		//include this in the existing hasmap
				}				//corresponding to the local interface
				//this builds the neighborsets for all local interfaces
				//now building a twohopset for all local interfaces
				if (!(twohop_of_interface_ptr=twohop_set.findp(data->L_local_iface_addr)))	//if there doesnt already exist
				{								//a twohopborset for this
					TwoHopSet twohop_of_interface; 				//local Interface, create one
					N2Set n2_set_of_interface;
					if ((IP_Vector_ptr=coverage.findp(main_address)))	//all nodes reachable from this neighbor are twohop neighbors
						for (int i=0;i<IP_Vector_ptr->size();i++)
						{
							IPAddress N_twohop_addr=(*(IP_Vector_ptr))[i];
							twohop_of_interface.insert (IPPair(main_address,N_twohop_addr),_twohopSet->find(IPPair(main_address,N_twohop_addr)));		//include this in the new created hasmap
							bool insert=false;

							{
								if ((neigh->N_willingness != OLSR_WILL_NEVER) && (N_twohop_addr!=_myMainIP))
								{
									neighbor_data *twohop_neighbor_data = (neighbor_data*) _neighborSet->find(N_twohop_addr);

									if (!twohop_neighbor_data) insert=true;
									else if (twohop_neighbor_data->N_status == OLSR_NOT_NEIGH) insert=true;
								}
							}
							if (insert)
							{
								Vector <IPAddress> * N2firsthopVector;

								if ((N2firsthopVector=n2_set_of_interface.findp(N_twohop_addr)))
									N2firsthopVector->push_back (main_address);
								else
								{
									Vector <IPAddress> IP_Vector;
									IP_Vector.push_back(main_address),
									n2_set_of_interface.insert (N_twohop_addr,IP_Vector);

								}

							}
						}
					twohop_set.insert (data->L_local_iface_addr,twohop_of_interface);	//insert it in the list for the corresponding
					n2_set.insert (data->L_local_iface_addr,n2_set_of_interface);
				}								//local interface address
				else 				//allmost the same as above, but adding the it to an existing list
				{
					N2=n2_set.findp(data->L_local_iface_addr);
					if ((IP_Vector_ptr=coverage.findp(main_address)))	//all nodes reachable from this neighbor are twohop neighbors
					{
						for (int i=0;i<IP_Vector_ptr->size();i++)
						{
							IPAddress N_twohop_addr=(*(IP_Vector_ptr))[i];
							twohop_of_interface_ptr->insert (IPPair(main_address,N_twohop_addr),_twohopSet->find(IPPair(main_address,N_twohop_addr)));		//include this in the existing TwoHopSet for this interface


							bool insert=false;

							{
								if ((neigh->N_willingness != OLSR_WILL_NEVER) && (N_twohop_addr!=_myMainIP))
								{
									neighbor_data * twohop_neighbor_data = (neighbor_data*) _neighborSet->find(N_twohop_addr);

									if (!twohop_neighbor_data) insert=true;
									else if (twohop_neighbor_data->N_status == OLSR_NOT_NEIGH) insert=true;
								}
							}
							if (insert)
							{
								Vector <IPAddress> * N2firsthopVector;

								if ((N2firsthopVector=N2->findp(N_twohop_addr)))
									N2firsthopVector->push_back (main_address);

								else
								{
									Vector <IPAddress> IP_Vector;
									IP_Vector.push_back(main_address),
									N2->insert (N_twohop_addr,IP_Vector);

								}

							}
						}
					}
				}
			}
		}
	}
	//twohopset for all local interfaces built.


#ifdef debug

	click_chatter ("Node %s\n",_myMainIP.unparse().cc());
	click_chatter ("general neighborset entries %d\n",_neighborSet->size());
	//print_neighbor_set();
	click_chatter ("general twohop_set entries %d\n",_twohopSet->size());
	//print_twohop_set();
	click_chatter ("coverage\n");

	for (HashMap<IPAddress, Vector <IPAddress> >::iterator iter=coverage.begin(); iter != coverage.end(); iter++)
	{
		click_chatter ("\tneighbor \t%s\n",iter.key().unparse().c_str());
		for (int i =0; i<iter.value().size(); i++)
			click_chatter ("\t\t reaches \t%s\n",iter.value()[i].unparse().c_str());
	}


	for (HashMap <IPAddress, NeighborSet>::iterator it=N_set.begin(); it; it++) //for over all Neighborsets for the local Interfaces
	{
		click_chatter ("local Interface: %s\n",it.key().unparse().c_str());
		N = &(it.value()); // Neighborset for this interface
		N2=n2_set.findp(it.key());
		twohopset=twohop_set.findp(it.key());
		click_chatter ("\tNeighborset\t\n");
		if (! N->empty())
		{
			for (NeighborSet::iterator iter = N->begin(); iter != N->end(); iter++)
			{
				neighbor_data *data = (neighbor_data *) iter.value();
				click_chatter("\tneighbor: %s\n", data->N_neigh_main_addr.unparse().c_str());
				click_chatter("\t\tstatus: %d\n", data->N_status );
				click_chatter("\t\twillingness: %d\n", data->N_willingness );
			}
		}
		else
		{
			click_chatter("\tNeighbor Set empty");
		}
		click_chatter ("\ttwohoset\t\n");
		if (! twohopset->empty() )
		{
			for (TwoHopSet::iterator iter = twohopset->begin(); iter != twohopSet->end(); iter++)
			{
				twohop_data *data = (twohop_data *) iter.value();
				click_chatter("\ttwohop neighbor: %s\n", data->N_twohop_addr.unparse().c_str());
				click_chatter("\t\tN_neigh_main_addr: %s\n", data->N_neigh_main_addr.unparse().c_str());
				click_chatter("\t\tN_time: %d\n", data->N_time.tv_sec );
			}
		}
		else
		{
			click_chatter("\tTwohop Set empty\n");
		}
		click_chatter ("\tN2\t\n");
		for (HashMap<IPAddress, Vector <IPAddress> >::iterator iter=N2->begin(); iter != N2->end(); iter++)
		{
			click_chatter ("\tn2 member \t%s\n",iter.key().unparse().c_str());
			for (int i =0; i<iter.value().size(); i++)
				click_chatter ("\t\t reachable through \t%s\n",iter.value()[i].unparse().c_str());
		}
	}
#endif
	for (HashMap <IPAddress, NeighborSet>::iterator it=N_set.begin(); it != N_set.end(); it++) //for over all Neighborsets for the local Interfaces
	{


		N = &(it.value()); // Neighborset for this interface
		N2=n2_set.findp(it.key());
		twohopset=twohop_set.findp(it.key());
#ifdef debug
		click_chatter ("computing for interface %s\n",it.key().unparse().c_str());
#endif

		//just like mpr computation for single interface



		//step 0 (optimization in case of multiple interfaces, rfc step 5)
		//if another interface has already elected a node as mpr, and this interface has a link to this node too,
		//this node can be elected as mpr as well

		if (!_mprSet->empty())
		{
			for (MPRSet::iterator iter=_mprSet->begin(); iter != _mprSet->end(); iter++)
				//for all mprs already elected
				for (HashMap <IPAddress, void *> ::iterator iterat = interfaceSet->begin(); iterat != interfaceSet->end(); iterat++)
				{ 	//for all interface
					interface_data *tuple = (interface_data *) iterat.value();			//addresses that match
					if (tuple->I_main_addr==iter.key())
					{					//the elected mpr as main address
						if (linkSet->find(IPPair(it.key(),tuple->I_iface_addr))) 			//check wheter there exists a link to this interface
						{ mprset.insert(iter.key(),iter.key());
#ifdef debug
							click_chatter ("inserting %d as already MPR for other interface and link exists\n",iter.key().unparse().c_str());
#endif
						}
					}
					if ((IP_Vector_ptr=coverage.findp(iter.key())))
					{
						for (int i=0;i<IP_Vector_ptr->size();i++)
							N2->remove((*(IP_Vector_ptr))[i]);
					}
				}
		}

		///@TODO step 1 RFC 3626 §8.3.1

		//step 2 and d_y
		for ( NeighborSet::iterator iter = N->begin(); iter != N->end(); iter++)
		{
			neighbor_data *neighbor = (neighbor_data *)iter.value();
			if (neighbor->N_willingness == OLSR_WILL_ALWAYS) //step 1 of the proposed heuristic in RFC, add all neighbors with willingness WILL_ALWAYS
			{

				mprset.insert(neighbor->N_neigh_main_addr, neighbor->N_neigh_main_addr);
				if ((IP_Vector_ptr=coverage.findp(neighbor->N_neigh_main_addr)))
				{
					for (int i=0;i<IP_Vector_ptr->size();i++)
						N2->remove((*(IP_Vector_ptr))[i]);
				}

			}
			int d_y=0;
			if ((IP_Vector_ptr=coverage.findp(neighbor->N_neigh_main_addr)))
				for (int i =0; i<IP_Vector_ptr->size(); i++)
				{
					if (!( N->find((*IP_Vector_ptr)[i])) && ((*IP_Vector_ptr)[i]!=_myMainIP))
						d_y++;
				}

			D_y->insert(neighbor->N_neigh_main_addr, d_y);
		}

		//step 3
		for(HashMap<IPAddress, Vector <IPAddress> >::iterator iter = N2->begin();iter != N2->end();iter++)
		{
			if (iter.value().size()==1)
			{
				mprset.insert(iter.value()[0],iter.value()[0]);
#ifdef debug
				click_chatter ("step 3 adding mpr %s \n",iter.value()[0].unparse().c_str());
#endif
				if ((IP_Vector_ptr=coverage.findp(iter.value()[0])))
				{
					for (int i=0;i<IP_Vector_ptr->size();i++)
					{
						N2->remove((*IP_Vector_ptr)[i]);
#ifdef debug
						click_chatter ("\t removing now covered %s \n",(*IP_Vector_ptr)[i].unparse().c_str());
#endif

					}
				}
			}
		}
		//step 4
		while (! N2->empty())
		{ //step 4
			int best_mpr_willingness = 0;
			int best_mpr_reachability = 0;
			int best_mpr_d_y = 0;
			IPAddress best_mpr;
			for (NeighborSet::iterator iter = N->begin(); iter != N->end(); iter++)
			{
				if (!mprset.findp(iter.key()))
				{
					neighbor_data* n_member = (neighbor_data *) iter.value();

					int reaches = 0;
					if ((IP_Vector_ptr=coverage.findp(n_member->N_neigh_main_addr)))
						for (int i=0;i<IP_Vector_ptr->size();i++)
							if (N2->findp((*IP_Vector_ptr)[i])) reaches++;
					if (reaches !=0)
					{
						int d_y=D_y->find(n_member->N_neigh_main_addr);
						if ( n_member->N_willingness > best_mpr_willingness )
						{

							best_mpr_willingness = n_member->N_willingness;
							best_mpr_reachability = reaches;
							best_mpr_d_y = d_y;
							best_mpr = n_member->N_neigh_main_addr;
						}
						else if (n_member->N_willingness == best_mpr_willingness)
							if (reaches > best_mpr_reachability)
							{
								best_mpr_reachability = reaches;
								best_mpr_d_y = d_y;
								best_mpr = n_member->N_neigh_main_addr;
							}
							else if (reaches == best_mpr_reachability)
								if (d_y>best_mpr_d_y)
								{
									best_mpr_d_y = d_y;
									best_mpr = n_member->N_neigh_main_addr;
								}
					}
				}
			}
			mprset.insert(best_mpr,best_mpr);
#ifdef debug
			click_chatter ("step 4 adding mpr %s \n",best_mpr.unparse().c_str());
#endif
			if ((IP_Vector_ptr=coverage.findp(best_mpr)))
				for (int i=0;i<IP_Vector_ptr->size();i++)
				{
					N2->remove((*IP_Vector_ptr)[i]);
#ifdef debug
					click_chatter ("\t removing now covered %s \n",(*IP_Vector_ptr)[i].unparse().c_str());
#endif

				}
			else click_chatter ("ERROR: mpr not covering anything");

		}


		for (MPRSet::iterator iter=mprset.begin(); iter != mprset.end(); iter++)
		{
			if (!_mprSet->findp(iter.key())) _mprSet->insert(iter.key(),iter.key());
		}

		mprset.clear();

	}


	/// == mvhaen ====================================================================================================
	// This piece of the code does not match the RFC and is optional
	if (_additional_mprs && _mprSet->size() < MIN_MPR)
	{
#ifdef debug
		click_chatter ("additional mprs are needed\n");
#endif
		// keep a track of how much MPRs we have chosen:
		int mpr_count = _mprSet->size();
#ifdef debug
		click_chatter ("so far we have %d mprs\n",mpr_count);
#endif
		// make sure that the tmp data structure is empty
		mprset.clear();
		int best_mpr_willingness = 0;
		//int best_mpr_reachability = 0;
		int best_mpr_d_y = 0;
		IPAddress best_mpr;
		while (mpr_count < MIN_MPR)
		{
			//check if all the N2 neighbors are covered ... if so adding additional MPRs will not work
			if (_neighborSet->size() == _mprSet->size() + mprset.size())
			{
#ifdef debug
				click_chatter ("breaking: all our neighbors are already MPR, so no way I can choose additional MPRs\n");
#endif
				break;
			}
			// this code is not optimized for multiple interfaces
			for (HashMap <IPAddress, NeighborSet>::iterator it=N_set.begin(); it != N_set.end(); it++) //for over all Neighborsets for the local Interfaces
			{
				N = &(it.value()); // Neighborset for this interface
				N2=n2_set.findp(it.key());
				twohopset=twohop_set.findp(it.key());
				// loop over all the neighbors that we can reach through that interface
				for (NeighborSet::iterator iter=N->begin(); iter != N->end(); iter++)
				{
					neighbor_data* n_member = (neighbor_data *) iter.value();
					// we will only choose those nodes that are not yet MPR
					if (!_mprSet->findp(n_member->N_neigh_main_addr))
					{
						// note that for now we use d_y to find the best likely mpr
						int d_y=D_y->find(n_member->N_neigh_main_addr);
						if (best_mpr_d_y >= d_y)
						{

							best_mpr_willingness = n_member->N_willingness;
							//best_mpr_reachability = reaches;
							best_mpr_d_y = d_y;
							best_mpr = n_member->N_neigh_main_addr;
						}
					}
				}
			}
#ifdef debug
			click_chatter ("chose %s as additional MPR\n",best_mpr.unparse().c_str());
#endif
			mprset.insert(best_mpr, best_mpr);
			mpr_count++;
			if (mpr_count == MIN_MPR)
			{
				break;
			}
		}
		for (MPRSet::iterator iter=mprset.begin(); iter != mprset.end(); iter++)
		{
			if (!_mprSet->findp(iter.key())) _mprSet->insert(iter.key(),iter.key());
		}
		mprset.clear();
	}
	/// == !mvhaen ===================================================================================================





	if (_additional_hello_message)
	{
		bool mpr_changed=false;
		if (_mprSet->size()!=old_mprset.size()) mpr_changed=true;
		else
			for (MPRSet::iterator iter=_mprSet->begin();iter != _mprSet->end(); iter++)
			if (!old_mprset.findp(iter.key())) {mpr_changed=true;break;}
		if (mpr_changed) _helloGenerator->notify_mpr_change(); //triggers reschedule of sending a hello message now!
	}

	//  click_chatter ("my Main IP %s\n",_myMainIP.unparse().c_str());
	//  print_mpr_set();
	//  click_chatter ("end of mpr computation\n\n");

#ifdef profiling_kernel
	_cyclesaccum+=(click_get_cycles()-cycles);

#endif

#endif
}

IPAddress *
OLSRNeighborInfoBase::find_mpr(const IPAddress &address)
{
	if (! _mprSet->empty() )
	{
		IPAddress *return_address = (IPAddress*) &(_mprSet->find(address));
		if (*return_address!=IPAddress() )
			return return_address;
	}
	return 0;
}


void
OLSRNeighborInfoBase::print_mpr_set()
{
	if (! _mprSet->empty() )
	{
		for (MPRSet::iterator iter = _mprSet->begin(); iter != _mprSet->end(); iter++)
		{
			IPAddress mpr = iter.value();
			click_chatter("MPR: %s\n", mpr.unparse().c_str());
		}
	}
	else
	{
		click_chatter("MPR Set empty\n");
	}
}

#ifdef profiling_kernel
String
OLSRNeighborInfoBase::read_handler(Element *e, void *thunk)
{
	OLSRNeighborInfoBase *cca = static_cast<OLSRNeighborInfoBase *>(e);
	switch ((uintptr_t)thunk)
	{
	case 0:
		return String(cca->_count) + "\n";
	case 1:
		return String(cca->_cyclesaccum) + "\n";

	default:
		return String();
	}
}
#endif




/// == mvhaen ====================================================================================================
void
OLSRNeighborInfoBase::additional_mprs_is_enabled(bool in)
{
	_additional_mprs = in;
}

int
OLSRNeighborInfoBase::additional_mprs_is_enabled_handler(const String &conf, Element *e, void *, ErrorHandler *)
{
	OLSRNeighborInfoBase* me = (OLSRNeighborInfoBase *) e;
	bool in;
	if (conf == "TRUE")
	{
		in = true;
	}
	else
	{
		in = false;
	}
	me->additional_mprs_is_enabled(in);

	return 0;
}

void
OLSRNeighborInfoBase::add_handlers()
{
	add_write_handler("additional_mprs_is_enabled", &additional_mprs_is_enabled_handler, (void *)0);
#ifdef profiling_kernel
	add_read_handler("count",read_handler,(void*) 0);
	add_read_handler("accum",read_handler,(void*) 1);
#endif
}
/// == !mvhaen ===================================================================================================


#include <click/bighashmap.cc>
#if EXPLICIT_TEMPLATE_INSTANCES
template class HashMap<IPAddress, void *>;
template class HashMap<IPPair, void *>;
template class HashMap<IPAddress, IPAddress>;
#endif

CLICK_ENDDECLS
EXPORT_ELEMENT(OLSRNeighborInfoBase);


