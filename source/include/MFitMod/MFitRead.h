#ifndef __MFitRead__
#define __MFitRead__
#include <gtlib/util.h>
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/VariablesInfo.h>
#include <gtlib_fitting/FitData.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MFitRead: public MModuleBase {
public:
	MFitRead(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MFitRead();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool scaleSprd_;
	std::string sigType_;
	std::string fitDesc_;
	std::string sigModel_; // signal files are under this subdirectory.
	std::string predInputModel_;
	int readSample_;

	gtlib::VariablesInfo varInfo_;
	gtlib::FitData* pFitData_;
	std::vector<float> vSumTargetToday_;
	std::vector<float> vSumBmpredToday_;
	std::vector<int> fitdates_;
	std::set<std::string> ignoreInputNames_;

	// Constructor
	void initialize();
	std::string getSigModel();

	// beginJob
	void readData();
	std::vector<int> get_fitdates();
	gtlib::DailyDataCount getDailyDataCount();
	int getNNewData(std::string path);
	void readData(int idate);
	void read_inputs(int idate);
	int read_input_file(int idate, std::ifstream& ifs);
	int read_pred_file(int idate, const std::string& targetName);
	void read_bm_pred(int idate);
	void calculate_fit_target(int idate);
	std::vector<int> get_location(hff::SignalLabel& sh, const std::vector<std::string>& names);
	std::set<int> getIgnoreInputIndex(const std::vector<std::string>& inputNames);
	std::vector<std::string> getPredInputNames();
};

#endif

