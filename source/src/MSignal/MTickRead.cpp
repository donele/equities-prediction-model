#include <MSignal/MTickRead.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
#include <map>
using namespace std;

MTickRead::MTickRead(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName, true),
verbose_(0),
ticker_choice_(""),
remove_break_(false),
nThreads_(1)
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("removeBreak") )
		remove_break_ = conf.find("removeBreak")->second == "true";

	// Tickers.
	if( conf.count("ticker") )
		ticker_choice_ = conf.find("ticker")->second;

	if( conf.count("marketTicker") )
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
					siQuote_.do_read = true;
				if( "nbbo" == source )
					siNbbo_.do_read = true;
				if( "trade" == source )
					siTrade_.do_read = true;
				if( "order" == source )
					siOrder_.do_read = true;
			}
		}
	}

	if( conf.count("nbboTrade") )
		nbbo_trade_ = conf.find("nbboTrade")->second == "true";

	// Directories.
	if( conf.count("quote_dir") )
	{
		siQuote_.dir_given = true;
		pair<mmit, mmit> dirs = conf.equal_range("quote_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				siQuote_.mktDirs[market].push_back(dir);
			}
		}
	}
	if( conf.count("nbbo_dir") )
	{
		siNbbo_.dir_given = true;
		pair<mmit, mmit> dirs = conf.equal_range("nbbo_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				siNbbo_.mktDirs[market].push_back(dir);
			}
		}
	}
	if( conf.count("trade_dir") )
	{
		siTrade_.dir_given = true;
		pair<mmit, mmit> dirs = conf.equal_range("trade_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				siTrade_.mktDirs[market].push_back(dir);
			}
		}
	}
	if( conf.count("order_dir") )
	{
		siOrder_.dir_given = true;
		pair<mmit, mmit> dirs = conf.equal_range("order_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				siOrder_.mktDirs[market].push_back(dir);
			}
		}
	}
}

MTickRead::~MTickRead()
{}

void MTickRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	tSources_.read(MEnv::Instance()->markets[0], MEnv::Instance()->sourceFlag);
	if(MEvent::Instance()->find("", "marketTickers"))
		marketTickers_ = MEvent::Instance()->get<map<string, vector<string> > >("", "marketTickers");

	if( MEnv::Instance()->multiThreadTicker )
		nThreads_ = MEnv::Instance()->nMaxThreadTicker;

	return;
}

void MTickRead::beginMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	longTicker_ = mto::longTicker(market);
	sessions_ = Sessions(market, idate);

//
//	Initialize TickAcess.
//
	set_directories(market, idate);
	init_TickAccess();

//
//	Ticker List.
//
	vector<string> namesQ;
	vector<string> namesN;
	vector<string> namesT;
	vector<string> namesO;

	if( siQuote_.do_read )
		taQ_.GetNames(idate, &namesQ);
	if( siNbbo_.do_read )
		taN_.GetNames(idate, &namesN);
	if( siTrade_.do_read )
		taT_.GetNames(idate, &namesT);
	if( siOrder_.do_read )
		taO_.GetNames(idate, &namesO);

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
		vector<string> univ_tickers = get_univ_tickers(market, idate);

		tickers = comp_ticker( univ_tickers, market );
	}
	else if( "fitting" == ticker_choice_ )
	{
		tickers = comp_ticker( marketTickers_[market], market );
	}

	if( !tickers.empty() )
	{
		sort(tickers.begin(), tickers.end());
		MEnv::Instance()->tickers = tickers;
	}

	if( verbose_ > 3 )
	{
		vector<string> tickers = MEnv::Instance()->tickers;
		int ts = tickers.size();
		//if( ts > 0 )
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

	if( verbose_ > 0 )
		cout << "MTickRead::beginMarketDay() " << idate << " " << market << ".\n";
	if( verbose_ > 1 )
		PrintMemoryInfoSimple();
	return;
}

void MTickRead::beginTicker(const string& ticker)
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	int nq = 0;
	int nn = 0;
	int nw = 0;
	int nt = 0;
	int no = 0;

	if( siTrade_.do_read )
	{
		TickAccessMulti<TradeInfo>& taT = get_TickAccess_trade();
		TickSeries<TradeInfo>& tsT = get_TickSeries_trade();

		taT.GetTickSeries( ticker, idate, &tsT );
		nt = tsT.NTicks();
		add_tickdata<TradeInfo>(ticker, "trades", tsT);
	}

	if( siQuote_.do_read )
	{
		TickAccessMulti<QuoteInfo>& taQ = get_TickAccess_quote();
		TickSeries<QuoteInfo>& tsQ = get_TickSeries_quote();

		taQ.GetTickSeries( ticker, idate, &tsQ );
		nq = tsQ.NTicks();
		add_tickdata<QuoteInfo>(ticker, "quotes", tsQ);
	}

	if( siNbbo_.do_read )
	{
		TickAccessMulti<QuoteInfo>& taN = get_TickAccess_nbbo();
		TickSeries<QuoteInfo>& tsN = get_TickSeries_nbbo();

		taN.GetTickSeries( ticker, idate, &tsN );
		nn = tsN.NTicks();
		add_tickdata<QuoteInfo>(ticker, "nbbo", tsN);
	}

	if( siOrder_.do_read )
	{
		TickAccessMulti<OrderData>& taO = get_TickAccess_order();
		TickSeries<OrderData>& tsO = get_TickSeries_order();

		taO.GetTickSeries( ticker, idate, &tsO );
		no = tsO.NTicks();
		add_tickdata<OrderData>(ticker, "orders", tsO);
	}

	if( verbose_ > 3 )
	{
		boost::mutex::scoped_lock lock(MEnv::Instance()->io_mutex);
		cout << " ticker: " << ticker << " #quote = " << nq << " #trade = " << nt << " #order = " << no << "\n";
	}

	return;
}

