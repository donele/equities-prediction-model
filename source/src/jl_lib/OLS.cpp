#include <jl_lib/OLS.h>
#include <jl_lib/jlutil.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <iostream>
#include <iterator>
using namespace std;
using namespace gtlib;

OLS::OLS()
{
}

OLS::OLS(int nInputs, double ridgeReg, int iDecomp)
:nInputs_(nInputs),
ridgeReg_(ridgeReg),
iDecomp_(iDecomp)
{
	if( iDecomp == 2 )
		coeffs_ = vector<float>(nInputs + 1);
	else
		coeffs_ = vector<float>(nInputs);
	vCorr_ = vector<Corr>(nInputs_);
	vvCorr_ = vector<vector<Corr> >(nInputs_);
	for( int i = 0; i < nInputs_; ++i )
		vvCorr_[i] = vector<Corr>(i + 1);
	vAcc_ = vector<Acc>(nInputs_);
}

int OLS::nPts() const
{
	return accY_.n;
}

int OLS::length() const
{
	int len = 0;
	if( iDecomp_ == 2 )
		len = nInputs_ + 1;
	else
		len = nInputs_;
	return len;
}

const vector<float>& OLS::copyCoeffs() const
{
	return coeffs_;
}

double OLS::pred(const vector<float>& v, int offset) const
{
	int N = v.size();

	double y = 0.;
	if( iDecomp_ == 2 )
	{
		if( offset > 0 )
		{
			cout << "OLS::pred() is not implemented in case offset > 0.\n";
			exit(6);
		}

		y = coeffs_[0];
		for( int i = 0; i < N; ++i )
		{
			if( coeffs_[i + 1] != -100. )
				y += coeffs_[i + 1] * v[i];
		}
	}
	else
	{
		for( int i = 0; i < N; ++i )
		{
			if( coeffs_[i + offset] != -100. )
				y += coeffs_[i + offset] * v[i];
		}
	}
	return y;
}

double OLS::pred(const vector<float>& v, const vector<float>& vSd) const
{
	int N = v.size();

	double y = 0.;
	if( iDecomp_ == 2 )
	{
		y = coeffs_[0];
		for( int i = 0; i < N; ++i )
		{
			if( coeffs_[i + 1] != -100. )
			{
				float stdev = vSd[i - 1];
				float input = (stdev > 0.) ? v[i] / stdev : 0.;
				y += coeffs_[i + 1] * input;
			}
		}
	}
	else
	{
		for( int i = 0; i < N; ++i )
		{
			if( coeffs_[i] != -100. )
			{
				float stdev = vSd[i];
				float input = (stdev > 0.) ? v[i] / stdev : 0.;
				y += coeffs_[i] * input;
			}
		}
	}
	return y;
}

void OLS::print_coeffs() const
{
	if( iDecomp_ == 2 )
	{
		for( int i = 0; i < nInputs_ + 1; ++i )
			cout << coeffs_[i] << endl;
	}
	else
	{
		for( int i = 0; i < nInputs_; ++i )
			cout << coeffs_[i] << endl;
	}
}

bool OLS::valid_coeffs() const
{
	bool valid = coeffs_[0] != -100. && coeffs_[0] != 0.;
	return valid;
}

void OLS::write_coeffs(ostream& ofs) const
{
	if( iDecomp_ == 2 )
	{
		for( int i = 0; i < nInputs_ + 1; ++i )
		{
			if( i == 0 )
				ofs << coeffs_[i];
			else
				ofs << "\t" << coeffs_[i];
		}
		ofs << "\n";
	}
	else
	{
		for( int i = 0; i < nInputs_; ++i )
		{
			if( i == 0 )
				ofs << coeffs_[i];
			else
				ofs << "\t" << coeffs_[i];
		}
		ofs << "\n";
	}
}

