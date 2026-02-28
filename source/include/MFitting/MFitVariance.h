#ifndef __MFitVariance__
#define __MFitVariance__
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>
#include <MFitting/HFTrees.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MFitVariance: public MModuleBase {
public:
	MFitVariance(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitVariance();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int nInput_;
	int nSpectator_;
	int cntFitData_;
	int cntOOSData_;
	std::string sigType_;
	bool do_fit_;
	bool do_analyze_;
	bool read_multithread_;

	std::string targetName_;
	std::vector<std::string> inputNames_;
	std::vector<std::string> spectatorNames_;
	int targetIndx_;
	std::vector<int> inputIndx_;
	std::vector<int> spectatorIndx_;

	std::vector<std::vector<float> > vvInputTarget_;
	std::vector<std::vector<float> > vvInputTargetOOS_;
	std::vector<std::vector<float> > vvSpectatorOOS_;
	std::vector<float> vPred_;
	std::vector<float> vPredOOS_;
	std::map<int, std::pair<int, int> > fitOffset_;
	std::map<int, std::pair<int, int> > oosOffset_;

	//std::string statPath_;
	//std::string predPath_;
	//std::string sigDir_;
	//std::string txtDir_;
	//std::string predDir_;

	// TreeBoost
	int nTrees_;
	int minPtsNode_;
	int maxLevels_;
	int monitor_;
	int nthreads_;
	int cattar_;
	int maxNodes_;
  	double shrinkage_;
	int treeMaxLevels_;
	int marketNumber_;
	int modelNumber_;
	int zmin_;
	int zmax_;
	int treeStep_;
	int writeDataAux_;
	int addBMtoTarget_;
	int isErrorCor_;
	std::string baseDir_;
	std::string fitDir_;
	std::string predDir_;

	void get_names(hff::SignalLabel& sh);
	void read_pred(int idate);
	void read_data(int idate);
	void loop_dates_thread(int iThread);
	void link_fit_data(MATRIXSHORT& data, std::vector<std::vector<float> >& vv, int offset1 = 0, int offset2 = 0);
	void sort_input(std::vector<std::vector<float> >& vv);
	void fit();
	void analyze();
};

#endif
