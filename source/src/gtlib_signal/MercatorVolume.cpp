#include <gtlib_signal/MercatorVolume.h>
#include <gtlib_model/mdl.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/sort_util.h>
#include <numeric>
using namespace std;

namespace gtlib {

MercatorVolume::MercatorVolume()
	:nq_(10),
	yyyymm_(0),
	norm_(0.)
{
}

void MercatorVolume::beginDay(const string& model, int idate)
{
	int yyyymm_read = getPrevMonth(idate);
	if( yyyymm_read != yyyymm_ )
	{
		yyyymm_ = yyyymm_read;
		read_db(model);
	}
}

int MercatorVolume::getPrevMonth(int idate)
{
	int yyyymm_prev = 0;
	int yyyymm = idate / 100;
	if( yyyymm % 100 == 1 )
		yyyymm_prev = (yyyymm / 100 - 1) * 100 + 12;
	else
		yyyymm_prev = yyyymm - 1;
	return yyyymm_prev;
}

void MercatorVolume::read_db(const string& model)
{
	vBinVol_.clear();

	string market = mdl::markets(model)[0];
	string selMarkets = mdl::selModelMarkets(model);
	if( !selMarkets.empty() )
		selMarkets = " and " + selMarkets;
	char cmd[2000];
	sprintf(cmd, "select o.symbol, count(*) from hforders o inner join hforderevents e "
			"on o.orderid = e.orderid and o.idate = e.idate "
			"where o.idate >= %d01 and e.idate <= %d31 %s "
			"group by o.symbol ",
			yyyymm_, yyyymm_, selMarkets.c_str());

	vector<vector<string>> vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);

	vector<float> vVol;
	vector<string> vTicker;
	for( auto& v : vv )
	{
		string ticker = trim(v[0]);
		float vol = atof(v[1].c_str());
		vTicker.push_back(ticker);
		vVol.push_back(vol);
	}

	getTickerBinMap(vTicker, vVol);
	getVBinVol(vTicker, vVol);
}

void MercatorVolume::getTickerBinMap(const vector<string>& vTicker, const vector<float>& vVol)
{
	mTickerIBin_.clear();
	int nSymbol = vTicker.size();
	int binSize = nSymbol / nq_;
	vector<int> vRank(nSymbol);
	gsl_rank<int, float>(vRank, vVol);
	for( int i = 0; i < nSymbol; ++i )
	{
		int iBin = min(vRank[i] / binSize, nq_ - 1);
		mTickerIBin_[vTicker[i]] = iBin;
	}
}

void MercatorVolume::getVBinVol(const vector<string>& vTicker, vector<float> vVol)
{
	sort(begin(vVol), end(vVol));
	int N = vVol.size();

	for( int i = 0; i < nq_; ++i )
	{
		int iFrom = i * N / nq_;
		int iTo = (i + 1) * N / nq_;
		float median = vVol[(iTo + iFrom) / 2];
		float binVol = median;
		vBinVol_.push_back(binVol);
	}
	norm_ = accumulate(begin(vBinVol_), end(vBinVol_), 0.) / nq_;

	bool debug = true;
	if( debug )
	{
		double minInt = norm_ / vBinVol_[nq_ - 1] * 30;
		double maxInt = norm_ / vBinVol_[0] * 30;
		char buf[200];
		sprintf(buf, "sampling interval min: %.1f max: %.1f\n", minInt, maxInt);
		cout << buf << flush;
	}
}

int MercatorVolume::getSampleInterval(const string& ticker, int stepSec) const
{
	int iBin = 0;
	auto it = mTickerIBin_.find(ticker);
	if( it != mTickerIBin_.end() )
		iBin = it->second;
	float binVol = vBinVol_[iBin];
	int sampleInterval = norm_ / binVol * stepSec;
	return sampleInterval;
}

} // namespace gtlib
