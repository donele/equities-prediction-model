#include <gtlib_signal/RestrictedUS.h>
#include <jl_lib/jlutil.h>
#include <fstream>
using namespace std;

namespace gtlib {

RestrictedUS::RestrictedUS()
	:lastIdate_(0)
{
	ifstream ifs("us_status.txt");
	string line;
	getline(ifs, line); // header.
	int prev_idate = 0;
	map<string, int> dayStatus;
	while( getline(ifs, line) )
	{
		auto sl = split(line);
		if( sl.size() == 4 && trim(sl[0]) != "idate" )
		{
			int idate = atoi(sl[0].c_str());
			string ticker = trim(sl[1]);
			int msecs = atoi(sl[2].c_str());

			if( idate > prev_idate && prev_idate > 0 )
			{
				m_[prev_idate] = dayStatus;
				dayStatus.clear();
			}

			dayStatus[ticker] = msecs;
			prev_idate = idate;
		}
	}
	if( !dayStatus.empty() )
		m_[prev_idate] = dayStatus;

	itd_ = m_.begin();
	lastIdate_ = m_.rbegin()->first;
}

RestrictedUS::~RestrictedUS()
{
}

void RestrictedUS::beginDay(int idate)
{
	itd_ = m_.find(idate);
}

bool RestrictedUS::isHalted(int idate, const string& ticker, int msecs)
{
	if( idate > lastIdate_ )
	{
		cerr << "RestrictedUS::isHalted() idate > lastIdate_\n";
		exit(29);
	}

	bool ret = false;
	if( idate != itd_->first )
		itd_ = m_.find(idate);
	if( itd_ != m_.end() )
	{
		auto itt = itd_->second.find(ticker);
		if( itt != itd_->second.end() )
		{
			int haltMsecs = itt->second;
			if( msecs >= haltMsecs )
				ret = true;
		}
	}
	return ret;
}

} // namespace gtlib
