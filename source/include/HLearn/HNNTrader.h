#ifndef __HNNTrader__
#define __HNNTrader__
#include <HLib/HModule.h>
#include <HLearnObj/NNBase.h>
#include <boost/random.hpp>
#include "optionlibs/TickData.h"
#include "TH1.h"
#include "TGraph.h"
#include "TProfile.h"
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HNNTrader: public HModule {
public:
	HNNTrader(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNNTrader();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	struct TickerDaySumm {
		TickerDaySumm():sumR0(0), sumR(0), sumP0(0), sumP(0){}
		std::vector<double> msec_series;
		std::vector<double> sign_series;
		std::vector<double> prc_series;
		std::vector<double> vol_series;
		double sumR0;
		double sumR;
		double sumP0;
		double sumP;
	};

	hnn::NNBase* nn_;
	std::string nnFile_;
	boost::mutex acc_mutex_;
	boost::mutex gen_mutex_;
	boost::mutex mpos_mutex_;

	std::vector<double> vDayErr_;
	std::map<std::string, int> mPos_;

	int lotSize_;
	int iDay_;
	int nHidden_;
	int maxPos_;
	int maxPosChg_;
	int maxShrPosChg_;
	double predMin_;
	double predMax_;
	double costMult_;
	double tradeInterval_; // mseconds
	bool costMultForced_;
	bool noTwoTradesAtSamePrice_;
	double thres_;
	double restore_;
	double randomTradeFreq_;
	int verbose_;
	int useBias_;
	int maxNTradesTickerDay_;

	Acc accError_;
	Acc accErrorDay_;

	Acc accDV_;
	Acc accReturn_;
	Acc accProfit_;
	Acc accReturn0_;
	Acc accProfit0_;

	Acc accDVDay_;
	Acc accReturnDay_;
	Acc accProfitDay_;
	Acc accReturn0Day_;
	Acc accProfit0Day_;

	double day_error_;
	int day_npoints_;
	std::string trainObj_;
	boost::mt19937 gen_;
	boost::variate_generator<boost::mt19937&, boost::uniform_real<> > variateGen_;

	TH1F hPredBuy_;
	TH1F hPredSell_;
	TH1F hTickerPosDay_;
	TH1F houtput_;
	TProfile prtn_;
	TH1F houtput0_;
	TProfile prtn0_;
	TH1F hreturn0_;
	TH1F hprofit0_;
	TH1F hreturn_;
	TH1F hprofit_;
	std::map<std::string, std::vector<double> > mvMsecs_;
	std::map<std::string, std::vector<double> > mvPrice_;
	std::map<std::string, std::vector<double> > mvPosition_;
	std::map<std::string, std::vector<double> > mvNormPos_;
	std::map<std::string, std::vector<double> > mvNormPos0_;
	std::map<std::string, std::vector<double> > mvReturn_;
	std::map<std::string, std::vector<double> > mvProfit_;
	std::map<std::string, std::vector<double> > mvReturn0_;
	std::map<std::string, std::vector<double> > mvProfit0_;

	void doTrade(QuoteInfo& quote, double deltaPos, TickerDaySumm& tds);
	TGraph make_graph(std::vector<double>& v);
	void plot_tickerDetail(std::string ticker, std::string var,
		std::vector<double> vMsecs, std::vector<double> v, bool cumulative = false);
	void get_mid_series(std::vector<double>& midM, const std::vector<double>& msecM,
		const std::vector<double>* msecQ, const std::vector<double>* midQ);
	void replace_zeros(std::vector<double>& series);
};

#endif