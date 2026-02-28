#include "HLib/HEvent.h"
#include "optionlibs/TickData.h"
#include <string>
#include <map>
using namespace std;

HEvent* HEvent::instance_ = 0;

HEvent* HEvent::Instance()
{
	static HEvent::Cleaner cleaner;
	if( instance_ == 0 )
		instance_ = new HEvent();
	return instance_;
}

HEvent::Cleaner::~Cleaner() {
	delete HEvent::instance_;
	HEvent::instance_ = 0;
}

HEvent::HEvent()
{}

void HEvent::clear()
{
	mp_.clear();
}

bool HEvent::find(string ticker, string desc) const
{
	return mp_.count(ticker) && mp_.find(ticker)->second.count(desc);
}

const void* HEvent::get(string ticker, string desc) const
{
	boost::mutex::scoped_lock lock(mp_mutex_);
	if( mp_.count(ticker) && mp_.find(ticker)->second.count(desc) )
		return mp_.find(ticker)->second.find(desc)->second; // mp_[ticker][desc]
	return 0;
}
