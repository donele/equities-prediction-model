#ifndef __MMakeSampleMicro__
#define __MMakeSampleMicro__
#include <MFramework/MModuleBase.h>
#include <gtlib_linmod/BiLinearModel.h>
#include <gtlib_linmod/BiLinearModelWeights.h>
#include <gtlib_signal/StatusChangeUS.h>
#include <gtlib_signal/Hedge.h>
#include <gtlib_signal/MercatorVolume.h>
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

class CLASS_DECLSPEC MMakeSampleMicro: public MModuleBase {
public:
	MMakeSampleMicro(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMakeSampleMicro();

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
	bool debugSigCal_;
	bool sample_all_;
	bool dayOK_;
	bool writeSigmoid_;
	bool preload_;
	bool earlySelectSig_;
	bool use_sigmoid_;
	bool smoothTarget_;
	bool discardOutliers_;
	bool removeRestricted_;
	bool doUniversalLinearModel_;
	bool doErrorCorrectionModel_;
	bool usUnivOverride_;
	bool removeHardToBorrow_;
	int verbose_;
	std::string weight_source_;
	std::string sampling_;
	int schedType_;
	int minMsecs_;
	int maxMsecs_;
	int filterHorizon_;
	int filterLag_;
	int nHedgeErrDay_;
	int desiredSamplesOm_;
	int desiredSamplesTm_;
	int openDelay_;
	bool write_ombin_reg_;
	bool write_ombin_tevt_;
	bool write_ombin_nevt_;
	bool write_tmbin_reg_;
	bool write_tmbin_tevt_;
	bool write_tmbin_nevt_;
	bool write_om_;
	bool write_reg_;
	bool write_evt_;
	std::string wmodel_;
	std::set<std::string> uids_;
	std::unordered_map<std::string, std::string> mTickerUid_;
	std::unordered_map<std::string, std::vector<int>> mSampleMsecs_;
	Sessions sessions_;
	gtlib::EstTime* pEstTime_;
	gtlib::Hedge* pHedge_;
	boost::mutex bilin_mutex_;
	boost::mutex est_mutex_;
	boost::mutex load_mutex_;
	boost::mutex hedge_mutex_;
	boost::mutex sig_mutex_;
	std::vector<std::string> restricted_;

	int cntDay_;
	int idate_;
	int idate_p_;
	int idate_p2_;
	int idate_p3_;
	int idate_p4_;
	int idate_p5_;
	int idate_p6_;
	int idate_p7_;
	int idate_p8_;
	int idate_n_;
	TickSources tSources_;
	gtlib::MercatorVolume mercVol_;
	std::vector<std::string> okECNs_;
	BiLinearModel biLinMod_;
	BiLinearModelWeights biLinModW_;
	std::vector<hff::SigC>* pSig_;
	gtlib::StatusChangeUS statusChange_;

	std::vector<Corr> vCorr_;
	std::vector<Corr> vCorrTot_;

	IdxFuturePred idxfp_;
	TickDataProviderMicro<TradeInfo,OrderDataMicro>* dpMicro_;
	TickDataProviderTrade<TradeInfo,OrderDataMicro>* dataProvider_;//=&dpMicro;
	std::string auctionMkts_;
	std::vector<std::thread> vWriteThread_;

	void set_restricted(const std::string& model, int idate);
	std::vector<int> getSampleMsecsTicker(int idate, const std::string& ticker, float medVolat);
	int getTmSampleScale();
	void selectSig(hff::SigC& sig);
	void select_sample_type(const std::string& desc, int sampleType, hff::SigC& sig);
	void select_events(std::vector<hff::StateInfo>& sI, int sampleType);
	void select_events_reg(std::vector<hff::StateInfo>& sI);
	void select_events_evt(std::vector<hff::StateInfo>& sI, int sampleType);
	void closeDBConnections(const std::string& market);
	gtlib::Hedge* getHedgeObject(const std::string& model, int idate);
	bool enough_day_ombin();
	void batch_update_biLin(int k, BiLinearModel::LinRegStatTicker* linRegStat, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI);
	void initTickProvider();
	void get_hedged_targets(hff::SigC& sig, const gtlib::TargetHedger* pHedger);
	void get_hedged_targets(hff::SigC& sig, const std::vector<hff::SigH>* pvSigh);
	void get_prediction(hff::SigC& sig, const std::string& uid, const std::string& ticker, int idate);
	void write_signal(gtlib::Hedge* pHedge, std::vector<hff::SigC>* pSig, bool enough_day_ombin);
	void write_sample_type(bool write_om, bool write_tm, int idate,
			const std::string& desc, int sampleType, std::vector<hff::SigC>* pSig);
	void write_signal_file(const std::string& baseDir, const std::string& model, int idate,
			const std::string& sigType, const std::string& desc, int sampleType, std::vector<hff::SigC>* pSig);
	void finish_file(bool do_write, std::ofstream& ofbin, std::ofstream& ofbintxt, int& cnt);
	void write_sigmoid_range();
	std::vector<int> getGoodDataIndex(std::vector<hff::StateInfo>& sI, int sampleType);
	std::vector<int> getSampledIndex(const std::vector<int>& vIndexTot, int desiredSamples);
	void linearModelEndDay();
	void printTakeRat();
};

#endif

