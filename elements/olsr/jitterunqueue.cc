
#include <click/config.h>
#include "jitterunqueue.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/standard/scheduleinfo.hh>
CLICK_DECLS

inline struct timeval
			mk_tval(int sec,int usec)
{
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = usec;
	while (1000000 <= tv.tv_usec)
	{
		tv.tv_usec -= 1000000;
		tv.tv_sec++;
	}
	return tv;
}


JitterUnqueue::JitterUnqueue()
		: _task(this)
{
}

JitterUnqueue::~JitterUnqueue()
{
}

int
JitterUnqueue::configure(Vector<String> &conf, ErrorHandler *errh)
{
	int maxdelay =0;
	int mindelay = 0;
	int result =  cp_va_parse(conf, this, errh,
	                          cpInteger, "maxdelay", &maxdelay,
	                          cpOptional,
	                          cpInteger, "mindelay", &mindelay,
	                          0);
	_maxdelay = make_timeval ((int) (maxdelay / 1000),(maxdelay % 1000));
	_mindelay = make_timeval ((int) (mindelay / 1000),(mindelay % 1000));
	_minmaxdiff = _maxdelay - _mindelay;
	_minmaxdiff_usec = _minmaxdiff.tv_usec + 1000000*_minmaxdiff.tv_sec;
	return result;
}

int
JitterUnqueue::initialize(ErrorHandler *errh)
{
	ScheduleInfo::initialize_task(this, &_task, errh);
	_signal = Notifier::upstream_empty_signal(this, 0, &_task);
	_expire.tv_sec=0;_expire.tv_usec=0;
	return 0;
}

bool
JitterUnqueue::run_task()
{
	// listening for notifications
	struct timeval now;
	click_gettimeofday(&now);

	//timeradd(&now, &_delay, &expires);
	bool worked = false;
	if (timercmp(&_expire,&now,<) || timercmp(&_expire,&now,==))
	{
		while (Packet *p = input(0).pull())
		{
			output(0).push(p);
			worked = true;

		}
		uint32_t delay_usec = (_minmaxdiff_usec) ? (random() % _minmaxdiff_usec) : 0;
		_expire = now + _mindelay + mk_tval(0,delay_usec);

		if ((!worked) && (!_signal)) // no Packet available
			return false;		// without rescheduling
	}
	_task.fast_reschedule();
	return worked;
}

/// == mvhaen ====================================================================================================
void
JitterUnqueue::set_maxdelay(int maxdelay)
{

	_maxdelay = make_timeval ((int) (maxdelay / 1000),(maxdelay % 1000));
	_minmaxdiff = _maxdelay - _mindelay;
	_minmaxdiff_usec = _minmaxdiff.tv_usec + 1000000*_minmaxdiff.tv_sec;
}

void
JitterUnqueue::set_mindelay(int mindelay)
{
	_mindelay = make_timeval ((int) (mindelay / 1000),(mindelay % 1000));
	_minmaxdiff = _maxdelay - _mindelay;
	_minmaxdiff_usec = _minmaxdiff.tv_usec + 1000000*_minmaxdiff.tv_sec;
}

int
JitterUnqueue::set_maxdelay_handler(const String &conf, Element *e, void *, ErrorHandler * errh)
{
	JitterUnqueue* me = (JitterUnqueue *) e;
	int maxdelay = 0;
	int res =  cp_va_parse(conf, me, errh,
	                          cpInteger, "maxdelay", &maxdelay,
	                          0);
	me->set_maxdelay(maxdelay);
	return res;
}

int
JitterUnqueue::set_mindelay_handler(const String &conf, Element *e, void *, ErrorHandler * errh)
{
	JitterUnqueue* me = (JitterUnqueue *) e;
	int mindelay = 0;
	int res =  cp_va_parse(conf, me, errh,
	                          cpInteger, "mindelay", &mindelay,
	                          0);
	me->set_mindelay(mindelay);
	return res;
}

void
JitterUnqueue::add_handlers()
{
	add_write_handler("set_maxdelay", set_maxdelay_handler, (void *)0);
	add_write_handler("set_mindelay", set_mindelay_handler, (void *)0);
}
/// == !mvhaen ===================================================================================================



CLICK_ENDDECLS
EXPORT_ELEMENT(JitterUnqueue)
ELEMENT_MT_SAFE(JitterUnqueue)
