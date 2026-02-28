#include <MSignal/SigSet.h>
#include <MFramework.h>
using namespace std;

SigSet* SigSet::instance_ = 0;

SigSet* SigSet::Instance()
{
	if( instance_ == 0 )
		instance_ = new SigSet();
	return instance_;
}

SigSet::SigSet()
{}

void SigSet::resize(int nThread)
{
	if( vSigInUse_.size() < nThread )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
		vSigInUse_.resize(nThread);
		vSig_.resize(nThread, hff::SigC(linMod, nonLinMod));
		nThreads_ = nThread;
	}
}

hff::SigC& SigSet::get_sig_object(int& iSig)
{
	boost::mutex::scoped_lock lock(sig_mutex_);
	for( int i = 0; i < nThreads_; ++i )
	{
		if( vSigInUse_[i] == 0 )
		{
			vSigInUse_[i] = 1;
			iSig = i;
			const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
			const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
			vSig_[i].init(linMod, nonLinMod);
			return vSig_[i];
		}
	}
	exit(6);
	return vSig_[0];
}

hff::SigC& SigSet::get_sig_object(int& iSig, int n)
{
	boost::mutex::scoped_lock lock(sig_mutex_);
	for( int i = 0; i < nThreads_; ++i )
	{
		if( vSigInUse_[i] == 0 )
		{
			vSigInUse_[i] = 1;
			iSig = i;
			const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
			const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
			vSig_[i].init(linMod, nonLinMod, n);
			return vSig_[i];
		}
	}
	exit(6);
	return vSig_[0];
}

hff::SigC& SigSet::get_sig_object(int& iSig, const vector<int>& vMsso)
{
	boost::mutex::scoped_lock lock(sig_mutex_);
	for( int i = 0; i < nThreads_; ++i )
	{
		if( vSigInUse_[i] == 0 )
		{
			vSigInUse_[i] = 1;
			iSig = i;
			const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
			const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
			//if( vMsso.empty() )
			//	vSig_[i].init(linMod, nonLinMod);
			//else
				vSig_[i].init(linMod, nonLinMod, vMsso);
			return vSig_[i];
		}
	}
	exit(6);
	return vSig_[0];
}

void SigSet::free_sig_object(int iSig)
{
	vSigInUse_[iSig] = 0;
}
