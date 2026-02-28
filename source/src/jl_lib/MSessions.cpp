#include <jl_lib/MSessions.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <vector>
#include <algorithm>
using namespace std;

MSessions::MSessions()
{}

MSessions::MSessions(const string& market, int idate)
{
	vector<pair<int, int> > b;
	if( market != "EF" ) // Do not exclude the intraday auction period at Xetra.
		b = mto::breaks(market, 0, idate);

	int nMSessions = b.size() + 1;
	for( int s = 0; s < nMSessions; ++s )
	{
		// Beginning of session
		int msec1 = 0;
		if( s == 0 ) // first session
			msec1 = mto::msecOpen(market, idate);
		else
			msec1 = b[s - 1].second;

		// End of session
		int msec2 = 0;
		if( s == nMSessions - 1 ) // last session
			msec2 = mto::msecClose(market, idate);
		else
			msec2 = b[s].first;

		v_.push_back(make_pair( msec1, msec2 ));
	}
}

bool MSessions::isAfterOpenBeforeClose(int msecs) const
{
	if(msecs > v_.begin()->first && msecs < v_.rbegin()->second)
		return true;
	return false;
}

bool MSessions::inSessionStrict(int msecs) const
{
	bool insession = find_if(v_.begin(), v_.end(), bind2nd(inRangeStrict(), msecs)) != v_.end();
	return insession;
}

bool MSessions::inSessionStrictWait(int msecs, int waitSec) const
{
	bool insession = find_if(v_.begin(), v_.end(), bind2nd(inRangeStrict(), msecs)) != v_.end();
	bool clearWait = true;
	for(auto& p : v_)
	{
		if(msecs >= p.first && msecs < p.first + waitSec*1000)
		{
			clearWait = false;
			break;
		}
	}
	return insession && clearWait;
}
