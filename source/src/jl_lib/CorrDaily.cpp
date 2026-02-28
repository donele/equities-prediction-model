#include <jl_lib/CorrDaily.h>
#include <deque>
using namespace std;

CorrDaily::CorrDaily()
	:readable_(false),
	maxSize_(0)
{
}

CorrDaily::CorrDaily(int maxSize)
	:readable_(false),
	maxSize_(maxSize),
	minNData_(100)
{}

void CorrDaily::endDay()
{
	if( corrBuf_.accXY.n > minNData_ )
		dqCorr_.push_back(corrBuf_);
	while( dqCorr_.size() > maxSize_ )
		dqCorr_.pop_front();

	corrSum_.clear();
	for( auto& eachCorr : dqCorr_ )
		corrSum_ += eachCorr;

	readable_ = true;
}

void CorrDaily::add(double x, double y)
{
	readable_ = false;
	corrBuf_.add(x, y);
}

double CorrDaily::corr() const
{
	double ret = -999.;
	if( readable_ )
		ret = corrSum_.corr();
	return ret;
}

double CorrDaily::stdevX() const
{
	double ret = -999.;
	if( readable_ )
		ret = corrSum_.accX.stdev();
	return ret;
}

double CorrDaily::stdevY() const
{
	double ret = -999.;
	if( readable_ )
		ret = corrSum_.accY.stdev();
	return ret;
}

