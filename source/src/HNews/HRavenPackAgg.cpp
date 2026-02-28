#include <HNews/HRavenPackAgg.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/NewsCatRP.h>
#include <optionlibs/TickData.h>
#include <map>
#include <jl_lib.h>
#include <string>
#include "TFile.h"
using namespace std;

HRavenPackAgg::HRavenPackAgg(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
verbose_(0),
interval_(60),
market_(""),
category_("")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("market") )
		market_ = conf.find("market")->second;
	if( conf.count("category") )
		category_ = conf.find("category")->second;
	if( conf.count("outdirBase") )
		outdirBase_ = conf.find("outdirBase")->second;
	category_size_ = category_.size();
}

HRavenPackAgg::~HRavenPackAgg()
{}

void HRavenPackAgg::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	assert_loopingOrder_dmt(); // Maket loop inside day loop.
	country_ = mto::country(market_);
	companies_ = CompaniesRP(market_);

	aggregationWindows_.insert(1);
	aggregationWindows_.insert(5);
	aggregationWindows_.insert(20);
	nWindow_ = aggregationWindows_.size();

	for( set<int>::iterator it = aggregationWindows_.begin(); it != aggregationWindows_.end(); ++it )
	{
		int nDay = *it;
		mNdayNewsagg_.insert( make_pair(nDay, NewsAggregate(market_, nDay, interval_)) );
	}
	return;
}

void HRavenPackAgg::beginMarket()
{
	return;
}

void HRavenPackAgg::beginDay()
{
	int idate = HEnv::Instance()->idate();
	if( verbose_ > 1 )
		cout << market_ << "\t" << idate << endl;

	companies_.beginDay(idate);

	msecOpen_ = mto::msecOpen(market_, idate);
	msecClose_ = mto::msecClose(market_, idate);
	nInterval_ = (msecClose_ - msecOpen_) / 1000 / interval_ + 1;
	for( map<int, NewsAggregate>::iterator it = mNdayNewsagg_.begin(); it != mNdayNewsagg_.end(); ++it )
		it->second.beginDay(idate, msecOpen_, msecClose_, nInterval_);

	if( !outdirBase_.empty() )
		outdir_ = outdirBase_ + "\\" + market_;
	else if( "US" == market_ )
		outdir_ = "\\\\smrc-nas10\\l\\tickC\\us\\news";
	else if( "C" == mto::region(market_) )
		outdir_ = "\\\\smrc-ltc-mrct16\\data00\\tickC\\ca\\news";
	else if( mto::isInternational(market_) )
		outdir_ = "\\\\smrc-ltc-mrct16\\data00\\tickC\\" + mto::region_long(market_) + "\\news\\" + mto::code(market_);

	process_market(idate);

	return;
}

void HRavenPackAgg::endDay()
{
	return;
}

void HRavenPackAgg::endMarket()
{
	return;
}

void HRavenPackAgg::endJob()
{
	return;
}

