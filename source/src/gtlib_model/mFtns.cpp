#include <gtlib_model/mFtns.h>
#include <gtlib_model/hff.h>
#include <gtlib_model/mdl.h>
#include <gtlib_signal/QuoteView.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GEX.h>
#include <boost/filesystem.hpp>
#include <numeric>
using namespace std;

namespace gtlib {

vector<int> getAllIdates(const std::string& model)
{
	vector<int> allIdates;
	if( model.size() >= 2 )
	{
		vector<string> markets = hff::markets(model);
		string model2 = model.substr(0, 2);
		vector<vector<string> > vvOK;
		if( "US" == model2 || "UF" == model2 )
		{
			char cmd[1000];
			sprintf(cmd, " select idate from tickdataelvok "
					//" where dataok = 1 and arcaok = 1 and inetok = 1 and nyseok = 1 "
					" where stocksvalid = 1 and enoughstock = 1 and arcaok = 1 and inetok = 1 and nyseok = 1 "
					" and (batsok = 1 or idate < 20071121) and (edgxok = 1 or idate < 20080710) "
					" order by idate ");
			GODBC::Instance()->read("equitydata", cmd, vvOK);
		}
		else if( "CA" == model2 )
		{
			char cmd[2000];
			sprintf(cmd, " select idate from tickdataok "
					" where market in ('J') and dataok = 1 and idate between 20050101 and 20090216 group by idate having count(*) = 1 "
					" union select idate from tickdataok "
					" where market in ('J', 'P', 'C') and dataok = 1 and idate between 20090217 and 20090831 group by idate having count(*) = 3 "
					" union select idate from tickdataok "
					" where market in ('J', 'P', 'H', 'C') and dataok = 1 and idate between 20090901 and 20130416 group by idate having count(*) = 4 "
					" union select idate from tickdataok "
					" where market in ('J', 'H', 'C') and dataok = 1 and idate between 20130417 and 20150918 group by idate having count(*) = 3 "
					" union select idate from tickdataok "
					" where market in ('J', 'C') and dataok = 1 and idate between 20150919 and 20180229 group by idate having count(*) = 2 "
					" union select idate from tickdataok "
					" where market in ('J', 'C', 'X') and dataok = 1 and idate between 20180301 and 20180311 group by idate having count(*) = 3 "
					" union select idate from tickdataok "
					" where market in ('J', 'C', 'X', 'E') and dataok = 1 and idate between 20180312 and 20180524 group by idate having count(*) = 4 "
					" union select idate from tickdataok "
					" where market in ('J', 'C', 'X', 'D', 'E') and dataok = 1 and idate between 20180525 and 99999999 group by idate having count(*) = 5 "
					" order by idate");
			GODBC::Instance()->read(mto::hf(markets[0]), cmd, vvOK);
		}
		else
		{
			string selMarket = "";
			for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
			{
				if( selMarket.empty() )
					selMarket += (string)"( ";
				else
					selMarket += (string)" or ";

				string market = *it;
				string code = mto::code(market);

				char buf[100];
				sprintf( buf, "market = '%s'", code.c_str() );
				selMarket += buf;
			}
			selMarket += " ) ";

			char cmd[1000];
			sprintf(cmd, " select idate from tickdataok "
					" where %s "
					" group by idate "
					" having sum(dataOK) >= %d "
					//" and count(*) = %d "
					" order by idate ",
					selMarket.c_str(),
					hff::minDataOK(model));
					//markets.size() );
			GODBC::Instance()->read(mto::hf(markets[0]), cmd, vvOK);
		}
		for( vector<vector<string> >::iterator it = vvOK.begin(); it != vvOK.end(); ++it )
		{
			int idate = atoi( (*it)[0].c_str() );
			if( idate > 0 )
				allIdates.push_back(idate);
		}
	}
	return allIdates;
}

map<string, vector<string>> getMarketTickers(const string& model2, const vector<string>& markets,
		int idateFrom, int idateTo, int nTickerMax)
{
	map<string, vector<string>> marketTickers;
	char cmd[1000];
	if( "US" == model2 || "CA" == model2 )
	{
		string market = markets[0];
		sprintf( cmd, "select distinct %s from stockcharacteristics "
				" where idate >= %d and idate <= %d "
				" %s "
				" and sectype >= 'A' and sectype <= 'Z' and sectype != 'P' and sectype != 'F' and sectype != 'X' "
				" and uniqueID is not null ",
				mto::compTicker(market).c_str(),
				idateFrom,
				idateTo,
				sel_univ(model2).c_str() );
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);

		int cnt = 0;
		vector<string> tickers;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if( !ticker.empty() && ++cnt <= nTickerMax )
				tickers.push_back(ticker);
		}

