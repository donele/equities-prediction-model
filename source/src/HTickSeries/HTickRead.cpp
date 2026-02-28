#include <HTickSeries.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/GEX.h>
#include <jl_lib/GODBC.h>
#include "optionlibs/TickData.h"
#include <vector>
#include <string>
#include <map>
using namespace std;

HTickRead::HTickRead(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
verbose_(1),
dd_(0),
read_date_(0),
format_("TickSeries"),
read_quote_(false),
read_nbbo_(false),
read_trade_(false),
read_order_(false),
read_return_(false),
nbbo_trade_(false),
quote_dir_given_(false),
nbbo_dir_given_(false),
trade_dir_given_(false),
order_dir_given_(false),
return_dir_given_(false),
return_dir_(""),
ticker_choice_("")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );

	if( conf.count("dd") )
	{
		dd_ = atoi( conf.find("dd")->second.c_str() );
		desc_ = (string)"_" + itos(dd_);
	}

	if( conf.count("format") )
		format_ = conf.find("format")->second;

	// Tickers.
	if( conf.count("ticker") )
		ticker_choice_ = conf.find("ticker")->second;
	else if( conf.count("tickerInUniverse") )
		ticker_choice_ = "univ";
	else if( conf.count("tickerInUniverseInclusive") )
		ticker_choice_ = "univInclusive";
	else if( conf.count("tickerNoss") )
		ticker_choice_ = "noss";
	else if( conf.count("nnSubset") )
	{
		ticker_choice_ = "nnSubset";
		nnSubsetSize_ = atoi( conf.find("nnSubset")->second.c_str() );
	}
	else if( conf.count("marketTicker") )
	{
		ticker_choice_ = "fixed";
		pair<mmit, mmit> tickers = conf.equal_range("marketTicker");
		for( mmit mi = tickers.first; mi != tickers.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string ticker = vs[1];
				marketTickers_[market].push_back(ticker);
			}
		}
	}

	// What to read.
	if( conf.count("read") )
	{
		pair<mmit, mmit> sources = conf.equal_range("read");
		for( mmit mi = sources.first; mi != sources.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			for( vector<string>::iterator it = vs.begin(); it != vs.end(); ++it )
			{
				string source = *it;
				if( "quote" == source )
					read_quote_ = true;
				if( "nbbo" == source )
					read_nbbo_ = true;
				if( "trade" == source )
					read_trade_ = true;
				if( "order" == source )
					read_order_ = true;
				if( "return" == source )
					read_return_ = true;
			}
		}
	}

	if( conf.count("nbboTrade") )
		nbbo_trade_ = conf.find("nbboTrade")->second == "true";

	// Directories.
	if( conf.count("quote_dir") )
	{
		quote_dir_given_ = true;
		pair<mmit, mmit> dirs = conf.equal_range("quote_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				m_quote_dir_[market].push_back(dir);
			}
		}
	}
	if( conf.count("nbbo_dir") )
	{
		nbbo_dir_given_ = true;
		pair<mmit, mmit> dirs = conf.equal_range("nbbo_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				m_nbbo_dir_[market].push_back(dir);
			}
		}
	}
	if( conf.count("trade_dir") )
	{
		trade_dir_given_ = true;
		pair<mmit, mmit> dirs = conf.equal_range("trade_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				m_trade_dir_[market].push_back(dir);
			}
		}
	}
	if( conf.count("order_dir") )
	{
		order_dir_given_ = true;
		pair<mmit, mmit> dirs = conf.equal_range("order_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				m_order_dir_[market].push_back(dir);
			}
		}
	}
	if( conf.count("return_dir") )
	{
		return_dir_given_ = true;
		pair<mmit, mmit> dirs = conf.equal_range("return_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				m_return_dir_[market] = dir;
			}
		}
	}

	// Outfile
	if( conf.count("outfile") )
		HEnv::Instance()->outfile( conf.find("outfile")->second );

	swThreadRead_.Reset();
}

HTickRead::~HTickRead()
{}

void HTickRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	//quote_name_ = (string)"quotes" + desc_;
	//nbbo_name_ = (string)"nbbo" + desc_;
	//trade_name_ = (string)"trades" + desc_;
	//order_name_ = (string)"orders" + desc_;

	if( read_quote_ )
		ptaQ_ = new TickAccessMulti<QuoteInfo>();
	if( read_nbbo_ )
		ptaN_ = new TickAccessMulti<QuoteInfo>();
	if( read_trade_ )
		ptaT_ = new TickAccessMulti<TradeInfo>();
	if( read_order_ )
		ptaO_ = new TickAccessMulti<OrderData>();
	if( read_return_ )
		ptaR_ = new TickAccess<ReturnData>();

	if( ticker_choice_ == "fitting" )
	{
		const vector<int>& idates = HEnv::Instance()->idates();
		const vector<string>& markets = HEnv::Instance()->markets();
		int nDates = HEnv::Instance()->nDates();
		int idateFrom = idates[0];
		int idateTo = idates[nDates - 1];
		for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
		{
			string market = *it;
			vector<string> tickers;

			char cmd[1000];
			sprintf( cmd, "select distinct %s from stockcharacteristics "
				" where market = '%s' and idate >= %d and idate <= %d and uniqueID is not null ",
				mto::compTicker(market).c_str(),
				mto::code(market).c_str(),
				idateFrom,
				idateTo );
			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string ticker = trim((*it)[0]);
				if( !ticker.empty() )
					tickers.push_back(ticker);
			}

			sort(tickers.begin(), tickers.end());
			marketTickers_[market] = tickers;
		}
	}

	return;
}

void HTickRead::beginMarketDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	read_date_ = advance_idate(market, idate, dd_);
	tickReadLog_.clear();

/*
*	Initialize TickAcess.
*/
	if( read_quote_ )
	{
		if( quote_dir_given_ )
			quote_dir_ = m_quote_dir_[market];
		else
			quote_dir_ = mto::bindirs(market, read_date_);

		ptaQ_->Clear();
		for( vector<string>::iterator it = quote_dir_.begin(); it != quote_dir_.end(); ++it )
		{
			string dir = *it;
			if( !dir.empty() )
			{
				ptaQ_->AddRoot(dir, mto::longTicker(market));
				if( verbose_ >= 2 )
					cout << "Quote dir " << dir << endl;
			}
		}
	}
	if( read_nbbo_ )
	{
		if( nbbo_dir_given_ )
			nbbo_dir_ = m_nbbo_dir_[market];
		else
			nbbo_dir_ = mto::nbbodirs(market, read_date_);

		ptaN_->Clear();
		for( vector<string>::iterator it = nbbo_dir_.begin(); it != nbbo_dir_.end(); ++it )
		{
			string dir = *it;
			if( !dir.empty() )
			{
				ptaN_->AddRoot(dir, mto::longTicker(market));
				if( verbose_ >= 2 )
					cout << "Nbbo dir " << dir << endl;
			}
		}
	}
	if( read_trade_ )
	{
		if( trade_dir_given_ )
			trade_dir_ = m_trade_dir_[market];
		else
		{
			if( nbbo_trade_ )
				trade_dir_ = nbbo_dir_;
			else
			{
				if( quote_dir_given_ )
					trade_dir_ = m_quote_dir_[market];
				else
					trade_dir_ = mto::bindirs(market, read_date_);
			}
		}

		ptaT_->Clear();
		for( vector<string>::iterator it = trade_dir_.begin(); it != trade_dir_.end(); ++it )
		{
			string dir = *it;
			if( !dir.empty() )
			{
				ptaT_->AddRoot(dir, mto::longTicker(market));
				if( verbose_ >= 2 )
					cout << "Trade dir " << dir << endl;
			}
		}
	}
	if( read_order_ )
	{
		if( order_dir_given_ )
			order_dir_ = m_order_dir_[market];
		else
			order_dir_ = mto::bindirsBook(market, read_date_);

		ptaO_->Clear();
		for( vector<string>::iterator it = order_dir_.begin(); it != order_dir_.end(); ++it )
		{
			string dir = *it;
			if( !dir.empty() )
			{
				ptaO_->AddRoot(dir, mto::longTicker(market));
				if( verbose_ >= 2 )
					cout << "Order dir " << dir << endl;
			}
		}
	}
	if( read_return_ )
	{
		if( return_dir_given_ )
			return_dir_ = m_return_dir_[market];
		else
			return_dir_ = mto::bindirReturn(market, read_date_);

		if( !return_dir_.empty() )
		{
			ptaR_->Init(return_dir_, mto::longTicker(market));

			TickSeries<ReturnData> ts;
			ptaR_->GetTickSeries( mto::retName(market), read_date_, &ts );
			int nTicks = ts.NTicks();
			add_tickdata_TS<ReturnData>(mto::retName(market), "returns", ts);
		}
	}

