#include <MFitting/TickerPosition.h>
#include <cstdlib>
using namespace std;

void TickerPosition::add(int msecs, int shares)
{
	if( (shares > 0 && position >= 0) || (shares < 0 && position <= 0) ) // accumulate the position.
	{
		position += shares;
		mMsecsPos[msecs] += shares;
	}
	else if( (shares > 0 && position < 0) || (shares < 0 && position > 0) ) //
	{
		position += shares;
		int remain = shares;
		for( map<int, int>::iterator it = mMsecsPos.begin(); it != mMsecsPos.end() && remain != 0; )
		{
			if( (it->second > 0 && remain < 0) || (it->second < 0 && remain > 0) ) // opposite sign.
			{
				int abs_remain = abs(remain);
				int abs_position = abs(it->second);
				if( abs_remain >= abs_position )
				{
					remain += it->second;
					mMsecsPos.erase(it++);
				}
				else
				{
					it->second += remain;
					remain = 0;
					++it;
				}
			}
			else
				exit(7);
		}

		if( remain != 0 )
			mMsecsPos[msecs] = remain;
	}
}

int TickerPosition::adjpos(int msecs, int hold)
{
	int pos = 0;
	int maxMsecs = msecs - hold * 60 * 1000;
	for( map<int, int>::iterator it = mMsecsPos.begin(); it != mMsecsPos.end(); ++it )
	{
		if( it->first <= maxMsecs )
			pos += it->second;
		else
			break;
	}
	return pos;
}

void TickerPosition::beginDay()
{
	int lastPos = position;
	mMsecsPos.clear();
	if( lastPos != 0 )
		mMsecsPos[0] = lastPos;
}
