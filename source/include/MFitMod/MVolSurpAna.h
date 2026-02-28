#ifndef __MVolSurpAna__
#define __MVolSurpAna__
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/VariablesInfo.h>
#include <gtlib_fitting/FitData.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
#include <gtlib_predana/PredAnaByQuantileTradeLastBin.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MVolSurpAna: public MModuleBase {
public:
	MVolSurpAna(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MVolSurpAna();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool debug_ana_;
	bool volDepThres_;
	bool tradedTickersOnly_;
	float flatFee_;
	float thres_;
	int nTargets_;
	int nQuantiles_;
	std::string coefModel_;
	std::string fitDir_;
	std::string fitDesc_;
	std::string coefFitDir_;
	std::vector<float> vWgts_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> sigModels_;
	std::vector<std::string> predModels_;
	std::string varAnaQuantile_;
	std::vector<int> fitdates_;
	gtlib::PredAnaByQuantileTradeLastBin* pbq_;

	void runAnaQuantileTrade();
};

#endif

