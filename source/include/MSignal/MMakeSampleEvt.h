#ifndef __MMakeSampleEvt__
#define __MMakeSampleEvt__
#include <MFramework/MModuleBase.h>
#include <gtlib_linmod/BiLinearModel.h>
#include <gtlib_linmod/BiLinearModelWeights.h>
#include <MSignal/sig.h>
#include <MSignal/sig_at.h>
#include <jl_lib/IndexBetaInfo.h>
#include <gtlib/EstTime.h>
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

class CLASS_DECLSPEC MMakeSampleEvt: public MModuleBase {
public:
	virtual ~MMakeSampleEvt();

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
	MMakeSampleEvt(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	bool debug_;
	bool debugFlashSell_;
	bool sample_all_;
	bool dayOK_;
	int verbose_;
	bool calculate_booksig_;
	std::string weight_source_;
	int nThreads_;
	int minMsecs_;
	int maxMsecs_;
	int filterHorizon_;
	int filterLag_;
	int nErrDay_;
	int sample_max_;
	bool longTicker_;
	bool write_ombin_reg_;
	bool write_ombin_tevt_;
	bool write_ombin_nevt_;
	bool write_ombin_eevt_;
	bool write_ombin_cevt_;
	bool write_tmbin_reg_;
	bool write_tmbin_tevt_;
	bool write_tmbin_eevt_;
	bool write_tmbin_cevt_;
	bool write_om_;
	bool txt_corr_;
	std::string wmodel_;
	boost::mutex sig_mutex_;
	boost::mutex om_mutex_;
	boost::mutex tm_mutex_;
	boost::mutex biLinMod_mutex_;
	boost::mutex tp_mutex_;
	std::set<std::string> uids_;
	std::unordered_map<std::string, std::string> mTickerUid_;
	MSessions sessions_;
	gtlib::EstTime* pEstTime_;

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
	const std::vector<IndexBetaInfo>* pvIndexBeta_;

	std::vector<Corr> vCorr_;
	std::vector<Corr> vCorrTot_;

	int cntOmBin_reg_;
	int cntOmBin_tevt_;
	int cntOmBin_nevt_;
	int cntOmBin_eevt_;
	int cntOmBin_cevt_;
	int cntTmBin_reg_;
	int cntTmBin_tevt_;
	int cntTmBin_eevt_;
	int cntTmBin_cevt_;
	std::ofstream omBin_reg_;
	std::ofstream omBinTxt_reg_;
	std::ofstream omBin_tevt_;
	std::ofstream omBinTxt_tevt_;
	std::ofstream omBin_nevt_;
	std::ofstream omBinTxt_nevt_;
	std::ofstream omBin_eevt_;
	std::ofstream omBinTxt_eevt_;
	std::ofstream omBin_cevt_;
	std::ofstream omBinTxt_cevt_;
	std::ofstream tmBin_reg_;
	std::ofstream tmBinTxt_reg_;
	std::ofstream tmBin_tevt_;
	std::ofstream tmBinTxt_tevt_;
	std::ofstream tmBin_eevt_;
	std::ofstream tmBinTxt_eevt_;
	std::ofstream tmBin_cevt_;
	std::ofstream tmBinTxt_cevt_;

	IdxFuturePred idxfp_;
	TickProviderClassic<TradeInfo, QuoteInfo, OrderData>* tp_;
	std::vector<int> sampleMsecsVect_;

	bool enough_day_ombin();
	void batch_update_biLin(int k, BiLinearModel::LinRegStatTicker* linRegStat, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI);
	void init_tp(TickProviderClassic<TradeInfo,QuoteInfo,OrderData>*& tp);
	std::map<std::string, double> GetMedianBooktrades(int date);
	void get_prediction_at(hff::SigC& sig, const std::string& uid, const std::string& ticker, int idate);
	void write_signal(hff::SigC& sig, const std::string& uid, const std::string& ticker, int desiredSamplesOm = -1, int desiredSamplesTm = -1);
	void select_events(std::vector<hff::StateInfo>& sI, int sampleType, int desiredSamplesOm, int desiredSamplesTm);
	void NewSig(int msecs, TickProviderClassic<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType);
	void read_linear_model();
	void open_sig_files();
	void finish_linear_model();
	void finish_file(bool do_write, std::ofstream& ofbin, std::ofstream& ofbintxt, int& cnt);
	void select_events_random(std::vector<hff::StateInfo>& sI, int sampleType, int desiredSamplesOm, int desiredSamplesTm);
	void select_events_deterministic(std::vector<hff::StateInfo>& sI, int sampleType, int desiredSamplesOm, int desiredSamplesTm);
	std::vector<int> getGoodDataIndex(std::vector<hff::StateInfo>& sI, int sampleType);
	std::vector<int> getSampledIndex(const std::vector<int>& vIndexTot, int desiredSamples);
};

#endif
