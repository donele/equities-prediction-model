#ifndef __HBiLinModTest__
#define __HBiLinModTest__
#include <HLib/HModule.h>
#include <HFitting/sig.h>
#include <gtlib_linmod/BiLinearModel.h>
#include <HLib.h>
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

class CLASS_DECLSPEC HBiLinModTest: public HModule {
public:
	HBiLinModTest(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HBiLinModTest();

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
	int verbose_;
	int pid_;
	int nThreads_;
	bool longTicker_;
	bool write_omtxt_;
	bool write_ombin_;
	bool write_tmtxt_;
	bool write_tmbin_;
	bool debug_signal_;
	bool debug_wgt_;
	//bool txt_corr_;
	boost::mutex biLinMod_mutex_;
	std::set<std::string> uids_;
	std::map<std::string, std::string> mTickerUid_;
	std::map<std::string, std::vector<std::string> > marketTickers_;
	std::map<std::string, std::map<int, std::vector<float> > > mUidTimeOm_;

	int mem_;
	int cntDay_;
	int idate_;
	int idate_p_;
	int idate_pp_;
	int idate_n_;
	bool hValid_;
	TickSources tSources_;
	std::vector<std::string> okECNs_;
	BiLinearModel biLinMod_;
	sig::HedgeInfo hInfo_;
	std::vector<hff::SigC> vSig_;
	std::vector<int> vSigInUse_;
	Corr corrPr_;
	Corr corrPrErr_;
	Corr corrPrTot_;
	Acc accPredErr_;

	hff::SigC& get_sig_object(int& iSig);
	void free_sig_object(int iSig);

	void read_signal(int idate);
	void get_prediction(hff::SigC& sig, std::string uid, int idate);
	void get_uid_map(std::string market, int idate_p);
};

#endif
