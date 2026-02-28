#include <jl_learn/RLobj.h>
#include <jl_lib/jlutil.h>
#include "math.h"
#include <numeric>
using namespace std;

void RLticker::add(double mid)
{
	vmid.push_back(mid);

	double rtn = 0;
	int ms = vmid.size();
	if( ms > 1 )
	{
		double z0 = vmid[ms-2];
		double z1 = vmid[ms-1];
		if( z0 > ltmb_ )
			rtn = (z1 - z0)/z0;
	}
	vrtn.push_back(rtn);

	++indxT;
	return;
}

double RLticker::pnl(double cost)
{
	double ret = 0;
	double mid = vmid[indxT];
	for( vector<RLtrade>::iterator it = trades.begin(); it != trades.end(); ++it )
	{
		double prc  = vmid[it->indx];
		int shr = it->shr;
		ret += (mid - prc) * shr - fabs(cost * mid * shr);
	}
	return ret;
}

double RLticker::pnl2(double cost)
{
	double ret = 0;
	double mid = vmid[indxT];
	for( vector<RLtrade>::iterator it = trades.begin(); it != trades.end(); ++it )
	{
		double prc  = vmid[it->indx];
		int shr = it->shr;
		double pnl = (mid - prc) * shr - fabs(cost * mid * shr);
		ret += pnl * pnl;
	}
	return ret;
}