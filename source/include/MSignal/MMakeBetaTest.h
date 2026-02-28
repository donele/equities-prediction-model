#ifndef __MMakeBetaTest__
#define __MMakeBetaTest__
#include <MFramework/MModuleBase.h>
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <unordered_map>
#include <string>
#include <map>
#include <vector>
#include <jl_lib/crossCompile.h>
#include <boost/thread.hpp>

class CLASS_DECLSPEC MMakeBetaTest: public MModuleBase {
public:
	MMakeBetaTest(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMakeBetaTest();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int verbose_;
	int len_;
	int ndays_;
	std::string fmodel_;
	boost::mutex corr_mutex_;
	std::map<std::string, std::vector<float>> mvPredIndex_;
	std::unordered_map<std::string, std::unordered_map<std::string, Corr>> mFilterTickerCorr_;
	std::unordered_map<std::string, int> mTickerVolrank_;

	std::map<std::string, double> read_volume(const std::string& market, int idate);
	void update_corr(const std::vector<float>& vRetTicker, const std::string& ticker);
	void read_index_preds();
	void read_index_targets();
	void get_ticker_volrank();
	void fill_ticker_volrank(const std::map<std::string, double>& mTickerVol, const std::vector<int>& volrank);
	std::vector<int> get_volrank(const std::map<std::string, double>& mTickerVol);
	void print_summary(const std::string& retName, const std::string& ticker);
};

#endif
