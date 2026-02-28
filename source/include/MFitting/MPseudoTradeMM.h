#ifndef __MPseudoTradeMM__
#define __MPseudoTradeMM__
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

class CLASS_DECLSPEC MPseudoTradeMM: public MModuleBase {
public:
	MPseudoTradeMM(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPseudoTradeMM();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool weightedPnl_;
	double fee_;
	double currPos_;
	std::string predName_;
	std::string market_;
	std::string filename_;
	std::ofstream ofs_;

	std::vector<int> totPos_;

	double cumIntra_;
	double cumCost_;
	double cum1_;
	double cumBM_;
	double cum130_;
	double cumPred_;
	double cumMMtm_;
	double cumMMPred_;
	double cumClcl_;

	Acc accIntra_;
	Acc accCost_;
	Acc acc1_;
	Acc accBM_;
	Acc acc130_;
	Acc accPred_;
	Acc accMMtm_;
	Acc accMMPred_;
	Acc accClcl_;

	Acc accTotal_;

	std::map<std::string, int> mPos_;
};

#endif
