#include <jl_lib/CorrMov.h>
#include <iostream>
#include <vector>
using namespace std;

CorrMov::CorrMov()
{
}

CorrMov::CorrMov(int fitDays)
:fitDays_(fitDays)
{
}

void CorrMov::beginDay(int idate, int idateFirst)
{
	//if( mCorr_.size() == fitDays_ )
	{
		corrSum_.clear();
		for( map<int, Corr>::iterator it = mCorr_.begin(); it != mCorr_.end(); ++it )
			corrSum_ += it->second;
		cout << corrSum_.cov() << endl;
	}

	// Add a new Corr.
	mCorr_[idate] = Corr();

	// Remove.
	for( map<int, Corr>::iterator it = mCorr_.begin(); it != mCorr_.end(); )
	{
		if( it->first < idateFirst )
			mCorr_.erase(it++);
		else
			++it;
	}

	corr_.clear();
}

void CorrMov::add(float v1, float v2)
{
	corr_.add( v1, v2 );
}

float CorrMov::getCov()
{
	return corrSum_.cov();
}
