#include <MFutModel/MTickReadFut.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
#include <map>
using namespace std;

MTickReadFut::MTickReadFut(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName, true),
verbose_(0)
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );

	// Products.
	if( conf.count("product") )
		products_.push_back(conf.find("product")->second);

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
				if( "trade" == source )
					siTrade_.do_read = true;
				if( "order" == source )
					siOrder_.do_read = true;
			}
		}
	}

	// Directories.
	if( conf.count("quote_dir") )
	{
		siQuote_.dir_given = true;
		siQuote_.dir = conf.find("quote_dir")->second;
	}
	if( conf.count("trade_dir") )
	{
		siTrade_.dir_given = true;
		siTrade_.dir = conf.find("quote_dir")->second;
	}
	if( conf.count("order_dir") )
	{
		siOrder_.dir_given = true;
		siOrder_.dir = conf.find("quote_dir")->second;
	}

	// Default directories.
	string dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\cme_test\\binLogL2");
	if( siQuote_.dir.empty() )
		siQuote_.dir = dir;
	if( siTrade_.dir.empty() )
		siTrade_.dir = dir;
	if( siOrder_.dir.empty() )
		siOrder_.dir = dir;
}

MTickReadFut::~MTickReadFut()
{}

void MTickReadFut::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void MTickReadFut::beginMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;

	vector<string> tickers;
	if( !products_.empty() )
	{
		for( vector<string>::iterator it = products_.begin(); it != products_.end(); ++it )
		{
			string product = *it;
			string ticker = cme::front_ticker(product, idate);
			if( !ticker.empty() )
				tickers.push_back(ticker);
		}

		sort(tickers.begin(), tickers.end());
		MEnv::Instance()->tickers = tickers;
	}

	if( verbose_ > 3 )
	{
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
		cout << "\MTickReadFut::beginMarketDay() " << idate << " " << market << ".\n";
	if( verbose_ > 1 )
		PrintMemoryInfoSimple();
	return;
}

void MTickReadFut::beginTicker(const string& ticker)
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	int nq = 0;
	int nw = 0;
	int nt = 0;
	int no = 0;

	if( siTrade_.do_read )
	{
		TickAccess<TradeInfo> ta(siTrade_.dir);
		TickSeries<TradeInfo> ts(200000, 1.2);
		ta.GetTickSeries( ticker, idate, &ts );
		nt = ts.NTicks();
		add_tickdata<TradeInfo>(ticker, "trades", ts);
	}

	if( siQuote_.do_read )
	{
		TickAccess<QuoteInfo> ta(siQuote_.dir);
		TickSeries<QuoteInfo> ts(200000, 1.2);
		ta.GetTickSeries( ticker, idate, &ts );
		nq = ts.NTicks();
		add_tickdata<QuoteInfo>(ticker, "quotes", ts);
	}

	if( siOrder_.do_read )
	{
		TickAccess<OrderData> ta(siOrder_.dir);
		TickSeries<OrderData> ts(200000, 1.2);
		ta.GetTickSeries( ticker, idate, &ts );
		no = ts.NTicks();
		add_tickdata<OrderData>(ticker, "orders", ts);
	}

	if( verbose_ > 3 )
	{
		boost::mutex::scoped_lock lock(MEnv::Instance()->io_mutex);
		cout << " ticker: " << ticker << " #quote = " << nq << " #trade = " << nt << " #order = " << no << "\n";
	}

	return;
}

void MTickReadFut::endTicker(const string& ticker)
{
	if( siQuote_.do_read )
		MEvent::Instance()->remove<vector<QuoteInfo> >(ticker, "quotes");
	if( siTrade_.do_read )
		MEvent::Instance()->remove<vector<TradeInfo> >(ticker, "trades");
	if( siOrder_.do_read )
		MEvent::Instance()->remove<vector<OrderData> >(ticker, "orders");
}

void MTickReadFut::endMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	if( verbose_ > 2 )
	{
		cout << "\MTickReadFut::endMarketDay() " << idate << " " << market << ".\n";
		PrintMemoryInfoSimple();
	}
}

void MTickReadFut::endJob()
{
}
