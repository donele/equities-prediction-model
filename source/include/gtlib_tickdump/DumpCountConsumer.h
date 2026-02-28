#ifndef __gtlib_tickdump_DumpCountConsumer__
#define __gtlib_tickdump_DumpCountConsumer__
#include <vector>
#include <string>
#include <optionlibs/TickData.h>

namespace gtlib {

class DumpCountConsumer : public TickConsumer<TradeInfo,QuoteInfo,OrderData> {
public:
	DumpCountConsumer(){}
	DumpCountConsumer(int msecsFrom, int msecsTo, int interval);
	virtual ~DumpCountConsumer(){}

	virtual void BookCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider){}
	virtual void NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void RegularCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider){}
	virtual void TimeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref){}
	virtual void VolumeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref) {}
	void print();

private:
	int msecsFrom_;
	int msecsTo_;
	int interval_;
	int nbins_;
	std::vector<int> cntBook_;
	std::vector<int> cntNbbo_;
	std::vector<int> cntTrade_;
	std::vector<int> cntBookTrade_;
};

} // namespace gtlib

#endif
