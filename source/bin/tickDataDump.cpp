#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/CrossStat.h>
#include <jl_lib/Arg.h>
#include <jl_lib/TickSources.h>
#include <gtlib_tickdump/DumpImpl.h>
#include <gtlib_tickdump/DumpTick.h>
#include <gtlib_tickdump/DumpNbboConsumer.h>
#include <optionlibs/TickData.h>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <functional>
#include <numeric>
#include <algorithm>
#include <limits.h>
using namespace std;
using namespace gtlib;

void exit_usage()
{
	cout << "usage:\n";
	cout << "tickDataDump.exe -dt <type> -m <market> -d <idate> (-t <ticker> -a <msecs> -n <nlines> -p <path> -c)\n";
	cout << " type = quote, nbbo, trade, booktrade, order, return\n";
	exit(0);
}

DumpTick* getDumpTick(const string& type, int idate,
		const vector<string>& dirs, int n)
{
	if( type == "order" )
		return new DumpOrder(idate, dirs, n);
	else if( type == "quote" || type == "nbbo" )
		return new DumpQuote(idate, dirs, n);
	else if( type == "trade" )
		return new DumpTrade(idate, dirs, n);
	else if( type == "return" )
		return new DumpReturn(idate, dirs, n);
	return nullptr;
}

int main(int argc, char* argv[])
{
	Arg arg(argc, argv);
	if( argc < 2 || !arg.isGiven("dt") || !arg.isGiven("m") || !arg.isGiven("d") )
		exit_usage();

	string method = arg.isGiven("c") ? "consumer" : "direct";
	string type = arg.getVal("dt");
	string market = arg.getVal("m");
	int idate = atoi(arg.getVal("d").c_str());
	int msecs = atoi(arg.getVal("a").c_str());
	int n = atoi(arg.getVal("n").c_str());
	string ticker = arg.getVal("t");
	vector<string> paths = arg.getVals("p");

	if( method == "direct" )
	{
		vector<string> dirs;
		if( !paths.empty() )
			dirs = paths;
		else
		{
			TickSources tickSrc(mto::region(market));
			dirs = tickSrc.getDirs(type, market, idate);
		}

		DumpTick* pd = getDumpTick(type, idate, dirs, n);
		if( !ticker.empty() )
			pd->dump(ticker, msecs);
		else
			pd->summ();
	}
	else if(method == "consumer" && !ticker.empty()
			&& (type == "quote" || type == "nbbo"))
	{
		int openMsecs = mto::msecOpen(market, idate);
		int closeMsecs = mto::msecClose(market, idate);
		TickProviderClassic<TradeInfo, QuoteInfo, OrderData> tp(idate, openMsecs, closeMsecs);

		vector<string> allDests = mto::dests(market);
		TickSources tickSrc(mto::region(market));
		for( auto& dest : allDests )
		{
			vector<string> dirs = tickSrc.bookdirectory(dest, idate);
			for( auto& dir : dirs )
				tp.AddBookRoot(dest[1], dir);
		}

		DumpNbboConsumer consumer(n);
		tp.SetConsumer(&consumer);
		tp.StartSymbol(ticker, 0., vector<int>());
		tp.Run();
	}
	return 0;
}
