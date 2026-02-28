#include <HLearn/NewsSignal.h>
#include <jl_lib.h>
using namespace std;

NewsSignal::NewsSignal()
{
	vDays_.push_back(1);
	vDays_.push_back(5);
	vDays_.push_back(20);
	vViews_.push_back("ticker");
	vViews_.push_back("sector");
	vViews_.push_back("market");
}

vector<string> NewsSignal::get_names()
{
	vector<string> ret;
	char name[100];

	ret.push_back("newsVol");

	for( vector<string>::iterator itv = vViews_.begin(); itv != vViews_.end(); ++itv )
	{
		string view = *itv;
		string pre = "";
		if( "sector" == view )
			pre = "s";
		else if( "market" == view )
			pre = "m";

		for( vector<int>::iterator itd = vDays_.begin(); itd != vDays_.end(); ++itd )
		{
			int nday = *itd;
			sprintf(name, "%ssent%d", pre.c_str(), nday);
			ret.push_back(name);
		}
	}

	for( vector<string>::iterator itv = vViews_.begin(); itv != vViews_.end(); ++itv )
	{
		string view = *itv;

		string pre = "";
		if( "ticker" == view )
			pre = "a";
		else if( "sector" == view )
			pre = "s";
		else if( "market" == view )
			pre = "m";

		for( vector<int>::iterator itd = vDays_.begin(); itd != vDays_.end(); ++itd )
		{
			int nday = *itd;
			for( vector<int>::iterator itd2 = itd; itd2 != vDays_.end(); ++itd2 )
			{
				int ndayRef = *itd2;

				if( "ticker" == view && nday != ndayRef )
				{
					sprintf(name, "%scosent%d_%d", pre.c_str(), nday, ndayRef);
					ret.push_back(name);
				}
				else if ( "ticker" != view )
				{
					sprintf(name, "%scosent%d_%d", pre.c_str(), nday, ndayRef);
					ret.push_back(name);
				}
			}
		}
	}

	for( vector<int>::iterator itd = vDays_.begin(); itd != vDays_.end(); ++itd )
	{
		int nday = *itd;
		for( vector<int>::iterator itd2 = itd; itd2 != vDays_.end(); ++itd2 )
		{
			int ndayRef = *itd2;

			sprintf(name, "mscosent%d_%d", nday, ndayRef);
			ret.push_back(name);
		}
	}

	return ret;
}

vector<float> NewsSignal::get_signals(string ticker, int msecs)
{
	vector<float> ret;

	string market = market_;
	if( "EU" == market_ )
		market = (string)"E" + ticker.substr(0,1);
	string sector = GTS::Instance()->get(market, idate_)->get_sector(ticker);

	float newsVol = get_volume(ticker, 1, msecs);
	ret.push_back(newsVol);

	for( vector<string>::iterator itv = vViews_.begin(); itv != vViews_.end(); ++itv )
	{
		string view = *itv;

		string tickerRef = "";
		if( "ticker" == view )
			tickerRef = ticker;
		else if( "sector" == view )
			//tickerRef = (string)"S_" + sector;
			tickerRef = get_sector_ticker(sector, ticker);
		else if( "market" == view )
			//tickerRef = "MARKET";
			tickerRef = get_market_ticker(ticker);

		for( vector<int>::iterator itd = vDays_.begin(); itd != vDays_.end(); ++itd )
		{
			int nday = *itd;

			float sent = get_sent(tickerRef, nday, msecs);
			ret.push_back(sent);
		}
	}

	for( vector<string>::iterator itv = vViews_.begin(); itv != vViews_.end(); ++itv )
	{
		string view = *itv;

		string tickerRef = "";
		if( "ticker" == view )
			tickerRef = ticker;
		else if( "sector" == view )
			tickerRef = get_sector_ticker(sector, ticker);
		else if( "market" == view )
			tickerRef = get_market_ticker(ticker);

		for( vector<int>::iterator itd = vDays_.begin(); itd != vDays_.end(); ++itd )
		{
			int nday = *itd;
			for( vector<int>::iterator itd2 = itd; itd2 != vDays_.end(); ++itd2 )
			{
				int ndayRef = *itd2;

				if( "ticker" == view && nday != ndayRef )
				{
					float sent = get_lagged_cosent(ticker, nday, tickerRef, ndayRef, msecs);
					ret.push_back(sent);
				}
				else if( "ticker" != view )
				{
					float sent = get_cosent(ticker, nday, tickerRef, ndayRef, msecs);
					ret.push_back(sent);
				}
			}
		}
	}

	for( vector<int>::iterator itd = vDays_.begin(); itd != vDays_.end(); ++itd )
	{
		int nday = *itd;
		for( vector<int>::iterator itd2 = itd; itd2 != vDays_.end(); ++itd2 )
		{
			int ndayRef = *itd2;
			string tickerSector = get_sector_ticker(sector, ticker);
			float sent = get_cosent_3way(ticker, nday, tickerSector, ndayRef, msecs);
			ret.push_back(sent);
		}
	}

	return ret;
}

