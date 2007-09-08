/*
 * timedsink.{cc,hh} -- element pulls packets periodically, discards them
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
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
#include "timedsink.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>
CLICK_DECLS

TimedSink::TimedSink()
  : _timer(this)
{
}

TimedSink::~TimedSink()
{
}

int
TimedSink::configure(Vector<String> &conf, ErrorHandler *errh)
{
  _interval = 500;
  return cp_va_kparse(conf, this, errh,
		      "INTERVAL", cpkP, cpSecondsAsMilli, &_interval,
		      cpEnd);
}

int
TimedSink::initialize(ErrorHandler *)
{
  _timer.initialize(this);
  _timer.schedule_after_msec(_interval);
  return 0;
}

void
TimedSink::run_timer(Timer *)
{
  Packet *p = input(0).pull();
  if (p)
    p->kill();
  _timer.reschedule_after_msec(_interval);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(TimedSink)
ELEMENT_MT_SAFE(TimedSink)
