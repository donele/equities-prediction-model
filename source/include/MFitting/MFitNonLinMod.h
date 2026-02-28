#ifndef __MFitNonLinMod__
#define __MFitNonLinMod__
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MFitNonLinMod: public MModuleBase {
public:
	virtual ~MFitNonLinMod();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	MFitNonLinMod(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	bool debug_;
	bool debug_corr_ticker_;
	int nInput_;
	int nSpectator_;
	int nTargets_;
	int cntFitData_;
	int cntOOSData_;
	std::string sigType_;
	std::string fitDesc_;
	std::string smodel_; // signal files are under this subdirectory.
	std::string coefModel_;
	bool do_corr_;
	bool do_fit_;
	bool do_fit_old_;
	bool do_analyze_;
	bool do_analyze_ins_;
	bool read_multithread_;
	bool rollingModelDate_;
	boost::timer timer_;

	std::string targetName_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> inputNames_;
	std::vector<std::string> zeroInputs_;
	std::vector<double> targetWeights_;
	std::vector<int> targetIndx_;
	std::vector<int> inputIndx_;
	std::vector<int> inputMask_;
	std::vector<std::vector<int> > spectatorIndx_;

	std::vector<std::vector<float> > vvInputTarget_;
	std::vector<std::vector<float> > vvInputTargetOOS_;
	std::vector<std::vector<float> > vvSpectator_;
	std::vector<std::vector<float> > vvSpectatorOOS_;
	std::map<int, std::pair<int, int> > fitOffset_;
	std::map<int, std::pair<int, int> > oosOffset_;

	std::vector<std::vector<int> > vvInputTargetIndx_;

	// TreeBoost
	int nTrees_;
	int minPtsNode_;
	int nthreads_;
	int maxNodes_;
  	double shrinkage_;
	int treeMaxLevels_;
	int marketNumber_;
	int modelNumber_;
	int zmin_;
	int zmax_;
	int addBMtoTarget_;
	double evQuantile_;
	std::string fitDir_;
	std::string statDir_;
	//std::string coefDir_;
	std::string predDir_;
	std::string predDirIns_;

	int get_max_horizon(const std::vector<std::string>& targetNames);
	void get_names(hff::SignalLabel& sh);
	void read_data(int idate);
	std::string get_bmpred_name(std::string targetName);
	void loop_dates_thread(int iThread);
	void corr_input();
	void fit();
	void sort_input();
	void sort_input(int iThread);
	void analyze(const std::string& sample);
	void write_prediction(const std::vector<int>& testDates, const std::vector<std::vector<float> >& vvSpectator,
			std::map<int, std::pair<int, int> >& mOffset, const std::string& predDir);
};

#endif
