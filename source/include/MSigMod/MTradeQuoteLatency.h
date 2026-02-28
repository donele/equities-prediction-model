#ifndef __MTradeQuoteLatency__
#define __MTradeQuoteLatency__
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

class CLASS_DECLSPEC MTradeQuoteLatency: public MModuleBase {
public:
	MTradeQuoteLatency(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MTradeQuoteLatency();

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
	bool preload_;
	bool removeRestricted_;
	bool usUnivOverride_;
	bool samplePosMaxPos_;
	bool persistAna_;
	int verbose_;
	int minMsecs_;
	int maxMsecs_;
	int openDelay_;
	bool allowNegSize_;
	int allowNegSizeFrom_;
	std::set<std::string> uids_;
	std::unordered_map<std::string, std::string> mTickerUid_;
	std::unordered_map<std::string, std::vector<int>> mSampleMsecs_;
	Sessions sessions_;
	gtlib::EstTime* pEstTime_;
	boost::mutex bilin_mutex_;
	boost::mutex est_mutex_;
	boost::mutex load_mutex_;
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
	gtlib::StatusChangeUS statusChange_;

	IdxFuturePred idxfp_;
	TickDataProviderMilli<TradeInfo, OrderDataMicro>* dpMilli_;
	TickDataProvider<OrderDataMicro>* dataProvider_;
	std::string auctionMkts_;

	void set_restricted(const std::string& model, int idate);
	void closeDBConnections(const std::string& market);
	void initTickProvider();
};

#endif

