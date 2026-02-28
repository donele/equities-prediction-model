#ifndef __HaltSim__
#define __HaltSim__
#include <jl_lib/MercatorTrade.h>
#include <jl_lib/jlutil.h>
#include <gtlib_signal/QuoteSample.h>
#include <vector>
#include <boost/thread.hpp>

class HaltCondition {
public:
	HaltCondition():haltLenSec_(0){}
	HaltCondition(int haltLenSec):haltLenSec_(haltLenSec){}
	bool isHalted(int msecs);
	int getNhalts();
protected:
	int haltLenSec_;
	std::vector<int> vStopMsecs_;
};

class HaltSim {
public:
	HaltSim();
	HaltSim(int id, double minPrice, int minMsecs);
	virtual ~HaltSim(){}
	virtual void beginDay(int idate){}
	void beginTicker(const std::string& ticker,
			const std::vector<MercatorTrade>& mts, const gtlib::QuoteSample& qSample);
	virtual HaltCondition* getHaltCondition(const std::string& ticker, const std::vector<MercatorTrade>& mts,
			const gtlib::QuoteSample& qSample) = 0;
	virtual std::string getDesc() = 0;
	void print();
	void setDebug();

protected:
	bool debug_;
	int id_;
	int nHalts_;
	double minPrice_;
	int minMsecs_;
	boost::mutex private_mutex_;

	int nTrades_;
	int nTradesStop_;
	Acc totPnl_;
	Acc diffHalt_;

	void updateTickerStat(double tickerPnl, double tickerPnlStop,
			int nTrades, int nTradesStop, int nHalts);
};

#endif
