#ifndef __GTA_NNtrader__
#define __GTA_NNtrader__
#include "GTModule.h"
#include "optionlibs/TickData.h"
#include "NN.h"
#include "NNobj.h"
#include "TH1.h"
#include "TGraph.h"
#include "TRandom3.h"
#include "TProfile.h"
#include <map>
#include <string>
#include <vector>

class __declspec(dllexport) GTA_NNtrader: public GTModule {
public:
	GTA_NNtrader(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~GTA_NNtrader();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void doTicker();
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

	NN* nn_;
	TRandom3 tr_;
	std::string nnFile_;

	std::vector<double> vDayErr_;
	std::vector<double> vEpochErr_;
	std::map<std::string, int> mPos_;

	int lotSize_;
	int iDay_;
	int nHidden_;
	int maxPos_;
	int maxPosChg_;
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

	NNperformance npError_;
	NNperformance npReturn0_;
	NNperformance npProfit0_;
	NNperformance npRpt0_;
	NNperformance npGpt0_;
	NNperformance npReturn_;
	NNperformance npProfit_;
	NNperformance npRpt_;
	NNperformance npGpt_;

	double day_error_;
	int day_npoints_;
	std::string trainObj_;

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
	void get_mid_series(std::vector<double>& midM, std::vector<double>& msecM,
		std::vector<double>* msecQ, std::vector<double>* midQ);
	void replace_zeros(std::vector<double>& series);
};

#endif