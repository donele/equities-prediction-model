#ifndef __MMakeBeta__
#define __MMakeBeta__
#include <MFramework/MModuleBase.h>
#include <MFramework.h>
#include <unordered_map>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <jl_lib/crossCompile.h>
#include <jl_lib/CorrDaily.h>
#include <boost/thread.hpp>

class CLASS_DECLSPEC MMakeBeta: public MModuleBase {
public:
	MMakeBeta(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMakeBeta();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	int verbose_;
	int horizon_;
	int cntDays_;
	int MALen_;
	std::string fmodel_;
	boost::mutex corr_mutex_;
	std::map<std::string, std::vector<float>> mvPredIndex_;
	std::unordered_map<std::string, std::unordered_map<std::string, CorrDaily>> mFilterTickerCorr_;
	std::set<std::string> uids_;
	std::unordered_map<std::string, std::string> mTickerUid_;

	void update_corr(const std::vector<float>& vRetTicker, const std::string& ticker);
	CorrDaily& getCorr(const std::string& retName, const std::string& ticker);
	void read_index_targets();
	void write_beta();
};

#endif
