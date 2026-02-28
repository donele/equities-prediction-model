#ifndef __MMakeSampleTP__
#define __MMakeSampleTP__
#include <MFramework/MModuleBase.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
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

class CLASS_DECLSPEC MMakeSampleTP: public MModuleBase {
public:
	MMakeSampleTP(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMakeSampleTP();

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
	bool dropBadTarget_;
	bool doUniversalLinearModel_;
	bool doErrorCorrectionModel_;
	bool usUnivOverride_;
	bool removeHardToBorrow_;
	bool sampleMercatorTrade_;
	bool onlyMercatorTrade_;
	bool sample_block_trade_;
	bool persistAna_;
	bool doSamplePendingUpdate_;
	bool requireTickValid_;
	bool requireTradeAfterFirstQuote_;
	bool rtrd_simple_def_;
	bool koreanTevtFilter_;
	bool allowEarlierQuote_;
	bool calculatePredSignal_;
	bool calculatePredSignalIt2_;
	bool sortTickers_;
	bool removeInvalidSigsEarly_;
	int modelDateOmIt0_;
	int modelDateTmIt0_;
	int modelDateOmIt1_;
	int roundTradeSampleTimeMsecs_;
	int verbose_;
	std::string weight_source_;
	std::string sampling_;
	int schedType_;
	int iHedgeAlg_;
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
	bool write_ombin_eevt_;
	bool write_ombin_cevt_;
	bool write_ombin_nevt_;
	bool write_tmbin_reg_;
	bool write_tmbin_tevt_;
	bool write_tmbin_eevt_;
	bool write_tmbin_cevt_;
	bool write_tmbin_nevt_;
	bool write_om_;
	bool write_reg_;
	bool write_evt_;
	bool write_npz_;
	bool allowNegSize_;
	int allowNegSizeFrom_;
	bool euNegSizeTest_;
	std::string fitDesc_;
	std::string descIt1_;
	std::string wmodel_;
	std::string coefModel_;
	std::string coefModelIt1_;
	std::set<std::string> uids_;
	std::unordered_map<std::string, std::string> mTickerUid_;
	std::unordered_map<std::string, std::vector<int>> mSampleMsecs_;
	Sessions sessions_;
	gtlib::EstTime* pEstTime_;
	gtlib::Hedge* pHedge_;
	boost::mutex est_mutex_;
	boost::mutex load_mutex_;
	boost::mutex hedge_mutex_;
	boost::mutex sig_mutex_;

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
	std::vector<hff::SigC>* pSig_;
	gtlib::StatusChangeUS statusChange_;
	gtlib::BoostedDecisionTree* fitterOmIt0_;
	gtlib::BoostedDecisionTree* fitterTmIt0_;
	gtlib::BoostedDecisionTree* fitterOmIt1_;

	TickDataProviderMilli<TradeInfo, OrderDataMicro>* dpMilli_;
	TickDataProvider<OrderDataMicro>* dataProvider_;
	std::string auctionMkts_;
	std::vector<std::thread> vWriteThread_;

	std::vector<int> mergeRegTradeTime(const std::vector<int>& regTime, const std::vector<int>& tradeTime);
	std::vector<int> getSampleMsecsTicker(std::vector<int> tradeTime, int idate,
			const std::string& ticker, float medVolat);
	std::vector<int> getSampleMsecsTicker(int idate, const std::string& ticker, float medVolat);
	int getTmSampleScale();
	void selectSig(hff::SigC& sig);
	void select_sample_type(const std::string& desc, int sampleType, hff::SigC& sig);
	void select_events(std::vector<hff::StateInfo>& sI, int sampleType);
	void select_events_reg(std::vector<hff::StateInfo>& sI);
	void select_events_evt(std::vector<hff::StateInfo>& sI, int sampleType);
	void closeDBConnections(const std::string& market);
	gtlib::Hedge* getHedgeObject(const std::string& model, int idate);
	void initTickProvider();
	void get_hedged_targets(hff::SigC& sig, const gtlib::TargetHedger* pHedger);

	void calculate_hedge(gtlib::Hedge* pHedge, std::vector<hff::SigC>* pSig);
	//void write_npz(gtlib::Hedge* pHedge, std::vector<hff::SigC>* pSig);
	void write_signal(std::vector<hff::SigC>* pSig, bool enough_day_ombin=true);
	void write_sample_type(bool write_om, bool write_tm, int idate,
		const std::string& desc, int sampleType, std::vector<hff::SigC>* pSig);
	void write_signal_file(const std::string& baseDir, const std::string& model, int idate,
		const std::string& sigType, const std::string& desc, int sampleType, std::vector<hff::SigC>* pSig);
	void finish_file(bool do_write, std::ofstream& ofbin, std::ofstream& ofbintxt, int& cnt);

	std::vector<int> getGoodDataIndex(std::vector<hff::StateInfo>& sI, int sampleType);
	std::vector<int> getSampledIndex(const std::vector<int>& vIndexTot, int desiredSamples);

	void calculate_pred_signals(hff::SigC& sig);
	void calculate_pred_signals_it2(hff::SigC& sig);
	int getModelDateToUse(int idate, const std::string& coefFitDir);
	std::vector<std::vector<float>> getOmInputsIt0(hff::SigC& sig);
	std::vector<std::vector<float>> getTmInputsIt0(hff::SigC& sig);
	std::vector<std::vector<float>> getOmInputsIt1(hff::SigC& sig);
};

#endif

