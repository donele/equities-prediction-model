#ifndef __MMakeSampleBB__
#define __MMakeSampleBB__
#include <MFramework/MModuleBase.h>
#include <gtlib_linmod/BiLinearModel.h>
#include <gtlib_linmod/BiLinearModelWeights.h>
#include <MSignal/sig.h>
#include <MSignal/sig_at.h>
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/TargetHedger.h>
#include <jl_lib/IndexBetaInfo.h>
#include <gtlib/EstTime.h>
#include <MFramework.h>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <boost/thread.hpp>

// Orderbook based signal calculation.

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MMakeSampleBB: public MModuleBase {
public:
	virtual ~MMakeSampleBB();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginMarket();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarket();
	virtual void endDay();
	virtual void endJob();

private:
	MMakeSampleBB(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	bool debug_;
	bool sample_all_;
	bool dayOK_;
	bool threadWrite_;
	bool multHedgedTarget_;
	bool irregularSampling_;
	bool preload_;
	int verbose_;
	std::string weight_source_;
	int iHedgeAlg_;
	int iTargetAlg_;
	int minMsecs_;
	int maxMsecs_;
	int filterHorizon_;
	int filterLag_;
	int nHedgeErrDay_;
	int desiredSamplesOm_;
	int desiredSamplesTm_;
	bool longTicker_;
	bool write_ombin_reg_;
	bool write_ombin_tevt_;
	bool write_ombin_eevt_;
	bool write_ombin_cevt_;
	bool write_tmbin_reg_;
	bool write_tmbin_tevt_;
	bool write_tmbin_eevt_;
	bool write_tmbin_cevt_;
	bool write_om_;
	std::string wmodel_;
	std::set<std::string> uids_;
	std::unordered_map<std::string, std::string> mTickerUid_;
	MSessions sessions_;
	gtlib::EstTime* pEstTime_;
	gtlib::Hedge* pHedge_;
	boost::mutex bilin_mutex_;
	boost::mutex est_mutex_;
	boost::mutex load_mutex_;
	boost::mutex hedge_mutex_;
	boost::mutex sig_mutex_;

	int cntDay_;
	int idate_;
	int idate_p_;
	int idate_pp_;
	int idate_n_;
	TickSources tSources_;
	std::vector<std::string> okECNs_;
	BiLinearModel biLinMod_;
	BiLinearModelWeights biLinModW_;
	std::vector<hff::SigC>* pSig_;

	std::vector<Corr> vCorr_;
	std::vector<Corr> vCorrTot_;

	IdxFuturePred idxfp_;
	TickDataProviderMilli<TradeInfo, OrderDataMicro>* dpMilli_;
	TickDataProvider<OrderDataMicro>* dataProvider_;
	//std::vector<int> sampleMsecsVect_;
	std::string auctionMkts_;
	std::vector<std::thread> vWriteThread_;

	std::vector<int> getSampleMsecs(int idate, const std::string& ticker);
	void closeDBConnections(const std::string& market);
	gtlib::Hedge* getHedgeObject(const std::string& model, int idate);
	bool enough_day_ombin();
	void batch_update_biLin(int k, BiLinearModel::LinRegStatTicker* linRegStat, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI);
	void initTickProvider();
	void get_hedged_targets(hff::SigC& sig, const gtlib::TargetHedger* pHedger);
	void get_hedged_targets(hff::SigC& sig, const std::vector<hff::SigH>* pvSigh);
	void get_prediction_at(hff::SigC& sig, const std::string& uid, const std::string& ticker, int idate);
	void write_signal(gtlib::Hedge* pHedge, std::vector<hff::SigC>* pSig, bool enough_day_ombin);
	void write_sample_type(bool write_om, bool write_tm, int idate,
		const std::string& desc, int sampleType, std::vector<hff::SigC>* pSig);
	void write_signal_file(const std::string& baseDir, const std::string& model, int idate,
		const std::string& sigType, const std::string& desc, int sampleType, std::vector<hff::SigC>* pSig);
	void select_events_reg(std::vector<hff::StateInfo>& sI);
	void select_events(std::vector<hff::StateInfo>& sI, int sampleType);
	void finish_file(bool do_write, std::ofstream& ofbin, std::ofstream& ofbintxt, int& cnt);
	std::vector<int> getGoodDataIndex(std::vector<hff::StateInfo>& sI, int sampleType);
	std::vector<int> getSampledIndex(const std::vector<int>& vIndexTot, int desiredSamples);
	void linearModelEndDay();
};

#endif