void HRavenPackAgg::process_market(int idate)
{
	vector<string> tickers = get_tickers(idate);

	QuoteTime nowUTC = QuoteTime(idate, 0, "UTC") - RealTime( 1/(24.0*60*60) );
	QuoteTime nextOpenUTC = GEX::Instance()->get(market_)->NextOpen(nowUTC);
	QuoteTime nextCloseUTC = GEX::Instance()->get(market_)->NextClose(nowUTC);
	int msecsFirstData = 24*60*60*1000;

	map<string, int> mTickerNotMatched;
	TickerSector tickerSector(market_, idate);
	set<string> sectors = tickerSector.get_sectors();

	const vector<string>* lines = static_cast<const vector<string>*>(HEvent::Instance()->get("", "lines"));
	for( vector<string>::const_iterator it = lines->begin(); it != lines->end(); ++it )
	{
		const string& line = *it;
		vector<string> sl = splitN(line, ',');

		QuoteTime lineTimeUTC = get_lineTime(sl[0]);
		if( lineTimeUTC > nextCloseUTC ) // write tickdata.
		{
			write_tickdata(nextCloseUTC, msecsFirstData, sectors);
			aggregate(idate);
			reset_times(lineTimeUTC, nextOpenUTC, nextCloseUTC, msecsFirstData);
		}

		string lineCountry = sl[1].substr(0, 2);
		if( lineCountry == country_ )
		{
			string company_id = sl[2];
			string ticker = companies_.ticker(company_id);
			string sector = tickerSector.get_sector(ticker);

			bool preOpen = nextOpenUTC < nextCloseUTC && lineTimeUTC < nextOpenUTC;
			int msecs = preOpen? get_msecs(nextOpenUTC): get_msecs(lineTimeUTC);
			if( msecs_valid(msecs, sl) )
			{
				if( msecs < msecsFirstData )
					msecsFirstData = msecs;

				if( binary_search(tickers.begin(), tickers.end(), ticker) )
					add_data(ticker, sector, msecs, sl);
				else
					++mTickerNotMatched[company_id];
			}
		}
	}

	for( map<string, int>::iterator it = mTickerNotMatched.begin(); it != mTickerNotMatched.end(); ++it )
	{
		if( it->second > 5 )
			printf( " company_id %s appeared %d times but not matched.\n", it->first.c_str(), it->second );
	}

	return;
}

void HRavenPackAgg::reset_times(QuoteTime lineTimeUTC, QuoteTime& nextOpenUTC, QuoteTime& nextCloseUTC, int& msecsFirstData)
{
	nextOpenUTC = GEX::Instance()->get(market_)->NextOpen(lineTimeUTC);
	nextCloseUTC = GEX::Instance()->get(market_)->NextClose(lineTimeUTC);
	msecsFirstData = 24*60*60*1000;
	return;
}

void HRavenPackAgg::aggregate(int idate)
{
	for( map<int, NewsAggregate>::iterator it = mNdayNewsagg_.begin(); it != mNdayNewsagg_.end(); ++it )
		it->second.aggregate(idate);
	return;
}

bool HRavenPackAgg::msecs_valid(int msecs, vector<string>& sl)
{
	bool valid = true;
	if( "U" == mto::region(market_) || "C" == mto::region(market_) )
	{
		if( (msecs >= 56400000 && msecs <= 56520000) || (msecs >= 57000000 && msecs <= 57300000) )
		{
			int rel = atoi(sl[3].c_str());
			if( rel != 100 )
				valid = false; // filter out the close imbalance reports.
		}
	}

	return valid;
}

int HRavenPackAgg::get_idate(string& line)
{
	int idate = 0;
	if( line.size() > 10 )
		idate = atoi(line.substr(0,4).c_str())*10000 + atoi(line.substr(5,2).c_str())*100 + atoi(line.substr(8,2).c_str());
	return idate;
}

vector<string> HRavenPackAgg::get_tickers(int idate)
{
	QuoteTime today(idate, 040000, mto::tz(market_));
	int idate_prev = (int)GEX::Instance()->get(market_)->PrevClose(today).Date();
	string selChara = mto::selChara(market_, idate_prev);
	if( market_ == "Ak" )
		selChara = " idate = " + itos(idate_prev) + " and (market = 'AK' or market = 'AQ') ";

	string cmd = (string)" select " + mto::compTicker(market_) + " from stockcharacteristics "
		+ " where " + mto::selChara(market_, idate_prev)
		+ " and " + mto::selType(market_);
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market_))->ReadTable(cmd, &vv);
	vector<string> tickers;
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		if( !ticker.empty() )
			tickers.push_back(ticker);
	}
	sort(tickers.begin(), tickers.end());
	return tickers;
}

QuoteTime HRavenPackAgg::get_lineTime(string& t)
{
	int idate = get_idate(t);
	int hhmmss = atoi( (t.substr(11,2) + t.substr(14,2) + t.substr(17,2)).c_str() );
	int mmm = atoi( t.substr(20,3).c_str() );
	QuoteTime lineTime(idate, hhmmss, "UTC");
	lineTime += RealTime( mmm/86400000.0 );
	return lineTime;
}

