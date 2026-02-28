#include <MFramework/MEvent.h>
#include <optionlibs/TickData.h>
#include <string>
#include <map>
using namespace std;

MEvent* MEvent::instance_ = 0;

MEvent* MEvent::Instance()
{
	static MEvent::Cleaner cleaner;
	if( instance_ == 0 )
		instance_ = new MEvent();
	return instance_;
}

MEvent::Cleaner::~Cleaner() {
	delete MEvent::instance_;
	MEvent::instance_ = 0;
}

MEvent::MEvent()
{}

void MEvent::clear()
{
	mp_.clear();
}

bool MEvent::find(const string& ticker, const string& desc) const
{
	return mp_.count(ticker) && mp_.find(ticker)->second.count(desc);
}

const void* MEvent::get(const string& ticker, const string& desc) const
{
	boost::mutex::scoped_lock lock(mp_mutex_);
	if( mp_.count(ticker) && mp_.find(ticker)->second.count(desc) )
		return mp_.find(ticker)->second.find(desc)->second; // mp_[ticker][desc]
	return 0;
}