/*
*	Ticker List.
*/
	vector<string> namesQ;
	vector<string> namesN;
	vector<string> namesW;
	vector<string> namesT;
	vector<string> namesO;

	if( read_quote_ )
		ptaQ_->GetNames(read_date_, &namesQ);
	if( read_nbbo_ )
		ptaN_->GetNames(read_date_, &namesN);
	if( read_trade_ )
		ptaT_->GetNames(read_date_, &namesT);
	if( read_order_ )
		ptaO_->GetNames(read_date_, &namesO);

	vector<string> tickers;
	if( "tickData" == ticker_choice_ )
	{
		vector<string> bin_tickers;
		vector<string> temp1;
		vector<string> temp2;
		get_union( namesQ, namesN, temp1 );
		get_union( temp1, namesT, temp2 );
		get_union( temp2, namesO, bin_tickers );

		tickers = comp_ticker( bin_tickers, market );
	}
	else if( "fixed" == ticker_choice_ )
	{
		tickers = comp_ticker( marketTickers_[market], market );
	}
	else if( "univ" == ticker_choice_ )
	{
		vector<string> univ_tickers = get_univ_tickers(market, read_date_);
		tickers = comp_ticker( univ_tickers, market );
	}
	else if( "univInclusive" == ticker_choice_ )
	{
		vector<int> idates = HEnv::Instance()->idates();
		if( !idates.empty() )
		{
			int idate1 = idates[0];
			int idate2 = idates[idates.size() - 1];
			vector<string> univ_tickers = get_univ_tickers(market, idate1, idate2);
			tickers = comp_ticker( univ_tickers, market );
		}
	}
	else if( "nnSubset" == ticker_choice_ )
	{
		string criteria = "";
		if( "US" == market )
			criteria = " and medVolatility < 0.02 and medNTrades > 2000 and sectype = 'C' and inuniverse = 1 ";
		else if( "CJ" == market )
			criteria = " and medVolatility < 0.04 and sectype = 'C' and inuniverse = 1 ";
		else if( mto::isInternational(market) )
			criteria = " and medVolatility < 0.04 and (sectype = '10' or sectype = '09') and inuniverse = 1 ";
		int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();
		string cmd = (string)" select " + mto::ts(market) + " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate_prev)
			+ criteria
			+ " order by medvolatility desc ";
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);

		vector<string> subset_tickers;
		for( vector<vector<string> >::iterator it = vv.begin();
			it != vv.end() && subset_tickers.size() < nnSubsetSize_; ++it )
		{
			string ticker = trim((*it)[0]);
			if( !ticker.empty() )
				subset_tickers.push_back(ticker);
		}

		tickers = comp_ticker( subset_tickers, market );
	}
	else if( "fitting" == ticker_choice_ )
	{
		tickers = comp_ticker( marketTickers_[market], market );
	}
	else if( "noss" == ticker_choice_ )
	{
		vector<string> noss_tickers = get_noss_tickers(market, idate);
		tickers = comp_ticker( noss_tickers, market );
	}

	if( !tickers.empty() )
	{
		sort(tickers.begin(), tickers.end());
		HEnv::Instance()->tickers(tickers);
	}

	if( verbose_ >= 1 )
	{
		vector<string> tickers = HEnv::Instance()->tickers();
		int ts = tickers.size();
		{
			cout << "\n";
			cout << "market: " << market << ", date: " << idate << ". " << "ticker(s): ";
			if( ts > 10 )
			{
				copy(tickers.begin(), tickers.begin() + 10, ostream_iterator<string>(cout, " "));
				cout << "... (" << ts << " tickers)";
			}
			else
				copy(tickers.begin(), tickers.end(), ostream_iterator<string>(cout, " "));
			cout << endl; // flush.
		}
	}

	return;
}

void HTickRead::beginTicker(const string& ticker)
{
	int nq = 0;
	int nn = 0;
	int nw = 0;
	int nt = 0;
	int no = 0;

	if( read_trade_ )
	{
		TickSeries<TradeInfo> ts;
		{
			boost::mutex::scoped_lock lock(trade_mutex_);
			ptaT_->GetTickSeries( ticker, read_date_, &ts );
		}
		nt = ts.NTicks();
		//add_tickdata<TradeInfo>(ticker, trade_name_, ts);
		add_tickdata_TS<TradeInfo>(ticker, getName("trades", "TickSeries"), ts);
		add_tickdata_vector<TradeInfo>(ticker, getName("trades", "vector"), ts);
	}

	if( read_quote_ )
	{
		TickSeries<QuoteInfo> ts;
		{
			boost::mutex::scoped_lock lock(quote_mutex_);
			ptaQ_->GetTickSeries( ticker, read_date_, &ts );
		}
		nq = ts.NTicks();
		//add_tickdata<QuoteInfo>(ticker, quote_name_, ts);
		add_tickdata_TS<QuoteInfo>(ticker, getName("quotes", "TickSeries"), ts);
		add_tickdata_vector<QuoteInfo>(ticker, getName("quotes", "vector"), ts);
	}

	if( read_nbbo_ )
	{
		TickSeries<QuoteInfo> ts;
		{
			boost::mutex::scoped_lock lock(nbbo_mutex_);
			ptaN_->GetTickSeries( ticker, read_date_, &ts );
		}
		nn = ts.NTicks();
		//add_tickdata<QuoteInfo>(ticker, nbbo_name_, ts);
		add_tickdata_TS<QuoteInfo>(ticker, getName("nbbo", "TickSeries"), ts);
		add_tickdata_vector<QuoteInfo>(ticker, getName("nbbo", "vector"), ts);
	}

	if( read_order_ )
	{
		TickSeries<OrderData> ts;
		{
			boost::mutex::scoped_lock lock(order_mutex_);
			ptaO_->GetTickSeries( ticker, read_date_, &ts );
		}
		no = ts.NTicks();
		//add_tickdata<OrderData>(ticker, order_name_, ts);
		add_tickdata_TS<OrderData>(ticker, getName("orders", "TickSeries"), ts);
		add_tickdata_vector<OrderData>(ticker, getName("orders", "vector"), ts);
	}

	return;
}

