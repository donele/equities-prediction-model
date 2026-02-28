#ifndef __MWriteHfuniv__
#define __MWriteHfuniv__
#include <MFramework/MModuleBase.h>
#include <jl_lib/QueryAggregator.h>
#include <MSignal/sig.h>
#include <gtlib_linmod/BiLinearModelWeights.h>
#include <MFramework.h>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

class CLASS_DECLSPEC MWriteHfuniv: public MModuleBase {
public:
	MWriteHfuniv(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MWriteHfuniv();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginMarket();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarket();
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	//bool debug_noss_;
	bool dbdate_monday_;
	bool freeze_univ_;
	bool udate_is_trading_day_;
	bool efficient_write_;
	int verbose_;
	int nThreads_;
	int db_offset_;
	bool write_weights_;
	bool write_weights_debug_;
	//int write_weights_horizon_;
	//int write_weights_lag_;
	//int iHorizon_;
	std::string dbname_;
	std::unordered_map<std::string, std::string> mTickerUid_;

	int idate_;
	int idate_p_;
	int idate_p_good_;
	//BiLinearModelWeights biLinMod_;
	int dbdate_;
	int dbdate_p_;
	std::set<std::string> portSyms_;
	std::vector<std::string> vSymbolInuniv_;
	QueryAggregator qa_;

	int get_nuniv(const std::string& model02, const std::string& market, int idate);
	int get_good_idate(const std::string& model02, const std::string& market, int idate);
	bool get_uid_map(const std::string& market, int idate);
	void calculate_hedge(hff::SigC& sig, const std::string& uid, const std::string& ticker, int idate);
	std::string select_exchange(const std::vector<std::string>& markets);
	void write_begin(int idate, int dbdate, const std::string& model, const std::vector<std::string>& markets, bool db_debug, int gpts);
	void write_ticker(const std::string& model, const std::string& market, const std::string& uid, const std::string& ticker, hff::SigC& sig);
	void write_end(const std::string& model, const std::string& market, char mCode, bool freeze_univ);
};

#endif
