#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/Book.h>
#include <string>
#include <algorithm>
using namespace std;

void order2quote( TickSeries<OrderData>& tsOin, TickSeries<QuoteInfo>& tsQ,
				 int lot, bool uncross, bool oneQuotePerMsec, char exCode )
{
	Book book(uncross);

	OrderData order;
	QuoteInfo last_quote;
	QuoteInfo quote;
	quote.askEx = exCode;
	quote.bidEx = exCode;
	tsOin.StartRead();
	int nTicks = tsOin.NTicks();
	for( int u=0; u<nTicks; ++u )
	{
		tsOin.Read(&order);

		// update book
		if( !book.update(order) )
		{
			//cerr << idate << " " << symbol << " " << order.msecs << endl;
			//cerr << book << endl;
		}

		// update the quote
		if( book.get_quote(quote, lot) )
		{
			if( oneQuotePerMsec )
			{
				if( last_quote.msecs > 0 && last_quote.msecs < quote.msecs )
					tsQ.Write(last_quote);
				last_quote = quote;
			}
			else
				tsQ.Write(quote);
		}
	}
	if( oneQuotePerMsec)
		tsQ.Write(last_quote);

	return;
}

void get_auction( const string& market, int idate, const string& ticker, const vector<TradeInfo>& trades,
		int openMsecs, int closeMsecs, double& o, int& oSize, double& c, int& cSize, int& oMsecs, int& cMsecs )
{
	int N = trades.size();
	if( N < 1 )
		return;

	oMsecs = 0;
	cMsecs = 0;

	if( "AT" == market )
	{
		const TradeInfo& tradeOpen = trades[0];
		if( tradeOpen.msecs < openMsecs + 30 * 60 * 1000 )
		{
			o = tradeOpen.price;
			oSize = tradeOpen.qty;
			oMsecs = tradeOpen.msecs;
		}

		const TradeInfo& tradeClose = trades[N - 1];
		if( tradeClose.msecs > closeMsecs )
		{
			c = tradeClose.price;
			cSize = tradeClose.qty;
			cMsecs = tradeClose.msecs;
		}

		if( oMsecs == 0 )
			oMsecs = tradeOpen.msecs;
		if( cMsecs == 0 ) // first trade after close.
		{
			for( auto it = trades.begin(); it != trades.end(); ++it )
			{
				if( it->msecs > closeMsecs )
				{
					cMsecs = it->msecs;
					break;
				}
			}
		}
	}
	else if( "AS" == market )
	{
		if( idate == 0 )
			exit(8);

		int oa_min_msecs = openMsecs - 10 * 60 * 1000 - 15 * 1000;
		int oa_max_msecs = openMsecs + 1000;
		get_auction_AS(trades, oa_min_msecs, oa_max_msecs, o, oSize, oMsecs);

		int ca_min_msecs = closeMsecs + 10 * 60 * 1000 - 15 * 1000;
		int ca_max_msecs = closeMsecs + 12 * 60 * 1000 + 1000;
		get_auction_AS(trades, ca_min_msecs, ca_max_msecs, c, cSize, cMsecs);
		//if( idate >= 20110118 || (idate >= 20100513 && idate < 20101111) || (idate >= 20080311 && idate < 20100226) ) // Quanthouse
		//{
		//	get_open_auction_AS(trades, openMsecs, o, oSize, oMsecs);
		//	get_close_auction_AS(trades, closeMsecs, c, cSize, cMsecs);
		//}
		//else // Reuters data.
		//{
		//	get_open_auction_AS_hist(trades, openMsecs, o, oSize, oMsecs);
		//	get_close_auction_AS_hist(trades, closeMsecs, c, cSize, cMsecs);
		//}

		if( oMsecs == 0 )
		{
			char letter = baseTicker(ticker)[0];
			if( letter >= 'A' && letter <= 'B' )
				oMsecs = openMsecs - 15 * 1000;
			else if( letter >= 'C' && letter <= 'F' )
				oMsecs = openMsecs + 2 * 60 * 1000;
			else if( letter >= 'G' && letter <= 'M' )
				oMsecs = openMsecs + 4 * 60 * 1000 + 15 * 1000;
			else if( letter >= 'N' && letter <= 'R' )
				oMsecs = openMsecs + 6 * 60 * 1000 + 30 * 1000;
			else if( letter >= 'S' && letter <= 'Z' )
				oMsecs = openMsecs + 8 * 60 * 1000 + 45 * 1000;
			else
				oMsecs = openMsecs;
		}
		if( cMsecs == 0 )
			cMsecs = closeMsecs + 10*60*1000;
	}
}

