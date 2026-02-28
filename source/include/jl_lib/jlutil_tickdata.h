#ifndef __jlutil_tickdata__
#define __jlutil_tickdata__
#include <optionlibs/TickData.h>
#include <vector>
#include <jl_lib/mto.h>
#include <jl_lib/Sessions.h>
#include <jl_lib/crossCompile.h>

FUNC_DECLSPEC void order2quote(TickSeries<OrderData>& tsOin, TickSeries<QuoteInfo>& tsQ, int lot,
									   bool uncross = false, bool oneQuotePerMsec = false, char exCode = '\0');
FUNC_DECLSPEC void get_auction(const std::string& market, int idate, const std::string& ticker,
		const std::vector<TradeInfo>& trades, int openMsecs, int closeMsecs,
		double& o, int& oSize, double& c, int& cSize, int& oMsecs, int& cMsecs);
FUNC_DECLSPEC void get_auction_AS( const std::vector<TradeInfo>& trades, int minMsecs, int maxMsecs,
		double& price, int& szie, int& msecs );

std::vector<float> get_return_1s_bpt(const std::vector<QuoteInfo>* pQuote, int openMsecs, int closeMsecs);
std::vector<float> get_return_1s(const std::vector<QuoteInfo>* pQuote, int openMsecs, int closeMsecs);

template<typename T>
std::vector<int> get_index_1s(const std::vector<T>* pTick, int openMsecs, int closeMsecs)
{

	int NS = closeMsecs / 1000 - openMsecs / 1000 + 1;
	std::vector<int> vIndx1s(NS, -1);
	if( pTick != nullptr )
	{
		int secMax = NS - 1;
		int nTick = pTick->size();
		int secT = -1; // Set to -1 in order to react on 0.
		int secV = 0;
		int indexT = -1; // -1 means no data available yet.
		for(int i = 0; i < nTick; ++i)
		{
			int msecs = (*pTick)[i].msecs;
			int msso = (msecs - openMsecs);
			if( msso > 0 ) // After open only.
			{
				int sec = msso / 1000; // Most recent second prior to msecs.
				if( sec > secT ) // New data available after secT.
				{
					secT = std::min(sec, secMax);
					while( secV <= secT ) // Fill up to the most recent second prior to msecs.
						vIndx1s[secV++] = indexT;
				}
				if( msecs < closeMsecs )
					indexT = i; // Update indexT for every tickdata after the open and before the close.
				else
					break;
			}
		}
		while( secV <= secMax ) // Any remaining.
			vIndx1s[secV++] = indexT;
	}
	return vIndx1s;
}

template<class T> CLASS_DECLSPEC
void read_tickdata(std::vector<T>& v, const std::string& market, int idate,
		const std::string& ticker, const std::vector<std::string>& dirs)
{
	TickAccessMulti<T> ta;
	for( auto it = dirs.begin(); it != dirs.end(); ++it )
		ta.AddRoot(*it, mto::longTicker(market));

	TickSeries<T> ts(200000, 1.2);
	ta.GetTickSeries( ticker, idate, &ts );
	int nTicks =  ts.NTicks();

	v.clear();
	v.reserve(nTicks);

	ts.StartRead();
	T object;
	for( int i = 0; i < nTicks; ++i )
	{
		ts.Read(&object);
		v.push_back(object);
	}
}
template<class T> CLASS_DECLSPEC
void read_tickdata(std::vector<T>& v, const std::string& market, int idate, const std::string& ticker,
		const std::vector<std::string>& dirs, const Sessions& sessions)
{
	TickAccessMulti<T> ta;
	for( auto it = dirs.begin(); it != dirs.end(); ++it )
		ta.AddRoot(*it, mto::longTicker(market));

	TickSeries<T> ts(200000, 1.2);
	ta.GetTickSeries( ticker, idate, &ts );
	int nTicks =  ts.NTicks();

	v.clear();
	v.reserve(nTicks);

	ts.StartRead();
	T object;
	for( int i = 0; i < nTicks; ++i )
	{
		ts.Read(&object);
		if( sessions.inSession(object.msecs) )
			v.push_back(object);
	}
}

#endif