		tickers = comp_ticker(tickers, market);
		sort(tickers.begin(), tickers.end());
		marketTickers[market] = tickers;
	}
	else if( "UF" == model2 )
	{
		string market = markets[0];
		sprintf( cmd, "select distinct %s from stockcharacteristics "
				" where idate >= %d and idate <= %d "
				" %s "
				" and sectype >= 'A' and sectype <= 'Z' and sectype = 'F' "
				" and uniqueID is not null ",
				mto::compTicker(market).c_str(),
				idateFrom,
				idateTo,
				sel_univ(model2).c_str());
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);

		int cnt = 0;
		vector<string> tickers;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if( !ticker.empty() && ++cnt <= nTickerMax )
				tickers.push_back(ticker);
		}

		tickers = comp_ticker(tickers, market);
		sort(tickers.begin(), tickers.end());
		marketTickers[market] = tickers;
	}
	else
	{
		for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
		{
			string market = *it;
			sprintf( cmd, "select distinct %s from stockcharacteristics "
					" where market = '%s' and idate >= %d and idate <= %d %s "
					" and uniqueID is not null ",
					mto::compTicker(market).c_str(),
					mto::code(market).c_str(),
					idateFrom,
					idateTo,
					sel_univ(model2).c_str() );
			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			int cnt = 0;
			vector<string> tickers;
			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string ticker = trim((*it)[0]);
				if( !ticker.empty() )
				{
					if( !ticker.empty() && ++cnt <= nTickerMax/markets.size() )
						tickers.push_back(ticker);
				}
			}

			tickers = comp_ticker(tickers, market);
			sort(tickers.begin(), tickers.end());
			marketTickers[market] = tickers;
		}
	}
	return marketTickers;
}

map<string, vector<string>> getMarketTickersHighVol(const string& model2, const vector<string>& markets,
		int idateFrom, int idateTo, double retain)
{
	map<string, vector<string>> marketTickers;
	char cmd[1000];
	if( "US" == model2 || "CA" == model2 )
	{
		string market = markets[0];
		sprintf( cmd, "select %s, sum(nTrades*close_) as dv from stockcharacteristics "
				" where idate >= %d and idate <= %d "
				" %s "
				" and sectype >= 'A' and sectype <= 'Z' and sectype != 'P' and sectype != 'F' and sectype != 'X' "
				" and uniqueID is not null "
				" group by ticker "
				" order by dv ",
				mto::compTicker(market).c_str(),
				idateFrom,
				idateTo,
				sel_univ(model2).c_str() );
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);

		vector<string> tickers;
		int N = vv.size();
		int iFrom = N * (1. - retain);
		for( int i = iFrom; i < N; ++i )
		{
			string ticker = trim(vv[i][0]);
			if( !ticker.empty() )
				tickers.push_back(ticker);
		}

		tickers = comp_ticker(tickers, market);
		//sort(tickers.begin(), tickers.end());
		marketTickers[market] = tickers;
	}
	else if( "UF" == model2 )
	{
		string market = markets[0];
		sprintf( cmd, "select %s, sum(nTrades*close_) as dv from stockcharacteristics "
				" where idate >= %d and idate <= %d "
				" %s "
				" and sectype >= 'A' and sectype <= 'Z' and sectype = 'F' "
				" and uniqueID is not null "
				" group by market, symbol "
				" order by dv ",
				mto::compTicker(market).c_str(),
				idateFrom,
				idateTo,
				sel_univ(model2).c_str() );
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);

		vector<string> tickers;
		int N = vv.size();
		int iFrom = N * (1. - retain);
		for( int i = iFrom; i < N; ++i )
		{
			string ticker = trim(vv[i][0]);
			if( !ticker.empty() )
				tickers.push_back(ticker);
		}

		tickers = comp_ticker(tickers, market);
		//sort(tickers.begin(), tickers.end());
		marketTickers[market] = tickers;
	}
	else
	{
		int nTicker = 0;
		for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
		{
			string market = *it;
			sprintf( cmd, "select %s, sum(nTrades*close_) as dv from stockcharacteristics "
					" where market = '%s' and idate >= %d and idate <= %d %s "
					" and uniqueID is not null "
					" group by market, symbol "
					" order by dv ",
					mto::compTicker(market).c_str(),
					mto::code(market).c_str(),
					idateFrom,
					idateTo,
					sel_univ(model2).c_str() );
			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			vector<string> tickers;
			int N = vv.size();
			int iFrom = N * (1. - retain);
			for( int i = iFrom; i < N; ++i )
			{
				string ticker = trim(vv[i][0]);
				if( !ticker.empty() )
				{
					++nTicker;
					tickers.push_back(ticker);
				}
			}

			tickers = comp_ticker(tickers, market);
			//sort(tickers.begin(), tickers.end());
			marketTickers[market] = tickers;
		}
	}
	return marketTickers;
}

