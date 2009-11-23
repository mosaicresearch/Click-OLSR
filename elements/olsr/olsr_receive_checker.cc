//TED 070504: Created

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/ipaddress.hh>
#include <click/router.hh>
#include "click_olsr.hh"
#include "olsr_receive_checker.hh"

CLICK_DECLS

OLSRReceiveChecker::OLSRReceiveChecker()
		:_timer(this), _period(15000)
{
}


OLSRReceiveChecker::~OLSRReceiveChecker()
{
}


int
OLSRReceiveChecker::configure(Vector<String> &conf, ErrorHandler *errh)
{
	int res = cp_va_parse(conf, this, errh,
	                      cpInteger, "Timeout value (msec)", &_period,
	                      cpElement, "OLSR Neighbor Infobase Element", &_neighborInfo,
	                      cpIPAddress, "my main IPAddress", &_myIP,
	                      cpEnd);
	if ( res < 0 )
		return res;
	if ( _period <= 0 )
		return errh->error("period must be greater than 0");
	return res;
}


int
OLSRReceiveChecker::initialize(ErrorHandler *)
{

	_timer.initialize(this);
	return 0;
}

void
OLSRReceiveChecker::run_timer(Timer *)
{
	_neighborInfo->additional_mprs_is_enabled(false);
}

void
OLSRReceiveChecker::push(int, Packet *packet)
{
	// if we start receiving packets we will need additional MPRs to guarantee connectivity.
	_neighborInfo->additional_mprs_is_enabled(true);
	// start a timer that after _period will stop the node from choosing additional_mprs
	if (_timer.scheduled())
	{
		_timer.reschedule_after_msec(_period);
	}
	else
	{
		_timer.schedule_after_msec(_period);

	}
	output(0).push(packet);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(OLSRReceiveChecker);


