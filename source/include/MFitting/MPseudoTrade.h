#ifndef __MPseudoTrade__
#define __MPseudoTrade__
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <MFitting/MPseudoTradePrep.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MPseudoTrade: public MModuleBase {
public:
	MPseudoTrade(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTrade();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool resetPos_;
	int verbose_;
	int maxShrChg_;
	bool weightedPnl_;
	bool addON_;
	double ONmult_;
	int minMsecsON_;
	int maxMsecsON_;
	double thres_;
	//double fee_;
	double maxPos_;
	double maxPosTicker_;
	double currPos_;
	std::string omTarName_;
	std::string sigModel_;
	std::string dir_;
	std::string predName_;
	std::string market_;
	std::string filename_;
	std::string om_desc_;
	std::ofstream ofs_;
	std::set<char> dest_;

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
