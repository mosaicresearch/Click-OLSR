// -*- c-basic-offset: 4 -*-
/*
 * setetherheader.{cc,hh} 
 * Robert Morris
 *
 * Copyright (c) 2005 University of Antwerp
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
#include "marketherheader.hh"

CLICK_DECLS

MarkEtherHeader::MarkEtherHeader()
{
}

MarkEtherHeader::~MarkEtherHeader()
{
}

int
MarkEtherHeader::configure(Vector<String> &conf, ErrorHandler *errh)
{

    return 0;
}

Packet *
MarkEtherHeader::simple_action(Packet *p)
{
	p->set_mac_header(p->data());
	return(p);
}

EXPORT_ELEMENT(MarkEtherHeader)

CLICK_ENDDECLS
