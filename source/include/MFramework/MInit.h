#ifndef __MInit__
#define __MInit__
#include <MFramework/MModuleBase.h>
#include "optionlibs/TickData.h"
#include <boost/thread.hpp>
#include <vector>
#include <string>
#include <map>
#include <set>

class MInit: public MModuleBase {
public:
	MInit(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MInit();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginMarketDay();
	virtual void beginDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int verbose_;
	int cntDay_;
	int maxNticker_;
	bool multiThreadModule_;
	bool multiThreadTicker_;
	bool requireDataOK_;
	int nMaxThreadTicker_;
	int d1_;
	int d2_;
	std::string ticker_choice_;

	int arg_idate(const std::string& sdate);
	void set_idate_list();
};

#endif
