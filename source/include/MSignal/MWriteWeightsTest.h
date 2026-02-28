#ifndef __MWriteWeightsTest__
#define __MWriteWeightsTest__
#include <MFramework/MModuleBase.h>
#include <MSignal/sig.h>
#include <gtlib_linmod/BiLinearModelWeights.h>
#include <MFramework.h>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MWriteWeightsTest: public MModuleBase {
public:
	MWriteWeightsTest(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MWriteWeightsTest();

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
	bool debug_noss_;
	bool freeze_univ_;
	bool udate_is_trading_day_;
	int verbose_;
	int nThreads_;
	int db_offset_;
	bool write_weights_;
	bool write_weights_debug_;
	int write_weights_horizon_;
	int write_weights_lag_;
	int iHorizon_;
	std::unordered_map<std::string, std::string> mTickerUid_;

	int idate_;
	int idate_p_;
	int idate_p_good_;
	BiLinearModelWeights biLinMod_;

	int get_nuniv(const std::string& model02, const std::string& market, int idate);
	int get_good_idate(const std::string& model02, const std::string& market, int idate);
	bool get_uid_map(const std::string& market, int idate);
	void get_prediction(hff::SigC& sig, const std::string& uid, const std::string& ticker, int idate);
};

#endif
