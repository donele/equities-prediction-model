#include <gtlib_fitana/AnaDayStatMaker.h>
#include <gtlib_fitana/EvStatMaker.h>
#include <gtlib_fitana/MevStatMaker.h>
#include <jl_lib/stat_util.h>
using namespace std;

namespace gtlib {

AnaDayStatMaker::AnaDayStatMaker()
{
}

void AnaDayStatMaker::addData(float target, float bmpred, float pred, float cost)
{
	vTarget_.push_back(target);
	vBmpred_.push_back(bmpred);
	vPred_.push_back(pred);
	vCost_.push_back(cost);
}

AnaDayStat AnaDayStatMaker::getDayStat()
{
	AnaDayStat ds;

	ds.npts = vTarget_.size();

	ds.cbm = corr(vTarget_, vBmpred_);
	ds.ctb = corr(vTarget_, vPred_);

	ds.evBm = getEvStat(vTarget_, vBmpred_);
	ds.evTb = getEvStat(vTarget_, vPred_);

	ds.mevBm = getMevStat(vTarget_, vBmpred_, vCost_);
	ds.mevTb = getMevStat(vTarget_, vPred_, vCost_);

	return ds;
}

EvStat AnaDayStatMaker::getEvStat(const vector<float>& vTarget, const vector<float>& vPred)
{
	EvStatMaker evsMaker(vTarget, vPred);
	return evsMaker.getEvStat();
}

MevStat AnaDayStatMaker::getMevStat(const vector<float>& vTarget, const vector<float>& vPred, const vector<float>& vCost)
{
	double thres = 0.;
	MevStatMaker mevsMaker(vTarget, vPred, vCost, thres);
	return mevsMaker.getMevStat();
}

} // namespace gtlib
