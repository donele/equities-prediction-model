#ifndef __MSampleCaptureRat__
#define __MSampleCaptureRat__
#include <MFramework/MModuleBase.h>
#include <jl_lib/MercatorTrade.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GCurr.h>
#include <boost/thread.hpp>
#include <map>
#include <string>
#include <vector>
#include <list>

class CLASS_DECLSPEC MSampleCaptureRat: public MModuleBase {
public:
	MSampleCaptureRat(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MSampleCaptureRat();

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
	std::vector<char> execType_;
	std::vector<int> schedType_;
	std::vector<int> vSampleFreq_;
	std::vector<int> vNSample_;
	int ntrade_;
	int tol_;

	std::string condVar_;
	double minCondVar_;
	double maxCondVar_;
	std::vector<std::string> vCondTickers_;

	std::vector<std::string> getCondTickers();
	std::vector<int> getTradeIndex(const std::vector<MercatorTrade>* pvmt);
	void intra_day_stat2(const std::vector<MercatorTrade>* pvmt, const std::string& ticker);
};


#endif
