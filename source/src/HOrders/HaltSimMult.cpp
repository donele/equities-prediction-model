#include <HOrders/HaltSimMult.h>
#include <HOrders/HaltSched.h>
using namespace std;
using namespace gtlib;

HaltSimMult::HaltSimMult()
	:debug_(false),
	nHalts_(0),
	minPrice_(0.),
	minMsecs_(0),
	nTrades_(0),
	nTradesStop_(0)
{
}

HaltSimMult::HaltSimMult(int id, int haltLenSec, double minPrice, int minMsecs)
	:debug_(false),
	id_(id),
	haltLenSec_(haltLenSec),
	nHalts_(0),
	minPrice_(minPrice),
	minMsecs_(minMsecs),
	nTrades_(0),
	nTradesStop_(0)
{
}

HaltSimMult::~HaltSimMult()
{
	for( auto p : vHaltCond_ )
		delete p;
}

void HaltSimMult::setDebug()
{
	debug_ = true;
}

void HaltSimMult::addCond(HaltCond* p)
{
	vHaltCond_.push_back(p);
}

void HaltSimMult::beginDay(int idate)
{
	for( auto p : vHaltCond_ )
		p->beginDay(idate);
}

void HaltSimMult::beginTicker(const string& ticker,
		const vector<MercatorTrade>& mts, const QuoteSample& qSample)
{
	HaltSched hSched(haltLenSec_);
	if( mts[0].price >= minPrice_ )
	{
		for( auto& p : vHaltCond_ )
			p->updateSched(hSched, ticker, mts, qSample, minMsecs_);
	}

	int nTradesStop = 0;
	double tickerPnl = 0.;
	double tickerPnlStop = 0.;
	int NT = mts.size();
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		tickerPnl += mt.gainC / basis_pts_ * mt.price * mt.qty;
		if( !hSched.isHalted(mt.msecs) )
		{
			tickerPnlStop += mt.gainC / basis_pts_ * mt.price * mt.qty;
			++nTradesStop;
		}
	}
	if( debug_ )
	{
		if( fabs(tickerPnl - tickerPnlStop) > 100. )
			printf("#%d %s pnl %.1f diffStop %.1f\n",
					id_, ticker.c_str(), tickerPnl, tickerPnlStop - tickerPnl);
	}
	updateTickerStat(tickerPnl, tickerPnlStop, NT, nTradesStop, hSched.getNhalts());
}

void HaltSimMult::updateTickerStat(double tickerPnl, double tickerPnlStop,
		int nTrades, int nTradesStop, int nHalts)
{
	boost::mutex::scoped_lock lock(private_mutex_);
	totPnl_.add(tickerPnl);
	diffHalt_.add(tickerPnlStop - tickerPnl);
	nTrades_ += nTrades;
	nTradesStop_ += nTradesStop;
	nHalts_ += nHalts;
}

void HaltSimMult::print()
{
	double pnlK = totPnl_.sum/1000.;
	double diffK = diffHalt_.sum/1000.;
	double diffErr = diffHalt_.stdev() * sqrt(diffHalt_.n) / 1000.;
	double tstat = diffK / diffErr;
	printf("#%d %d halts: pnl(K) %.1f diff(K) %.1f (t%+.2f) #trd %d d %d\n",
			id_, nHalts_, pnlK, diffK, tstat, nTrades_, nTrades_ - nTradesStop_);
	for( auto& p : vHaltCond_ )
		printf("    %s\n", p->getDesc().c_str());
}

