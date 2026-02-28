#ifndef __GTA_NNtrainer__
#define __GTA_NNtrainer__
#include "GTModule.h"
//#include "optionlibs/TickData.h"
#include "NN.h"
#include "NN_bpnn.h"
#include "NN_bptt.h"
#include "NNobj.h"
#include "TH1.h"
#include "TGraph.h"
#include "TProfile.h"
#include <map>
#include <string>
#include <vector>

class __declspec(dllexport) GTA_NNtrainer: public GTModule {
public:
	GTA_NNtrainer(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~GTA_NNtrainer();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void doTicker();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	NN* nn_;

	std::vector<std::vector<double> > vvWgts_;
	std::vector<double> vDayErr_;
	std::vector<double> vEpochErr_;
	std::map<std::string, int> mPos_;

	bool debug_;
	int iDay_;
	int nHidden_;
	int maxPos_;
	int lotSize_;
	double costMult_;
	double learn_;
	double maxWgt_;
	double poslim_;
	int verbose_;
	double biasOut_;

	NNperformance npError_;
	NNperformance npReturn0_;
	NNperformance npProfit0_;
	NNperformance npRpt0_;
	NNperformance npGpt0_;
	NNperformance npReturn_;
	NNperformance npProfit_;
	NNperformance npRpt_;
	NNperformance npGpt_;

	// BPTT.
	double sigP_;
	double n_P_;
	double sum_P_;
	double sum_P2_;
	double n_Pday_;
	double sum_Pday_;
	double sum_Pday2_;
	double nTradeDay_;
	double nShareDay_;
	bool trainingDetail_;

	int iEpoch_;
	bool noTwoTradesAtSamePrice_;
	bool bold_driver_;
	double bold_driver_scale_up_;
	double bold_driver_scale_down_;
	int nNoBold_;
	double epoch_rms_prev_;
	int nticker_trained_day_;
	int nticker_trained_market_;
	std::string trainObj_;

	// Common Plots.
	TH1F* houtput_;
	TProfile* prtn_;

	// Plots for maxProfitPos.
	TH1F* houtput0_;
	TProfile* prtn0_;
	TH1F* hreturn0_;
	TH1F* hprofit0_;
	TH1F* hreturn_;
	TH1F* hprofit_;
	std::map<std::string, std::vector<double> > mvMsecs_;
	std::map<std::string, std::vector<double> > mvPrice_;
	std::map<std::string, std::vector<double> > mvPosition_;
	std::map<std::string, std::vector<double> > mvNormPos_;
	std::map<std::string, std::vector<double> > mvNormPos0_;
	std::map<std::string, std::vector<double> > mvReturn0_;
	std::map<std::string, std::vector<double> > mvProfit0_;
	std::map<std::string, std::vector<double> > mvReturn_;
	std::map<std::string, std::vector<double> > mvProfit_;

	void doTicker_minPredErr();
	void doTicker_maxProfitPos();

	TGraph* make_graph(std::vector<double>& v);
	void plot_tickerDetail(std::string ticker, std::string var,
		std::vector<double> vMsecs, std::vector<double> v, bool cumulative = false);
};

#endif