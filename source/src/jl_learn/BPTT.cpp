#include <jl_learn/BPTT.h>
#include "TRandom3.h"
#include "math.h"
#include <iostream>
#include <fstream>
using namespace std;

BPTT::BPTT()
{}

BPTT::BPTT(string filename)
:biasOut_(1),
sigP_(1)
{
	ifstream ifs(filename.c_str());
	if( ifs.is_open() )
	{
		// Read the file
		ifs >> L_;

		N_ = vector<int>(L_);
		for( int l=0; l<L_; ++l )
			ifs >> N_[l];

		// weights.
		// w_[l] is a matrix of (number of nodes on the upper layer * number of nodes on the lower layer).
		w_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l=0; l<L_ - 1; ++l )
			w_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		for( int l=0; l<L_ - 1; ++l )
			for( int i=1; i<N_[l+1]; ++i )
				for( int j=0; j<N_[l]; ++j )
					ifs >> w_[l][i][j];

		// number of weights.
		for( int l=0; l<L_ - 1; ++l )
			nwgts_ += (N_[l + 1] - 1) * (N_[l]);
	}
}

BPTT::BPTT(vector<int>& N, double learn, double maxWgt, int verbose, int useBias)
:learn_(learn),
maxWgt_(maxWgt),
biasOut_(1),
verbose_(verbose),
useBias_(useBias),
poslim_(0),
nwgts_(0),
F_p_(0),
last_mid_(0),
P_(0),
sigP_(1)
{
	// Number of nodes per layer. Add bias nodes.
	N_ = vector<int>(N.begin(), N.end());
	for( vector<int>::iterator it = N_.begin(); it != N_.end(); ++it )
		*it += 1;
	++N_[0]; // previous output is added to the input vector.

	// Number of layers from input to output.
	L_ = N_.size();

	// Local learning rates.
	vector<double> v(L_ - 1);
	local_learn_ = vector<double>(L_ - 1);
	for( int l = L_ - 2; l >= 0; --l )
	{
		if( l == L_ - 2 )
			v[l] = 1.0 / N_[l];
		else
			v[l] = static_cast<double>(N_[l+1]) * v[l+1] / N_[l];

		local_learn_[l] = 1.0 / ( N_[l] * sqrt(v[l]) );
	}

	// number of weights.
	// Subtract one from the upper layer, as weight into bias node is not used.
	for( int l=0; l<L_ - 1; ++l )
		nwgts_ += (N_[l + 1] - 1) * (N_[l]);

	// node outputs.
	// l=0: input nodes, 0<l<L_-1: hidden nodes, l=L_-1: output nodes.
	// Each layer includes a bias node with output = 1.
	ff_ = vector<vector<double> >(L_, vector<double>());
	for( int l=0; l<L_; ++l )
		ff_[l] = vector<double>(N_[l], 0);

	// errors.
	// l=0: input nodes (not used), 0<l<L_-1: hidden nodes, l=L_-1: output nodes.
	// Each layer includes a bias node that is not used.
	e_ = vector<vector<double> >(L_, vector<double>());
	for( int l=0; l<L_; ++l )
		e_[l] = vector<double>(N_[l], 0);

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

	// weights.
	// w_[l] is a matrix of (number of nodes on the upper layer * number of nodes on the lower layer).
	w_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		w_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	// weight changes.
	dw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		dw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	init_wgts();
}

BPTT::~BPTT()
{
	delete tr_;
}

void BPTT::init_wgts()
{
	// initialize the weights.
	tr_ = new TRandom3(0);
	for( int l=0; l<L_ - 1; ++l )
	{
		for( int i=1; i<N_[l+1]; ++i )
		{
			double range = 1.0 / sqrt((double)(N_[l]));
			for( int j=0; j<N_[l]; ++j )
			{
				if( j == 0 )
					w_[l][i][j] = 2.0*(tr_->Rndm() - 0.5) * range;
				else
					w_[l][i][j] = 2.0*(tr_->Rndm() - 0.5) * range;
			}
		}
	}

	return;
}

void BPTT::start_ticker(double position)
{
	P_p_ = position;

	// p_dFdw_.
	p_dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		p_dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	return;
}

void BPTT::set_position(double position)
{
	P_p_ = position;
	return;
}

vector<double> BPTT::forward(vector<double>& input, QuoteInfo& quote)
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
	P_ = F_ * sigP_;
	if( P_ > P_p_ + quote_.askSize )
		P_ = P_p_ + quote_.askSize;
	else if( P_ < P_p_ - quote_.bidSize )
		P_ = P_p_ - quote_.bidSize;
	else
		P_ = ceil( P_ - 0.5 );
// R.
	double R = get_R();

	vector<double> ret;
	ret.push_back(F_);
	ret.push_back(P_);
	ret.push_back(R);
	return ret;
}

void BPTT::backprop()
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
				//if( maxWgt_ > 0 && fabs(val) > maxWgt_ )
				//{
				//	cout << val << endl;
				//}
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
				//if( maxWgt_ > 0 && fabs(val) > maxWgt_ )
				//{
				//	cout << val << endl;
				//}
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
				//if( maxWgt_ > 0 && fabs(val) > maxWgt_ )
				//{
				//	cout << "wgt " << val << endl;
				//}
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
				if( j == 0 ) // bias nodes
					w_[l][i][j] += useBias_ * learn_ * local_learn_[l] * dw_[l][i][j];
				else
					w_[l][i][j] += learn_ * local_learn_[l] * dw_[l][i][j];
			}
		}
	}

	p_dFdw_ = dFdw_;
	last_mid_ = (quote_.bid + quote_.ask) / 2.0;
	P_p_ = P_;

	return;
}

