#include <jl_learn/NN_bptt.h>
#include "TRandom3.h"
#include "math.h"
#include <iostream>
#include <fstream>
using namespace std;

NN_bptt::NN_bptt(string filename)
:NN(filename),
maxRet_(true)
{
	if( filename.find("maxProfit") != string::npos )
		maxRet_ = false;
}

NN_bptt::NN_bptt(vector<int>& N, double learn, double maxWgt, int verbose, double biasOut, double costMult,
				 double sigP, double poslim, double maxPos, int lotSize, bool maxRet)
:NN(N, learn, maxWgt, verbose, biasOut, sigP, poslim, maxPos, costMult, lotSize, true),
last_mid_(0),
maxRet_(maxRet),
F_(0),
F_p_(0),
P_(0),
P_p_(0)
{
	// pFpw_.
	pFpw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		pFpw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	// dFdw_.
	dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	// p_dFdw_.
	p_dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		p_dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));
}

NN_bptt::~NN_bptt()
{}

void NN_bptt::start_ticker(double position)
{
	last_mid_ = 0;
	P_p_ = position;

	// p_dFdw_.
	p_dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		p_dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	return;
}

void NN_bptt::set_position(double position)
{
	P_p_ = position;
	return;
}

vector<double> NN_bptt::forward(vector<double>& input, QuoteInfo& quote)
{
	quote_ = quote;

// Input Layer.

	// input layer bias node.
	ff_[0][0] = biasOut_;

	// input layer second node = last output.
	ff_[0][1] = P_p_ / sigP_;

	// input layer other nodes.
	int ninputs = input.size();
	for( int i=0; i<ninputs; ++i )
		ff_[0][i+2] = input[i];

// Hidden layers.

	for( int l=1; l<L_ - 1; ++l )
	{
		// bias node.
		ff_[l][0] = biasOut_;

		// other nodes.
		for( int i=1; i<N_[l]; ++i )
		{
			double net = 0;
			for( int j=0; j<N_[l-1]; ++j )
				net += w_[l-1][i][j] * ff_[l-1][j];
			ff_[l][i] = tanh(net);
		}
	}

// Output layer.

	// bias node, not used.
	ff_[L_ - 1][0] = 0;

	// other nodes.
	for( int i=1; i<N_[L_ - 1]; ++i )
	{
		double net = 0;
		for( int j=0; j<N_[L_ - 2]; ++j )
			net += w_[L_ - 2][i][j] * ff_[L_ - 2][j];
		ff_[L_ - 1][i] = net;
	}

// output.
	F_ = ff_[L_ - 1][1];
	if( verbose_ > 3 )
		cout << " F_ " << F_ << endl;

// P_ = position.
	P_ = F_ * sigP_;
	if( P_ > P_p_ + quote_.askSize * lotSize_ )
		P_ = P_p_ + quote_.askSize * lotSize_;
	else if( P_ < P_p_ - quote_.bidSize * lotSize_ )
		P_ = P_p_ - quote_.bidSize * lotSize_;
	else
		P_ = ceil( P_ - 0.5 );

	// maxPos adjust.
	if( P_ > maxPos_ )
		P_ = maxPos_;
	else if( P_ < -maxPos_ )
		P_ = -maxPos_;

// R = return at time t.
	double R0 = get_R0();
	double R = get_R();

	vector<double> ret;
	ret.push_back(F_);
	ret.push_back(P_);
	ret.push_back(R0);
	ret.push_back(R);
	return ret;
}

vector<double> NN_bptt::forward_trade(vector<double>& input, QuoteInfo& quote)
{
	vector<double> ret = forward(input, quote);
	last_mid_ = (quote_.bid + quote_.ask) / 2.0;
	//P_p_ = P_;
	return ret;
}

void NN_bptt::backprop(double error)
{
// Node errors.

	// Set the error of the output layer to unity. Assume only one output node.
	e_[L_ - 1][1] = 1.0;

	// errors of the hidden layers.
	for( int l=L_ - 2; l>0; --l )
	{
		for( int j=1; j<N_[l]; ++j ) // this layer. ignore the bias node.
		{
			double this_err = 0;
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
				this_err += e_[l+1][i] * w_[l][i][j];
			e_[l][j] = this_err * (1.0 - ff_[l][j] * ff_[l][j]);
		}
	}

// Calculate pF/pw.
	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double val = e_[l+1][i] * ff_[l][j];
				pFpw_[l][i][j] = val;
			}
		}
	}	

// Calculate dF/dw.
	double pFpF_p = get_pFpF_p();

	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double val = 0;
				val = pFpw_[l][i][j] + pFpF_p * p_dFdw_[l][i][j];
				dFdw_[l][i][j] = val;
			}
		}
	}