void HRavenPackAgg::write_tickdata(QuoteTime nextCloseUTC, int msecsFirstData, set<string>& sectors)
{
	nextCloseUTC.SetTimeZone(mto::tz(market_));
	int idate = (int)nextCloseUTC.Date();
	if( msecsFirstData < mto::msecOpen(market_, idate) + 60*60*1000 ) // Need to update
	{

		// Add QuoteInfo for the tickers without news.
		add_tickers_without_news(idate);

		TickStorage<QuoteInfo> tsQ;

		// Import tickers with news.
		for(map<string, TickSeries<QuoteInfo> >::iterator itQ = mTs_.begin(); itQ != mTs_.end(); ++itQ)
		{
			string ticker = itQ->first;
			tsQ.Import(ticker, itQ->second);
		}

		// Import sectors and the market.
		import_sectors(tsQ, sectors);
		import_market(tsQ);

		if( verbose_ >= 1 )
			cout << "writing at " << outdir_ << "\n";
		tsQ.WriteBIN(idate, outdir_, mto::longTicker(market_));
		mTs_.clear();
	}
	return;
}

void HRavenPackAgg::import_sectors(TickStorage<QuoteInfo>& tsQ, set<string>& sectors)
{
	for( set<string>::iterator it = sectors.begin(); it != sectors.end(); ++it )
	{
		string sector = *it;
		vector<vector<int> > avg;
		for( set<int>::iterator it = aggregationWindows_.begin(); it != aggregationWindows_.end(); ++it )
		{
			int nDay = *it;
			avg.push_back(mNdayNewsagg_.find(nDay)->second.get_sector_sent(sector));
		}

		vector<int> avgSize;
		for( vector<vector<int> >::iterator it = avg.begin(); it != avg.end(); ++it )
			avgSize.push_back(it->size());

		TickSeries<QuoteInfo> ts;
		for( int i=0; i<nInterval_; ++i )
		{
			int msecs = msecOpen_ + i * interval_ * 1000;

			QuoteInfo quote;
			quote.msecs = msecs;

			if( nWindow_ > 0 && i < avgSize[0] )
				quote.bidEx = avg[0][i];
			if( nWindow_ > 1 && i < avgSize[1] )
				quote.bidSize = avg[1][i];
			if( nWindow_ > 2 && i < avgSize[2] )
				quote.bid = avg[2][i];
			if( nWindow_ > 3 && i < avgSize[3] )
				quote.ask = avg[3][i];
			if( nWindow_ > 4 && i < avgSize[4] )
				quote.askSize = avg[4][i];
			if( nWindow_ > 5 && i < avgSize[5] )
				quote.askEx = avg[5][i];

			ts.Write(quote);

		}
		string temp_ticker = (string)"S_" + sector;
		string ticker = mto::compTicker(temp_ticker, market_);
		tsQ.Import(ticker, ts);
	}
	return;
}

void HRavenPackAgg::import_market(TickStorage<QuoteInfo>& tsQ)
{
	vector<vector<int> > avg;
	for( set<int>::iterator it = aggregationWindows_.begin(); it != aggregationWindows_.end(); ++it )
	{
		int nDay = *it;
		avg.push_back(mNdayNewsagg_.find(nDay)->second.get_market_sent());
	}

	vector<int> avgSize;
	for( vector<vector<int> >::iterator it = avg.begin(); it != avg.end(); ++it )
	{
		avgSize.push_back(it->size());
	}

	TickSeries<QuoteInfo> ts;
	for( int i=0; i<nInterval_; ++i )
	{
		int msecs = msecOpen_ + i * interval_ * 1000;

		QuoteInfo quote;
		quote.msecs = msecs;

		if( nWindow_ > 0 && i < avgSize[0] )
			quote.bidEx = avg[0][i];
		if( nWindow_ > 1 && i < avgSize[1] )
			quote.bidSize = avg[1][i];
		if( nWindow_ > 2 && i < avgSize[2] )
			quote.bid = avg[2][i];
		if( nWindow_ > 3 && i < avgSize[3] )
			quote.ask = avg[3][i];
		if( nWindow_ > 4 && i < avgSize[4] )
			quote.askSize = avg[4][i];
		if( nWindow_ > 5 && i < avgSize[5] )
			quote.askEx = avg[5][i];

		ts.Write(quote);

	}
	string temp_ticker = "MARKET";
	string ticker = mto::compTicker(temp_ticker, market_);
	tsQ.Import(ticker, ts);

	return;
}