string sel_univ(const string& model2)
{
	string ret = "";
	if( "US" == model2 || "CA" == model2 )
		ret = " and (inuniverse = 1 or (close_ > .5 and close_ * medvolume > 60000 and medVolatility > .005 and close_ < 900 and medmedsprd >= .00008 and medmedsprd < .04))";
	else
		ret = " and inuniverse = 1 ";
	return ret;
}

vector<float> getThresSeries()
{
	//return {0., 1., 2., 3., 5., 7., 10., 13., 16., 19., 22., 25.};
	return {-100., 0., 1., 2., 3., 5., 10.};
}

vector<float> getBreakSeries()
{
	return {0, 10, 20, 30, 100, 1000};
}

vector<float> getMaxPosSeries()
{
	//return {2, 16, 64, 128, 256, 0};
	return {1, 2, 4, 8, 16, 64, 128, 256, 512, 0};
}

vector<pair<float, float>> getSprdSimpleRanges(const string& model)
{
	vector<pair<float, float>> ranges;
	ranges.push_back(make_pair(0., 20.));
	ranges.push_back(make_pair(20., 100.));
	ranges.push_back(make_pair(0., 100.));
	return ranges;
}

vector<pair<float, float>> getSprdFineRanges(const string& model)
{
	vector<pair<float, float>> ranges;
	double rangeMin = 0.;
	double rangeLen = 2.;
	while( rangeMin + rangeLen < 100. )
	{
		ranges.push_back(make_pair(rangeMin, rangeMin + rangeLen));
		rangeMin += rangeLen;
		if( rangeMin >= 20. )
			rangeLen = 5.;
	}
	return ranges;
}

vector<pair<float, float>> getSprdRanges(const string& model)
{
	vector<pair<float, float>> ranges;
	string model02 = model.substr(0, 2);
	if( model02 == "US" ||  model02 == "UF" )
	{
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 100.));
	}
	else if( model02 == "CA" )
	{
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 200.));
	}
	//else if( model02 == "EU" )
	else if( model02[0] == 'E' )
	{
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 100.));
	}
	else if( model02 == "AT" )
	{
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 200.));
	}
	else if( model02 == "AH" )
	{
		ranges.push_back(make_pair(0., 40.));
		ranges.push_back(make_pair(40., 200.));
	}
	else if( model02 == "AS" )
	{
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 200.));
	}
	else if( model02 == "KR" )
	{
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 100.));
	}
	else if( model02 == "MJ" )
	{
		ranges.push_back(make_pair(0., 5.));
		ranges.push_back(make_pair(5., 100.));
	}
	else if( model02 == "SS" )
	{
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 100.));
	}
	return ranges;
}