void HTickRead::endTicker(const string& ticker)
{
	//if( format_ == "TickSeries" )
	{
		string storageType = "TickSeries";
		if( read_quote_ )
			HEvent::Instance()->remove<TickSeries<QuoteInfo> >(ticker, getName("quotes", storageType));
		if( read_nbbo_ )
			HEvent::Instance()->remove<TickSeries<QuoteInfo> >(ticker, getName("nbbo", storageType));
		if( read_trade_ )
			HEvent::Instance()->remove<TickSeries<TradeInfo> >(ticker, getName("trades", storageType));
		if( read_order_ )
			HEvent::Instance()->remove<TickSeries<OrderData> >(ticker, getName("orders", storageType));
	}
	//else if( format_ == "vector" )
	{
		string storageType = "vector";
		if( read_quote_ )
			HEvent::Instance()->remove<vector<QuoteInfo> >(ticker, getName("quotes", storageType));
		if( read_nbbo_ )
			HEvent::Instance()->remove<vector<QuoteInfo> >(ticker, getName("nbbo", storageType));
		if( read_trade_ )
			HEvent::Instance()->remove<vector<TradeInfo> >(ticker, getName("trades", storageType));
		if( read_order_ )
			HEvent::Instance()->remove<vector<OrderData> >(ticker, getName("orders", storageType));
	}
}

void HTickRead::endMarketDay()
{
	string market = HEnv::Instance()->market();
	if( read_return_ )
	{
		//if( format_ == "TickSeries" )
			HEvent::Instance()->remove<TickSeries<ReturnData> >(mto::retName(market), "returns");
		//else if( format_ == "vector" )
		//	HEvent::Instance()->remove<vector<ReturnData> >(mto::retName(market), "vreturns");
	}

	return;
}

void HTickRead::endJob()
{
	char buf[200];
	sprintf(buf, "\n%20s %19s %13s\n", moduleName_.c_str(), "Real Time", "CPU Time");
	cout << buf;

	//printTime("threadRead", swThreadRead_.RealTime(), swThreadRead_.CpuTime());
	return;
}

string HTickRead::getName(const string& tickType, const std::string& storageType)
{
	string name;
	if( storageType == "TickSeries" )
		name = tickType + desc_ + "TS";
	else if( storageType == "vector" )
		name = tickType + desc_;
	return name;
}

vector<string> HTickRead::get_noss_tickers(string market, int idate)
{
	int udate = idate;
	udate = prevClose(market, udate);
	udate = prevClose(market, udate);
	udate = prevClose(market, udate);

	string model = market;
	if( market == "AK" || market == "AQ" )
		model = "KR";
	else if( market[0] == 'E' )
		model = "EU";

	bool read_all = true;
	if( model == "KR" || model == "EU" )
		read_all = false;

	int yyyy = idate / 10000;
	int yyyymm = idate / 100;
	string logdir = xpf("\\\\smrc-nas10\\l\\package\\gt\\log\\paramjobs");
	char logpath[1000];
	sprintf(logpath, "%s\\%d\\%d\\param_%s_u%d.log", logdir.c_str(), yyyy, yyyymm, model.c_str(), idate);

	vector<string> tickers;
	ifstream ifs(xpf(logpath).c_str());
	string line;
	while( getline(ifs, line) )
	{
		vector<string> sl = split(line, ' ');
		if( sl.size() > 4 && sl[0] == "Ticker-specific" )
		{
			string ticker = sl[3];
			if( read_all || market[1] == ticker[0] )
				tickers.push_back(ticker);
		}
	}
	return tickers;
}
