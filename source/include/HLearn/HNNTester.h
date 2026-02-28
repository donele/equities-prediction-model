#ifndef __HNNTester__
#define __HNNTester__
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

class CLASS_DECLSPEC HNNTester: public HModule {
public:
	HNNTester(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HNNTester();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int verbose_;
	std::string trainObj_;
	int targetIndx_;
	int predIndx_;

	int nTickers_;
	std::string nnFile_;
	hnn::NNBase* nn_;
	boost::mutex acc_mutex_;
	boost::mutex corr_mutex_;

	// process_error()
	std::vector<double> vDayErr_;
	std::map<double, int> mDecileNNout_;
	Acc accError_;
	Acc accErrorDay_;
	TH1F houtput_;
	TProfile prtn_;
	Acc accX_[10];
	Acc accY_[10];

	// process_corr()
	Corr corrRef_;
	Corr corrTest_;
	std::vector<double> vCorrRef_;
	std::vector<double> vCorrTest_;

	// process_eval()
	TH1F hpred_ref_;
	TH1F hpred_test_;
	std::map<double, int> mPercentilePredRef_;
	std::map<double, int> mPercentilePredTest_;
	EconVal econValRef_;
	EconVal econValTest_;
	std::vector<double> vEconValRef_;
	std::vector<double> vEconValTest_;
	std::vector<double> vBiasRef_;
	std::vector<double> vBiasTest_;

	void process_error(double error, double nnOutput, double trainTarget);
	void process_corr(double specTarget, double specPred, double pred);
	void process_eval(double specTarget, double specPred, double pred);

	TGraph make_graph(std::vector<double>& v);
	TGraph make_graph_ma(std::vector<double>& v, int n);
	TGraph make_graph_diff(TGraph& grRef, TGraph& grTest);
	TGraph make_graph_absrat(TGraph& grRef, TGraph& grTest);
	void fillQuantileTable(std::map<double, int>& mQuantile, TH1F& hist, int nq);
};

#endif