vector<pair<float, float>> getSprdMoreRanges(const string& model)
{
	vector<pair<float, float>> ranges;
	string model02 = model.substr(0, 2);
	if( model02 == "US" ||  model02 == "UF" )
	{
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 100.));
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 20.));
		ranges.push_back(make_pair(20., 30.));
		ranges.push_back(make_pair(30., 100.));
	}
	else if( model02 == "CA" )
	{
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 200.));
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 20.));
		ranges.push_back(make_pair(20., 40.));
		ranges.push_back(make_pair(40., 200.));
	}
	//else if( model02 == "EU" )
	else if( model02[0] == 'E' )
	{
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 100.));
		ranges.push_back(make_pair(0., 5.));
		ranges.push_back(make_pair(5., 10.));
		ranges.push_back(make_pair(10., 20.));
		ranges.push_back(make_pair(20., 100.));
	}
	else if( model02 == "AT" )
	{
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 200.));
		ranges.push_back(make_pair(0., 5.));
		ranges.push_back(make_pair(5., 10.));
		ranges.push_back(make_pair(10., 20.));
		ranges.push_back(make_pair(20., 200.));
	}
	else if( model02 == "AH" )
	{
		ranges.push_back(make_pair(0., 40.));
		ranges.push_back(make_pair(40., 200.));
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 40.));
		ranges.push_back(make_pair(40., 80.));
		ranges.push_back(make_pair(80., 200.));
	}
	else if( model02 == "AS" )
	{
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 200.));
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 20.));
		ranges.push_back(make_pair(20., 40.));
		ranges.push_back(make_pair(40., 200.));
	}
	else if( model02 == "KR" )
	{
		ranges.push_back(make_pair(0., 20.));
		ranges.push_back(make_pair(20., 100.));
		ranges.push_back(make_pair(0., 15.));
		ranges.push_back(make_pair(15., 20.));
		ranges.push_back(make_pair(20., 30.));
		ranges.push_back(make_pair(30., 100.));
	}
	else if( model02 == "MJ" )
	{
		ranges.push_back(make_pair(0., 5.));
		ranges.push_back(make_pair(5., 100.));
		ranges.push_back(make_pair(0., 3.));
		ranges.push_back(make_pair(3., 5.));
		ranges.push_back(make_pair(5., 10.));
		ranges.push_back(make_pair(10., 100.));
	}
	else if( model02 == "SS" )
	{
		ranges.push_back(make_pair(0., 10.));
		ranges.push_back(make_pair(10., 100.));
		ranges.push_back(make_pair(0., 5.));
		ranges.push_back(make_pair(5., 10.));
		ranges.push_back(make_pair(10., 20.));
		ranges.push_back(make_pair(20., 100.));
	}
	return ranges;
}

void get_mid_series(vector<double>& vMid1s, const vector<QuoteInfo>* quotes, int openMsecs, int closeMsecs, int lagMsecs, bool fill, bool remove_cross)
{
	int nQuotes = quotes->size();
	int n1sec = (closeMsecs - openMsecs) / 1000 + 1;

	// Lag the quotes.
	vector<QuoteInfo> laggedQuotes;
	laggedQuotes.reserve(nQuotes);
	int tnQuotes = 0;   // need to drop quotes after the close
	while ( (tnQuotes < nQuotes) && (((*quotes)[tnQuotes].msecs - openMsecs) * 0.001 < n1sec - 1) )
	{
		laggedQuotes.push_back( (*quotes)[tnQuotes] );
		laggedQuotes[tnQuotes].msecs -= lagMsecs;
		++tnQuotes;
	}

	// Initialize with -1.
	vMid1s = vector<double>(n1sec, -1.);

	double firstMidQt = 0.;
	for( int i = 0; i < tnQuotes; ++i )
	{
		int msecs = laggedQuotes[i].msecs;

		// Quotes after the market open.
		if( msecs + lagMsecs >= openMsecs )
		{
			// msecs [0, 999] written to t = 1 sec. [1000, 1999] to 2, etc.
			int index1s = (msecs - openMsecs) / 1000 + 1;
			if( index1s < 0 )
				index1s = 0;

			// Write mid if valid, 0 otherwise.
			const QuoteInfo& quote_p = laggedQuotes[i];
			if( (!remove_cross || quote_p.ask > quote_p.bid) && valid_quote(quote_p) )
			{
				double mid = .5 * (quote_p.ask + quote_p.bid);
				vMid1s[index1s] = mid;
				if( firstMidQt == 0. )
					firstMidQt = mid;
			}
			else
				vMid1s[index1s] = 0.;
		}
	}

	// Replace -1's.
	{
		int indx = 0;
		for( ; indx < n1sec; ++indx )
		{
			if( vMid1s[indx] >= 0. )
				break;
		}
		for( ; indx < n1sec; ++indx )
		{
			if( vMid1s[indx] < 0. )
				vMid1s[indx] = vMid1s[indx - 1];
		}
	}

	// Replace 0's.
	if( fill )
	{
		int indx = 0;

		// Before the first valid quote.
		for( ; indx < n1sec; ++indx )
		{
			if( vMid1s[indx] <= 0. )
				vMid1s[indx] = firstMidQt;
			else
				break;
		}

		// After the first valid quote.
		for( ; indx < n1sec; ++indx )
		{
			if( vMid1s[indx] <= 0. )
				vMid1s[indx] = vMid1s[indx - 1];
		}
	}
}

