#ifndef __gtlib_signal_QuoteView__
#define __gtlib_signal_QuoteView__
#include <optionlibs/TickData.h>
#include <jl_lib/Sessions.h>
#include <vector>

namespace gtlib {

class QuoteView {
public:
	QuoteView(){}
	QuoteView(const std::vector<QuoteInfo>* quotes, const std::vector<TradeInfo>* trades, const Sessions& session,
			int openMsecs, int closeMsecs, int interval, bool reqVal, bool fillZeros, bool allowCross);
	void addQuote(const QuoteInfo& quote);
	void addTrade(const TradeInfo& trade);
	void endTicker();
	float getMid(int msecs) const;
	float getMidExact(int msecs) const;
	float getHiLo(int msecs);
	float getDv(int msecs);
	float getShare(int msecs);
	int getShare(int msecs, int lagSec);
private:
	bool reqVal_;
	bool fillZeros_;
	bool allowCross_;
	int interval_;
	int openMsecs_;
	int closeMsecs_;
	int nSample_;
	int prevIndex_;
	const std::vector<QuoteInfo>* quotes_;
	const std::vector<TradeInfo>* trades_;

	std::vector<float> vMid_;
	std::vector<int> vShare_;
	int nTrades_;
	int tIndx_;
	int curMsecs_;
	float timeFrac_;
	float high_;
	float low_;
	float dv_;
	float share_;
	void advanceTrade(int msecs);
};

} // namespace gtlib

#endif
