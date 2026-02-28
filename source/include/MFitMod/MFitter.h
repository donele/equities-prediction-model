#ifndef __MFitter__
#define __MFitter__
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/VariablesInfo.h>
#include <gtlib_fitting/FitData.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MFitter: public MModuleBase {
public:
	MFitter(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitter();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool prune_;
	bool wgt_;
	bool wgtTradable_;
	bool wgtTradableConstWgt_;
	bool wgtTradableNaturalUnit_;
	bool wgtTradableNaturalToCost_;
	bool wgtTradableNaturalToNegCost_;
	bool debugWgtTradable_;
	bool doCorr_;
	bool costWgtTar_;
	std::vector<std::string> forceCutInput_;
	std::string sigType_;
	std::string fitDesc_;
	std::string fitDir_;
	std::string coefModel_;
	std::string coefDir_;
	std::string modelSource_;

	int minTreeWgt_;
	float wgtFacLimit_;
	float wgtTradableMaxPredAdjApplyCut_;
	float wgtTradableMaxSprdApplyCut_;
	float wgtTradableMaxWgt_;
	float reduction_;
	int nForceCutLevels_;
	int forceCutTreeLower_;
	int forceCutTreeUpper_;
	int nTrees_;
	float shrinkage_;
	int minPtsNode_;
	int maxLeafNodes_;
	int maxTreeLevels_;
	float maxMu_;
	float decayFactor_;
	gtlib::FitData* pFitData_;
	gtlib::BoostedDecisionTreeParam bdtParam_;

	// Constructor
	void initialize();
	int getNTrees();

	// beginJob
	void corrInput();
	void fit();
	std::string getCoefPath();
	void writePred();
};

#endif
