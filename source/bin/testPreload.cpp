#include <gtlib/util.h>
#include <jl_lib/jlutil.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <iostream>
using namespace std;

int main() {
	string market = "US";
	auto* dpMilli_ = new TickDataProviderMilli<TradeInfo, OrderDataMicro>();

	// nbbodir.
	dpMilli_->AddTradeRoot("/mnt/gf1/tickC/us/book/nbbo/1");
	dpMilli_->AddTradeRoot("/mnt/gf1/tickC/us/book/nbbo/2");

	bool isLfs = true;
	bool forceLfs = true;
	if( isLfs )
	{
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/lfs");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/lfs");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/lfs");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/lfs");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/lfs");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/lfs");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/lfs");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/lfs");
	}
	else
	{
		if( forceLfs )
		{
			dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/lfs");
			dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/lfs");
			dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/lfs");
			dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/lfs");
			dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/lfs");
			dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/lfs");
			dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/lfs");
			dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/lfs");
		}

		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/1");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/10");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/11");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/12");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/13");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/14");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/15");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/16");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/17");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/18");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/19");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/2");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/20");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/3");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/4");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/5");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/6");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/7");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/8");
		dpMilli_->AddBookRoot('P', "/mnt/gf0/book_us/arcabook/9");

		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/1");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/10");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/11");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/12");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/13");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/14");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/15");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/16");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/17");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/18");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/19");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/2");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/20");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/3");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/4");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/5");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/6");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/7");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/8");
		dpMilli_->AddBookRoot('Q', "/mnt/gf0/book_us/inetbook/9");

		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/1");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/10");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/11");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/12");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/13");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/14");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/15");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/16");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/17");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/18");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/19");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/2");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/20");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/3");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/4");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/5");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/6");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/7");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/8");
		dpMilli_->AddBookRoot('N', "/mnt/gf0/book_us/nyseultrabook/9");

		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/1");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/10");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/11");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/12");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/13");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/14");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/15");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/16");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/17");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/18");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/19");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/2");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/20");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/3");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/4");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/5");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/6");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/7");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/8");
		dpMilli_->AddBookRoot('D', "/mnt/gf0/book_us/edgx/9");

		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/1");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/10");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/11");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/12");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/13");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/14");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/15");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/16");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/17");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/18");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/19");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/2");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/20");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/3");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/4");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/5");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/6");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/7");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/8");
		dpMilli_->AddBookRoot('J', "/mnt/gf0/book_us/edga/9");

		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/1");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/10");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/11");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/12");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/13");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/14");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/15");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/16");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/17");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/18");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/19");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/2");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/20");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/3");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/4");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/5");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/6");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/7");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/8");
		dpMilli_->AddBookRoot('B', "/mnt/gf0/book_us/bx/9");

		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/1");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/10");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/11");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/12");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/13");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/14");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/15");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/16");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/17");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/18");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/19");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/2");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/20");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/3");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/4");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/5");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/6");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/7");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/8");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/9");

		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/1");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/10");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/11");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/12");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/13");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/14");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/15");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/16");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/17");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/18");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/19");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/2");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/20");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/3");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/4");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/5");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/6");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/7");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/8");
		dpMilli_->AddBookRoot('Y', "/mnt/gf0/book_us/byx/9");
	}

	int idate = 20160601;
	auto tickers = get_univ_tickers("US", idate);
	vector<string> subset(tickers.begin(), tickers.begin() + 10);

	// preload
	cout << " Begin preload " << getTimerInfoSimple() << endl;
	dpMilli_->PreloadData(idate, subset);
	cout << " End preload " << getTimerInfoSimple() << endl;
}
