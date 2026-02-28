#ifndef __SignalCalculatorTest__
#define __SignalCalculatorTest__
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class SignalCalculatorTest: public TickConsumer<TradeInfo,QuoteInfo,OrderData> {
public:
	double regularSampleProbability;
	double quoteSampleProbability;
	double tradeSampleProbability;
	double booktradeSampleProbability;

	SignalCalculatorTest();
	SignalCalculatorTest(const std::string& uid, const std::string& ticker, hff::SigC& sig,
		Sessions* sessions, int openMsecs, int closeMsecs, const std::vector<hff::OrderQty>* tradeQty, int exploratoryDelay);
	virtual ~SignalCalculatorTest();
	const std::vector<QuoteInfo>& getQuotes() const;
	const TradeInfo& getFirstTrade() const;
	const QuoteInfo& getFirstQuote() const;
	void endTicker();
	void returnFromTickprovider();
	void debug_fillImbBook(TickSources& ts, const std::vector<std::string>& markets, int idate, const std::string& ticker);
	void debug_booktrade_prob();
	void debug_rz_signal();
	void calculate_targets(hff::SigC& sig, TCM_classic& tcmc);
	void calculate_targets(hff::SigC& sig, const std::vector<QuoteInfo>& quotes);

	virtual void NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void RegularCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TimeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref);
	virtual void VolumeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref) {}

private:
	bool debug_fillImbBook_;
	bool debug_booktrade_prob_;
	std::string uid_;
	std::string ticker_;
	int openMsecs_;
	int closeMsecs_;
	int exploratoryDelay_;
	int n1sec_;
	int cnt_;
	int msecsLastEvent_;
	int validt_;
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

	QuoteInfo firstQuote_;
	TradeInfo firstTrade_;
	QuoteInfo lastNbbo_;
	TradeInfo lastTrade_;
	TradeInfo lastBookTrade_;

	hff::SigC* psig_;
	const hff::SigH* psigh_;
	Sessions* pSessions_;
	std::map<std::string, std::vector<QuoteInfo> > mTob_;
	std::vector<int> vfi_;
	std::map<int, TradeInfo> mTrades_;
	std::map<int, QuoteInfo> mCompBkTop_;

	void addQuote(const QuoteInfo& quote);
	void addTrade(const TradeInfo& trade);
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
	void calculate_mi_signals(hff::SigC& sig, hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	float get_fillImb(const QuoteInfo& quote, const TradeInfo& trade);
	float get_fillImb(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void get_top_comp_book(QuoteInfo& compBkTop, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void get_dsOL(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, QuoteInfo& quoteFrom, double& SaOL, double& SbOL);
};

#endif
