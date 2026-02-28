#include <jl_lib/OLSmov.h>
#include <jl_lib/jlutil.h>
#include <vector>
#include <stdio.h>
using namespace std;

OLSmov::OLSmov()
:nInputs_(0)
{
}

OLSmov::OLSmov(int nInputs, double ridgeReg, int iDecomp)
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
	vAccRaw_ = vector<Acc>(nInputs_);
	vAccRawTot_ = vector<Acc>(nInputs_);
	vSd_ = vector<float>(nInputs_);
}

void OLSmov::initOLS()
{
	ols_ = OLS(nInputs_, ridgeReg_, iDecomp_);
}

int OLSmov::nPts()
{
	return ols_.nPts();
}

void OLSmov::getCoeffs(bool ridge, bool use_cov, int verbose)
{
	ols_.getCoeffs(ridge, use_cov, verbose);
}

void OLSmov::clear()
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
	for( vector<Acc>::iterator it = vAccRaw_.begin(); it != vAccRaw_.end(); ++it )
		it->clear();

	// calculate vsd_, and init totAccRaw_.
	int N = vAccRawTot_.size();
	for( int i = 0; i < N; ++i )
		vSd_[i] = vAccRawTot_[i].stdev();
	//cout << "[";
	//copy(vSd_.begin(), vSd_.end(), ostream_iterator<float>(cout, " "));
	//cout << "]\n";
	for( vector<Acc>::iterator it = vAccRawTot_.begin(); it != vAccRawTot_.end(); ++it )
		it->clear();
}

void OLSmov::add(const vector<float>& input, float target, int offset)
{
	int N = input.size();

	for( int i = 0; i < N; ++i )
	{
		float i_stdev = vSd_[i];
		float i_input = input[i];
		float i_input_norm = (i_stdev > 0.) ? i_input / i_stdev : 0.;
		//if( i_stdev == 0. )
		//	i_stdev = 1.;


		vCorr_[i + offset].add( i_input_norm, target );
		for( int j = 0; j <= i; ++j )
		{
			float j_stdev = vSd_[j];
			float j_input = input[j];
			float j_input_norm = (j_stdev > 0.) ? j_input / j_stdev : 0.;
			//if( j_stdev == 0. )
			//	j_stdev = 1.;


			vvCorr_[i + offset][j + offset].add( i_input_norm, j_input_norm );
		}
		vAcc_[i + offset].add( i_input_norm );

		vAccRaw_[i + offset].add( i_input ); // Raw input.
	}
	accY_.add( target );
}

const OLS& OLSmov::getOLS()
{
	return ols_;
}

void OLSmov::write_coeffs(ostream& os)
{
	ols_.write_coeffs(os, vSd_);
}

double OLSmov::pred(const vector<float>& input)
{
	return ols_.pred(input, vSd_);
}

void OLSmov::saveCorrs(ostream& os)
{
	for( vector<Corr>::iterator it = vCorr_.begin(); it != vCorr_.end(); ++it )
		os << (*it);
	for( vector<vector<Corr> >::iterator it1 = vvCorr_.begin(); it1 != vvCorr_.end(); ++it1 )
		for( vector<Corr>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2 )
			os << (*it2);
	for( vector<Acc>::iterator it = vAcc_.begin(); it != vAcc_.end(); ++it )
		os << (*it);
	os << accY_;
	for( vector<Acc>::iterator it = vAccRaw_.begin(); it != vAccRaw_.end(); ++it )
		os << (*it);
}

void OLSmov::loadCorrs(istream& is, bool valid)
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
	for( vector<Acc>::iterator it = vAccRaw_.begin(); it != vAccRaw_.end(); ++it )
		is >> (*it);

	if( valid )
	{
		ols_.addCorr(vCorr_, vvCorr_, vAcc_, accY_);

		// Add up the accRaw's.
		int N = vAccRaw_.size();
		for( int i = 0; i < N; ++i )
			vAccRawTot_[i] += vAccRaw_[i];
	}
}