void get_mid_series_smooth(vector<double>& vMid1s, const vector<QuoteInfo>* quotes, int openMsecs, int closeMsecs, int lagMsecs, bool fill, bool remove_cross)
{
	int nQuotes = quotes->size();
	int n1sec = (closeMsecs - openMsecs) / 1000 + 1;

	// Lag the quotes.
	vector<QuoteInfo> laggedQuotes;
	laggedQuotes.reserve(nQuotes);
	int tnQuotes = 0;   // need to drop quotes after the close
	while ( (tnQuotes < nQuotes) && (((*quotes)[tnQuotes].msecs - openMsecs) * 0.001 < n1sec - 1) )
	{
		laggedQuotes.push_back( (*quotes)[tnQuotes] );
		laggedQuotes[tnQuotes].msecs -= lagMsecs;
		++tnQuotes;
	}

	// Initialize with -1.
	vMid1s = vector<double>(n1sec, -1.);

	double firstMidQt = 0.;
	double avgMid = 0.;
	for( int i = 0; i < tnQuotes; ++i )
	{
		int msecs = laggedQuotes[i].msecs;

		// Quotes after the market open.
		if( msecs + lagMsecs >= openMsecs )
		{
			// msecs [0, 999] written to t = 1 sec. [1000, 1999] to 2, etc.
			int index1s = (msecs - openMsecs) / 1000 + 1;
			if( index1s < 0 )
				index1s = 0;

			// Write mid if valid, 0 otherwise.
			const QuoteInfo& quote_p = laggedQuotes[i];
			if( (!remove_cross || quote_p.ask > quote_p.bid) && valid_quote(quote_p) )
			{
				double mid = .5 * (quote_p.ask + quote_p.bid);
				if(avgMid == 0. || fabs(avgMid - mid) < ltmb_)
					avgMid = mid;
				else
					avgMid = .5*avgMid + .5*mid;

				vMid1s[index1s] = avgMid;
				if( firstMidQt == 0. )
					firstMidQt = mid;
			}
			else
				vMid1s[index1s] = 0.;
		}
	}

	// Replace -1's.
	{
		int indx = 0;
		for( ; indx < n1sec; ++indx )
		{
			if( vMid1s[indx] >= 0. )
				break;
		}
		for( ; indx < n1sec; ++indx )
		{
			if( vMid1s[indx] < 0. )
				vMid1s[indx] = vMid1s[indx - 1];
		}
	}

	// Replace 0's.
	if( fill )
	{
		int indx = 0;

		// Before the first valid quote.
		for( ; indx < n1sec; ++indx )
		{
			if( vMid1s[indx] <= 0. )
				vMid1s[indx] = firstMidQt;
			else
				break;
		}

		// After the first valid quote.
		for( ; indx < n1sec; ++indx )
		{
			if( vMid1s[indx] <= 0. )
				vMid1s[indx] = vMid1s[indx - 1];
		}
	}
}

