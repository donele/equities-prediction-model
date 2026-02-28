#ifndef __MFitNtrees__
#define __MFitNtrees__
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/VariablesInfo.h>
#include <gtlib_fitting/FitData.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MFitNtrees: public MModuleBase {
public:
	MFitNtrees(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitNtrees();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool write_;
	int nTargets_;
	int default_;
	float minSprd_;
	float maxSprd_;
	std::string fitDir_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> sigModels_;
	std::vector<std::string> predModels_;
	std::vector<int> fitdates_;

	void oosTest();
	int getBestNtree(const std::vector<std::pair<int, float>>& vp);
	int interpolateBestNtree(int prevNtree, int ntree, float prevBias, float bias);
	void writeDatabase(int nTrees);
};

#endif

