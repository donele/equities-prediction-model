#ifndef __MMakeSampleEvt__
#define __MMakeSampleEvt__
#include <MFramework/MModuleBase.h>
#include <MSignal/sig.h>
#include <MSignal/sig_at.h>
#include <MFramework.h>
#include <map>
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
	MMakeSampleEvt(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MMakeSampleEvt();

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
	bool debug_booktrade_signal_;
	bool debug_booktrade_prob_;
	bool debug_rz_signal_;
	bool sampleAtWrite_;
	bool sample_all_;
	bool dayOK_;
	bool debug_fillImbBook_;
	int verbose_;
	bool calculate_booksig_;
	bool use_db_weights_;
	int pid_;
	int nThreads_;
	int minMsecs_;
	int maxMsecs_;
	int filterHorizon_;
	int filterLag_;
	int nErrDay_;
	int sample_max_;
	bool longTicker_;
	bool write_omtxt_reg_;
	bool write_ombin_reg_;
	bool write_omtxt_tevt_;
	bool write_ombin_tevt_;
	bool write_ombin_nevt_;
	bool write_ombin_eevt_;
	//bool write_tmtxt_;
	//bool write_tmbin_;
	bool write_tmtxt_reg_;
	bool write_tmbin_reg_;
	bool write_tmtxt_tevt_;
	bool write_tmbin_tevt_;
	bool write_tmbin_eevt_;
	bool write_om_;
	bool txt_corr_;
	std::string wmodel_;
	boost::mutex sig_mutex_;
	boost::mutex om_mutex_;
	boost::mutex tm_mutex_;
	boost::mutex biLinMod_mutex_;
	boost::mutex tta_mutex_;
	boost::mutex oba_mutex_;
	std::set<std::string> uids_;
	std::map<std::string, std::string> mTickerUid_;
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

	int cntOmBin_reg_;
	int cntOmBin_tevt_;
	int cntOmBin_nevt_;
	int cntOmBin_eevt_;
	int cntTmBin_reg_;
	int cntTmBin_tevt_;
	int cntTmBin_eevt_;
	std::ofstream omBin_reg_;
	std::ofstream omBinTxt_reg_;
	std::ofstream omTxt_reg_;
	std::ofstream omBin_tevt_;
	std::ofstream omBinTxt_tevt_;
	std::ofstream omTxt_tevt_;
	std::ofstream omBin_nevt_;
	std::ofstream omBinTxt_nevt_;
	std::ofstream omBin_eevt_;
	std::ofstream omBinTxt_eevt_;
	//std::ofstream tmBin_;
	//std::ofstream tmBinTxt_;
	//std::ofstream tmTxt_;
	std::ofstream tmBin_reg_;
	std::ofstream tmBinTxt_reg_;
	std::ofstream tmTxt_reg_;
	std::ofstream tmBin_tevt_;
	std::ofstream tmBinTxt_tevt_;
	std::ofstream tmTxt_tevt_;
	std::ofstream tmBin_eevt_;
	std::ofstream tmBinTxt_eevt_;

	IdxFuturePred idxfp_;
	TickProviderClassic<TradeInfo, QuoteInfo, OrderData>* tp_;
	std::vector<int> sampleMsecsVect_;
	std::vector<std::string> nyse_names_;
	std::map<std::string, int> first_trades_;
	std::map<std::string, double> medNbooktradesmap_;

	void init_tp(TickProviderClassic<TradeInfo,QuoteInfo,OrderData>*& tp);
	std::map<std::string, double> GetMedianBooktrades(int date);
	void get_prediction_at(hff::SigC& sig, std::string uid, std::string ticker, int idate);
	void write_signal(hff::SigC& sig, std::string uid, std::string ticker, int desiredSamplesOm = -1, int desiredSamplesTm = -1);
	void select_events(std::vector<hff::StateInfo>& sI, int sampleType, int desiredSamplesOm, int desiredSamplesTm);
	void update_biLin(int k, int iT, int timeIdx, std::string uid, hff::SigC& sig, hff::StateInfo& sI);
	void NewSig(int msecs, TickProviderClassic<TradeInfo,QuoteInfo,OrderData>* provider, int sampleType);
	void GetFirstTradeTimeBook(int idate, int openMsecs);
};

#endif