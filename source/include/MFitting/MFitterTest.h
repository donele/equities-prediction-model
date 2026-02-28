#ifndef __MFitterTest__
#define __MFitterTest__
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/VariablesInfo.h>
#include <gtlib_fitting/FitData.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MFitterTest: public MModuleBase {
public:
	MFitterTest(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitterTest();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool writePred_;
	bool doAna_;
	bool doAnaSprdQuantile_;
	bool doAnaSprdQuantileOmTm_;
	std::string sigType_;
	std::string fitDesc_;
	std::string smodel_; // signal files are under this subdirectory.
	std::string fitDir_;
	boost::timer timer_;

	int nTrees_;
	float shrinkage_;
	float flatFee_;
	int minPtsNode_;
	int maxLeafNodes_;
	int maxTreeLevels_;
	gtlib::VariablesInfo varInfo_;
	gtlib::FitData* pFitData_;
	gtlib::BoostedDecisionTreeParam bdtParam_;
	std::vector<float> vSumTargetToday_;
	std::vector<float> vSumBmpredToday_;
	std::vector<int> fitdates_;

	// Constructor
	void initialize();
	std::string getSigModel();
	std::string getSigType();
	int getNTrees();

	// beginJob
	void readData();
	gtlib::DailyDataCount getDailyDataCount();
	int getNNewData(std::string path);
	void readData(int idate);
	void read_sig_file(int idate);
	void read_bm_pred(int idate);
	void calculate_fit_target(int idate);
	std::vector<int> get_index(hff::SignalLabel& sh, std::vector<std::string>& names);

	void printElapsed();
	void corr_input();
	void fit();
	std::string getCoefPath();
	void oosTest();
};

#endif