bool valid_quote(const QuoteInfo& quote, const double medMedSprd, const double minSpreadMMS, const double maxSpreadMMS)
{
	double min_price = 0.01;
	double skip_qt = 500.;
	double mid = .5 * (quote.ask + quote.bid);
	bool valid_size = quote.bidSize > 0 && quote.askSize > 0;
	bool valid_price = mid > min_price;

	double sprd = basis_pts_ * (quote.ask - quote.bid) / mid;
	bool valid_sprd = false;
	if( medMedSprd == 0. )
		valid_sprd = fabs(sprd) <= skip_qt;
	else
	{
		bool valid_sprd_lower = (minSpreadMMS == 0.) ? sprd > -skip_qt : sprd > basis_pts_ * minSpreadMMS * medMedSprd;
		bool valid_sprd_upper = (maxSpreadMMS == 0.) ? sprd < skip_qt : sprd < basis_pts_ * maxSpreadMMS * medMedSprd;
		valid_sprd = valid_sprd_lower && valid_sprd_upper;
	}
	//bool valid_sprd = (medMedSprd == 0.) ? (fabs(sprd) <= skip_qt_) : sprd > -2. * medMedSprd && (sprd < skip_qt_ || sprd < 5. * medMedSprd);

	bool valid = valid_size && valid_sprd && valid_price;
	//bool valid = quote.bidSize > 0 && quote.askSize > 0 && fabs(sprd) <= skip_qt_ && mid > min_price_ /*&& quote.ask - quote.bid > 0.00005*/;
	return valid;
}

bool valid_trade(const TradeInfo& trade, const QuoteInfo& quote)
{
	bool tradeok = trade.msecs > 0 && trade.price > 0. && trade.qty > 0;
	return tradeok;
}

unordered_map<string, float> getClose(const string& model, int idate)
{
	vector<string> mkts = mdl::markets(model.substr(0, 2));
	string market = mkts[0];

	char cmd[2000];
	sprintf( cmd, "select %s, close_ from stockcharacteristics where idate = %d",
			mto::compTicker(market).c_str(), idate);

	unordered_map<string, float> m;
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		float closePrc = atof((*it)[1].c_str());
		if( !ticker.empty() )
		{
			m[ticker] = closePrc;
		}
	}
	return m;
}

unordered_map<string, float> getVolatNorm(const string& model, int idate)
{
	vector<string> mkts = mdl::markets(model.substr(0, 2));
	string market = mkts[0];

	//int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();
	char cmd[2000];
	sprintf( cmd, "select %s, high, low, volatility, medvolatility from stockcharacteristics where idate = %d",
			mto::compTicker(market).c_str(), idate);

	unordered_map<string, float> mvol;
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		float highPrc = atof((*it)[1].c_str());
		float lowPrc = atof((*it)[2].c_str());
		float vol = atof((*it)[3].c_str());
		float medvolat = atof((*it)[4].c_str());
		//if( !ticker.empty() && vol > 0. && medvolat > 0.)
		if(!ticker.empty())
		{
			if(medvolat > 0.)
			{
				if(lowPrc > 0. && highPrc > lowPrc)
				{
					float lhl = log(highPrc / lowPrc);
					mvol[ticker] = lhl*lhl / medvolat;
				}
				else
					//mvol[ticker] = log(1.)*log(1.) / medvolat;
					mvol[ticker] = -1;
			}
		}
	}
	return mvol;
}

unordered_map<string, float> getLogVol(const string& model, int idate)
{
	vector<string> mkts = mdl::markets(model.substr(0, 2));
	string market = mkts[0];

	int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();
	char cmd[2000];
	sprintf( cmd, "select %s, medVolume from stockcharacteristics where idate = %d",
			mto::compTicker(market).c_str(), idate_prev);

	unordered_map<string, float> mvol;
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		float vol = atof((*it)[1].c_str());
		if( !ticker.empty() && vol > 1. )
			mvol[ticker] = log(vol);
	}
	return mvol;
}

