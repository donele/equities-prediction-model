#ifndef __GTA_Btrader__
#define __GTA_Btrader__
#include "GTModule.h"
#include "GenBPNN.h"
#include <map>
#include <string>

class __declspec(dllexport) GTA_Btrader: public GTModule {
public:
	GTA_Btrader(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~GTA_Btrader();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void doTicker();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	GenBPNN bpnn_;

	int iModel_;
	int nInput_;
	int nOutput_;
	int nHidden_;
	int maxPos_;
	double thres_;
	double thresIncr_;
	double cost_;
	int verbose_;
	int iEpoch_;
	std::string modelFile_;
};

#endif