double BPTT::get_R()
{
	double mid = (quote_.bid + quote_.ask) / 2.0;
	double r = 0;
	if( last_mid_ > 0 )
		r = (mid - last_mid_) / last_mid_;

	double g = get_g();

	double R = P_p_ * r - g - poslim_ * P_p_ * P_p_;

	return R;
}

double BPTT::get_g()
{
	double g = 0;
	double sprd = (quote_.ask - quote_.bid) / (quote_.ask + quote_.bid); // half the spread.
	if( P_ >= P_p_ - quote_.bidSize && P_ <= P_p_ + quote_.askSize )
		g = fabs( P_ - P_p_ ) * sprd;
	else
		g = (P_ - P_p_ - quote_.askSize) * (P_ - P_p_ + quote_.bidSize)
			* (P_ * P_ - (2 * P_p_ - quote_.bidSize + quote_.askSize) * P_
				+ sprd / (quote_.bidSize + quote_.askSize) + (P_p_ - quote_.bidSize) * (P_p_ + quote_.askSize));
	return g;
}

double BPTT::get_pRpP()
{
	double ret = - get_pgpP();
	return ret;
}

double BPTT::get_pRpP_p()
{
	double mid = (quote_.bid + quote_.ask) / 2.0;
	double r = 0;
	if( last_mid_ > 0 )
		r = (mid - last_mid_) / last_mid_;
	double pgpP_p = get_pgpP_p();
	double ret = r - pgpP_p - 2.0 * poslim_ * P_p_;
	return ret;
}

double BPTT::get_pgpP()
{
	double sprd = (quote_.ask - quote_.bid) / (quote_.ask + quote_.bid); // half the spread.
	double ret = 0;
	if( P_ >= P_p_ - quote_.bidSize && P_ <= P_p_ + quote_.askSize )
	{
		ret = (P_ - P_p_ > 0)?sprd:-sprd;
	}
	else
	{
		double X = P_ * P_ - (2.0 * P_p_ - quote_.bidSize + quote_.askSize) * P_ + sprd / (quote_.askSize + quote_.bidSize)
			+ (P_p_ - quote_.bidSize) * (P_p_ + quote_.askSize);
		ret = (P_ - P_p_ + quote_.bidSize) * X
			+ (P_ - P_p_ - quote_.askSize) * X
			+ (P_ - P_p_ - quote_.askSize) * (P_ - P_p_ + quote_.bidSize) * (2.0 * P_ - 2.0 * P_p_ + quote_.bidSize - quote_.askSize);
	}
	return ret;
}

double BPTT::get_pgpP_p()
{
	double sprd = (quote_.ask - quote_.bid) / (quote_.ask + quote_.bid);
	double ret = 0;
	if( P_ >= P_p_ - quote_.bidSize && P_ <= P_p_ + quote_.askSize )
	{
		ret = (P_ - P_p_ > 0)?-sprd:sprd;
	}
	else
	{
		double X = P_ * P_ - (2.0 * P_p_ - quote_.bidSize + quote_.askSize) * P_ + sprd / (quote_.askSize + quote_.bidSize)
			+ (P_p_ - quote_.bidSize) * (P_p_ + quote_.askSize);
		ret = -(P_ - P_p_ + quote_.bidSize) * X
			- (P_ - P_p_ - quote_.askSize) * X
			+ (-2.0 * P_ + 2.0 * P_p_ - quote_.bidSize + quote_.askSize);
	}
	return ret;
}

double BPTT::get_pFpF_p()
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


int BPTT::get_nwgts()
{
	return nwgts_;
}

vector<double> BPTT::get_wgts()
{
	vector<double> wgts;
	for( int l=0; l<L_ - 1; ++l )
		for( int i=1; i<N_[l+1]; ++i )
			for( int j=0; j<N_[l]; ++j )
				wgts.push_back(w_[l][i][j]);

	return wgts;
}

vector<int> BPTT::get_nnodes()
{
	vector<int> nnodes;
	for( vector<int>::iterator it = N_.begin(); it != N_.end(); ++it )
		nnodes.push_back(*it);
	return nnodes;
}

void BPTT::save()
{
	wOld_ = w_;
	return;
}

void BPTT::restore()
{
	w_ = wOld_;
	return;
}

void BPTT::scale_learn(double f)
{
	learn_ *= f;
	cout << "BPTT::scale_learn " << f << endl;
	return;
}


ostream& operator <<(ostream& os, BPTT& obj)
{
	vector<int> nnodes = obj.get_nnodes();
	vector<double> wgts = obj.get_wgts();
	int nlayers = nnodes.size();

	os << nlayers << "\t";
	copy( nnodes.begin(), nnodes.end(), ostream_iterator<int>(os, "\t"));
	os.precision(4);
	copy( wgts.begin(), wgts.end(), ostream_iterator<double>(os, "\t"));

	return os;
}
