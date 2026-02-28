#ifndef __gtlib_tickdump_DumpNbboConsumer__
#define __gtlib_tickdump_DumpNbboConsumer__
#include <vector>
#include <string>
#include <optionlibs/TickData.h>

namespace gtlib {

class DumpNbboConsumer : public TickConsumer<TradeInfo,QuoteInfo,OrderData> {
public:
	DumpNbboConsumer(){}
	DumpNbboConsumer(int nPrint);
	virtual ~DumpNbboConsumer(){}

	virtual void BookCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider){}
	virtual void NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider){}
	virtual void BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider){}
	virtual void RegularCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider){}
	virtual void TimeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref){}
	virtual void VolumeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref) {}

private:
	int nPrint_;
	int cnt_;
};

} // namespace gtlib

#endif
