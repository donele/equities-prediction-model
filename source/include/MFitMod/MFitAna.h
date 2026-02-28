#ifndef __MFitAna__
#define __MFitAna__
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/VariablesInfo.h>
#include <gtlib_fitting/FitData.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MFitAna: public MModuleBase {
public:
	MFitAna(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitAna();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool debug_ana_;
	bool debugPredAna_;
	bool volDepThres_;
	bool tradedTickersOnly_;
	bool doAna_;
	bool doAnaThres_;
	bool doAnaDay_;;
	bool doAnaBreak_;
	bool doAnaMaxPos_;
	bool doAnaNTrees_;
	bool doAnaQuantile_;
	bool doAnaQuantileTrade_;
	bool doAnaQuantileFeatures_;
	bool doAnaSprdQuantileUH_;
	char exch_;
	float flatFee_;
	float anaThres_;
	float anaBreak_;
	float anaMaxPos_;
	int nTargets_;
	int nQuantiles_;
	std::string coefModel_;
	std::string fitDir_;
	std::string fitDesc_;
	std::string coefFitDir_;
	std::vector<float> vThres_;
	std::vector<float> vWgts_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> sigModels_;
	std::vector<std::string> predModels_;
	std::vector<std::string> varAnaQuantile_;
	std::vector<int> fitdates_;

	void oosTest();
	void runAna();
	void runAnaNTrees();
	void runAnaQuantile();
	void runAnaQuantileUH();
	void runAnaQuantileTrade();
};

#endif

