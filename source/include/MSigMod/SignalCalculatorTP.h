#ifndef __SignalCalculatorTP__
#define __SignalCalculatorTP__
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/QuoteSample.h>
#include <gtlib_signal/StatusChangeUS.h>
#include <jl_lib.h>
#include <jl_lib/jlutil.h>
#include <optionlibs/TickData.h>
#include <string>
#include <vector>

class SignalCalculatorTP: public TickConsumer<TradeInfo,QuoteInfo,OrderData> {
public:
	int quoteSampleStep_;
	int tradeSampleStep_;
	int booktradeSampleStep_;
	bool doExploratory;

	SignalCalculatorTP();
	SignalCalculatorTP(int idate, const std::string& uid, const std::string& ticker, hff::SigC& sig,
		Sessions* sessions, int openMsecs, int closeMsecs, const std::vector<hff::OrderQty>* tradeQty, int exploratoryDelay);
	virtual ~SignalCalculatorTP();
	void setDiscardOutliers(bool tf);
	void setDropBadTarget(bool tf);
	void requireTradeAfterFirstQuote(bool tf);
	void setRtrdSimpleDef(bool tf);
	void setStatusChange(gtlib::StatusChangeUS* p);
	void setOpenDelay(int delay);
	void doPersistAna();
	void samplePendingUpdate(bool tf);
	const std::vector<QuoteInfo>& getQuotes() const;
	const TradeInfo& getFirstTrade() const;
	const QuoteInfo& getFirstQuote() const;
	void endTicker();
	void returnFromTickprovider();
	void setDebug();
	void koreanTevtFilter(hff::SigC& sig, bool allowEarlierQuote);
	void calculate_targets(hff::SigC& sig, const std::vector<QuoteInfo>& quotes, bool smoothTarget=false);
	void calculate_targets_next(hff::SigC& sig, const std::vector<QuoteInfo>& quotesNext);
	void calculate_exchange_vol(hff::SigC& sig);
	void setTmSampleMsecs(const std::vector<int>& sampleMsecs, const int scale);
	void setQuoteSampleStep(int step);
	void setTradeSampleStep(int step);
	void setBooktradeSampleStep(int step);
	void sampleBlockTrade();
	void excludeBlockTrade();

	virtual void NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void RegularCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TimeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref);
	virtual void VolumeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref) {}

private:
	bool debug_;
	bool discardOutliers_;
	bool dropBadTarget_;
	bool sample_block_trade_;
	bool invalid_trade_appeared_;
	bool exclude_block_trade_;
	bool persistAna_;
	bool quoteUpdatePending_;
	bool doSamplePendingUpdate_;
	bool requireTradeAfterFirstQuote_;
	bool rtrd_simple_def_;
	int openDelay_;
	int debugNBookTrade_;
	int debugNAllTaken_;
	int idate_;
	std::string uid_;
	std::string ticker_;
	int openMsecs_;
	int closeMsecs_;
	int exploratoryDelay_;
	int n1sec_;
	int cnt_;
	int msecsLastEvent_;
	int validt_;
	float curr_wgt_usd_; // local price * curr_wgt_usd_ = price in usd
	float curr_wgt_usd_local_; // price in usd * curr_wgt_usd_local_ = local price
	float lastFillImb_;
	float lastFillImbOld_;
	float high_;
	float low_;
	float high900_;
	float low900_;
	float f600_;
	float f3600_;
	float miNormFac_;
	float ret_on_;
	float adjPrevClose_;
	const std::vector<hff::OrderQty>* tradeQty_;
	std::vector<QuoteInfo> quotes_;
	gtlib::QuoteSample qSample_;
	std::vector<TradeInfo> trades_;
	std::vector<int> miTradeVolCum_;
	std::set<int> tmSampleMsecs_;
	gtlib::StatusChangeUS* pStatusChange_;
	Acc accTrd_;
	SampleCounter nbboCounter_;
	SampleCounter tradeCounter_;
	SampleCounter booktradeCounter_;

	QuoteInfo firstQuote_;
	TradeInfo firstTrade_;
	QuoteInfo lastNbbo_;
	QuoteInfo lastNbboWithLessTime_;
	QuoteInfo nbboBeforeLastBookTrade_;
	QuoteInfo nbboBeforeLastBookTradeWithLessTime_;
	TradeInfo lastTrade_;
	TradeInfo lastBookTrade_;
	TradeInfo lastBookTradeBeforeNbbo_;

	hff::SigC* psig_;
	const hff::SigH* psigh_;
	Sessions* pSessions_;
	std::vector<int> vfi_;
	std::map<int, TradeInfo> mTrades_;
	std::map<int, QuoteInfo> mCompBkTop_;
	std::map<int, float> mBlockTrade_;

	void addQuote(const QuoteInfo& quote);
	void addTrade(const TradeInfo& trade);
	void addBookTrade(const TradeInfo& trade);
	double get_mret_upto1s(int msecs, int length_msec, int firstQtMsec, double firstMidQt, double mqut, int lag = 0);
	double get_mret_upto1s(int msecs, int length_msec, int firstQtMsec, double firstMidQt, double mqut,
		TickProvider<TradeInfo, QuoteInfo, OrderData>* provider, int lag = 0);
	bool NewSig(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType);
	void calculate_trade_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	void calculate_quote_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs, TickProvider<TradeInfo, QuoteInfo, OrderData>* provider);
	void calculate_fillimb_signals(hff::StateInfo& sI, int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType);
	void calculate_exploratory_signals(hff::StateInfo& sI, int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType);
	void calculate_tob_signals(hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_book_signals(hff::SigC& sig, hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_atf_signals(hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_bookcnt_signals(hff::SigC& sig, hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_mi_signals(hff::SigC& sig, hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	float get_fillImb(const QuoteInfo& quote, const TradeInfo& trade);
	float get_fillImb(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void get_top_comp_book(QuoteInfo& compBkTop, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void get_dsOL(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, QuoteInfo& quoteFrom, double& SaOL, double& SbOL);
	bool allTaken(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	bool isBlockTrade(const TradeInfo& trade, bool record);
};

#endif
