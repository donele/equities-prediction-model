#ifndef __HaltSimMult__
#define __HaltSimMult__
#include <HOrders/HaltCond.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/MercatorTrade.h>
#include <gtlib_signal/QuoteSample.h>
#include <vector>
#include <boost/thread.hpp>

class HaltSimMult {
public:
	HaltSimMult();
	HaltSimMult(int id, int haltLenSec, double minPrice, int minMsecs);
	~HaltSimMult();
	void beginDay(int idate);
	void beginTicker(const std::string& ticker,
			const std::vector<MercatorTrade>& mts, const gtlib::QuoteSample& qSample);
	void print();
	void setDebug();
	void addCond(HaltCond* p);

protected:
	bool debug_;
	int id_;
	int haltLenSec_;
	int nHalts_;
	double minPrice_;
	int minMsecs_;
	boost::mutex private_mutex_;

	std::vector<HaltCond*> vHaltCond_;

	int nTrades_;
	int nTradesStop_;
	Acc totPnl_;
	Acc diffHalt_;

	void updateTickerStat(double tickerPnl, double tickerPnlStop,
			int nTrades, int nTradesStop, int nHalts);
};

#endif
