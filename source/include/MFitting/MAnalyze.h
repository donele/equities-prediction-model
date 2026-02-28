#ifndef __MAnalyze__
#define __MAnalyze__
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC MAnalyze: public MModuleBase {
public:
	MAnalyze(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MAnalyze();

	virtual void beginJob();
	virtual void endJob();

private:
	bool debug_;
	int nInput_;
	int nSpectator_;
	int nTargets_;
	double minThres_;
	double maxThres_;
	std::string market_;
	std::string desc_;

	std::vector<int> vNrows_;
	std::vector<std::string> spectatorNames_;
	std::vector<std::string> targetNames_;
	std::vector<std::string> predNames_;
	std::vector<double> targetWeights_;
	std::vector<double> predWeights_;
	std::map<int, int> idateNtradeT_;
	std::map<int, int> idateNtradeW_;
	std::string targetFilter_;

	int addBMtoTarget_;
	std::string anaDir_;

	void analyze_oos();
	void analyze_ins();
	std::string get_ana_dir(bool is_oos);
	void prepare_data(std::vector<std::vector<float> >& vvSpectator, const std::vector<std::vector<std::vector<float> > >* pvvvSpectatorOOS);
	void analyze(std::vector<std::vector<float> >& vvvSpectator, const std::map<int, std::pair<int, int> >& mOffset, bool is_oos);
};

#endif