void get_auction_AS( const vector<TradeInfo>& trades, int minMsecs, int maxMsecs, double& ret_price, int& ret_size, int& ret_msecs )
{
	int minGap = 50;
	int maxGap = 200;
	int N = trades.size();
	int prev_msecs = 0;
	int max_cnt = 0;
	for( int i = 0; i < N; ++i )
	{
		const TradeInfo& trade = trades[i];
		if( trade.msecs > maxMsecs )
			break;

		// Begin subset.
		if( trade.msecs > minMsecs && trade.msecs > prev_msecs + minGap )
		{
			int sum = 0;
			int cnt = 0;
			int auction_msecs = trade.msecs;
			int prev_trade_msecs = auction_msecs;
			double auction_price = trade.price;
			for( int j = i; j < N; ++j )
			{
				const TradeInfo& trade2 = trades[j];
				if( fabs((float)auction_price - trade2.price) < 1e-4 && trade2.msecs - prev_trade_msecs < maxGap && trade2.msecs - auction_msecs < 5000 )
				{
					sum += trade2.qty;
					++cnt;
					prev_trade_msecs = trade2.msecs;
				}
				else
					break;
			}
			if( cnt > max_cnt )
			{
				max_cnt = cnt;
				ret_price = auction_price;
				ret_size = sum;
				ret_msecs = auction_msecs;
			}
		}

		prev_msecs = trade.msecs;
	}
}

vector<float> get_return_1s_bpt(const vector<QuoteInfo>* pQuote, int openMsecs, int closeMsecs)
{
	vector<float> vRet = get_return_1s(pQuote, openMsecs, closeMsecs);
	std::transform(vRet.begin(), vRet.end(), vRet.begin(), [](float x) {return x * basis_pts_;});
	return vRet;
}

vector<float> get_return_1s(const vector<QuoteInfo>* pQuote, int openMsecs, int closeMsecs)
{
	double sprdMax = 0.05;
	double retMax = 0.02;
	int NS = closeMsecs / 1000 - openMsecs / 1000;
	std::vector<float> vRet(NS, 0.);
	std::vector<int> vInd = get_index_1s(pQuote, openMsecs, closeMsecs); // vInd.size() = NS + 1.
	for( int i = 0; i < NS; ++i )
	{
		float ret = 0.;
		int fromIndx = vInd[i];
		int toIndx = vInd[i + 1];
		if( fromIndx >= 0 && toIndx >= 0 )
		{
			const QuoteInfo& quoteFrom = (*pQuote)[fromIndx];
			const QuoteInfo& quoteTo = (*pQuote)[toIndx];

			double midFrom = get_mid(quoteFrom);
			double midTo = get_mid(quoteTo);
			bool valid_price = midFrom > 0. && midTo > 0.;

			double sprdFrom = get_sprd(quoteFrom.bid, quoteFrom.ask);
			double sprdTo = get_sprd(quoteTo.bid, quoteTo.ask);
			bool valid_sprd = fabs(sprdFrom) < sprdMax && fabs(sprdTo) < sprdMax;

			double temp_ret = midTo / midFrom - 1.;
			bool valid_ret = fabs(temp_ret) < retMax;

			if( valid_price && valid_sprd && valid_ret )
				ret = temp_ret;
		}
		vRet[i] = ret;
	}
	return vRet;
}
