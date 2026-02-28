#ifndef __HTradeAnaGainSigMult__
#define __HTradeAnaGainSigMult__
#include <HOrders/HaltSimMult.h>
#include <HLib/HModule.h>
#include <jl_lib/MercatorTrade.h>
#include <jl_lib/jlutil.h>
#include <TFile.h>
#include <TH1.h>
#include <TProfile.h>
#include <string>
#include <vector>
#include <map>
#include <boost/thread.hpp>

class CLASS_DECLSPEC HTradeAnaGainSigMult: public HModule {
public:
	HTradeAnaGainSigMult(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTradeAnaGainSigMult();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	bool debug_;
	int id_;
	int haltLenSec_;
	int verbose_;
	double minPrice_;
	int minMsecs_;

	HaltSimMult* hs_;
	void createHaltCondition(const std::multimap<std::string, std::string>& conf);
};


#endif
