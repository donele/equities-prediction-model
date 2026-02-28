#include <jl_lib/MercatorTrade.h>
#include <jl_lib/jlutil.h>
#include <algorithm>
using namespace std;
using namespace gtlib;

CondVars::CondVars()
	:dayVolat(0.),
	dayVolatSurprise(0.),
	dayVolume(0.),
	dayVolumeSurprise(0.),
	volat(0.),
	volatSurprise(0.),
	dv(0.),
	dvSurprise(0.),
	share(0.),
	shareSurprise(0.),
	mercatorTrade(0.),
	mercatorTradeSurprise(0.),
	share1s(0.),
	share60s(0.),
	share3600s(0.)
{
}

MercatorTrade::MercatorTrade(double ms, int sn, double pr, int qt,
		double p1, double p10, double md, char ex, int sch, int oqt,
		double op)
	:msecs(ms),
	sign(sn),
	price(pr),
	qty(qt),
	pred1(p1),
	pred10(p10),
	pred(p1 + p10),
	mid(md),
	execType(ex),
	schedType(sch),
	oqty(oqt),
	cost(0.),
	gain1(0.),
	gain61(0.),
	gainC(0.)
{
	if(oqty < qty)
		oqty = qty;
	if(price < ltmb_)
		price = op;
}
