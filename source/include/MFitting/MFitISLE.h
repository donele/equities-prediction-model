#ifndef __MFitISLE__
#define __MFitISLE__
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

class CLASS_DECLSPEC MFitISLE: public MModuleBase {
public:
	MFitISLE(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitISLE();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool addBMtoTarget_;
	int nInput_;
	int nSpectator_;
	double rat_sample_;
	std::string fitDesc_;
	std::string sigType_;
	bool do_fit_;
	bool do_analyze_;
	bool do_analyze_ins_;
	bool read_multithread_;
	double step_;

	std::vector<int> fitDates_;
	std::vector<int> oosDates_;
	std::string targetName_;
	std::vector<std::string> inputNames_;
	std::vector<std::string> spectatorNames_;
	int targetIndx_;
	std::vector<int> inputIndx_;
	std::vector<int> spectatorIndx_;

	std::vector<std::vector<float> > vvInputTarget_;
	std::vector<std::vector<float> > vvInputTargetOOS_;
	std::vector<std::vector<float> > vvSpectator_;
	std::vector<std::vector<float> > vvSpectatorOOS_;
	std::map<int, std::pair<int, int> > fitOffset_;
	std::map<int, std::pair<int, int> > oosOffset_;

	std::string statPath_;
	std::string predPath_;
	std::string sigDir_;
	std::string txtDir_;

	// TreeBoost
	int ntrees_;
	int minPtsNode_;
	int nthreads_;
	int maxNodes_;
  	double shrinkage_;
	int treeMaxLevels_;
	std::string baseDir_;
	std::string fitDir_;
	std::string predDir_;
	std::string predDirIns_;

	std::vector<int> sample_fitDates();
	void pre_read_data( std::vector<int>& dates, std::map<int, std::pair<int, int> >& mOffset,
		std::vector<std::vector<float> >& vvInput, std::vector<std::vector<float> >& vvSpect );
	void read_data(int idate, std::vector<std::vector<float> >& vvInput, std::vector<std::vector<float> >& vvSpect, std::map<int, std::pair<int, int> >& mOffset);
	void get_names(hff::SignalLabel& sh);
	void fit();
	void analyze(std::string sample);
};

#endif
