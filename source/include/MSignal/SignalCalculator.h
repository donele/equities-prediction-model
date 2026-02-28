#ifndef __SignalCalculator__
#define __SignalCalculator__
#include <gtlib_model/hff.h>
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

class SignalCalculator: public TickConsumer<TradeInfo,QuoteInfo,OrderData> {
public:
	double regularSampleProbability;
	double quoteSampleProbability;
	double tradeSampleProbability;
	double booktradeSampleProbability;

	SignalCalculator();
	SignalCalculator(const std::string& ticker, hff::SigC& sig, const std::vector<QuoteInfo>* quotes, const std::vector<TradeInfo>* trades,
		MSessions* sessions, int openMsecs, int closeMsecs, int exploratoryDelay, const std::vector<QuoteInfo>* sip);
	virtual ~SignalCalculator();
	void returnFromTickprovider();
	void set_news(const std::vector<QuoteInfo>* news);
	void debug_fillImbBook(TickSources& ts, const std::vector<std::string>& markets, int idate, const std::string& ticker);
	void debug_booktrade_prob();

	virtual void NbboCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void BookTradeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void RegularCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	virtual void TimeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref);
	virtual void VolumeCB(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, void* ref) {}

private:
	bool debug_fillImbBook_;
	bool debug_booktrade_prob_;
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
	float fVolM600_;
	float fVolM3600_;
	float ret_on_;
	std::vector<int> vTindx1s_;
	std::vector<int> vQindx1s_;
	std::vector<int> vSindx1s_;
	std::vector<double> vMid1s_;
	std::vector<double> trdVolu1sCum_;
	std::vector<double> sumTrdPrc1sCum_;
	std::vector<double> nTrds1sCum_;
	std::vector<int> vMaxBidSize1s_;
	std::vector<int> vMaxAskSize1s_;

	QuoteInfo firstQuote_;
	QuoteInfo lastNbbo_;
	TradeInfo lastTrade_;
	TradeInfo lastBookTrade_;

	hff::SigC* psig_;
	MSessions* pSessions_;
	const std::vector<QuoteInfo>* quotes_;
	const std::vector<TradeInfo>* trades_;
	const std::vector<QuoteInfo>* news_;
	const std::vector<QuoteInfo>* sip_;
	const hff::SigH* psigh_;
	std::map<std::string, std::vector<QuoteInfo> > mTob_;
	std::vector<int> vfi_;
	std::map<int, TradeInfo> mTrades_;
	std::map<int, QuoteInfo> mCompBkTop_;

	double get_mret_upto1s(int msecs, int length_msec, int firstQtMsec, double firstMidQt, double mqut,
		TickProvider<TradeInfo, QuoteInfo, OrderData>* provider, int lag = 0);
	bool NewSig(int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType);
	void calculate_trade_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	void calculate_quote_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs, TickProvider<TradeInfo, QuoteInfo, OrderData>* provider);
	void calculate_fillimb_signals(hff::StateInfo& sI, int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType);
	void calculate_exploratory_signals(hff::StateInfo& sI, int msecs, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType);
	void calculate_targets(hff::SigC& sig, hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_tob_signals(hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_book_signals(hff::SigC& sig, hff::StateInfo& sI, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void calculate_news_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	void calculate_signals(hff::SigC& sig, hff::StateInfo& sI, int msecs);
	float get_fillImb(const QuoteInfo& quote, const TradeInfo& trade);
	float get_fillImb(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void get_top_comp_book(QuoteInfo& compBkTop, TickProvider<TradeInfo,QuoteInfo,OrderData>* provider);
	void get_dsOL(TickProvider<TradeInfo,QuoteInfo,OrderData>* provider, QuoteInfo& quoteFrom, double& SaOL, double& SbOL);
};

#endif
