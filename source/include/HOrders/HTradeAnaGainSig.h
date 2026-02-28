#ifndef __HTradeAnaGainSig__
#define __HTradeAnaGainSig__
#include <HOrders/HaltSim.h>
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

class CLASS_DECLSPEC HTradeAnaGainSig: public HModule {
public:
	HTradeAnaGainSig(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HTradeAnaGainSig();

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
	int verbose_;
	double minPrice_;
	int minMsecs_;

	std::vector<HaltSim*> vhs_;
	void createHaltCondition(const std::multimap<std::string, std::string>& conf);
};


#endif
