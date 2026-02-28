#include <HTickSeries/TickSeriesFtns.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <algorithm>
#include <iostream>
using namespace std;

namespace TickSeriesFtns
{
	void get_quote_series( vector<double>& msecQ, vector<double>& midQ,
				vector<double>& bidQ, vector<double>& askQ,
				const TickSeries<QuoteInfo>* ptsQ, vector<pair<int, int> >& sessions)
	{
		int ntq = ptsQ->NTicks();
		const_cast<TickSeries<QuoteInfo>*>(ptsQ)->StartRead();
		QuoteInfo quote;
		for( int n = 0; n < ntq; ++n )
		{
			const_cast<TickSeries<QuoteInfo>*>(ptsQ)->Read(&quote);
			int ms = quote.msecs;
			bool inrange = find_if(sessions.begin(), sessions.end(), bind2nd(inRange(), ms)) != sessions.end();
			if( inrange )
			{
				double bid = quote.bid;
				double ask = quote.ask;
				if( bid < ask )
				{
					double mid = (ask + bid)/2.0;
					double sprd = (ask - bid)/mid;
					if( fabs(sprd) < 0.05 )
					{
						msecQ.push_back(quote.msecs);
						midQ.push_back(mid);
						bidQ.push_back(bid);
						askQ.push_back(ask);
					}
				}
			}
		}
		return;
	}

	void get_quote_series( vector<double>& msecQ, vector<QuoteInfo>& Q,
				const TickSeries<QuoteInfo>* ptsQ, Sessions& sessions)
	{
		int ntq = ptsQ->NTicks();
		const_cast<TickSeries<QuoteInfo>*>(ptsQ)->StartRead();
		QuoteInfo quote;
		for( int n = 0; n < ntq; ++n )
		{
			const_cast<TickSeries<QuoteInfo>*>(ptsQ)->Read(&quote);
			int ms = quote.msecs;
			bool inrange = sessions.isAfterOpenBeforeClose(ms);
			if( inrange )
			{
				msecQ.push_back(quote.msecs);
				Q.push_back(quote);
			}
		}
		return;
	}

	void get_quote_1s_series( vector<double>& msecQ1s, vector<double>& midQ1s,
				vector<double>& bidQ1s, vector<double>& askQ1s,
				vector<double>& msecQ, vector<double>& midQ,
				vector<double>& bidQ, vector<double>& askQ)
	{
		int NQ = msecQ.size();

		// Fill 1 sec quote price series
		if( NQ > 0 )
		{
			int sec1Q = msecQ[0] / 1000 + 1;
			int sec2Q = msecQ[NQ - 1] / 1000;
			if( sec1Q <= sec2Q )
			{
				msecQ1s = vector<double>(sec2Q - sec1Q + 1, 0);
				midQ1s = vector<double>(sec2Q - sec1Q + 1, 0);
				bidQ1s = vector<double>(sec2Q - sec1Q + 1, 0);
				askQ1s = vector<double>(sec2Q - sec1Q + 1, 0);
				{
					int iQ = NQ - 1;
					for( int sec = sec2Q; sec >= sec1Q; --sec )
					{
						msecQ1s[sec - sec1Q] = sec*1000;
						while( iQ > 0 && msecQ[iQ]/1000 >= sec )
							--iQ;

						if( msecQ[iQ] / 1000 < sec )
						{
							double bid = bidQ[iQ];
							double ask = askQ[iQ];
							double mid = (ask + bid) / 2.0;
							int index = sec - sec1Q;
							midQ1s[index] = mid;
							bidQ1s[index] = bid;
							askQ1s[index] = ask;

							if( iQ + 1 < NQ )
								++iQ;
						}
					}
				}
			}

			// Replace zeros.
			replace_zeros(midQ);
			replace_zeros(midQ1s);
		}
		return;
	}
	
	void replace_zeros(vector<double>& series)
	{
		double last = 0;
		int N = series.size();
		for( int i=0; i<N; ++i )
		{
			if( series[i] < 1e-6 )
				series[i] = last;

			if( series[i] > 1e-6 )
				last = series[i];
		}

		int izero = 0;
		for( int i=0; i<N; ++i )
		{
			double prc = series[i];
			if( prc > 1e-6 )
			{
				for( int j=0; j<izero; ++j )
					series[j] = prc;
				break;
			}
			else
				++izero;
		}

		for( int i=0; i<N; ++i )
		{
			double prc = series[i];
			if( prc < 1e-6 )
			{
				cout << i << "th price " << prc << " out of range" << endl;
				exit(4);
			}
		}
		return;
	}
}