void OLS::write_coeffs(ostream& ofs, vector<float>& vSd) const
{
// Normalization constant vSd.
	//cout << "[sd ";
	//copy(vSd.begin(), vSd.end(), ostream_iterator<float>(cout, " "));
	//cout << "]\n";

	//cout << "[c1 ";
	//copy(coeffs_.begin(), coeffs_.end(), ostream_iterator<float>(cout, " "));
	//cout << "]\n";


	if( iDecomp_ == 2 )
	{
		for( int i = 0; i < nInputs_ + 1; ++i )
		{
			if( i == 0 )
				ofs << coeffs_[i];
			else
			{
				float i_stdev = vSd[i - 1];
				float coeff = (i_stdev > 0.) ? coeffs_[i] / i_stdev : 0.;
				ofs << "\t" << coeff;
			}
		}
		ofs << "\n";
	}
	else
	{
		for( int i = 0; i < nInputs_; ++i )
		{
			float i_stdev = vSd[i];
			float coeff = (i_stdev > 0.) ? coeffs_[i] / i_stdev : 0.;
			if( i == 0 )
				ofs << coeff;
			else
				ofs << "\t" << coeff;
		}
		ofs << "\n";
	}
}

const Corr& OLS::get_corr(int i, int j) const
{
	return vvCorr_[i][j];
}

double OLS::coeff(int i) const
{
	return coeffs_[i];
}

void OLS::add(const float input, float target)
{
	vCorr_[0].add(input, target);
	vvCorr_[0][0].add(input, input);
	vAcc_[0].add(input);
	accY_.add(target);
}

void OLS::add(const vector<float>& input, float target)
{
	for( int i = 0; i < nInputs_; ++i )
	{
		vCorr_[i].add(input[i], target);

		for( int j = 0; j <= i; ++j )
			vvCorr_[i][j].add(input[i], input[j]);

		vAcc_[i].add(input[i]);
	}
	accY_.add(target);
}

void OLS::addCorr(const vector<Corr>& vCorr)
{
	for( int i = 0; i < nInputs_; ++i )
		vCorr_[i] += vCorr[i];
}

void OLS::addCorr(const vector<vector<Corr> >& vvCorr)
{
	for( int i = 0; i < nInputs_; ++i )
		for( int j = 0; j <= i; ++j )
			vvCorr_[i][j] += vvCorr[i][j];
}

void OLS::addCorr(const vector<Corr>& vCorr, const vector<vector<Corr> >& vvCorr, const vector<Acc>& vAcc, const Acc& accY)
{
	for( int i = 0; i < nInputs_; ++i )
		vCorr_[i] += vCorr[i];
	for( int i = 0; i < nInputs_; ++i )
		for( int j = 0; j <= i; ++j )
			vvCorr_[i][j] += vvCorr[i][j];
	for( int i = 0; i < nInputs_; ++i )
		vAcc_[i] += vAcc[i];
	accY_ += accY;
}

void OLS::addCorr(const CovMatSet& cSet)
{
	for( int i = 0; i < nInputs_; ++i )
		vCorr_[i] += cSet.vCorr[i];
	for( int i = 0; i < nInputs_; ++i )
		for( int j = 0; j <= i; ++j )
			vvCorr_[i][j] += cSet.vvCorr[i][j];
	for( int i = 0; i < nInputs_; ++i )
		vAcc_[i] += cSet.vAcc[i];
	accY_ += cSet.accY;
}

