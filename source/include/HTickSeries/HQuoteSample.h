#ifndef __HQuoteSample__
#define __HQuoteSample__
#include <HLib/HModule.h>
#include <jl_lib/Sessions.h>
#include "optionlibs/TickData.h"
#include <map>
#include <string>
#include <vector>

class HQuoteSample: public HModule {
public:
	HQuoteSample(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HQuoteSample();

	virtual void beginJob();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endJob();

private:
	bool debug_;
	bool requireValidQuote_;
	bool fillZeros_;
	bool allowCross_;
	int msecOpen_;
	int msecClose_;
	Sessions sessions_;
};

#endif
