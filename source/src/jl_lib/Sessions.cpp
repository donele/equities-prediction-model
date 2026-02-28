#include <jl_lib/Sessions.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <vector>
#include <algorithm>
using namespace std;

Sessions::Sessions()
{}

Sessions::Sessions(const string& market, int idate, int delayOpen, int delayClose)
{
	vector<pair<int, int> > b = mto::breaks(market, 0, idate);
	int nSessions = b.size() + 1;
	for( int s = 0; s < nSessions; ++s )
	{
		int delay1 = 0;
		int delay2 = 0;

		// Beginning of session
		int msec1 = 0;
		if( s == 0 ) // first session
		{
			msec1 = mto::msecOpen(market, idate);
			delay1 = delayOpen;
		}
		else
			msec1 = b[s - 1].second;

		// End of session
		int msec2 = 0;
		if( s == nSessions - 1 ) // last session
		{
			msec2 = mto::msecClose(market, idate);
			delay2 = delayClose;
		}
		else
			msec2 = b[s].first;

		v_.push_back(make_pair( msec1 + delay1 * 1000, msec2 - delay2 * 1000 ));
	}
}

void Sessions::add_session(const pair<int, int>& p)
{
	v_.push_back(p);
	return;
}

bool Sessions::isAfterOpenBeforeClose(int msecs) const
{
	if(msecs > v_.begin()->first && msecs < v_.rbegin()->second)
		return true;
	return false;
}

bool Sessions::inSession(int msecs) const
{
	bool insession = find_if(v_.begin(), v_.end(), bind2nd(inRange(), msecs)) != v_.end();
	return insession;
}

bool Sessions::inSession(int msecs1, int msecs2) const
{
	if( msecs1 <= msecs2 )
	{
		for( auto it = v_.begin(); it != v_.end(); ++it )
			if( it->first <= msecs1 && msecs2 <= it->second )
				return true;
	}
	return false;
}
	
bool Sessions::inSessionStrict(int msecs) const
{
	bool insession = find_if(v_.begin(), v_.end(), bind2nd(inRangeStrict(), msecs)) != v_.end();
	return insession;
}

bool Sessions::inSessionStrictWait(int msecs, int waitSec) const
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

int Sessions::length()
{
	int len = 0;
	for( auto it = v_.begin(); it != v_.end(); ++it )
		len += it->second - it->first;
	return len;
}