unordered_map<string, float> getVolDepThres(const string& model, int idate)
{
	vector<string> mkts = mdl::markets(model.substr(0, 2));
	string market = mkts[0];

	string symbolTitle = "symbol";
	if( mto::isInternational(market) )
		symbolTitle = "exchange + ':' + symbol";

	char cmd[2000];
	sprintf( cmd, "select %s, thresin from hforderparams where idate = %d",
			symbolTitle.c_str(),
			idate);

	unordered_map<string, float> mthres;
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		float thres = atof((*it)[1].c_str());
		if( !ticker.empty() && thres > 0. )
			mthres[ticker] = thres;
	}
	return mthres;
}

unordered_set<string> getTradedTickers(const string& model, int idate)
{
	vector<string> mkts = mdl::markets(model.substr(0, 2));
	string market = mkts[0];

	char cmd[2000];
	sprintf( cmd, "select distinct symbol from hforderevents where idate = %d and qty > 0", idate);

	unordered_set<string> tradedTickers;
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		if( !ticker.empty() )
			tradedTickers.insert(ticker);
	}
	return tradedTickers;
}

vector<int> getSampleMsecs(int openMsecs, int closeMsecs, int stepSec, int seed)
{
	int stepMsecs = stepSec * 1000;
	vector<int> sampleMsecs;
	if( stepMsecs > 0 && std::abs(seed) < stepMsecs / 2 )
	{
		int noise = seed;
		int tMin = openMsecs + stepMsecs;
		int tMax = closeMsecs;
		for( int msecs = tMin; msecs < tMax; msecs += stepMsecs, noise *= -1 )
			sampleMsecs.push_back(msecs + noise);
	}
	return sampleMsecs;
}

vector<int> getSampleMsecsIrreg(const string& ticker, int idate, int openMsecs, int closeMsecs, int stepSec)
{
	string baseTicker = base_name(ticker);
	int N = baseTicker.size();
	double prod = idate / 10000.;
	for( int i = 0; i < N; ++i )
		prod = fmod(prod * baseTicker[i], stepSec);
	int seed = 1000 * (static_cast<int>(ceil(prod)) % (stepSec * 2 / 3) - stepSec / 3);
	return getSampleMsecs(openMsecs, closeMsecs, stepSec, seed);
}

vector<int> getSampleMsecs(const MercatorVolume& mercVol, const string& ticker, int openMsecs, int closeMsecs, int stepSec)
{
	int tickerStepSec = mercVol.getSampleInterval(ticker, stepSec);
	vector<int> vMsecs = getSampleMsecs(openMsecs, closeMsecs, tickerStepSec);
	return vMsecs;
}

vector<int> getSampleMsecsVolat(int openMsecs, int closeMsecs, int stepSec, float medVolat, const vector<TradeInfo>* trades, Sessions& sessions)
{
	static map<float, float> volatWgt = {
		{0.014, 0.405}, {0.039, 0.504}, {0.06, 0.548}, {0.087, 0.865}, {0.126, 0.950}, {0.194, 1.561},
		{0.237, 1.636}, {0.302, 1.754}, {0.418, 2.749}, {0.566, 3.805}, {max_float_, 3.805}};
	QuoteView qv(nullptr, trades, sessions, openMsecs, closeMsecs, 1000, true, false, false);
	vector<float> vWgt;
	int nSecs = (closeMsecs - openMsecs) / 1000;
	if(trades == nullptr || trades->empty())
	{
		vWgt = vector<float>(nSecs, 1.);
	}
	else
	{
		for(int msecs = openMsecs; msecs <= closeMsecs; msecs += 1000)
		{
			double volat = medVolat > 0 ? qv.getHiLo(msecs) / medVolat : 0.;
			double wgt = volatWgt.lower_bound(volat)->second;
			vWgt.push_back(wgt);
		}
	}
	vector<int> vMsecs;
	double sumWgt = 0.;
	int prevStep = 0.;
	for(int i = 0; i < nSecs; ++i)
	{
		sumWgt += vWgt[i];
		if(sumWgt >= prevStep * stepSec)
		{
			vMsecs.push_back(i * 1000 + openMsecs);
			prevStep = sumWgt / stepSec + 1;
		}
	}
	return vMsecs;
}

} // namespace gtlib
