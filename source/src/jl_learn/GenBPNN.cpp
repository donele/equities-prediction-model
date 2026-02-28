#include <jl_learn/GenBPNN.h>
#include "TRandom3.h"
#include "math.h"
#include <iostream>
#include <fstream>
#include <iterator>

using namespace std;

GenBPNN::GenBPNN()
{}

GenBPNN::GenBPNN(string filename)
:biasOut_(1e-3)
{
	ifstream ifs(filename.c_str());
	if( ifs.is_open() )
	{
		ifs >> L_;

		N_ = vector<int>(L_);
		for( int l=0; l<L_; ++l )
			ifs >> N_[l];

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

		// outputs.
		f_ = vector<vector<double> >(L_, vector<double>());
		for( int l=0; l<L_; ++l )
			f_[l] = vector<double>(N_[l], 0);
	}

}

GenBPNN::GenBPNN(vector<int>& N, double learn, double maxWgt, int verbose, int useBias)
:lambda_(0.3),
learn_(learn),
maxWgt_(maxWgt),
biasOut_(1e-3),
verbose_(verbose),
useBias_(useBias),
nwgts_(0)
{
	// Number of nodes per layer. Add bias nodes.
	N_ = vector<int>(N.begin(), N.end());
	for( vector<int>::iterator it = N_.begin(); it != N_.end(); ++it )
		*it += 1;

	// Number of layers from input to output.
	L_ = N_.size();

	// Number of output nodes excluding the bias node.
	nOutput_ = N_[L_ - 1];

	// weights.
	// w_[l] is a matrix of (number of nodes on the upper layer X number of nodes on the lower layer).
	w_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		w_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	w2_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		w2_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	// number of weights.
	// Subtract one from the upper layer, as weight into bias node is not used.
	for( int l=0; l<L_ - 1; ++l )
		nwgts_ += (N_[l + 1] - 1) * (N_[l]);

	// weight updates.
	dw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		dw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	// grad for TD.
	grad_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
	for( int l=0; l<L_ - 1; ++l )
		grad_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

	// outputs.
	// l=0: input nodes, 0<l<L_-1: hidden nodes, l=L_-1: output nodes.
	// Each layer includes a bias node with output = 1.
	f_ = vector<vector<double> >(L_, vector<double>());
	for( int l=0; l<L_; ++l )
		f_[l] = vector<double>(N_[l], 0);

	f2_ = vector<vector<double> >(L_, vector<double>());
	for( int l=0; l<L_; ++l )
		f2_[l] = vector<double>(N_[l], 0);

	// errors.
	// l=0: input nodes (not used), 0<l<L_-1: hidden nodes, l=L_-1: output nodes.
	// Each layer includes a bias node that is not used.
	e_ = vector<vector<double> >(L_, vector<double>());
	for( int l=0; l<L_; ++l )
		e_[l] = vector<double>(N_[l], 0);

	// initialize the weights.
	tr_ = new TRandom3(0);
	for( int l=0; l<L_ - 1; ++l )
		for( int i=1; i<N_[l+1]; ++i )
			for( int j=0; j<N_[l]; ++j )
			{
				if( j == 0 )
					w_[l][i][j] = 2.0*(tr_->Rndm() - 0.5)*1e-6;
				else
					w_[l][i][j] = 2.0*(tr_->Rndm() - 0.5);
			}
}

GenBPNN::~GenBPNN()
{
	delete tr_;
}

void GenBPNN::train(vector<double>& input, vector<double>& target)
{
	vector<double> output = forward(input);
	vector<double> error(nOutput_);
	for( int i=0; i<nOutput_; ++i )
		error[i] = target[i] - output[i];
	backprop(error);

	return;
}

vector<double> GenBPNN::forward(vector<double>& input)
{
	// input layer bias node.
	f_[0][0] = 1.0;

	// input layer other nodes.
	int ninputs = input.size();
	for( int i=0; i<ninputs; ++i )
		f_[0][i+1] = input[i];

	// hidden layers.
	for( int l=1; l<L_ - 1; ++l )
	{
		// bias node.
		f_[l][0] = biasOut_;

		// other nodes.
		for( int i=1; i<N_[l]; ++i )
		{
			double net = 0;
			for( int j=0; j<N_[l-1]; ++j )
				net += w_[l-1][i][j] * f_[l-1][j];
			f_[l][i] = tanh(net);
		}
	}

	vector<double> output;
	// output layer.
	// bias node, not used.
	f_[L_ - 1][0] = 0;
	// other nodes.
	for( int i=1; i<N_[L_ - 1]; ++i )
	{
		double net = 0;
		for( int j=0; j<N_[L_ - 2]; ++j )
			net += w_[L_ - 2][i][j] * f_[L_ - 2][j];
		f_[L_ - 1][i] = net;
		output.push_back(f_[L_ - 1][i]);
	}

	return output;
}

