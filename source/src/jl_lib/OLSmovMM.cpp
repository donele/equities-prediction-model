#include <jl_lib/OLSmovMM.h>
#include <jl_lib/jlutil.h>
#include <vector>
#include <stdio.h>
using namespace std;
using namespace gtlib;

OLSmovMM::OLSmovMM()
:nInputs_(0)
{
}

OLSmovMM::OLSmovMM(int nInputs, double ridgeReg, int iDecomp)
:nInputs_(nInputs),
ridgeReg_(ridgeReg),
iDecomp_(iDecomp),
ols_(OLS(nInputs, ridgeReg, iDecomp))
{
	vCorr_ = vector<Corr>(nInputs_);
	vvCorr_ = vector<vector<Corr> >(nInputs_);
	for( int i = 0; i < nInputs_; ++i )
		vvCorr_[i] = vector<Corr>(i + 1);
	vAcc_ = vector<Acc>(nInputs_);
}

void OLSmovMM::initOLS()
{
	ols_ = OLS(nInputs_, ridgeReg_, iDecomp_);
}

int OLSmovMM::nPts()
{
	return ols_.nPts();
}

void OLSmovMM::getCoeffs(bool ridge, bool use_cov, int verbose)
{
	ols_.getCoeffs(ridge, use_cov, verbose);
}

void OLSmovMM::clear()
{
	// Init.
	for( vector<Corr>::iterator it = vCorr_.begin(); it != vCorr_.end(); ++it )
		it->clear();
	for( vector<vector<Corr> >::iterator it1 = vvCorr_.begin(); it1 != vvCorr_.end(); ++it1 )
		for( vector<Corr>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2 )
			it2->clear();
	for( vector<Acc>::iterator it = vAcc_.begin(); it != vAcc_.end(); ++it )
		it->clear();
	accY_.clear();
}

void OLSmovMM::add(const vector<float>& input, float target, int offset)
{
	int N = input.size();

	for( int i = 0; i < N; ++i )
	{
		vCorr_[i + offset].add( input[i], target );
		for( int j = 0; j <= i; ++j )
			vvCorr_[i + offset][j + offset].add( input[i], input[j] );
		vAcc_[i + offset].add( input[i] );
	}
	accY_.add( target );
}

void OLSmovMM::add(const OLSmovMM& rhs)
{
	for( int i = 0; i < nInputs_; ++i )
		vCorr_[i] += rhs.vCorr_[i];
	for( int i = 0; i < nInputs_; ++i )
		for( int j = 0; j <= i; ++j )
			vvCorr_[i][j] += rhs.vvCorr_[i][j];
	for( int i = 0; i < nInputs_; ++i )
		vAcc_[i] += rhs.vAcc_[i];
	accY_ += rhs.accY_;
}

const OLS& OLSmovMM::getOLS()
{
	return ols_;
}

void OLSmovMM::write_coeffs(ostream& os)
{
	ols_.write_coeffs(os);
}

void OLSmovMM::saveCorrs(ostream& os)
{
	for( vector<Corr>::iterator it = vCorr_.begin(); it != vCorr_.end(); ++it )
		os << (*it);
	for( vector<vector<Corr> >::iterator it1 = vvCorr_.begin(); it1 != vvCorr_.end(); ++it1 )
		for( vector<Corr>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2 )
			os << (*it2);
	for( vector<Acc>::iterator it = vAcc_.begin(); it != vAcc_.end(); ++it )
		os << (*it);
	os << accY_;
}

void OLSmovMM::loadCorrs(istream& is, bool valid)
{
	for( vector<Corr>::iterator it = vCorr_.begin(); it != vCorr_.end(); ++it )
		it->clear();
	for( vector<vector<Corr> >::iterator it1 = vvCorr_.begin(); it1 != vvCorr_.end(); ++it1 )
		for( vector<Corr>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2 )
			it2->clear();
	for( vector<Acc>::iterator it = vAcc_.begin(); it != vAcc_.end(); ++it )
		it->clear();
	accY_.clear();

	for( vector<Corr>::iterator it = vCorr_.begin(); it != vCorr_.end(); ++it )
		is >> (*it);
	for( vector<vector<Corr> >::iterator it1 = vvCorr_.begin(); it1 != vvCorr_.end(); ++it1 )
		for( vector<Corr>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2 )
			is >> (*it2);
	for( vector<Acc>::iterator it = vAcc_.begin(); it != vAcc_.end(); ++it )
		is >> (*it);
	is >> accY_;

	if( valid )
		ols_.addCorr(vCorr_, vvCorr_, vAcc_, accY_);
}

void OLSmovMM::loadCorrs(const CovMatSet& cSet)
{
	ols_.addCorr(cSet);
}

void OLSmovMM::saveCorrsBlock(ostream& os, int blockSize)
{
	for( vector<Corr>::iterator it = vCorr_.begin(); it != vCorr_.end(); ++it )
		os << (*it);
	os << flush;

	int N = vvCorr_.size();
	for( int i = 0; i < N; ++i )
	{
		int jFrom = i / blockSize * blockSize;
		int jTo = jFrom + i % blockSize + 1;
		for( int j = jFrom; j < jTo; ++j )
			os << vvCorr_[i][j];
	}
	os << flush;

	for( vector<Acc>::iterator it = vAcc_.begin(); it != vAcc_.end(); ++it )
		os << (*it);
	os << accY_;
	os << flush;
}

void OLSmovMM::loadCorrsBlock(istream& is, int blockSize, bool valid)
{
	int N = vvCorr_.size();

	for( vector<Corr>::iterator it = vCorr_.begin(); it != vCorr_.end(); ++it )
		it->clear();

	for( int i = 0; i < N; ++i )
	{
		int jFrom = i / blockSize * blockSize;
		int jTo = jFrom + i % blockSize + 1;
		for( int j = jFrom; j < jTo; ++j )
			vvCorr_[i][j].clear();
	}

	for( vector<Acc>::iterator it = vAcc_.begin(); it != vAcc_.end(); ++it )
		it->clear();
	accY_.clear();

	for( vector<Corr>::iterator it = vCorr_.begin(); it != vCorr_.end(); ++it )
		is >> (*it);

	for( int i = 0; i < N; ++i )
	{
		int jFrom = i / blockSize * blockSize;
		int jTo = jFrom + i % blockSize + 1;
		for( int j = jFrom; j < jTo; ++j )
			is >> vvCorr_[i][j];
	}

	for( vector<Acc>::iterator it = vAcc_.begin(); it != vAcc_.end(); ++it )
		is >> (*it);
	is >> accY_;

	if( valid )
		ols_.addCorr(vCorr_, vvCorr_, vAcc_, accY_);
}