// Calculate dw.
	double pRpP = get_pRpP();
	double pRpP_p = get_pRpP_p();

	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double val = pRpP * sigP_ * dFdw_[l][i][j] + pRpP_p * sigP_ * p_dFdw_[l][i][j];
				dw_[l][i][j] = val;
			}
		}
	}

// Update the weights.
	for( int l=0; l<L_ - 1; ++l )
	{
		for( int i=1; i<N_[l+1]; ++i )
		{
			for( int j=0; j<N_[l]; ++j )
			{
				if( !useBias_ && j == 0 ) // bias nodes
					w_[l][i][j] = 0;
				else
				{
					double dw = learn_ * local_learn_[l] * dw_[l][i][j];
					dw_[l][i][j] = 0;
					w_[l][i][j] += dw;
					if( maxWgt_ > 0 && nWgtWarnings_ < 10 && fabs(dw) > maxWgt_ )
					{
						printf("NN_bptt dw %.2f w %.2f layer %d iNode %d jNode %d\n", dw, w_[l][i][j], l, i, j);
						cout << flush;
						if( ++nWgtWarnings_ >= 10 )
							cout << "NN_bptt supressing further warnings on large weights." << endl;
					}
				}
			}
		}
	}

	p_dFdw_ = dFdw_;
	last_mid_ = (quote_.bid + quote_.ask) / 2.0;
	P_p_ = P_;

	return;
}

double NN_bptt::get_R()
{
	double R0 = get_R0();
	double g = get_g();
	double R = R0 - g;

	return R;
}

double NN_bptt::get_R0()
{
	double mid = (quote_.bid + quote_.ask) / 2.0;
	double R0 = 0;
	if( last_mid_ > 1e-6 )
	{
		R0 = P_p_ * (mid - last_mid_) / last_mid_;
	}
	return R0;
}

double NN_bptt::get_g()
{
	double g = 0;
	double D = (quote_.ask - quote_.bid) * costMult_;
	double mid = (quote_.bid + quote_.ask) / 2.0;
	if( mid > 1e-6 )
		g = fabs( P_ - P_p_ ) * D / 2.0 / mid;
	return g;
}

double NN_bptt::get_pRpP()
{
	double ret = - get_pgpP();
	return ret;
}

double NN_bptt::get_pgpP()
{
	double pgpP = 0;
	double D = (quote_.ask - quote_.bid) * costMult_;
	if( fabs(P_ - P_p_) > 0.5 )
	{
		double denom = 1.0;
		if( maxRet_ )
		{
			double mid = (quote_.bid + quote_.ask) / 2.0;
			denom = mid;
		}

		if( P_ >= P_p_ - quote_.bidSize * lotSize_ && P_ <= P_p_ + quote_.askSize * lotSize_ )
		{
			double v = D / 2.0 / denom;
			pgpP = (P_ - P_p_ > 0)?v:-v;
		}
		else
		{
			double X = P_ * P_
				- (2.0 * P_p_ - quote_.bidSize * lotSize_ + quote_.askSize * lotSize_) * P_
				+ D / 2.0 / denom / (quote_.askSize * lotSize_ + quote_.bidSize * lotSize_)
				+ (P_p_ - quote_.bidSize * lotSize_) * (P_p_ + quote_.askSize * lotSize_);
			pgpP = (P_ - P_p_ + quote_.bidSize * lotSize_) * X
				+ (P_ - P_p_ - quote_.askSize * lotSize_) * X
				+ (P_ - P_p_ - quote_.askSize * lotSize_) * (P_ - P_p_ + quote_.bidSize * lotSize_)
					* (2.0 * P_ - 2.0 * P_p_ + quote_.bidSize * lotSize_ - quote_.askSize * lotSize_);
		}
	}
	return pgpP;
}

double NN_bptt::get_pRpP_p()
{
	double mid = (quote_.bid + quote_.ask) / 2.0;

	double r = 0;
	if( last_mid_ > 1e-6 ) 
	{
		if( maxRet_ )
			r = (mid - last_mid_) / last_mid_;
		else
			r = mid - last_mid_;
	}

	double posFactor = 0;
	if( maxRet_ )
		posFactor = 2.0 * poslim_ * P_p_;
	else
		posFactor = 2.0 * poslim_ * mid * P_p_;

	double pgpP_p = get_pgpP_p();
	double ret = r - pgpP_p - posFactor;
	return ret;
}

double NN_bptt::get_pgpP_p()
{
	double pgpP_p = -get_pgpP();
	return pgpP_p;
}

double NN_bptt::get_pFpF_p()
{
	double ret = 0;
	int l = 0; // layer.
	{
		int j = 1; // last position.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double val = e_[l+1][i] * w_[l][i][j];
				ret += val;
			}
		}
	}	

	return ret;
}
