#ifndef __MPrintTrades__
#define __MPrintTrades__
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/VariablesInfo.h>
#include <gtlib_fitting/FitData.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MPrintTrades: public MModuleBase {
public:
	MPrintTrades(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MPrintTrades();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	float thres_;
	float flatFee_;
	float omWgt_;
	float tmWgt_;
	std::string coefModel_;
	std::string fitDir_;
	std::string fitDesc_;
	std::string coefFitDir_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> sigModels_;
	std::vector<std::string> predModels_;
	std::vector<int> fitdates_;

	void print();
};

#endif

