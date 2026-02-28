#ifndef __MMakeSample_at__
#define __MMakeSample_at__
#include <MFramework/MModuleBase.h>
#include <MSignal/sig.h>
#include <MSignal/sig_at.h>
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

class CLASS_DECLSPEC MMakeSample_at: public MModuleBase {
public:
	virtual ~MMakeSample_at();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginMarket();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarket();
	virtual void endDay();
	virtual void endJob();

private:
	MMakeSample_at(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	bool debug_;
	bool dayOK_;
	int verbose_;
	bool calculate_booksig_;
	bool do_universal_linear_model_;
	bool do_error_correction_model_;
	bool use_db_weights_;
	int pid_;
	int nThreads_;
	int minMsecs_;
	int maxMsecs_;
	int filterHorizon_;
	int filterLag_;
	int nErrDay_;
	bool longTicker_;
	bool write_ombin_;
	bool write_tmbin_;
	bool write_weights_;
	bool txt_corr_;
	boost::mutex sig_mutex_;
	boost::mutex om_mutex_;
	boost::mutex tm_mutex_;
	boost::mutex biLinMod_mutex_;
	boost::mutex tta_mutex_;
	boost::mutex oba_mutex_;
	std::set<std::string> uids_;
	std::unordered_map<std::string, std::string> mTickerUid_;
	MSessions sessions_;

	int mem_;
	int cntDay_;
	int idate_;
	int idate_p_;
	int idate_pp_;
	int idate_n_;
	TickSources tSources_;
	std::vector<std::string> okECNs_;
	BiLinearModel biLinMod_;
	BiLinearModelWeights biLinModW_;

	std::vector<Corr> vCorr_;
	std::vector<Corr> vCorrTot_;

	int cntOmBin_;
	int cntTmBin_;
	std::ofstream omBin_;
	std::ofstream omBinTxt_;
	std::ofstream tmBin_;
	std::ofstream tmBinTxt_;

	std::map<boost::thread::id, TickTobAcc*> mIdTta_;
	std::map<boost::thread::id, std::map<std::string, OrderBkAcc<OrderData> > > mIdObaMap_;

	void get_prediction_at(hff::SigC& sig, const std::string& uid, const std::string& ticker, int idate);
	void write_signal(const hff::SigC& sig, const std::string& uid, const std::string& ticker);
	TickTobAcc* get_tta(const std::string& market, int idate);
	std::map<std::string, OrderBkAcc<OrderData> >& get_obaMap(const std::string& market, int idate);
	void update_biLin(int k, int iT, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI);
};

#endif
