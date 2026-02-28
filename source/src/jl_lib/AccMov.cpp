#include <jl_lib/AccMov.h>
#include <vector>
using namespace std;

AccMov::AccMov()
{
}

AccMov::AccMov(int fitDays)
	:fitDays_(fitDays)
{
}

void AccMov::beginDay(int idate, int idateFirst)
{
	accSum_.clear();
	for( map<int, Acc>::iterator it = mAcc_.begin(); it != mAcc_.end(); ++it )
		accSum_ += it->second;

	// Add a new Acc.
	mAcc_[idate] = Acc();

	// Remove.
	for( map<int, Acc>::iterator it = mAcc_.begin(); it != mAcc_.end(); )
	{
		if( it->first < idateFirst )
			mAcc_.erase(it++);
		else
			++it;
	}

	pAcc_ = &mAcc_[idate];
}

void AccMov::add(float val)
{
	pAcc_->add(val);
}

float AccMov::getRMS()
{
	return accSum_.RMS();
}

float AccMov::getMean()
{
	return accSum_.mean();
}
