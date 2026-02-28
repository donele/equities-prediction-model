#ifndef __GTA_Btrainer__
#define __GTA_Btrainer__
#include "GTModule.h"
#include "optionlibs/TickData.h"
#include "GenBPNN.h"
#include "Butil.h"
#include "BPTT.h"
#include "TH1.h"
#include "TGraph.h"
#include "TProfile.h"
#include <map>
#include <string>
#include <vector>

class __declspec(dllexport) GTA_Btrainer: public GTModule {
public:
	GTA_Btrainer(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~GTA_Btrainer();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void doTicker();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	GenBPNN bpnn_;
	BPTT bptt_;

	std::vector<std::vector<double> > vvWgts_;
	std::vector<double> vDayErr_;
	std::vector<double> vEpochErr_;
	std::map<std::string, int> mPos_;
	std::vector<Bsample>* sample_;

	int iDay_;
	int iModel_;
	int nInput_;
	int nOutput_;
	int nHidden_;
	int maxPos_;
	double cost_;
	double learn_;
	double gamma_;
	int rewardTime_;
	int lenSeries_;
	double thres_;
	double thresIncr_;
	double maxWgt_;
	int verbose_;
	int useBias_;
	int iEpoch_;
	bool writeRoot_;
	bool bold_driver_;
	double epoch_error_;
	double epoch_rms_prev_;
	double day_error_;
	int day_npoints_;
	int epoch_npoints_;
	int nticker_trained_day_;
	int nticker_trained_market_;
	std::string updateFreq_;
	std::string trainObj_;

	// Common Plots.
	TH1F* houtput_;
	TProfile* prtn_;

	// Plots for maxProfitPos.

	int FtoInt(double F);
	int FtoInt(double F, QuoteInfo& quote);
	TGraph* make_graph(std::vector<double>& v);
	void doTicker_minPredErr();
	void doTicker_minErrTD();
	void doTicker_maxProfit();
	void doTicker_maxProfitPos();
};

#endif