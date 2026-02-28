#include <HLearnObj/NNBase.h>
#include <jl_lib/Random.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <ctime>
using namespace std;

namespace hnn {
	NNBase::NNBase()
	:biasOut_(1),
	nWgtWarnings_(0),
	maxWgt_(10),
	learn_(0.1),
	wgtDecay_(0),
	verbose_(0),
	L_(0)
	{}

	NNBase::NNBase(const NNBase& rhs)
	{
		nWgtWarnings_ = 0;
		learn_ = rhs.learn_;
		maxWgt_= rhs.maxWgt_;
		verbose_ = rhs.verbose_;
		useBias_ = rhs.useBias_;
		biasOut_ = rhs.biasOut_;
		wgtDecay_ = rhs.wgtDecay_;

		L_ = rhs.L_;
		local_learn_ = rhs.local_learn_;
		N_ = rhs.N_;
		ff_ = rhs.ff_;
		e_ = rhs.e_;
		dw_ = rhs.dw_;
		w_ = rhs.w_;
		wOld_ = rhs.wOld_;
		nwgts_ = rhs.nwgts_;

		reset_dw();
	}

	NNBase::NNBase(vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut, bool recurrent)
	:learn_(learn),
	wgtDecay_(wgtDecay * learn),
	maxWgt_(maxWgt),
	biasOut_(biasOut),
	useBias_(true),
	verbose_(verbose)
	{
		if( fabs(biasOut_) < 1e-10 )
			useBias_ = false;

		// Number of nodes per layer. Add bias nodes.
		N_ = vector<int>(N.begin(), N.end());
		for( vector<int>::iterator it = N_.begin(); it != N_.end(); ++it )
			*it += 1;

		if( recurrent )
		{
			++N_[0]; // previous output is added to the input vector.
		}

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
				v[l] = static_cast<double>(N_[l + 1]) * v[l + 1] / N_[l];

			local_learn_[l] = 1.0 / ( N_[l] * sqrt(v[l]) );
		}

		// number of weights.
		// Subtract one from the upper layer, as weight into bias node is not used.
		nwgts_ = 0;
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

		// weights.
		// w_[l] is a matrix of (number of nodes on the upper layer * number of nodes on the lower layer).
		w_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			w_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		// weight changes.
		dw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			dw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		init_wgts();
	}

	NNBase::~NNBase()
	{}

	void NNBase::init_wgts()
	{
		// initialize the weights.
		for( int l = 0; l < L_ - 1; ++l )
		{
			for( int i = 1; i < N_[l + 1]; ++i )
			{
				double range = 1.0 / sqrt((double)(N_[l]));
				for( int j = 0; j < N_[l]; ++j )
					w_[l][i][j] = Random::Instance()->next(range);
			}
		}

		return;
	}

	void NNBase::reset_dw()
	{
		for( int l = 0; l < L_ - 1; ++l ) for( int i = 1; i < N_[l + 1]; ++i ) for( int j = 0; j < N_[l]; ++j )
			dw_[l][i][j] = 0;
		return;
	}

	void NNBase::update_dw(const NNBase& rhs)
	{
		const vector<vector<vector<double> > >& rhs_dw = rhs.get_dw();
		//{
			//boost::mutex::scoped_lock lock(dw_mutex_);
			for( int l = 0; l < L_ - 1; ++l ) for( int i = 1; i < N_[l + 1]; ++i ) for( int j = 0; j < N_[l]; ++j )
				dw_[l][i][j] += rhs_dw[l][i][j];
		//}
		return;
	}

	bool NNBase::weights_valid()
	{
		int nValid = 0;
		int nInvalid = 0;
		int nInd = 0;
		for( int l = 0; l < L_ - 1; ++l ) for( int i = 1; i < N_[l + 1]; ++i ) for( int j = 0; j < N_[l]; ++j )
		{
			if( fabs(w_[l][i][j]) > 0. && fabs(w_[l][i][j]) < maxWgt_  )
				++nValid;
			else
				++nInvalid;

			if( w_[l][i][j] != w_[l][i][j] ) // checks for indefiniteness.
				++nInd;
		}
		bool valid = nValid > nInvalid && nInd == 0;
		return valid;
	}

	void NNBase::update_wgts()
	{
		// update weights.
		for( int l = 0; l < L_ - 1; ++l ) for( int i = 1; i < N_[l + 1]; ++i ) for( int j = 0; j < N_[l]; ++j )
		{
			if( !useBias_ && j == 0 ) // bias nodes
				w_[l][i][j] = 0;
			else
			{
				double dw = learn_ * (local_learn_[l] * dw_[l][i][j] - wgtDecay_ * w_[l][i][j]);
				dw_[l][i][j] = 0;
				w_[l][i][j] += dw;
				if( maxWgt_ > 0 && nWgtWarnings_ < 10 && fabs(dw) > maxWgt_ )
				{
					char buf[200];
					if( fabs(dw) > 1e4 || fabs(w_[l][i][j]) > 1e4 )
						sprintf(buf, " Invalid values. layer %d iNode %d jNode %d\n", l, i, j);
					else
						sprintf(buf, "dw %.2f w %.2f layer %d iNode %d jNode %d\n", dw, w_[l][i][j], l, i, j);
					cout << buf;
					if( ++nWgtWarnings_ >= 10 )
						cout << "suppressing further warnings on large weights.\n";
				}
			}
		}
		return;
	}

	const vector<vector<vector<double> > >& NNBase::get_dw() const
	{
		return dw_;
	}

	int NNBase::get_nwgts() const
	{
		return nwgts_;
	}

	vector<double> NNBase::get_wgts() const
	{
		vector<double> wgts;
		for( int l = 0; l < L_ - 1; ++l ) for( int i = 1; i < N_[l + 1]; ++i ) for( int j = 0; j < N_[l]; ++j )
			wgts.push_back(w_[l][i][j]);

		return wgts;
	}

	vector<int> NNBase::get_nnodes() const
	{
		vector<int> nnodes;
		for( vector<int>::const_iterator it = N_.begin(); it != N_.end(); ++it )
			nnodes.push_back(*it);
		return nnodes;
	}

	void NNBase::save()
	{
		wOld_ = w_;
		return;
	}

	void NNBase::restore()
	{
		w_ = wOld_;
		return;
	}

	void NNBase::scale_learn(double f)
	{
		learn_ *= f;
		cout << "NN::scale_learn " << f << endl;
		return;
	}

	void NNBase::set_learn(double learn)
	{
		learn_ = learn;
		return;
	}

	void NNBase::set_wgtDecay(double wgtDecay)
	{
		wgtDecay_ = wgtDecay;
		return;
	}

}

ostream& operator <<(ostream& os, hnn::NNBase& obj)
{
	return obj.write(os);
}

istream& operator >>(istream& is, hnn::NNBase& obj)
{
	return obj.read(is);
}