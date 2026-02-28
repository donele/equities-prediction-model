#include <jl_learn/NN_bpnn.h>
#include "math.h"
#include <iostream>
#include <fstream>
using namespace std;

NN_bpnn::NN_bpnn(string filename)
:NN(filename)
{}

NN_bpnn::NN_bpnn(vector<int>& N, double learn, double maxWgt, int verbose, double biasOut)
:NN(N, learn, maxWgt, verbose, biasOut, 0, 0, 0, 1, 1, false)
{}

NN_bpnn::~NN_bpnn()
{}

vector<double> NN_bpnn::forward(vector<double>& input, QuoteInfo& quote)
{
	// input layer bias node.
	ff_[0][0] = 1.0;

	// input layer other nodes.
	int ninputs = input.size();
	for( int i=0; i<ninputs; ++i )
		ff_[0][i+1] = input[i];

	// hidden layers.
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

	vector<double> output;
	// output layer.
	// bias node, not used.
	ff_[L_ - 1][0] = 0;
	// other nodes.
	for( int i=1; i<N_[L_ - 1]; ++i )
	{
		double net = 0;
		for( int j=0; j<N_[L_ - 2]; ++j )
			net += w_[L_ - 2][i][j] * ff_[L_ - 2][j];
		ff_[L_ - 1][i] = net;
		output.push_back(ff_[L_ - 1][i]);
	}

	return output;
}

vector<double> NN_bpnn::forward_trade(vector<double>& input, QuoteInfo& quote)
{
	vector<double> ret = forward(input, quote);
	return ret;
}

void NN_bpnn::backprop(double error)
{
	// error of the output layer.
	for( int j=1; j<N_[L_ - 1]; ++j )
		e_[L_ - 1][j] = error; 

	// error of the hidden layers.
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

	// update dw_.
	for( int l=L_ - 2; l>=0; --l )
	{
		for( int j=0; j<N_[l]; ++j ) // this layer.
		{
			for( int i=1; i<N_[l+1]; ++i ) // upper layer. ignore the bias node.
			{
				double dw = e_[l+1][i] * ff_[l][j];
				dw_[l][i][j] += dw;
			}
		}
	}

	// update weights.
	for( int l=0; l<L_ - 1; ++l )
		for( int i=1; i<N_[l+1]; ++i )
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
						printf("NN_bpnn dw %.2f w %.2f layer %d iNode %d jNode %d\n", dw, w_[l][i][j], l, i, j);
						cout << flush;
						if( ++nWgtWarnings_ >= 10 )
							cout << "NN_bpnn supressing further warnings on large weights." << endl;
					}
				}
			}

	return;
}