void MTickRead::endTicker(const string& ticker)
{
	if( siQuote_.do_read )
		MEvent::Instance()->remove<vector<QuoteInfo> >(ticker, "quotes");
	if( siNbbo_.do_read )
		MEvent::Instance()->remove<vector<QuoteInfo> >(ticker, "nbbo");
	if( siTrade_.do_read )
		MEvent::Instance()->remove<vector<TradeInfo> >(ticker, "trades");
	if( siOrder_.do_read )
		MEvent::Instance()->remove<vector<OrderData> >(ticker, "orders");
}

void MTickRead::endMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	if( verbose_ > 2 )
	{
		cout << "\MTickRead::endMarketDay() " << idate << " " << market << ".\n";
		PrintMemoryInfoSimple();
	}
	taQ_.Clear();
	taN_.Clear();
	taT_.Clear();
	taO_.Clear();

	mtaQ_.clear();
	mtaN_.clear();
	mtaT_.clear();
	mtaO_.clear();

	mtsQ_.clear();
	mtsN_.clear();
	mtsT_.clear();
	mtsO_.clear();
}

void MTickRead::endJob()
{
}

void MTickRead::set_directories(const std::string& market, int idate)
{
	if( siQuote_.do_read )
	{
		if( siQuote_.dir_given )
			siQuote_.dirs = siQuote_.mktDirs[market];
		else
			siQuote_.dirs = tSources_.stocksdirectory(market, idate);
	}
	if( siNbbo_.do_read )
	{
		if( siNbbo_.dir_given )
			siNbbo_.dirs = siNbbo_.mktDirs[market];
		else
			siNbbo_.dirs = tSources_.nbbodirectory(market, idate);
	}
	if( siTrade_.do_read )
	{
		if( nbbo_trade_ )
		{
			siTrade_.dirs = siNbbo_.dirs;
		}
		else
		{
			if( siTrade_.dir_given )
				siTrade_.dirs = siTrade_.mktDirs[market];
			else
			{
				if( siQuote_.dir_given ) // If quote dir is given, use it for the trade, too.
					siTrade_.dirs = siTrade_.mktDirs[market];
				else
					siTrade_.dirs = tSources_.nbbodirectory(market, idate);
			}
		}
	}
	if( siOrder_.do_read )
	{
		if( siOrder_.dir_given )
			siOrder_.dirs = siOrder_.mktDirs[market];
		else
			siOrder_.dirs = tSources_.bookdirectory(market, idate);
	}
}

void MTickRead::init_TickAccess()
{
	if( siQuote_.do_read )
	{
		for( vector<string>::iterator itd = siQuote_.dirs.begin(); itd != siQuote_.dirs.end(); ++itd )
		{
			string dir = *itd;
			if( !dir.empty() )
			{
				taQ_.AddRoot(dir, longTicker_);

				if( verbose_ > 3 )
					cout << "Quote dir " << dir << endl;
			}
		}
	}
	if( siNbbo_.do_read )
	{
		for( vector<string>::iterator itd = siNbbo_.dirs.begin(); itd != siNbbo_.dirs.end(); ++itd )
		{
			string dir = *itd;
			if( !dir.empty() )
			{
				taN_.AddRoot(dir, longTicker_);

				if( verbose_ > 3 )
					cout << "Nbbo dir " << dir << endl;
			}
		}
	}
	if( siTrade_.do_read )
	{
		for( vector<string>::iterator itd = siTrade_.dirs.begin(); itd != siTrade_.dirs.end(); ++itd )
		{
			string dir = *itd;
			if( !dir.empty() )
			{
				taT_.AddRoot(dir, longTicker_);

				if( verbose_ > 3 )
					cout << "Trade dir " << dir << endl;
			}
		}
	}
	if( siOrder_.do_read )
	{
		for( vector<string>::iterator itd = siOrder_.dirs.begin(); itd != siOrder_.dirs.end(); ++itd )
		{
			string dir = *itd;
			if( !dir.empty() )
			{
				taO_.AddRoot(dir, longTicker_);

				if( verbose_ > 3 )
					cout << "Order dir " << dir << endl;
			}
		}
	}
}