int HRavenPackAgg::get_msecs(QuoteTime qt)
{
	qt.SetTimeZone(mto::tz(market_));
	int idate = (int)qt.Date();
	QuoteTime midnight(idate, 0, mto::tz(market_));
	int msecs = ROUND(86400000. * (qt - midnight).Days());
	return msecs;
}

void HRavenPackAgg::add_data(string ticker, string sector, int msecs, vector<string>&sl)
{
	int rel = atoi(sl[3].c_str());
	int ess = atoi(sl[5].c_str());
	int ens = atoi(sl[6].c_str());

	bool valid_category = true;
	if( !category_.empty() )
	{
		string sCat = sl[4];
		valid_category =  sCat.size() >= category_size_ && category_ == sCat.substr(0, category_size_);
	}

	//if( ens == 100 )
	if( valid_category && rel >= 90 )
	{
		// Add.
		for( map<int, NewsAggregate>::iterator it = mNdayNewsagg_.begin(); it != mNdayNewsagg_.end(); ++it )
			it->second.add_data(ticker, sector, msecs, sl);

		// QuoteInfo.
		vector<int> avg;
		for( set<int>::iterator it = aggregationWindows_.begin(); it != aggregationWindows_.end(); ++it )
		{
			int nDay = *it;
			avg.push_back(mNdayNewsagg_.find(nDay)->second.get_ticker_sent(ticker));
		}
		for( set<int>::iterator it = aggregationWindows_.begin(); it != aggregationWindows_.end(); ++it )
		{
			int nDay = *it;
			if( nDay - 1 > 0 )
				avg.push_back(mNdayNewsagg_.find(nDay)->second.get_ticker_sent_lag(ticker));
		}
		avg.push_back(mNdayNewsagg_.find(1)->second.get_volume(ticker));
		int navg = avg.size();

		QuoteInfo quote;
		quote.msecs = msecs;

		if( navg > 0 )
			quote.bidEx = avg[0];
		if( navg > 1 )
			quote.bidSize = avg[1];
		if( navg > 2 )
			quote.bid = avg[2];
		if( navg > 3 )
			quote.ask = avg[3];
		if( navg > 4 )
			quote.askSize = avg[4];
		if( navg > 5 )
			quote.askEx = avg[5];
	
		map<string, TickSeries<QuoteInfo> >::iterator it_symbol = mTs_.find(ticker);
		if( it_symbol == mTs_.end() ) // new symbol
		{
			mTs_.insert( make_pair(ticker, TickSeries<QuoteInfo>(3, 1.1)) );
			it_symbol = mTs_.find(ticker);
		}
		it_symbol->second.Write(quote);
	}

	return;
}

