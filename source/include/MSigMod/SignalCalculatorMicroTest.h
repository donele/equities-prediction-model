#ifndef __SignalCalculatorMicroTest__
#define __SignalCalculatorMicroTest__
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/QuoteSample.h>
#include <gtlib_signal/StatusChangeUS.h>
#include <jl_lib.h>
#include <jl_lib/jlutil.h>
#include <optionlibs/TickData.h>
#include <string>
#include <vector>

class SignalCalculatorMicroTest: public TCS_book_micro {
public:
	int quoteSampleStep_;
	int tradeSampleStep_;
	int booktradeSampleStep_;

	SignalCalculatorMicroTest();
	SignalCalculatorMicroTest(int idate, const std::string& uid, const std::string& ticker, hff::SigC& sig,
			Sessions* sessions, int openMsecs, int closeMsecs, const std::string& auctionMkts,
			const std::vector<hff::OrderQty>* tradeQty);
	virtual ~SignalCalculatorMicroTest();
	void setDiscardOutliers(bool tf);
	void setStatusChange(gtlib::StatusChangeUS* p);
	void setOpenDelay(int delay);
	const std::vector<QuoteInfo>& getQuotes() const;
	const TradeInfo& getFirstTrade() const;
	const QuoteInfo& getFirstQuote() const;
	void endTicker();
	void returnFromTickprovider();
	void setDebug();
	void calculate_targets(hff::SigC& sig, const std::vector<QuoteInfo>& quotes, bool smoothTarget=false);
	void setTmSampleMsecs(const std::vector<int>& sampleMsecs, const int scale);
	void setQuoteSampleStep(int step);
	void setTradeSampleStep(int step);
	void setBooktradeSampleStep(int step);
	void setPersistence(int iSig);

	virtual void NbboCB(UsecsTime ti,const QuoteDataMicro& qdm);
	virtual void TradeCB(UsecsTime ti,const TradeDataMicro& trade);
	virtual void BookTradeCB(UsecsTime ti,const TradeDataMicro& trade);
	virtual void RegularCB(UsecsTime ti);

	class TaskPersistence :public TickConsumerTask<UsecsTime>
	{
	public:
		TaskPersistence(SignalCalculatorMicroTest* sigcal, int iSig);
		virtual TaskPersistence* MakeCopy(void);
		virtual void Execute(UsecsTime ti);
	private:
		int iSig_;
		SignalCalculatorMicroTest* sigcal_;
	};

private:
	bool debug_;
	bool discardOutliers_;
	int openDelay_;
	int idate_;
	std::string uid_;
	std::string ticker_;
	int openMsecs_;
	int closeMsecs_;
	int n1sec_;
	int cnt_;
	int msecsLastEvent_;
	int validt_;
	float lastFillImb_;
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

	hff::SigC* psig_;
	const hff::SigH* psigh_;
	Sessions* pSessions_;
	std::vector<int> vfi_;
	std::map<int, TradeInfo> mTrades_;
	std::map<int, QuoteInfo> mCompBkTop_;

	void addQuote(const QuoteInfo& quote);
	void addTrade(const TradeInfo& trade);
	void addBookTrade(const TradeInfo& trade);
	double get_mret_upto1s(int msecs, int length_msec, int firstQtMsec, double firstMidQt, double mqut, int lag = 0);
	bool NewSig(int msecs, int sampleType);
	void calculate_trade_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	void calculate_quote_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	void calculate_fillimb_signals(hff::StateInfo& sI, int msecs, int sampleType);
	void calculate_tob_signals(hff::StateInfo& sI);
	void calculate_book_signals(hff::SigC& sig, hff::StateInfo& sI);
	void calculate_bookcnt_signals(hff::SigC& sig, hff::StateInfo& sI);
	void calculate_mi_signals(hff::SigC& sig, hff::StateInfo& sI);
	void calculate_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	float get_fillImb(const QuoteInfo& quote, const TradeInfo& trade);
	float get_fillImb();
	void get_top_comp_book(QuoteInfo& compBkTop);
};

#endif
