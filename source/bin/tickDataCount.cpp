#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/Arg.h>
#include <jl_lib/TickSources.h>
#include <gtlib_tickdump/DumpCountConsumer.h>
#include <gtlib/EstTime.h>
#include <optionlibs/TickData.h>
#include <string>
#include <chrono>
using namespace std;
using namespace gtlib;

void exit_usage()
{
	cout << "usage:\n";
	cout << "tickDataCount.exe -m <market> -d <idate> -f <msecsFrom> -t <msecsTo> -v <interval> (-n <ntickers>)\n";
	exit(0);
}

TickProviderClassic<TradeInfo, QuoteInfo, OrderData>* getTickProvider(const string& market, int idate, bool debug);

int main(int argc, char* argv[])
{
	Arg arg(argc, argv);
	if( argc < 2 || !arg.isGiven("m") || !arg.isGiven("d") )
		exit_usage();

	bool debug = arg.isGiven("b");
	string market = arg.getVal("m");
	int idate = atoi(arg.getVal("d").c_str());
	int msecsFrom = atoi(arg.getVal("f").c_str());
	int msecsTo = atoi(arg.getVal("t").c_str());
	int interval = atoi(arg.getVal("v").c_str());
	int maxNtickers = atoi(arg.getVal("n").c_str());

	auto tp = getTickProvider(market, idate, debug);
	vector<string> tickers = get_univ_tickers(market, idate);
	int nTickers = tickers.size();

	DumpCountConsumer consumer(msecsFrom, msecsTo, interval);
	tp->SetConsumer(&consumer);
	gtlib::EstTime estTime(nTickers);

	for( int i = 0; i < nTickers; ++i )
	{
		if( maxNtickers > 0 && i >= maxNtickers )
			break;
		string ticker = tickers[i];

		if( debug )
			estTime.beginTicker();

		tp->StartSymbol(ticker, max_double_, vector<int>());
		tp->Run();
	}
	if( debug )
		estTime.endDay();
	consumer.print();
	return 0;
}

TickProviderClassic<TradeInfo, QuoteInfo, OrderData>* getTickProvider(const string& market, int idate, bool debug)
{
	int openMsecs = mto::msecOpen(market, idate);
	int closeMsecs = mto::msecClose(market, idate);
	auto tp = new TickProviderClassic<TradeInfo, QuoteInfo, OrderData>(idate, openMsecs, closeMsecs);

	vector<string> allDests = mto::dests(market);
	TickSources tickSrc(mto::region(market));

	// Nbbo dir
	auto dirs = tickSrc.nbbodirectory(market, idate);
	for( auto& dir : dirs )
	{
		tp->AddNbboRoot(dir);
		tp->AddTradeRoot(dir);
	}

	// Book dir
	for( auto& dest : allDests )
	{
		vector<string> dirs = tickSrc.bookdirectory(dest, idate);
		for( auto& dir : dirs )
		{
			if( debug )
				cout << dir << endl;
			tp->AddBookRoot(dest[1], dir);
			tp->AddBookTradeRoot(dest[1], dir);
		}
	}
	return tp;
}
