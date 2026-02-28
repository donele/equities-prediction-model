#include <gtlib_tickdump/DumpCountConsumer.h>
#include <vector>
using namespace std;

namespace gtlib {

DumpCountConsumer::DumpCountConsumer(int msecsFrom, int msecsTo, int interval)
	:msecsFrom_(msecsFrom),
	msecsTo_(msecsTo),
	interval_(interval),
	nbins_((msecsTo - msecsFrom) / interval)
{
	cntBook_ = vector<int>(nbins_);
	cntNbbo_ = vector<int>(nbins_);
	cntTrade_ = vector<int>(nbins_);
	cntBookTrade_ = vector<int>(nbins_);
}

void DumpCountConsumer::NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( msecs >= msecsFrom_ && msecs < msecsTo_ )
		++cntNbbo_[(msecs - msecsFrom_)/interval_];
}

void DumpCountConsumer::TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( msecs >= msecsFrom_ && msecs < msecsTo_ )
		++cntTrade_[(msecs - msecsFrom_)/interval_];
}

void DumpCountConsumer::BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider)
{
	if( msecs >= msecsFrom_ && msecs < msecsTo_ )
		++cntBookTrade_[(msecs - msecsFrom_)/interval_];
}

void DumpCountConsumer::print()
{
	char buf[200];
	sprintf(buf, "%8s %8s %8s %8s %8s %8s\n", "msFrom", "msTo", "book", "nbbo", "trade", "booktrade");
	cout << buf;

	for( int i = 0; i < nbins_; ++i )
	{
		int msFrom = msecsFrom_ + i * interval_;
		int msTo = msFrom + interval_;
		sprintf(buf, "%8d %8d %8d %8d %8d %8d\n", msFrom, msTo,
				cntBook_[i], cntNbbo_[i], cntTrade_[i], cntBookTrade_[i]);
		cout << buf;
	}
}

} // namespace gtlib

