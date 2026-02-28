#include <HLearnObj/BPNNbatch.h>
#include <math.h>
#include <iostream>
#include <fstream>
using namespace std;

namespace hnn {
	BPNNbatch::BPNNbatch()
	{}

	BPNNbatch::BPNNbatch(const BPNNbatch & rhs)
	:NNBase(rhs)
	{}

	BPNNbatch::BPNNbatch(vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut)
	:NNBase(N, learn, wgtDecay, maxWgt, verbose, biasOut, /*0, 0, 0, 1, 1, */false)
	{}

	BPNNbatch::~BPNNbatch()
	{}

	vector<double> BPNNbatch::train(const vector<float>& input, const QuoteInfo& quote, const double target)
	{
		vector<double> output = forward(input, quote);
		double error = target - output[0];
		backprop(error);
		return output;
	}

	vector<double> BPNNbatch::trading_signal(const vector<float>& input, const QuoteInfo& quote, const double lastPos)
	{
		vector<double> output = forward(input, quote);
		return output;
	}

	std::ostream& BPNNbatch::write(ostream& str) const
	{
		vector<int> nnodes = get_nnodes();
		vector<double> wgts = get_wgts();
		int nlayers = nnodes.size();

		str << nlayers << "\t";
		copy( nnodes.begin(), nnodes.end(), ostream_iterator<int>(str, "\t"));
		str << endl;

		char buf[200];
		for( int i = 0; i < wgts.size(); ++i )
		{
			sprintf( buf, "%15.10f\t", wgts[i] );
			str << buf;
		}
		str << endl;

		return str;
	}

	std::istream& BPNNbatch::read(istream& str)
	{
		str >> L_;

		N_ = vector<int>(L_);
		for( int l = 0; l < L_; ++l )
			str >> N_[l];

		// weights.
		// w_[l] is a matrix of (number of nodes on the upper layer * number of nodes on the lower layer).
		w_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			w_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		for( int l = 0; l < L_ - 1; ++l )
			for( int i = 1; i < N_[l + 1]; ++i )
				for( int j = 0; j < N_[l]; ++j )
					str >> w_[l][i][j];

		// number of weights.
		for( int l = 0; l < L_ - 1; ++l )
			nwgts_ += (N_[l + 1] - 1) * (N_[l]);

		// node outputs.
		// l = 0: input nodes, 0<l < L_-1: hidden nodes, l = L_-1: output nodes.
		// Each layer includes a bias node with output = 1.
		ff_ = vector<vector<double> >(L_, vector<double>());
		for( int l = 0; l < L_; ++l )
			ff_[l] = vector<double>(N_[l], 0);

		// errors.
		// l = 0: input nodes (not used), 0<l < L_-1: hidden nodes, l = L_-1: output nodes.
		// Each layer includes a bias node that is not used.
		e_ = vector<vector<double> >(L_, vector<double>());
		for( int l = 0; l < L_; ++l )
			e_[l] = vector<double>(N_[l], 0);

		// weight changes.
		dw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			dw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		return str;
	}

	vector<double> BPNNbatch::forward(const vector<float>& input, const QuoteInfo& quote)
	{
		// input layer bias node.
		ff_[0][0] = 1.0;

		// input layer other nodes.
		int ninputs = input.size();
		for( int i = 0; i < ninputs; ++i )
			ff_[0][i + 1] = input[i];

		// hidden layers.
		for( int l = 1; l < L_ - 1; ++l )
		{
			// bias node.
			ff_[l][0] = biasOut_;

			// other nodes.
			for( int i = 1; i < N_[l]; ++i )
			{
				double net = 0;
				for( int j = 0; j < N_[l - 1]; ++j )
					net += w_[l - 1][i][j] * ff_[l - 1][j];
				ff_[l][i] = tanh(net);
			}
		}

		vector<double> output;
		// output layer.
		// bias node, not used.
		ff_[L_ - 1][0] = 0;
		// other nodes.
		for( int i = 1; i < N_[L_ - 1]; ++i )
		{
			double net = 0;
			for( int j = 0; j < N_[L_ - 2]; ++j )
				net += w_[L_ - 2][i][j] * ff_[L_ - 2][j];
			ff_[L_ - 1][i] = net;
			output.push_back(ff_[L_ - 1][i]);
		}

		return output;
	}

	void BPNNbatch::backprop(double error)
	{
		// error of the output layer.
		for( int j = 1; j < N_[L_ - 1]; ++j )
			e_[L_ - 1][j] = error; 

		// error of the hidden layers.
		for( int l = L_ - 2; l > 0; --l )
		{
			for( int j = 1; j < N_[l]; ++j ) // this layer. ignore the bias node.
			{
				double this_err = 0;
				for( int i = 1; i < N_[l + 1]; ++i ) // upper layer. ignore the bias node.
					this_err += e_[l + 1][i] * w_[l][i][j];
				e_[l][j] = this_err * (1.0 - ff_[l][j] * ff_[l][j]);
			}
		}

		// update dw_.
		for( int l = L_ - 2; l >= 0; --l )
		{
			for( int j = 0; j < N_[l]; ++j ) // this layer.
			{
				for( int i = 1; i < N_[l + 1]; ++i ) // upper layer. ignore the bias node.
				{
					double dw = e_[l + 1][i] * ff_[l][j];
					dw_[l][i][j] += dw;
				}
			}
		}

		return;
	}
}