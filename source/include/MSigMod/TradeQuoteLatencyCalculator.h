#ifndef __TradeQuoteLatencyCalculator__
#define __TradeQuoteLatencyCalculator__
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/QuoteSample.h>
#include <gtlib_signal/StatusChangeUS.h>
#include <jl_lib.h>
#include <jl_lib/jlutil.h>
#include <optionlibs/TickData.h>
#include <string>
#include <vector>

class TradeQuoteLatencyCalculator: public TickConsumer<TradeInfo,QuoteInfo,OrderData> {
public:
	int quoteSampleStep_;
	int tradeSampleStep_;
	int booktradeSampleStep_;
	bool doExploratory;

	TradeQuoteLatencyCalculator();
	TradeQuoteLatencyCalculator(int idate, const std::string& uid, const std::string& ticker,
		Sessions* sessions, int openMsecs, int closeMsecs);
	virtual ~TradeQuoteLatencyCalculator();
	void setStatusChange(gtlib::StatusChangeUS* p);
	void setOpenDelay(int delay);
	const std::vector<QuoteInfo>& getQuotes() const;
	void endTicker();
	void setDebug();

	virtual void NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void RegularCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider) {}
	virtual void TimeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref) {}
	virtual void VolumeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref) {}

private:
	bool debug_;
	bool sample_block_trade_;
	bool exclude_block_trade_;
	int openDelay_;
	int idate_;
	std::string uid_;
	std::string ticker_;
	int openMsecs_;
	int closeMsecs_;
	int n1sec_;
	int cnt_;
	int msecsLastEvent_;
	float curr_wgt_usd_; // local price * curr_wgt_usd_ = price in usd
	float curr_wgt_usd_local_; // price in usd * curr_wgt_usd_local_ = local price
	std::vector<QuoteInfo> quotes_;
	std::vector<TradeInfo> trades_;
	gtlib::StatusChangeUS* pStatusChange_;

	QuoteInfo firstQuote_;
	TradeInfo firstTrade_;
	QuoteInfo lastNbbo_;
	TradeInfo lastTrade_;
	TradeInfo bidTakingTrade_;
	TradeInfo askTakingTrade_;
	TradeInfo lastBookTrade_;

	Sessions* pSessions_;
	std::vector<int> vfi_;
	std::map<int, QuoteInfo> mCompBkTop_;

	void update_trade(int msecs);
	void reset_level_changing_trade(int msecs);
};

#endif