void HRavenPackAgg::add_tickers_without_news(int idate)
{
	NewsAggregate& nagg = mNdayNewsagg_.rbegin()->second;
	vector<string> tickers;
	nagg.get_tickers_without_news(tickers);

	for( vector<string>::iterator itt = tickers.begin(); itt != tickers.end(); ++itt )
	{
		string ticker = *itt;

		vector<int> avg;
		for( set<int>::iterator it = aggregationWindows_.begin(); it != aggregationWindows_.end(); ++it )
		{
			int nDay = *it;
			avg.push_back(mNdayNewsagg_.find(nDay)->second.get_ticker_sent_lag(ticker));
		}
		for( set<int>::iterator it = aggregationWindows_.begin(); it != aggregationWindows_.end(); ++it )
		{
			int nDay = *it;
			if( nDay - 1 > 0 )
				avg.push_back(mNdayNewsagg_.find(nDay)->second.get_ticker_sent_lag(ticker));
		}
		int navg = avg.size();

		QuoteInfo quote;
		quote.msecs = msecOpen_;

		if( navg > 0 )
			quote.bidEx = avg[0];
		if( navg > 1 )
			quote.bidSize = avg[1];
		if( navg > 2 )
			quote.bid = avg[2];
		if( navg > 3 )
			quote.ask = avg[3];
		if( navg > 4 )
			quote.askSize = avg[4];
		if( navg > 5 )
			quote.askEx = avg[5];
	
		map<string, TickSeries<QuoteInfo> >::iterator it_symbol = mTs_.find(ticker);
		if( it_symbol == mTs_.end() ) // new symbol
		{
			mTs_.insert( make_pair(ticker, TickSeries<QuoteInfo>(3, 1.1)) );
			it_symbol = mTs_.find(ticker);
		}
		it_symbol->second.Write(quote);
	}

	return;
}

/* ----------------------------------------------------------------------------------
* Class NewsAggregate
* -------------------------------------------------------------------------------- */

HRavenPackAgg::NewsAggregate::NewsAggregate(string market, int nDayAgg, int interval)
:market_(market),
nDayAgg_(nDayAgg),
interval_(interval)
{}

void HRavenPackAgg::NewsAggregate::beginDay(int idate, int msecOpen, int msecClose, int nInterval)
{
	msecOpen_ = msecOpen;
	msecClose_ = msecClose;
	nInterval_ = nInterval;
	vMarketAcc_ = vector<Acc>(nInterval_);
	return;
}

void HRavenPackAgg::NewsAggregate::add_data(string ticker, string sector, int msecs, vector<string>& sl)
{
	int rel = atoi(sl[3].c_str());
	int ess = atoi(sl[5].c_str());
	int ens = atoi(sl[6].c_str());
	int css = atoi(sl[8].c_str());

	//if( rel == 100 && ens == 100 )
	if( rel >= 90 )
	{
		int sent = (rel == 100)? ess: css;

		Acc& accVol = tickerVol_[ticker];
		accVol.add(1);

		if( ens == 100 )
		{
			// Add to the ticker sentiment.
			Acc& acc = tickerAcc_[ticker];
			acc.add(sent);

			// Add to the sector and market sentiment.
			int iInterval = ceil( double(msecs - msecOpen_)/1000/interval_ );
			if( iInterval < nInterval_ )
			{
				if( !sector.empty() )
				{
					map<string, vector<Acc> >::iterator it = mvSectorAcc_.find(sector);
					if( it == mvSectorAcc_.end() )
						mvSectorAcc_[sector] = vector<Acc>(nInterval_);
					mvSectorAcc_[sector][iInterval].add(sent);
				}
				vMarketAcc_[iInterval].add(sent);
			}
		}
	}
	return;
}

int HRavenPackAgg::NewsAggregate::get_volume(string ticker)
{
	Acc& accVol = tickerVol_[ticker];
	DailySummary& ds = tickerDailyVol_[ticker];
	Acc sum = ds.agg + accVol;
	double vol = sum.n;
	return vol;
}

int HRavenPackAgg::NewsAggregate::get_ticker_sent(string ticker)
{
	Acc& acc = tickerAcc_[ticker];
	DailySummary& ds = tickerDailySummary_[ticker];
	int avg = get_avg_sent(ds, acc);
	return avg;
}

int HRavenPackAgg::NewsAggregate::get_ticker_sent_lag(string ticker)
{
	DailySummary& ds = tickerDailySummary_[ticker];
	int lag = ds.agg.mean();
	return lag;
}

void HRavenPackAgg::NewsAggregate::get_tickers_without_news(vector<string>& tickers)
{
	for( map<string, DailySummary>::iterator it = tickerDailySummary_.begin(); it != tickerDailySummary_.end(); ++it )
	{
		string ticker = it->first;
		if( !tickerVol_.count(ticker) )
			tickers.push_back(ticker);
	}
	return;
}

