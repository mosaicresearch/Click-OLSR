// -*- c-basic-offset: 4 -*-
/*
 * lineariplookup.{cc,hh} -- element looks up next-hop address in linear
 * routing table
 * Robert Morris, Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2002 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "olsr_lineariplookup.hh"
#include <click/ipaddress.hh>
#include <click/straccum.hh>
#include <click/error.hh>
CLICK_DECLS

/**
 * completely clears the routing table
 */
void
OLSRLinearIPLookup::clear()
{
     _t.clear();   		//clear table
     
    // get rid of caches
    _last_addr = IPAddress();
#ifdef IP_RT_CACHE2
    _last_addr2 = IPAddress();
#endif
}

const IPRoute*
OLSRLinearIPLookup::lookup_iproute(const IPAddress& dst) const
{
	int index = lookup_entry(dst);
	if (index == -1) return 0;
	else return &_t[index];
}

void 
OLSRLinearIPLookup::update(const IPAddress& dst, const IPAddress& gw, int port, int extra) {
	int index = lookup_entry(dst);
	if (index == -1) return;
	
	_t[index].gw = gw;
	_t[index].port = port;
	_t[index].extra = extra;
}

#include <click/vector.cc>
CLICK_ENDDECLS
ELEMENT_REQUIRES(LinearIPLookup)
EXPORT_ELEMENT(OLSRLinearIPLookup)