vector<float> OLS::getCoeffs(bool ridge, bool use_cov, int verbose)
{
	//if( 0 == iDecomp_ )
	//{
	//	// Create N x N matrix.
	//	int N = nInputs_;
	//	TMatrixD mat(N, N);
	//	for( int p1 = 0; p1 < nInputs_; ++p1 )
	//	{
	//		for( int p2 = 0; p2 < nInputs_; ++p2 )
	//		{
	//			int q1 = p1;
	//			int q2 = p2;
	//			if( q1 < q2 )
	//			{
	//				q2 = p1;
	//				q1 = p2;
	//			}
	//			if( use_cov )
	//				mat[p1][p2] = vvCorr_[q1][q2].cov();
	//			else
	//				mat[p1][p2] = vvCorr_[q1][q2].accXY.sum;
	//		}
	//	}

	//	// Ridge.
	//	if( ridge )
	//	{
	//		double ridgeReg = 100 * 20. / HEnv::Instance()->linearModel().gridInterval;
	//		for( int p = 0; p < nInputs_; ++p )
	//			mat[p][p] += ridgeReg;
	//	}

	//	// Vector b.
	//	TVectorD b(N);
	//	for( int i = 0; i < nInputs_; ++i )
	//	{
	//		if( use_cov )
	//			b[i] = vCorr_[i].cov();
	//		else
	//			b[i] = vCorr_[i].accXY.sum;
	//	}

	//	// Set the diagnoal to 1 if it is zero.
	//	for( int i = 0; i < nInputs_; ++i )
	//		if( mat[i][i] == 0. )
	//			mat[i][i] = 1.;

	//	// Decomposition.
	//	TDecompLU* lu = new TDecompLU(mat);
	//	//TDecompSVD* lu = new TDecompSVD(mat);

	//	// Solve for vector x.
	//	bool ok = false;
	//	TVectorD x = lu->Solve(b, ok);

	//	// Coefficients.
	//	//coeffs_ = vector<float>(nInputs_);
	//	if( ok )
	//	{
	//		double d1 = 0;
	//		double d2 = 0;
	//		lu->Det(d1, d2);
	//		double det = d1 * pow((double)2., (double)d2);

	//		// Store coeffs.
	//		for( int i=0; i<nInputs_; ++i )
	//			coeffs_[i] = x[i];
	//	}
	//	else
	//	{
	//		cout << "Decomposition has failed with tol " << lu->GetTol() << endl;
	//		for( int i=0; i<nInputs_; ++i )
	//			coeffs_[i] = -100;
	//	}

	//	delete lu;

	//	vector<Corr>().swap(vCorr_);
	//	vector<vector<Corr> >().swap(vvCorr_);
	//}
	if( 1 == iDecomp_ )
	{
		double **scov;
		scov = new double*[nInputs_];
		for( int i = 0; i < nInputs_; ++i )
			scov[i] = new double[nInputs_];

		double *rhs = new double[nInputs_];

		// scov
		for( int j = 0; j < nInputs_; ++j )
		{
			for( int k = 0; k < nInputs_; ++k )
			{
				if( j > k )
				{
					if( use_cov )
						scov[j][k] = scov[k][j] = vvCorr_[j][k].cov();
					else
						scov[j][k] = scov[k][j] = vvCorr_[j][k].accXY.sum;
				}
				else
				{
					if( use_cov )
						scov[j][k] = scov[k][j] = vvCorr_[k][j].cov();
					else
						scov[j][k] = scov[k][j] = vvCorr_[k][j].accXY.sum;
				}
			}
		}
		if( verbose == 1 )
		{
			for( int i = 0; i < nInputs_; ++i )
			{
				if( i == 0 )
					cout << scov[i][i];
				else
					cout << " " << scov[i][i];
			}
			cout << "\n";
		}

		// rhs
		for( int j = 0; j < nInputs_; ++j )
		{
			if( use_cov )
				rhs[j] = vCorr_[j].cov();
			else
				rhs[j] = vCorr_[j].accXY.sum;
		}

		// Ridge
		if( verbose == 1 )
			cout << "ridge " << ridgeReg_ << endl;
		//double ridgeReg = 0.;
		//if( gridInterval_ > 0 )
		{
			//ridgeReg = 100 * 20. / gridInterval_;
			//double ridgeReg = 100;
			//ridgeReg *= 20. / gridInterval_;
			for( int i = 0; i < nInputs_; ++i )
				scov[i][i] += ridgeReg_;
		}

		// Set the diagnoal to 1 if it is zero.
		for( int i = 0; i < nInputs_; ++i )
		{
			if( scov[i][i] == 0. )
				scov[i][i] = 1.;
		}

		if( verbose > 2 )
		{
			cout << "scov" << endl;
			for( int i = 0; i < nInputs_; ++i )
				for( int j = 0; j < nInputs_; ++j )
					cout << scov[i][j] << endl;
		}

		double det;
		int *idx = new int[nInputs_];
		ludcmp(scov, nInputs_, idx, &det);
		if( det == 0. )
		{
			cout << "Decomposition has failed. " << endl;
			for( int i = 0; i < nInputs_; ++i )
				rhs[i] = -100;
		}
		else
			lubksb(scov, nInputs_, idx, rhs);

		for( int i=0; i<nInputs_; ++i )
			coeffs_[i] = rhs[i];

		if( verbose > 2 )
		{
			cout << "coeffs" << endl;
			for( int i=0; i<nInputs_; ++i )
				cout << rhs[i] << endl;
		}

		vector<Corr>().swap(vCorr_);
		vector<vector<Corr> >().swap(vvCorr_);

		delete [] idx;
		for( int i = 0; i < nInputs_; ++i )
			delete [] scov[i];
		delete [] scov;
		delete rhs;
	}
	else if( 2 == iDecomp_ )
	{
		double **scov;
		scov = new double*[nInputs_ + 1];
		for( int i = 0; i < nInputs_ + 1; ++i )
			scov[i] = new double[nInputs_ + 1];

		double *rhs = new double[nInputs_ + 1];

		// scov
		for( int j = 0; j < nInputs_; ++j )
		{
			for( int k = 0; k < nInputs_; ++k )
			{
				if( j > k )
				{
					if( use_cov )
						scov[j][k + 1] = scov[k][j + 1] = vvCorr_[j][k].cov();
					else
						scov[j][k + 1] = scov[k][j + 1] = vvCorr_[j][k].accXY.sum;
				}
				else
				{
					if( use_cov )
						scov[j][k + 1] = scov[k][j + 1] = vvCorr_[k][j].cov();
					else
						scov[j][k + 1] = scov[k][j + 1] = vvCorr_[k][j].accXY.sum;
				}
			}
		}
		for( int j = 0; j < nInputs_; ++j )
		{
			if( use_cov )
			{
				scov[j][0] = vAcc_[j].mean();
				scov[nInputs_][j + 1] = vCorr_[j].cov();
			}
			else
			{
				scov[j][0] = vAcc_[j].sum;
				scov[nInputs_][j + 1] = vCorr_[j].accXY.sum;
			}
		}
		if( use_cov )
			scov[nInputs_][0] = accY_.mean();
		else
			scov[nInputs_][0] = accY_.sum;

		// rhs
		for( int j = 0; j < nInputs_; ++j )
		{
			if( use_cov )
				rhs[j] = vCorr_[j].cov();
			else
				rhs[j] = vCorr_[j].accXY.sum;
		}
		if( use_cov )
			rhs[nInputs_] = accY_.RMS() * accY_.RMS();
		else
			rhs[nInputs_] = accY_.sum2;

		//double ridgeReg = 0.;
		//if( gridInterval_ > 0 )
		{
			//ridgeReg = 100 * 20. / gridInterval_;
			//double ridgeReg = 100;
			//ridgeReg *= 20. / gridInterval_;
			for( int i = 0; i < nInputs_; ++i )
				scov[i][i] += ridgeReg_;
		}
		//double ridgeReg = 100;
		//ridgeReg *= 20. / gridInterval_;
		//for( int i = 0; i < nInputs_; ++i )
		//	scov[i][i] += ridgeReg;

		// Set the diagnoal to 1 if it is zero.
		for( int i = 0; i < nInputs_; ++i )
		{
			if( scov[i][i] == 0. )
				scov[i][i] = 1.;
		}

		if( verbose > 2 )
		{
			cout << "scov" << endl;
			for( int i = 0; i < nInputs_ + 1; ++i )
				for( int j = 0; j < nInputs_ + 1; ++j )
					cout << scov[i][j] << endl;
		}

		double det;
		int* idx = new int[nInputs_ + 1];
		ludcmp(scov, nInputs_ + 1, idx, &det);
		if( det == 0. )
		{
			cout << "Decomposition has failed. " << endl;
			for( int i = 0; i < nInputs_ + 1; ++i )
				rhs[i] = -100.;
		}
		else
			lubksb(scov, nInputs_ + 1, idx, rhs);

		for( int i = 0; i < nInputs_ + 1; ++i )
			coeffs_[i] = rhs[i];

		if( verbose > 2 )
		{
			cout << "coeffs" << endl;
			for( int i = 0; i < nInputs_ + 1; ++i )
				cout << rhs[i] << endl;
		}

		vector<Corr>().swap(vCorr_);
		vector<vector<Corr> >().swap(vvCorr_);
		vector<Acc>().swap(vAcc_);
		accY_.clear();

		delete [] idx;
		for( int i = 0; i < nInputs_ + 1; ++i )
			delete [] scov[i];
		delete [] scov;
		delete [] rhs;
	}
	return coeffs_;
}

ostream& operator <<(ostream& os, const OLS& obj)
{
	os << obj.nInputs_ << endl;
	copy(obj.coeffs_.begin(), obj.coeffs_.end(), ostream_iterator<float>(os, "\t"));
	os << endl;

	return os;
}

istream& operator >>(std::istream& is, OLS& obj)
{
	is >> obj.nInputs_;
	obj.coeffs_ = vector<float>(obj.nInputs_);
	for( int i = 0; i < obj.nInputs_; ++i )
		is >> obj.coeffs_[i];
	return is;
}
