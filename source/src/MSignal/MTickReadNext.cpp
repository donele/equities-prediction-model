#include <MSignal/MTickReadNext.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
#include <map>
using namespace std;
using std::string;

MTickReadNext::MTickReadNext(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName, true),
	verbose_(0),
	preload_(true),
	ticker_choice_(""),
	remove_break_(false)
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("preload") )
		preload_ = atoi( conf.find("preload")->second.c_str() );
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
				if( "sip" == source )
					siSip_.do_read = true;
				if( "trade" == source )
					siTrade_.do_read = true;
				if( "order" == source )
					siOrder_.do_read = true;
				if( "news" == source )
					siNews_.do_read = true;
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
	if( conf.count("sip_dir") )
	{
		siSip_.dir_given = true;
		pair<mmit, mmit> dirs = conf.equal_range("sip_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				siSip_.mktDirs[market].push_back(dir);
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
	if( conf.count("news_dir") )
	{
		siNews_.dir_given = true;
		pair<mmit, mmit> dirs = conf.equal_range("news_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				string market = vs[0];
				string dir = vs[1];
				siNews_.mktDirs[market].push_back(dir);
			}
		}
	}
}

MTickReadNext::~MTickReadNext()
{}

void MTickReadNext::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	tSources_.read(MEnv::Instance()->markets[0], MEnv::Instance()->sourceFlag);
	if(MEvent::Instance()->find("", "marketTickers"))
		marketTickers_ = MEvent::Instance()->get<map<string, vector<string> > >("", "marketTickers");
	return;
}

void MTickReadNext::beginMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	longTicker_ = mto::longTicker(market);
	sessions_ = MSessions(market, idate);

	set_directories(market, idate);

//
//	Ticker List.
//
	vector<string> namesQ;
	vector<string> namesN;
	vector<string> namesT;
	vector<string> namesO;

	if( siQuote_.do_read )
	{
		TickAccessMulti<QuoteInfo> ta;
		for( vector<string>::iterator it = siQuote_.dirs.begin(); it != siQuote_.dirs.end(); ++it )
			ta.AddRoot(*it, longTicker_);
		ta.GetNames(idate, &namesQ);
	}
	if( siNbbo_.do_read )
	{
		TickAccessMulti<QuoteInfo> ta;
		for( vector<string>::iterator it = siNbbo_.dirs.begin(); it != siNbbo_.dirs.end(); ++it )
			ta.AddRoot(*it, longTicker_);
		ta.GetNames(idate, &namesN);
	}
	if( siSip_.do_read )
	{
		TickAccessMulti<QuoteInfo> ta;
		for( vector<string>::iterator it = siSip_.dirs.begin(); it != siSip_.dirs.end(); ++it )
			ta.AddRoot(*it, longTicker_);
		ta.GetNames(idate, &namesQ);
	}
	if( siTrade_.do_read )
	{
		TickAccessMulti<TradeInfo> ta;
		for( vector<string>::iterator it = siTrade_.dirs.begin(); it != siTrade_.dirs.end(); ++it )
			ta.AddRoot(*it, longTicker_);
		ta.GetNames(idate, &namesT);
	}
	if( siOrder_.do_read )
	{
		TickAccessMulti<OrderData> ta;
		for( vector<string>::iterator it = siOrder_.dirs.begin(); it != siOrder_.dirs.end(); ++it )
			ta.AddRoot(*it, longTicker_);
		ta.GetNames(idate, &namesO);
	}

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

	if(preload_)
	{
		for(string& ticker : MEnv::Instance()->tickers)
			readTickData(ticker);
	}

	if( verbose_ > 0 )
		cout << "\MTickReadNext::beginMarketDay() " << idate << " " << market << ".\n";
	if( verbose_ > 1 )
		PrintMemoryInfoSimple();
	return;
}

void MTickReadNext::beginTicker(const string& ticker)
{
	if(!preload_)
		readTickData(ticker);
}

void MTickReadNext::readTickData(const string& ticker)
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	int nq = 0;
	int nn = 0;
	int nt = 0;
	int no = 0;
	int nw = 0;
	int ns = 0;

	if( siTrade_.do_read )
		read_data<TradeInfo>(siTrade_.dirs, "trades", nt, ticker, idate);
	if( siQuote_.do_read )
		read_data<QuoteInfo>(siQuote_.dirs, "quotes", nq, ticker, idate);
	if( siNbbo_.do_read )
		read_data<QuoteInfo>(siNbbo_.dirs, "nbbo", nn, ticker, idate);
	if( siSip_.do_read )
		read_data<QuoteInfo>(siSip_.dirs, "sip", ns, ticker, idate);
	if( siOrder_.do_read )
		read_data<OrderData>(siQuote_.dirs, "orders", no, ticker, idate);
	if( siNews_.do_read )
		read_data<QuoteInfo>(siNews_.dirs, "news", nw, ticker, idate);

	if( verbose_ > 3 )
	{
		boost::mutex::scoped_lock lock(MEnv::Instance()->io_mutex);
		cout << " ticker: " << ticker << " #quote = " << nq << " #trade = " << nt << " #order = " << no << " #news = " << nw << "\n";
	}
}

void MTickReadNext::endTicker(const string& ticker)
{
	if( siQuote_.do_read )
		MEvent::Instance()->remove<vector<QuoteInfo> >(ticker, "quotes");
	if( siNbbo_.do_read )
		MEvent::Instance()->remove<vector<QuoteInfo> >(ticker, "nbbo");
	if( siSip_.do_read )
		MEvent::Instance()->remove<vector<QuoteInfo> >(ticker, "sip");
	if( siTrade_.do_read )
		MEvent::Instance()->remove<vector<TradeInfo> >(ticker, "trades");
	if( siOrder_.do_read )
		MEvent::Instance()->remove<vector<OrderData> >(ticker, "orders");
	if( siNews_.do_read )
		MEvent::Instance()->remove<vector<QuoteInfo> >(ticker, "news");
}

void MTickReadNext::endMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	if( verbose_ > 2 )
	{
		cout << "MTickReadNext::endMarketDay() " << idate << " " << market << ".\n";
		PrintMemoryInfoSimple();
	}
}

void MTickReadNext::endJob()
{
}

void MTickReadNext::set_directories(const std::string& market, int idate)
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
	if( siSip_.do_read )
	{
		if( siSip_.dir_given )
			siSip_.dirs = siSip_.mktDirs[market];
		else
			siSip_.dirs = tSources_.sipdirectory(market, idate);
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
	if( siNews_.do_read )
	{
		if( siNews_.dir_given )
			siNews_.dirs = siNews_.mktDirs[market];
		//else
		//	siNews_.dirs = tSources_.newsdirectory(market, idate);
	}
}
