#ifndef CLICK_jitterUNQUEUE_HH
#define CLICK_jitterUNQUEUE_HH
#include <click/element.hh>
#include <click/gaprate.hh>
#include <click/task.hh>
#include <click/notifier.hh>
CLICK_DECLS



class JitterUnqueue : public Element
{
public:

	JitterUnqueue();
	~JitterUnqueue();

	const char *class_name() const	{ return "JitterUnqueue"; }
	const char *processing() const	{ return PULL_TO_PUSH; }
	const char *port_count() const  { return "1/1"; }

	int configure(Vector<String> &, ErrorHandler *);
	int initialize(ErrorHandler *);

	bool run_task();
	
	void add_handlers();

protected:

	void set_maxdelay(int maxdelay);
	void set_mindelay(int mindelay);

	static int set_maxdelay_handler(const String &conf, Element *e, void *, ErrorHandler * errh);
	static int set_mindelay_handler(const String &conf, Element *e, void *, ErrorHandler * errh);
	
	Task _task;
	NotifierSignal _signal;

private:
	struct timeval _expire;
	struct timeval _mindelay;
	struct timeval _maxdelay;
	struct timeval _minmaxdiff;
	uint32_t _minmaxdiff_usec;

};

CLICK_ENDDECLS
#endif