double HRavenPackAgg::NewsAggregate::get_avg_sent(DailySummary& ds, Acc& acc)
{
	Acc sum = ds.agg + acc;
	double avg = sum.mean();
	return avg;
}

vector<int> HRavenPackAgg::NewsAggregate::get_sector_sent(string sector)
{
	vector<int> ret;
	map<string, vector<Acc> >::iterator it = mvSectorAcc_.find(sector);
	if( it != mvSectorAcc_.end() )
	{
		vector<Acc>& vAcc = it->second;
		int vAccSize = vAcc.size();

		Acc sumAcc;
		for( int i=0; i<nInterval_ && i < vAccSize; ++i )
		{
			int msecs = msecOpen_ + i * interval_ * 1000;

			sumAcc += vAcc[i];
			int avg = get_avg_sent(marketDailySummary_, sumAcc);
			ret.push_back(avg);
		}
	}

	return ret;
}

vector<int> HRavenPackAgg::NewsAggregate::get_market_sent()
{
	vector<int> ret;

	Acc sumAcc;
	for( int i=0; i<nInterval_; ++i )
	{
		int msecs = msecOpen_ + i * interval_ * 1000;

		sumAcc += vMarketAcc_[i];
		int avg = get_avg_sent(marketDailySummary_, sumAcc);
		ret.push_back(avg);
	}

	return ret;
}

void HRavenPackAgg::NewsAggregate::aggregate(int idate)
{
	QuoteTime outOfAgg = QuoteTime(idate, 0, mto::tz(market_));
	for( int i=0; i<nDayAgg_-1; ++i )
		outOfAgg = GEX::Instance()->get(market_)->PrevClose(outOfAgg);
	int outOfAggDate = (int)outOfAgg.Date();

	// For each ticker.
	for( map<string, DailySummary>::iterator it = tickerDailySummary_.begin(); it != tickerDailySummary_.end(); ++it )
	{
		// Delete old dates, add the current date, and aggregate the valid dates.
		string ticker = it->first;
		DailySummary& ds = it->second;

		add_acc_today(ds, idate, ticker);
		aggregate(ds, outOfAggDate);
	}

	// For each ticker.
	for( map<string, DailySummary>::iterator it = tickerDailyVol_.begin(); it != tickerDailyVol_.end(); ++it )
	{
		// Delete old dates, add the current date, and aggregate the valid dates.
		string ticker = it->first;
		DailySummary& ds = it->second;

		add_acc_today(ds, idate, ticker);
		aggregate(ds, outOfAggDate);
	}

	// Market.
	add_acc_today_market(idate);
	aggregate(marketDailySummary_, outOfAggDate);

	tickerAcc_.clear();
	tickerVol_.clear();
	for( vector<Acc>::iterator it = vMarketAcc_.begin(); it != vMarketAcc_.end(); ++it )
		it->clear();

	return;
}

void HRavenPackAgg::NewsAggregate::add_acc_today(DailySummary& ds, int idate, string ticker)
{
	Acc& acc = tickerAcc_[ticker];
	ds.idateAcc[idate] = acc;
	return;
}

void HRavenPackAgg::NewsAggregate::aggregate(DailySummary& ds, int outOfAggDate)
{
	Acc accAgg;
	for( map<int, Acc>::iterator itd = ds.idateAcc.begin(); itd != ds.idateAcc.end(); )
	{
		int idate = itd->first;
		if( idate <= outOfAggDate )
			ds.idateAcc.erase(itd++);
		else
		{
			accAgg += itd->second;
			++itd;
		}
	}
	ds.agg = accAgg;
	return;
}

void HRavenPackAgg::NewsAggregate::add_acc_today_market(int idate)
{
	Acc sumDay;
	for( vector<Acc>::iterator it = vMarketAcc_.begin(); it != vMarketAcc_.end(); ++it )
	{
		sumDay += *it;
	}
	marketDailySummary_.idateAcc[idate] = sumDay;
	return;
}
