#ifndef __MPseudoTradePred__
#define __MPseudoTradePred__
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <MFitting/MPseudoTradePrep.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

// 20131213: Seems to be created to do rollingModelDate, supporting one prediction only.

class CLASS_DECLSPEC MPseudoTradePred: public MModuleBase {
public:
	MPseudoTradePred(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradePred();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int verbose_;
	int maxShrChg_;
	bool weightedPnl_;
	bool rollingModelDate_;
	double thres_;
	double fee_;
	double maxPos_;
	double maxPosTicker_;
	double currPos_;
	std::string dir_;
	std::string predName_;
	std::string market_;
	std::string filename_;
	std::string desc_;
	std::ofstream ofs_;

	Acc accIntra130_;
	Acc accIntra_;
	Acc accClcl_;
	Acc accClop_;
	Acc accPred_;
	Acc accNtrd_;
	Acc accDV_;

	Acc accTotal_;

	std::map<std::string, int> mPos_;

	void endDay_new();
	void get_pred_sprd(std::vector<double>& vPred, std::vector<double>& vSprd);
};

#endif
