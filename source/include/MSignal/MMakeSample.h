#ifndef __MMakeSample__
#define __MMakeSample__
#include <MFramework/MModuleBase.h>
#include <MSignal/sig.h>
#include <gtlib_linmod/BiLinearModel.h>
#include <gtlib_linmod/BiLinearModelN.h>
#include <gtlib_linmod/BiLinearModelWeights.h>
#include <jl_lib/IndexBetaInfo.h>
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

class CLASS_DECLSPEC MMakeSample: public MModuleBase {
public:
	virtual ~MMakeSample();

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
	MMakeSample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	bool debug_;
	bool dayOK_;
	bool debug_wgt_;
	bool debugErrCorr_;
	bool debug_sprd_;
	bool debug_quote_index_;
	bool batchUpdateBiLin_;
	int verbose_;
	int cntOkTicker_;
	bool verbose_ols_;
	bool calculate_booksig_;
	bool do_universal_linear_model_;
	bool do_error_correction_model_;
	bool use_calculated_weights_;
	bool use_db_weights_;
	bool use_debug_db_weights_;
	bool do_linear_long_horizon_;
	bool normalized_linear_model_;
	bool debug_quote_trade_order_;
	bool hedge_at_;
	int nThreads_;
	int minMsecs_;
	int maxMsecs_;
	int filterHorizon_;
	int filterLag_;
	double ridgeUniv_;
	double ridgeSS_;
	bool longTicker_;
	bool write_ombin_;
	bool write_tmbin_;
	bool write_weights_;
	bool debug_ombin_;
	std::string wmodel_;
	boost::mutex sig_mutex_;
	boost::mutex om_mutex_;
	boost::mutex tm_mutex_;
	boost::mutex biLinMod_mutex_;
	boost::mutex varMod_mutex_;
	boost::mutex tta_mutex_;
	boost::mutex oba_mutex_;
	std::set<std::string> uids_;
	std::unordered_map<std::string, std::string> mTickerUid_;
	MSessions sessions_;

	bool hedge_warning_issued_;
	int mem_;
	int cntDay_;
	int idate_;
	int idate_p_;
	int idate_pp_;
	int idate_n_;
	TickSources tSources_;
	std::vector<std::string> okECNs_;
	BiLinearModel biLinMod_;
	BiLinearModelN biLinModN_;
	BiLinearModelWeights biLinModW_;
	BiLinearModel biLinModT_;
	BiLinearModelN biLinModTN_;

	std::vector<Corr> vCorr_;
	std::vector<Corr> vCorrTot_;

	std::string bue_uid_;
	hff::StateInfo* bue_sI_;

	int cntOmBin_;
	int cntTmBin_;
	std::ofstream omBin_;
	std::ofstream omBinTxt_;
	std::ofstream tmBin_;
	std::ofstream tmBinTxt_;

	std::map<boost::thread::id, TickTobAcc*> mIdTta_;
	std::map<boost::thread::id, std::map<std::string, OrderBkAcc<OrderData> > > mIdObaMap_;

	bool writeOmToday();
	void setOkEcns();
	void openSigFile();
	void linModBeginDay();
	void get_prediction(hff::SigC& sig, const std::string& uid, const std::string& ticker, int idate);
	void write_signal(const hff::SigC& sig, const std::string& uid, const std::string& ticker);
	TickTobAcc* get_tta(const std::string& market, int idate);
	std::map<std::string, OrderBkAcc<OrderData> >& get_obaMap(const std::string& market, int idate);
	void update_biLin(int k, int iT, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI);
	void batch_update_biLin(int k, BiLinearModel::LinRegStatTicker* linRegStat, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI);
};

#endif
