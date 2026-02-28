#include <jl_lib/DayLoop.h>
#include <jl_lib/jlutil.h>
#include "optionlibs/TickData.h"
#include <jl_lib/OneDayStat.h>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
using namespace std;

DayLoop::DayLoop()
{}

DayLoop::~DayLoop()
{}

void DayLoop::set_valid_symbols( const set<string>& valid_symbols )
{
	valid_symbols_ = valid_symbols;
	return;
}

bool DayLoop::read_binary( const string& bin_dir, int idate, OneDayStat* ods, bool longTicker,
						  int nTickerMin, int symbolSizeMax )
{
	vector<string> names;

	QuoteInfo quote;
	TickAccess<QuoteInfo> taQ(xpf(bin_dir), longTicker);
	TickSeries<QuoteInfo> tsQ;
	set<string> names_quote;
	taQ.GetNames(idate,&names);
	for( vector<string>::const_iterator it = names.begin(); it != names.end(); ++it )
		names_quote.insert(*it);

	TradeInfo trade;
	TickAccess<TradeInfo> taT(xpf(bin_dir), longTicker);
	TickSeries<TradeInfo> tsT;
	set<string> names_trade;
	taT.GetNames(idate,&names);
	for( vector<string>::const_iterator it = names.begin(); it != names.end(); ++it )
		names_trade.insert(*it);

	set<string> names_all;
	set_union(names_quote.begin(), names_quote.end(), names_trade.begin(), names_trade.end(), inserter(names_all, names_all.begin()) );
	int nsize = names_all.size();

	cout << idate << " quote: " << names_quote.size() << " trade: " << names_trade.size() << endl;

	if( nsize < nTickerMin )
		return false; // Not a trading day. Go to the next day.

	set<string> temp_valid_symbols;
	for( set<string>::iterator it = names_all.begin(); it != names_all.end(); ++it )
	{
		string symbol = *it;
		if( valid_symbols_.count(symbol) )
			temp_valid_symbols.insert(symbol);
	}
	valid_symbols_ = temp_valid_symbols;

	for( set<string>::const_iterator it = names_all.begin(); it != names_all.end(); ++it )
	{
		string symbol = (*it);

		if( symbol.size() > symbolSizeMax )
			continue;
		if( !valid_symbols_.empty() && !valid_symbols_.count(symbol) )
			continue;

		// Read Trade binary
		taT.GetTickSeries( symbol.c_str(), idate, &tsT );
		int nTticks = tsT.NTicks();
		tsT.StartRead();
		for( int u=0; u<nTticks; ++u )
		{
			tsT.Read(&trade);
			ods->fill_trade( symbol, trade );
		}

		// Read Quote binary
		taQ.GetTickSeries( symbol.c_str(), idate, &tsQ );
		int nQticks = tsQ.NTicks();
		tsQ.StartRead();
		for( int u=0; u<nQticks; ++u )
		{
			tsQ.Read(&quote);
			ods->fill_quote( symbol, quote );
		}
		
		ods->finish_name( symbol );
	}

	ods->finish_day();
	return true;
}
