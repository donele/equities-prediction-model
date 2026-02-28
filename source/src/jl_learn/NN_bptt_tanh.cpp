#include <jl_learn/NN_bptt_tanh.h>
#include "TRandom3.h"
#include "math.h"
#include <iostream>
#include <fstream>
using namespace std;

NN_bptt_tanh::NN_bptt_tanh(string filename)
:NN(filename),
outScale_(1)
{
	if( filename.find("maxProfit") != string::npos )
		maxRet_ = false;
}

NN_bptt_tanh::NN_bptt_tanh(vector<int>& N, double learn, double maxWgt, int verbose, double biasOut, double costMult,
				double sigP, double poslim, double maxPos, int lotSize, bool maxRet)
:NN(N, learn, maxWgt, verbose, biasOut, sigP, poslim, maxPos, costMult, lotSize, true),
last_mid_(0),
outScale_(1),
maxRet_(maxRet),
F_(0),
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

NN_bptt_tanh::~NN_bptt_tanh()
{}

void NN_bptt_tanh::start_ticker(double position)
{
	last_mid_ = 0;
	P_p_ = position;

	// p_dFdw_.
	p_dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		p_dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	return;
}

void NN_bptt_tanh::set_position(double position)
{
	P_p_ = position;
	return;
}

vector<double> NN_bptt_tanh::forward(vector<double>& input, QuoteInfo& quote)
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
	double error = 0;
	double net = 0;
	//for( int i=1; i<N_[L_ - 1]; ++i )
	int i = 1; // only one output node.
	{
		for( int j=0; j<N_[L_ - 2]; ++j )
			net += w_[L_ - 2][i][j] * ff_[L_ - 2][j];

		if( net >= 0 ) // buy
		{
			F_ = quote_.askSize * lotSize_ * tanh(outScale_ * net) + P_p_;
			error = quote_.askSize * lotSize_ * outScale_ * (1.0 - tanh(outScale_ * net) * tanh(outScale_ * net));
		}
		else // sell
		{
			F_ = quote_.bidSize * lotSize_ * tanh(outScale_ * net) + P_p_;
			error = quote_.bidSize * lotSize_ * outScale_ * (1.0 - tanh(outScale_ * net) * tanh(outScale_ * net));
		}
		ff_[L_ - 1][i] = F_;
	}

	P_ = ceil( F_ - 0.5 ); // Position.

	// maxPos adjust.
	if( P_ > maxPos_ )
		P_ = maxPos_;
	else if( P_ < -maxPos_ )
		P_ = -maxPos_;

	double R0 = get_R0(); // R0 = (z_t - z_t-1) / z_t-1.
	double R = get_R(); // R = R0 + |F_t - F_t-1| * spread / 2.0.

	// Set the error of the output layer.
	e_[L_ - 1][1] = error;

	vector<double> ret;
	ret.push_back(tanh(net));
	ret.push_back(P_);
	ret.push_back(R0);
	ret.push_back(R);

	return ret;
}

vector<double> NN_bptt_tanh::forward_trade(vector<double>& input, QuoteInfo& quote)
{
	vector<double> ret = forward(input, quote);
	last_mid_ = (quote_.bid + quote_.ask) / 2.0;
	//P_p_ = P_;
	return ret;
}

void NN_bptt_tanh::backprop(double error)
{
// Node errors.

	// The error of the output layer is set in the function train().

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
	double pRpF = get_pRpF();
	double pRpF_p = get_pRpF_p();

	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double val = pRpF * dFdw_[l][i][j] + pRpF_p * p_dFdw_[l][i][j];
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
						printf("NN_bptt_tanh dw %.2f w %.2f layer %d iNode %d jNode %d\n", dw, w_[l][i][j], l, i, j);
						cout << flush;
						if( ++nWgtWarnings_ >= 10 )
							cout << "NN_bptt_tanh supressing further warnings on large weights." << endl;
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

double NN_bptt_tanh::get_R()
{
	double R0 = get_R0();
	double g = get_g();
	double R = R0 - g;

	return R;
}

double NN_bptt_tanh::get_R0()
{
	double mid = (quote_.bid + quote_.ask) / 2.0;
	double R0 = 0;
	if( last_mid_ > 1e-6 )
	{
		R0 = P_p_ * (mid - last_mid_) / last_mid_;
	}
	return R0;
}

double NN_bptt_tanh::get_g()
{
	double D = (quote_.ask - quote_.bid) * costMult_;
	double g = 0;
	double mid = (quote_.bid + quote_.ask) / 2.0;
	if( mid > 1e-6 )
		g = fabs(P_ - P_p_) * D / 2.0 / mid;
	return g;
}

double NN_bptt_tanh::get_pRpF()
{
	double pgpF = get_pgpF();
	return - pgpF;
}

double NN_bptt_tanh::get_pgpF()
{
	double D = (quote_.ask - quote_.bid) * costMult_;
	double pgpF = 0;
	if( fabs(P_ - P_p_) > 0.5 )
	{
		if( maxRet_ )
		{
			double mid = (quote_.bid + quote_.ask) / 2.0;
			double v = D / 2.0 / mid;
			pgpF = (P_ >= P_p_) ? v : -v;
		}
		else
		{
			double v = D / 2.0;
			pgpF = (P_ >= P_p_) ? v : -v;
		}
	}
	return pgpF;
}

double NN_bptt_tanh::get_pRpF_p()
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

	double pgpF_p = get_pgpF_p();
	double ret = r - pgpF_p - posFactor;

	return ret;
}

double NN_bptt_tanh::get_pgpF_p()
{
	double ret = -get_pgpF();
	return ret;
}

double NN_bptt_tanh::get_pFpF_p()
{
	double sum = 0;
	int l = 0; // layer.
	{
		int j = 1; // last position.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double val = e_[l+1][i] * w_[l][i][j];
				sum += val;
			}
		}
	}
	//double ret = sum / sigP_ + 1.0;
	double ret = sum / sigP_;
	return ret;
}