void NewsSignal::read_news(string market, int idate, string newsDir)
{
	mDayTickerTimeVol_.clear();
	mDayTickerTimeSent_.clear();
	mDayTickerTimeSentLag_.clear();
	market_ = market;
	idate_ = idate;

	vector<string> markets;
	if( market == "EU" )
	{
		markets.push_back("EA");
		markets.push_back("EB");
		markets.push_back("EI");
		markets.push_back("EP");
		markets.push_back("EF");
		markets.push_back("EL");
		markets.push_back("ED");
		markets.push_back("EM");
		markets.push_back("EZ");
		markets.push_back("EX");
		markets.push_back("EC");
		markets.push_back("EW");
		markets.push_back("EY");
	}
	else
		markets.push_back(market);
	
	TickAccessMulti<QuoteInfo> ta;
	for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
	{
		string m = *it;
		//string newsdir = (string)"\\\\smrc-ltc-mrct43\\f\\jelee\\work\\hf\\ravenpack\\1.4\\earnings\\" + m;
		string newsdir = newsDir + "\\" + m;
		ta.AddRoot(newsdir, mto::longTicker(market));
	}

	vector<string> symbols;
	ta.GetNames(idate, &symbols);

	TickSeries<QuoteInfo> ts;
	QuoteInfo quote;
	for( vector<string>::iterator it = symbols.begin(); it != symbols.end(); ++it )
	{
		string ticker = *it;
		if( !ticker.empty() )
		{
			ta.GetTickSeries(ticker, idate, &ts);

			int ntq = ts.NTicks();
			int lastMsecs = 0;
			ts.StartRead();
			QuoteInfo quote;
			for( int n=0; n<ntq; ++n )
			{
				ts.Read(&quote);
				mDayTickerTimeSent_[1][ticker][quote.msecs] = quote.bidEx;
				mDayTickerTimeSent_[5][ticker][quote.msecs] = quote.bidSize;
				mDayTickerTimeSent_[20][ticker][quote.msecs] = quote.bid;
				mDayTickerTimeSentLag_[5][ticker][quote.msecs] = quote.ask;
				mDayTickerTimeSentLag_[20][ticker][quote.msecs] = quote.askSize;
				mDayTickerTimeVol_[1][ticker][quote.msecs] = quote.askEx;
			}
		}
	}
	return;
}

float NewsSignal::get_volume(string ticker, int nday, int msecs)
{
	int ret = 0;
	map<int, map<string, map<int, int> > >::iterator itn = mDayTickerTimeVol_.find(nday);
	if( itn != mDayTickerTimeVol_.end() )
	{
		map<string, map<int, int> >& m1 = itn->second;
		map<string, map<int, int> >::iterator itt = m1.find(ticker);
		if( itt != m1.end() )
		{
			map<int, int>& m2 = itt->second;
			for( map<int, int>::iterator itt2 = m2.begin(); itt2 != m2.end(); ++itt2 )
			{
				if( itt2->first <= msecs )
					ret = itt2->second;
				else
					break;
			}
		}
	}
	return ret;
}

