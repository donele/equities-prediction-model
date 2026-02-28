#include <gtlib_fastsim/fastsimObjs.h>
using namespace std;

namespace gtlib {

DailySimStat::DailySimStat()
	:retIntra(0.),
	retClop(0.),
	retClcl(0.),
	intra(0.),
	clcl(0.),
	clop(0.),
	dv(0.),
	dvBuy(0.),
	n(0),
	absDolPos(0.),
	sgnDolPos(0.)
{
}

void DailySimStat::print(const string& name)
{
	printf("%8s %d %5.2f %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f P %5.1f %5.1f %.2f\n",
			name.c_str(), n, retIntra, retClop, retClcl, dv, intra, clop, clcl, absDolPos, sgnDolPos, turnover);
}

SampleData::SampleData(float s, float c, float pr, float pc, int bs, int as, const string& t, int m)
	:sprd(s),
	cost(c),
	pred(pr),
	price(pc),
	bidSize(bs),
	askSize(as),
	ticker(t),
	msecs(static_cast<double>(m))
{
}

bool operator<(SampleData const& a, SampleData const& b)
{
	return a.msecs < b.msecs;
}

int SampleData::getDPos(double restore, int pos, int maxShrTrd, double maxDolPos,
		double dolPosNet, double maxDolPosNet, double thres, int openMsecs, int closeMsecs)
{
	double restoreFacPos = -(pos*price) / maxDolPos;
	double restoreFacTime = (msecs - openMsecs) / (closeMsecs - openMsecs);
	double adj = (maxDolPos > 0.) ? (restoreFacPos*restoreFacTime*restore) : 0.;
	double totPred = pred + adj;
	int side = getSide(totPred, thres);
	if( side != 0 )
	{
		int shrTrd = getShrTrd(side, maxShrTrd, pos, maxDolPos, dolPosNet, maxDolPosNet);
		int dPos = side * shrTrd;
		return dPos;
	}
	return 0;
}

double SampleData::getTradePrice(int dPos)
{
	double ret = 0.;
	double fac = 10000.;
	if( dPos > 0 )
	{
		double ask0 = sprd * price / 2. + price;
		double ask = ceil(ask0 * fac - 0.5) / fac;
		ret = ask;
	}
	else if ( dPos < 0 )
	{
		double bid0 = price - sprd * price / 2.;
		double bid = ceil(bid0 * fac - 0.5) / fac;
		ret = bid;
	}
	return ret;
}

int SampleData::getSide(double totPred, double thres)
{
	int side = 0;
	if( totPred > cost * basis_pts_ + thres )
		side = 1;
	else if( -totPred > cost * basis_pts_ + thres )
		side = -1;
	return side;
}

int SampleData::getShrTrd(int side, int maxShrTrd, int pos, double maxDolPos, double dolPosNet, double maxDolPosNet)
{
	int dPosTicker = getShrTrd(side, maxShrTrd, pos*price, maxDolPos);
	int dPosNet = getShrTrd(side, maxShrTrd, dolPosNet, maxDolPosNet);
	if( abs(dPosNet) < abs(dPosTicker) )
		return dPosNet;
	else
		return dPosTicker;
	return 0;
}

int SampleData::getShrTrd(int side, int maxShrTrd, double dolPos, double maxDolPos)
{
	int shrTrd = 0;
	int allowedMax = (side > 0) ? min(askSize, maxShrTrd) : min(bidSize, maxShrTrd);
	if( price > ltmb_ )
	{
		if( maxDolPos <= 0.)
			shrTrd = allowedMax;
		else if( abs(dolPos) < maxDolPos )
			shrTrd = min(allowedMax, static_cast<int>((maxDolPos - side*dolPos) / price));
		else if( dolPos > maxDolPos && side < 0 )
			shrTrd = min(allowedMax, static_cast<int>(maxDolPos / price));
		else if( dolPos < -maxDolPos && side > 0 )
			shrTrd = min(allowedMax, static_cast<int>(maxDolPos / price));
	}
	return shrTrd;
}

} // namespace gtlib
