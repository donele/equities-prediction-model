#include <gtlib_linmod/ARMulti.h>
using namespace std;

namespace gtlib {

ARMulti::ARMulti(int nSeries, int filterLen, int horizon, int targetLag)
:nSeries_(nSeries),
nPredictors_(nSeries - 1),
filterLen_(filterLen),
horizon_(horizon),
targetLag_(targetLag)
{
	int lagMax = filterLen + horizon + targetLag;
	corrs_ = vector<vector<vector<Corr> > >(nSeries, vector<vector<Corr> >(nSeries, vector<Corr>(lagMax)));
}

ARMulti::ARMulti(const string& path)
{
	ifstream ifs(path.c_str());
	ifs >> (*this);
}

bool ARMulti::coeffsGood()
{
	return coeffs_[0][0] != -100;
}

void ARMulti::add(int iSer1, int iSer2, int lag, double v1, double v2)
{
	corrs_[iSer1][iSer2][lag].add(v1, v2);
	return;
}

void ARMulti::add(const hff::IndexFilter& filter, const vector<const vector<ReturnData>*>& vp)
{
	int totlen = vp[0]->size();
	int NF = filter.names.size();
	int lagMax = filter.length + filter.horizon;

	for( int iSer1 = 0; iSer1 < NF; ++iSer1 ) // series 1.
	{
		for( int iSer2 = 0; iSer2 < NF; ++iSer2 ) // series 2. (series 1 lags series 2, or series 2 predicts series 1)
		{
			for( int i = filter.skip; i < totlen; ++i ) // index of series 1.
			{
				double val1 = clip_off( (*vp[iSer1])[i].ret, filter.clip[iSer1] );

				int jBegin = i;
				int jEnd = i + lagMax; // exclusive.
				if( jEnd > totlen )
					jEnd = totlen;

				vector<Corr>& vCorr = corrs_[iSer1][iSer2];

				for( int j = jBegin; j < jEnd; ++j ) // index of series 2.
				{
					double val2 = clip_off( (*vp[iSer2])[j].ret, filter.clip[iSer2] );
					int lag = j - i;
					vCorr[lag].add(val1, val2);
				}
			}
		}
	}
}

vector<vector<float> > ARMulti::getCoeffs(const string& target, int verbose)
{
	coeffs_ = vector<vector<float> >(nPredictors_, vector<float>(filterLen_, -100));

	if( verbose > 2 )
	{
		// Print Cov and Corr.
		for( int i = 0; i < nSeries_; ++i )
		{
			// Series numbers.
			printf("%3d", i);
			for( int j = 0; j < nSeries_; ++j )
				printf(" %10d %6s", j, " ");

			// Column title.
			printf("\nlag");
			for( int j = 0; j < nSeries_; ++j )
				printf(" %10s %6s", "cov", "corr");

			// Values.
			for( int k=0; k<=filterLen_; ++k )
			{
				printf("\n%3d", k);
				for( int j = 0; j < nSeries_; ++j )
					printf(" %10.1e %6.3f", corrs_[i][j][k].cov(), corrs_[i][j][k].corr());
			}
			printf("\n");
		}
	}

	//if( 0 )
	//{
	//	// Create N x N matrix.
	//	int N = filterLen_ * nPredictors_;
	//	TMatrixD mat(N, N);
	//	for( int p1 = 0; p1 < nPredictors_; ++p1 )
	//	{
	//		for( int p2 = 0; p2 < nPredictors_; ++p2 )
	//		{
	//			int rowBegin = p1 * filterLen_;
	//			int rowEnd = (p1 + 1) * filterLen_; // exclusive.
	//			for( int irow = rowBegin; irow < rowEnd; ++irow )
	//			{
	//				int colBegin = p2 * filterLen_;
	//				int colEnd = (p2 + 1) * filterLen_; // exclusive.
	//				for( int icol = colBegin; icol < colEnd; ++icol )
	//				{
	//					int lag = irow % filterLen_ - icol % filterLen_;
	//					if( lag > 0 )
	//						mat[irow][icol] = corrs_[p2 + 1][p1 + 1][lag].cov();
	//					else
	//						mat[irow][icol] = corrs_[p1 + 1][p2 + 1][-lag].cov();
	//				}
	//			}
	//		}
	//	}

	//	// Decomposition.
	//	TDecompLU* lu = new TDecompLU(mat);
	//	//TDecompSVD* lu = new TDecompSVD(mat);

	//	// Vector b.
	//	TVectorD b(N);
	//	for( int p = 0; p < nPredictors_; ++p )
	//	{
	//		for( int i=0; i<filterLen_; ++i )
	//		{
	//			if( target == "spot" )
	//				b[i] = corrs_[p + 1][0][i + horizon_].cov();
	//			else if( target == "sum" )
	//			{
	//				for( int j=targetLag_; j<horizon_; ++j )
	//					b[i] += corrs_[p + 1][0][i + j + 1].cov();
	//			}
	//		}
	//	}

	//	// Solve for vector x.
	//	bool ok = false;
	//	TVectorD x = lu->Solve(b, ok);

	//	// Coefficients.
	//	if( ok )
	//	{
	//		if( verbose > 0 )
	//		{
	//			double d1 = 0;
	//			double d2 = 0;
	//			lu->Det(d1, d2);
	//			double det = d1 * pow((double)2., (double)d2);

	//			printf("\nDet = %12.3e (%12.3e %12.3e)\n", det, d1, d2);
	//			printf("cond = %12.3e\n", lu->Condition());
	//			printf("\nCoeffs calculated.\n");

	//			if( verbose > 1 )
	//			{
	//				for( int i=0; i<filterLen_; ++i )
	//				{
	//					printf("%3d", i);
	//					for( int p = 0; p < nPredictors_; ++p )
	//						printf(" %13.9f", x[p * filterLen_ + i]);
	//					printf("\n");
	//				}
	//			}
	//		}

	//		// Store coeffs.
	//		for( int i=0; i<filterLen_; ++i )
	//			for( int p = 0; p < nPredictors_; ++p )
	//				coeffs_[p][i] = x[p * filterLen_ + i];
	//	}

	//	delete lu;
	//}
	//else
	{
		int N = filterLen_ * nPredictors_;

		double **scov;
		scov = new double*[N];
		for( int i = 0; i < N; ++i )
			scov[i] = new double[N];

		double *rhs = new double[N];

		// scov
		for( int p1 = 0; p1 < nPredictors_; ++p1 )
		{
			for( int p2 = 0; p2 < nPredictors_; ++p2 )
			{
				int rowBegin = p1 * filterLen_;
				int rowEnd = (p1 + 1) * filterLen_;
				for( int irow = rowBegin; irow < rowEnd; ++irow )
				{
					int colBegin = p2 * filterLen_;
					int colEnd = (p2 + 1) * filterLen_;
					for( int icol = colBegin; icol < colEnd; ++icol )
					{
						int lag = irow % filterLen_ - icol % filterLen_;
						if( lag > 0 )
							scov[irow][icol] = corrs_[p2 + 1][p1 + 1][lag].cov();
						else
							scov[irow][icol] = corrs_[p1 + 1][p2 + 1][-lag].cov();
					}
				}
			}
		}

		// rhs
		for( int p = 0; p < nPredictors_; ++p )
		{
			for( int i = 0; i < filterLen_; ++i )
			{
				int idx = p * filterLen_ + i;
				if( target == "spot" )
					rhs[idx] = corrs_[p + 1][0][i + horizon_].cov();
				else if( target == "sum" )
				{
					rhs[idx] = 0.;
					for( int j = targetLag_; j < targetLag_ + horizon_; ++j )
						rhs[idx] += corrs_[p + 1][0][i + j + 1].cov();
				}
			}
		}

		double det;
		//int* idx;
		//idx = (int*)calloc(N, sizeof(int));
		int *idx = new int[N];
		ludcmp(scov, N, idx, &det);
		if( det == 0. )
		{
			cout << "Decomposition has failed. " << endl;
			for( int i = 0; i < N; ++i )
				rhs[i] = -100;
		}
		else
			lubksb(scov, N, idx, rhs);

		// Store coeffs.
		for( int i = 0; i < filterLen_; ++i )
			for( int p = 0; p < nPredictors_; ++p )
				coeffs_[p][i] = rhs[p * filterLen_ + i];

		delete [] idx;
		for( int i = 0; i < N; ++i )
			delete [] scov[i];
		delete [] scov;
		delete [] rhs;
	}

	return coeffs_;
}

vector<vector<float> >& ARMulti::coeffs()
{
	return coeffs_;
}

float ARMulti::pred(const vector<vector<float>>& v)
{
	double y = 0;
	for( int p = 0; p < nPredictors_; ++p )
	{
		for( int i = 0; i < filterLen_; ++i )
			y += coeffs_[p][i] * v[p][filterLen_ - i - 1];
	}
	return y;
}

ostream& operator <<(ostream& os, const ARMulti& obj)
{
	os << obj.nPredictors_ << endl;
	os << obj.filterLen_ << endl;
	for( auto it = begin(obj.coeffs_); it != end(obj.coeffs_); ++it )
		for( auto& x : (*it) )
			os << x << "\t";
	os << endl;

	return os;
}

istream& operator >>(std::istream& is, ARMulti& obj)
{
	is >> obj.nPredictors_;
	obj.nSeries_ = obj.nPredictors_ + 1;
	is >> obj.filterLen_;
	obj.coeffs_ = vector<vector<float> >(obj.nPredictors_, vector<float>(obj.filterLen_));
	for( int p = 0; p < obj.nPredictors_; ++p )
		for( int i = 0; i < obj.filterLen_; ++i )
			is >> obj.coeffs_[p][i];
	return is;
}

} // namespace gtlib
