#ifndef __MMakeHedgeAt__
#define __MMakeHedgeAt__
#include <MFramework/MModuleBase.h>
#include <MFramework.h>
#include <jl_lib/TickSources.h>
#include <jl_lib/Sessions.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GTH.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MMakeHedgeAt: public MModuleBase {
public:
	virtual ~MMakeHedgeAt();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginMarket();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	MMakeHedgeAt(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	bool debug_;
	bool dayOK_;
	bool doNorthfield_;
	bool usMarketHedge_;
	std::string hedging_method_; // "nf", "mkt", "factor"
	int nThreads_;
	int verbose_;
	int idate_;
	int idate_p_;
	int idate_pp_;
	int idate_n_;
	int iTicker_;
	std::string market_;
	std::vector<std::string> tickers_;
	std::set<std::string> uids_;
	std::vector<hff::SigH> vSigH_;
	boost::mutex priv_mutex_;
	boost::mutex q_mutex_;
	boost::mutex t_mutex_;
	TickSources tSources_;
	Sessions sessions_;

	void hedge_begin_market();
	void hedge_begin_ticker_thread(int iThread);
	void hedge_begin_ticker(int iTicker);
	void get_hedge();
	void get_hedge_signal(NorthfieldHedge& nfh);
};

#endif
