#ifndef __HNNTrainer__
#define __HNNTrainer__
#include "HLib/HModule.h"
#include <string>
#include <map>
#include <HLearnObj/NNBase.h>
#include <jl_lib/jlutil.h>
#include "TH1.h"
#include "TGraph.h"
#include "TProfile.h"
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HNNTrainer: public HModule {
public:
	HNNTrainer(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNNTrainer();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	hnn::NNBase* nn_;
	std::vector<std::vector<double> > vvWgts_;
	std::vector<double> vDayErr_;
	std::vector<double> vEpochErr_;
	std::map<std::string, int> mPos_;
	boost::mutex nn_mutex_;
	boost::mutex acc_mutex_;
	boost::mutex mv_mutex_;
	boost::mutex mpos_mutex_;

	bool doClone_;
	std::string updateFreq_; // input, ticker, day, epoch.
	std::string monFreq_;
	std::string trainObj_;
	std::string nnFile_;
	int verbose_;
	int nHidden_;
	int maxPos_;
	int maxPosChg_;
	int lotSize_;
	double costMult_;
	double maxWgt_;
	double poslim_;
	double biasOut_;
	double sigP_;
	bool noTwoTradesAtSamePrice_;
	int earlyEpoch_;
	bool bold_driver_;
	double bold_driver_scale_up_;
	double bold_driver_scale_down_;

	int iDay_;
	int iEpoch_;
	double learn_;
	double wgtDecay_;
	double nTradeDay_;
	double nShareDay_;
	double epoch_rms_prev_;
	int nticker_trained_day_;
	int nticker_trained_market_;
	bool trainingDetail_;

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

	Acc accP_;
	Acc accPDay_;

	// Common Plots.
	TH1F* houtput_;
	TProfile* prtn_;

	// Plots for maxProfit.
	TH1F* houtput0_;
	TProfile* prtn0_;
	TH1F* hreturn_;
	TH1F* hprofit_;
	TH1F* hreturn0_;
	TH1F* hprofit0_;
	std::map<std::string, std::vector<double> > mvMsecs_;
	std::map<std::string, std::vector<double> > mvPrice_;
	std::map<std::string, std::vector<double> > mvPosition_;
	std::map<std::string, std::vector<double> > mvNormPos_;
	std::map<std::string, std::vector<double> > mvNormPos0_;
	std::map<std::string, std::vector<double> > mvReturn0_;
	std::map<std::string, std::vector<double> > mvProfit0_;
	std::map<std::string, std::vector<double> > mvReturn_;
	std::map<std::string, std::vector<double> > mvProfit_;

	//
	TH1F hTarget_;
	TH1F hnnOut1_;
	TH1F hnnOut2_;

	void doTicker_minErr(std::string ticker, hnn::NNBase* nnClone);
	void doTicker_maxProfit(std::string ticker, hnn::NNBase* nnClone);
	TGraph* make_graph(std::vector<double>& v);
	void plot_tickerDetail(std::string ticker, std::string var,
		std::vector<double> vMsecs, std::vector<double> v, bool cumulative = false);
	double get_performance();
	void print_epoch();
	void record_weights();
};

#endif