TickAccessMulti<QuoteInfo>& MTickRead::get_TickAccess_quote()
{
	if( nThreads_ > 1 )
	{
		boost::thread::id id = boost::this_thread::get_id();
		map<boost::thread::id, TickAccessMulti<QuoteInfo> >::iterator it = mtaQ_.find(id);
		if( it == mtaQ_.end() )
		{
			for( vector<string>::iterator itd = siQuote_.dirs.begin(); itd != siQuote_.dirs.end(); ++itd )
			{
				string dir = *itd;
				if( !dir.empty() )
				{
					boost::mutex::scoped_lock lock(mtaQ_mutex_);
					mtaQ_[id].AddRoot(dir, longTicker_);
				}
			}
			it = mtaQ_.find(id);
		}
		return it->second;
	}

	return taQ_;
}

TickAccessMulti<QuoteInfo>& MTickRead::get_TickAccess_nbbo()
{
	if( nThreads_ > 1 )
	{
		boost::thread::id id = boost::this_thread::get_id();
		map<boost::thread::id, TickAccessMulti<QuoteInfo> >::iterator it = mtaN_.find(id);
		if( it == mtaN_.end() )
		{
			for( vector<string>::iterator itd = siNbbo_.dirs.begin(); itd != siNbbo_.dirs.end(); ++itd )
			{
				string dir = *itd;
				if( !dir.empty() )
				{
					boost::mutex::scoped_lock lock(mtaN_mutex_);
					mtaN_[id].AddRoot(dir, longTicker_);
				}
			}
			it = mtaN_.find(id);
		}
		return it->second;
	}

	return taN_;
}

TickAccessMulti<TradeInfo>& MTickRead::get_TickAccess_trade()
{
	if( nThreads_ > 1 )
	{
		boost::thread::id id = boost::this_thread::get_id();
		map<boost::thread::id, TickAccessMulti<TradeInfo> >::iterator it = mtaT_.find(id);
		if( it == mtaT_.end() )
		{
			for( vector<string>::iterator itd = siTrade_.dirs.begin(); itd != siTrade_.dirs.end(); ++itd )
			{
				string dir = *itd;
				if( !dir.empty() )
				{
					boost::mutex::scoped_lock lock(mtaT_mutex_);
					mtaT_[id].AddRoot(dir, longTicker_);
				}
			}
			it = mtaT_.find(id);
		}
		return it->second;
	}

	return taT_;
}

TickAccessMulti<OrderData>& MTickRead::get_TickAccess_order()
{
	if( nThreads_ > 1 )
	{
		boost::thread::id id = boost::this_thread::get_id();
		map<boost::thread::id, TickAccessMulti<OrderData> >::iterator it = mtaO_.find(id);
		if( it == mtaO_.end() )
		{
			for( vector<string>::iterator itd = siOrder_.dirs.begin(); itd != siOrder_.dirs.end(); ++itd )
			{
				string dir = *itd;
				if( !dir.empty() )
				{
					boost::mutex::scoped_lock lock(mtaO_mutex_);
					mtaO_[id].AddRoot(dir, longTicker_);
				}
			}
			it = mtaO_.find(id);
		}
		return it->second;
	}

	return taO_;
}

TickSeries<QuoteInfo>& MTickRead::get_TickSeries_quote()
{
	if( nThreads_ > 1 )
	{
		boost::thread::id id = boost::this_thread::get_id();
		map<boost::thread::id, TickSeries<QuoteInfo> >::iterator it = mtsQ_.find(id);
		if( it == mtsQ_.end() )
		{
			boost::mutex::scoped_lock lock(mtsO_mutex_);
			return mtsQ_[id];
		}
		return it->second;
	}

	return tsQ_;
}

TickSeries<QuoteInfo>& MTickRead::get_TickSeries_nbbo()
{
	if( nThreads_ > 1 )
	{
		boost::thread::id id = boost::this_thread::get_id();
		map<boost::thread::id, TickSeries<QuoteInfo> >::iterator it = mtsN_.find(id);
		if( it == mtsN_.end() )
		{
			boost::mutex::scoped_lock lock(mtsN_mutex_);
			return mtsN_[id];
		}
		return it->second;
	}

	return tsN_;
}

TickSeries<TradeInfo>& MTickRead::get_TickSeries_trade()
{
	if( nThreads_ > 1 )
	{
		boost::thread::id id = boost::this_thread::get_id();
		map<boost::thread::id, TickSeries<TradeInfo> >::iterator it = mtsT_.find(id);
		if( it == mtsT_.end() )
		{
			boost::mutex::scoped_lock lock(mtsT_mutex_);
			return mtsT_[id];
		}
		return it->second;
	}

	return tsT_;
}

TickSeries<OrderData>& MTickRead::get_TickSeries_order()
{
	if( nThreads_ > 1 )
	{
		boost::thread::id id = boost::this_thread::get_id();
		map<boost::thread::id, TickSeries<OrderData> >::iterator it = mtsO_.find(id);
		if( it == mtsO_.end() )
		{
			boost::mutex::scoped_lock lock(mtsO_mutex_);
			return mtsO_[id];
		}
		return it->second;
	}

	return tsO_;
}