void GenBPNN::backprop(vector<double>& error)
{
	// error of the output layer.
	for( int j=1; j<N_[L_ - 1]; ++j )
		e_[L_ - 1][j] = error[j - 1]; 

	// error of the hidden layers.
	for( int l=L_ - 2; l>0; --l )
	{
		for( int j=1; j<N_[l]; ++j ) // this layer. ignore the bias node.
		{
			double this_err = 0;
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
				this_err += e_[l+1][i] * w_[l][i][j];
			e_[l][j] = this_err * (1.0 - f_[l][j] * f_[l][j]);
		}
	}

	// update dw_.
	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double dw = e_[l+1][i] * f_[l][j];
				dw_[l][i][j] += dw;
			}
		}
	}

	return;
}

void GenBPNN::backpropTD(vector<double>& error)
{
	// Set the error of the output layer to one and calculate grad_. Use only one output node for simplicity.
	e_[L_ - 1][1] = 1.0; 

	// error of the hidden layers.
	for( int l=L_ - 2; l>0; --l )
	{
		for( int j=1; j<N_[l]; ++j ) // this layer. ignore the bias node.
		{
			double this_err = 0;
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
				this_err += e_[l+1][i] * w_[l][i][j];
			e_[l][j] = this_err * (1.0 - f_[l][j] * f_[l][j]);
		}
	}

	// update grad_.
	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double grad = e_[l+1][i] * f_[l][j];
				grad_[l][i][j] = grad + lambda_ * grad_[l][i][j];
			}
		}
	}

	// update dw_.
	double theError = error[0];
	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double dw = theError * grad_[l][i][j];
				dw_[l][i][j] += dw;
			}
		}
	}

	return;
}

void GenBPNN::reset_grad()
{
	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				grad_[l][i][j] = 0;
			}
		}
	}

	return;
}

void GenBPNN::update_wgt()
{
	// store the previous w_.
	for( int l=0; l<L_ - 1; ++l )
		for( int i=1; i<N_[l+1]; ++i )
			for( int j=0; j<N_[l]; ++j )
				w2_[l][i][j] = w_[l][i][j];

	// store the previous f_.
	for( int l=0; l<L_; ++l )
		for( int i=1; i<N_[l]; ++i )
			f2_[l][i] = f_[l][i];

	for( int l=0; l<L_ - 1; ++l )
		for( int i=1; i<N_[l+1]; ++i )
			for( int j=0; j<N_[l]; ++j )
			{
				double dw = 0;
				if( j == 0 )
					dw = useBias_ * learn_ * dw_[l][i][j];
				else
					dw = learn_ * dw_[l][i][j];
				w_[l][i][j] += dw;
				if( maxWgt_ > 0 && fabs(dw) > maxWgt_ )
				{
					printf("dw %.2f w %.2f layer %d iNode %d jNode %d\n", dw, w_[l][i][j], l, i, j);
					cout << flush;
				}
				dw_[l][i][j] = 0;
			}

	return;
}

void GenBPNN::alt()
{
	vector<vector<double> > temp_f = f_;
	f_ = f2_;
	f2_ = temp_f;

	vector<vector<vector<double> > > temp_w = w_;
	w_ = w2_;
	w2_ = temp_w;

	return;
}

void GenBPNN::save()
{
	wOld_ = w_;
	return;
}

void GenBPNN::restore()
{
	w_ = wOld_;
	return;
}

void GenBPNN::scale_learn(double f)
{
	learn_ *= f;
	cout << "GenBPNN::scale_learn " << f << endl;
	return;
}

int GenBPNN::get_nwgts()
{
	return nwgts_;
}

vector<double> GenBPNN::get_wgts()
{
	vector<double> wgts;
	for( int l=0; l<L_ - 1; ++l )
		for( int i=1; i<N_[l+1]; ++i )
			for( int j=0; j<N_[l]; ++j )
				wgts.push_back(w_[l][i][j]);

	return wgts;
}

vector<int> GenBPNN::get_nnodes()
{
	vector<int> nnodes;
	for( vector<int>::iterator it = N_.begin(); it != N_.end(); ++it )
		nnodes.push_back(*it);
	return nnodes;
}

ostream& operator <<(ostream& os, GenBPNN& obj)
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
