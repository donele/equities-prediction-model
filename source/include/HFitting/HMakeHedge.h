#ifndef __HMakeHedge__
#define __HMakeHedge__
#include <HLib/HModule.h>
#include <HLib.h>
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

class CLASS_DECLSPEC HMakeHedge: public HModule {
public:
	HMakeHedge(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HMakeHedge();

	virtual void beginJob();
	virtual void beginDay();
	virtual void beginMarket();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endJob();

private:
	bool debug_;
	bool dayOK_;
	int verbose_;
	int idate_;
	int idate_p_;
	int idate_pp_;
	int idate_n_;
	std::set<std::string> uids_;
	std::map<std::string, hff::SigH> mSigH_;
	boost::mutex priv_mutex_;

	void get_hedge();
};

#endif