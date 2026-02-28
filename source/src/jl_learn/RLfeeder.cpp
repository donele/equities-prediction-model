#include <jl_learn/RLfeeder.h>
#include <jl_learn/RLobj.h>
#include <jl_lib/jlutil.h>
#include <map>
using namespace std;

RLfeeder::RLfeeder()
{}

bool RLfeeder::empty()
{
	return tickerPrcs_.empty();
}

void RLfeeder::addTicker(string ticker, vector<QuoteInfo>& series)
{
	tickerPrcs_.insert( make_pair(ticker, series) );
	map<string, vector<QuoteInfo> >::iterator mit = tickerPrcs_.find(ticker);
	vqi itB = mit->second.begin();
	vqi itE = mit->second.end();
	tickerIters_.insert( make_pair(ticker, make_pair(itE, itB)) );
	tickerEnds_.insert( make_pair(ticker, itE) );
	return;
}

bool RLfeeder::advance()
{
	while(1)
	{
		// Find the ticker with a tick at the closest future.
		multimap<int, string> mTimeTicker;
		map<string, pair<vqi, vqi> >::iterator it;
		for( it = tickerIters_.begin(); it != tickerIters_.end(); ++it )
		{
			string ticker = it->first;
			vqi it2 = it->second.second;
			vqi itE = tickerEnds_[ticker];
			if( it2 != itE )
				mTimeTicker.insert( make_pair(it2->msecs, ticker) );
		}

		if( !mTimeTicker.empty() )
		{
			int msecs = mTimeTicker.begin()->first;
			multimap<int, string>::iterator itm;
			multimap<int, string>::iterator upperBound = mTimeTicker.upper_bound(msecs);

			// Advance the iterators for the markets.
			for( itm = mTimeTicker.begin(); itm != upperBound; ++itm )
			{
				string ticker = itm->second;
				vqi& it1 = tickerIters_[ticker].first;
				vqi& it2 = tickerIters_[ticker].second;
				it1 = it2++;

				double bid = it1->bid;
				double ask = it1->ask;
				if( bid > ltmb_ && ask > ltmb_ && ask > bid )
				{
					double sprd = 2.0 * (ask - bid) / (ask + bid);
					if( sprd < 0.05 )
					{
						double mid = (ask + bid) / 2.0;
						ri_ = RLinput(ticker, mid);
						return true;
					}
				}
			}
		}
		else
			return false;
	}

	return false;
}

RLinput RLfeeder::next()
{
	return ri_;
}

void RLfeeder::endDay()
{
	ri_ = RLinput("", 0);
	tickerPrcs_.clear();
	tickerIters_.clear();
	tickerEnds_.clear();

	return;
}