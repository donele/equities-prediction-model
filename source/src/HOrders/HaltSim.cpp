#include <HOrders/HaltSim.h>
using namespace std;
using namespace gtlib;

HaltSim::HaltSim()
	:debug_(false),
	nHalts_(0),
	minPrice_(0.),
	minMsecs_(0),
	nTrades_(0),
	nTradesStop_(0)
{
}

HaltSim::HaltSim(int id, double minPrice, int minMsecs)
	:debug_(false),
	id_(id),
	nHalts_(0),
	minPrice_(minPrice),
	minMsecs_(minMsecs),
	nTrades_(0),
	nTradesStop_(0)
{
}

void HaltSim::setDebug()
{
	debug_ = true;
}

void HaltSim::updateTickerStat(double tickerPnl, double tickerPnlStop,
		int nTrades, int nTradesStop, int nHalts)
{
	boost::mutex::scoped_lock lock(private_mutex_);
	totPnl_.add(tickerPnl);
	diffHalt_.add(tickerPnlStop - tickerPnl);
	nTrades_ += nTrades;
	nTradesStop_ += nTradesStop;
	nHalts_ += nHalts;
}

void HaltSim::beginTicker(const string& ticker,
		const vector<MercatorTrade>& mts, const QuoteSample& qSample)
{
	HaltCondition* hc = getHaltCondition(ticker, mts, qSample);
	int nTradesStop = 0;
	double tickerPnl = 0.;
	double tickerPnlStop = 0.;
	int NT = mts.size();
	for( int iT = 0; iT < NT; ++iT )
	{
		const MercatorTrade& mt = mts[iT];
		tickerPnl += mt.gainC / basis_pts_ * mt.price * mt.qty;
		if( !hc->isHalted(mt.msecs) )
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
	updateTickerStat(tickerPnl, tickerPnlStop, NT, nTradesStop, hc->getNhalts());
	delete hc;
}

void HaltSim::print()
{
	double pnlK = totPnl_.sum/1000.;
	double diffK = diffHalt_.sum/1000.;
	double diffErr = diffHalt_.stdev() * sqrt(diffHalt_.n) / 1000.;
	double tstat = diffK / diffErr;
	string desc = getDesc();
	char buf[200];
	sprintf(buf, "#%d %d halts(%s): pnl(K) %.1f diff(K) %.1f (t%+.2f) #trd %d d %d\n",
			id_, nHalts_, desc.c_str(), pnlK, diffK, tstat, nTrades_, nTrades_ - nTradesStop_);
	cout << buf;
}

bool HaltCondition::isHalted(int msecs)
{
	for( int stopMsecs : vStopMsecs_ )
		if( msecs >= stopMsecs && msecs < stopMsecs + haltLenSec_ * 1000 )
			return true;
	return false;
}

int HaltCondition::getNhalts()
{
	return vStopMsecs_.size();
}
