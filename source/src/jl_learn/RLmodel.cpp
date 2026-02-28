#include <jl_learn/RLmodel.h>
#include <jl_learn/RLobj.h>
#include "TRandom3.h"
#include <cmath>
#include <iterator>
#include <vector>
#include <time.h>
#include <iostream>
using namespace std;

RLmodel::RLmodel()
{}

RLmodel::RLmodel(int interval, int maxPos, double learn, double thres, double thresIncr,
				 double cost, double adap, double maxWgt, int verbose)
:maxPos_(maxPos),
interval_(interval),
adap_(adap),
cost_(cost),
learn_(learn),
thres_(thres),
thresIncr_(thresIncr),
maxWgt_(maxWgt),
verbose_(verbose)
{
	dFiF_ = FtoInt(1.0) / (1.0 - thres_);
}

RLmodel::~RLmodel()
{
	delete tr_;
}

int RLmodel::get_nwgts()
{
	return wgt_.size();
}

vector<double> RLmodel::get_wgts()
{
	return wgt_;
}

void RLmodel::initWgt()
{
	tr_ = new TRandom3(0);
	int nw = wgt_.size();
	double factor = 1.0;
	for( int i=0; i<nw; ++i )
		wgt_[i] = 2.0*(tr_->Rndm() - 0.5)*factor;
	wgt_[0] = 0;
	return;
}

void RLmodel::initWgt(int i)
{
	double factor = 1.0;
	wgt_[i] = 2.0*(tr_->Rndm() - 0.5)*factor;
	return;
}

int RLmodel::FtoInt(double F)
{
	double absF = fabs(F);
	int absFint = 0;
	if( absF > thres_ )
	{
		int f = ceil( (absF - thres_)/thresIncr_ );
		absFint = max(maxPos_, f);
	}

	int Fint = (F > 0)?absFint:(-absFint);

	return Fint;
}