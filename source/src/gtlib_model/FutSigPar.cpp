#include <gtlib_model/FutSigPar.h>
using namespace std;

FutSigPar::FutSigPar(string projName_)
:openMsecs(0),
closeMsecs(24*60*60*1000),
gridInterval(0),
n1secs(0),
gpts(0),
num_time_slices(1),
om_num_basic_sig(0),
om_num_sig(0),
om_num_err_sig(0),
skip_first_secs_(0),
om_ntrees(0),
tm_ntrees(0),
nOmHorizon(0),
nTmHorizon(0),
projName(projName_)
{
}

void FutSigPar::addOmHorizon(int horizon, int lag)
{
	vOmHorizonLag.push_back(make_pair(horizon, lag));
	++nOmHorizon;
}

void FutSigPar::addTmHorizon(int horizon, int lag)
{
	vTmHorizonLag.push_back(make_pair(horizon, lag));
	++nTmHorizon;
}