float NewsSignal::get_sent(string ticker, int nday, int msecs)
{
	int ret = 0;
	map<int, map<string, map<int, int> > >::iterator itn = mDayTickerTimeSent_.find(nday);
	if( itn != mDayTickerTimeSent_.end() )
	{
		map<string, map<int, int> >& m1 = itn->second;
		map<string, map<int, int> >::iterator itt = m1.find(ticker);
		if( itt != m1.end() )
		{
			map<int, int>& m2 = itt->second;
			for( map<int, int>::iterator itt2 = m2.begin(); itt2 != m2.end(); ++itt2 )
			{
				if( itt2->first <= msecs )
					ret = itt2->second;
				else
					break;
			}
		}
	}
	return ret;
}

float NewsSignal::get_lagged_sent(string ticker, int nday, int msecs)
{
	int ret = 0;
	map<int, map<string, map<int, int> > >::iterator itn = mDayTickerTimeSentLag_.find(nday);
	if( itn != mDayTickerTimeSentLag_.end() )
	{
		map<string, map<int, int> >& m1 = itn->second;
		map<string, map<int, int> >::iterator itt = m1.find(ticker);
		if( itt != m1.end() )
		{
			map<int, int>& m2 = itt->second;
			for( map<int, int>::iterator itt2 = m2.begin(); itt2 != m2.end(); ++itt2 )
			{
				if( itt2->first <= msecs )
					ret = itt2->second;
				else
					break;
			}
		}
	}
	return ret;
}

float NewsSignal::get_cosent(string ticker, int nday, string tickerRef, int ndayRef, int msecs)
{
	float ret = 0;
	int sent_ticker = get_sent(ticker, nday, msecs);
	int sent_ref = get_sent(tickerRef, ndayRef, msecs);
	//if( sent_ticker > 0 && sent_ref > 0 )
	//	ret = (sent_ticker - 50) * (sent_ref - 50);
	if( sent_ticker > 0 && sent_ref > 0 )
		ret = sent_ticker - sent_ref;
	return ret;
}

float NewsSignal::get_lagged_cosent(string ticker, int nday, string tickerRef, int ndayRef, int msecs)
{
	float ret = 0;
	int sent_ticker = get_sent(ticker, nday, msecs);
	int sent_ref = get_lagged_sent(tickerRef, ndayRef, msecs);
	//if( sent_ticker > 0 && sent_ref > 0 )
	//	ret = (sent_ticker - 50) * (sent_ref - 50);
	if( sent_ticker > 0 && sent_ref > 0 )
		ret = sent_ticker - sent_ref;
	return ret;
}

float NewsSignal::get_cosent_3way(string ticker, int nday, string tickerSector, int ndayRef, int msecs)
{
	float ret = 0;
	int sent_ticker = get_sent(ticker, nday, msecs);
	int sent_sector = get_sent(tickerSector, ndayRef, msecs);
	int sent_market = get_sent("MARKET", ndayRef, msecs);
	if( sent_ticker > 0 && sent_sector > 0 && sent_market > 0 )
	{
		if( (sent_ticker > 50 && sent_sector > 50 && sent_market > 50) || (sent_ticker < 50 && sent_sector < 50 && sent_market < 50) )
			ret = (sent_ticker - 50) * (sent_sector - 50) * (sent_market - 50);
	}
	return ret;
}

string NewsSignal::get_sector_ticker(string sector, string ticker)
{
	string ret = (string)"S_" + sector;

	if( "EU" == market_ )
		ret = (string)ticker.substr(0, 1) + ":" + ret;

	return ret;
}

string NewsSignal::get_market_ticker(string ticker)
{
	string ret = "MARKET";

	if( "EU" == market_ )
		ret = (string)ticker.substr(0, 1) + ":MARKET";

	return ret;
}
