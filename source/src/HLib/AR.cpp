#include <HLib/AR.h>
#include <iostream>
#include <iterator>
#include <TMatrixD.h>
#include <TVectorD.h>
#include <TDecompLU.h>
#include <TDecompSVD.h>
using namespace std;

AR::AR(int filterLen, int horizon)
:filterLen_(filterLen),
horizon_(horizon)
{
	corrs_.resize(filterLen + horizon);
}

void AR::add(int lag, double v0, double v1)
{
	corrs_[lag].add(v0, v1);
	return;
}

vector<double> AR::getCoeffs(string target)
{
	printf("\nlag   cov     corr\n");
	for( int i=0; i<=filterLen_; ++i )
		printf("%3d %10.1e %6.3f\n", i, corrs_[i].cov(), corrs_[i].corr());

	TMatrixD mat(filterLen_, filterLen_);
	for( int i=0; i<filterLen_; ++i )
	{
		for( int j=0; j<filterLen_; ++j )
		{
			int lag = abs(-i + j);
			mat[i][j] = corrs_[lag].cov();
		}
	}

	// Decomposition.
	TDecompLU* lu = new TDecompLU(mat);
	//TDecompSVD* lu = new TDecompSVD(mat);

	// Vector b.
	TVectorD b(filterLen_);
	for( int i=0; i<filterLen_; ++i )
	{
		if( target == "spot" )
			b[i] = corrs_[i + horizon_].cov();
		else if( target == "sum" )
		{
			for( int j=0; j<horizon_; ++j )
				b[i] += corrs_[i + j + 1].cov();
		}
	}

	// Solve vector x.
	bool ok = false;
	TVectorD x = lu->Solve(b, ok);

	coeffs_.clear();
	if( ok )
	{
		double d1 = 0;
		double d2 = 0;
		lu->Det(d1, d2);
		double det = d1 * pow((double)2., (double)d2);

		printf("\nDet = %12.3e (%12.3e %12.3e)\n", det, d1, d2);
		printf("cond = %12.3e\n", lu->Condition());
		printf("\nCoeffs calculated.\n");
		for( int i=0; i<filterLen_; ++i )
		{
			printf("%3d %10.6f\n", i, x[i]);
			coeffs_.push_back(x[i]);
		}
		printf("\n");
	}

	return coeffs_;
}

double AR::pred(vector<double>& v)
{
	double y = 0;
	for( int i=0; i<filterLen_; ++i )
		y += coeffs_[filterLen_ - i - 1] * v[i];
	return y;
}

ostream& operator <<(ostream& os, const AR& obj)
{
	os << obj.filterLen_ << endl;
	copy(obj.coeffs_.begin(), obj.coeffs_.end(), ostream_iterator<double>(os, "\t"));
	os << endl;

	return os;
}

istream& operator >>(std::istream& is, AR& obj)
{
	is >> obj.filterLen_;
	obj.coeffs_ = vector<double>(obj.filterLen_);
	for( int i=0; i<obj.filterLen_; ++i )
		is >> obj.coeffs_[i];
	return is;
}
