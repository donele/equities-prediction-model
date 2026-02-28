#include <jl_learn/NN.h>
#include "math.h"
#include <jl_learn/Random.h>
#include <iostream>
#include <fstream>
using namespace std;

NN::NN()
{}

NN::NN(string filename)
:biasOut_(1),
nWgtWarnings_(0),
verbose_(0)
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

		ifs >> sigP_ >> poslim_ >> maxPos_ >> costMult_ >> lotSize_;
		if( sigP_ < 1 )
			sigP_ = 100;
		if( maxPos_ < 1 )
			maxPos_ = 1000000;
		if( costMult_ < 1e-6 )
			costMult_ = 1.0;
		if( lotSize_ < 1 )
			lotSize_ = 100;

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

		// weight changes.
		dw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l=0; l<L_ - 1; ++l )
			dw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));
	}
}

NN::NN(vector<int>& N, double learn, double maxWgt, int verbose, double biasOut, double sigP, double poslim,
	   double maxPos, double costMult, int lotSize, bool recurrent)
:learn_(learn),
maxWgt_(maxWgt),
biasOut_(biasOut),
useBias_(true),
sigP_(sigP),
poslim_(poslim),
maxPos_(maxPos),
costMult_(costMult),
lotSize_(lotSize),
verbose_(verbose),
nwgts_(0)
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

NN::~NN()
{}

void NN::init_wgts()
{
	// initialize the weights.
	//tr_ = new TRandom3(0);
	for( int l=0; l<L_ - 1; ++l )
	{
		for( int i=1; i<N_[l+1]; ++i )
		{
			double range = 1.0 / sqrt((double)(N_[l]));
			for( int j=0; j<N_[l]; ++j )
			{
				w_[l][i][j] = Random::Instance()->next(range);
				//w_[l][i][j] = 2.0*(tr_->Rndm() - 0.5) * range;
			}
		}
	}
	//delete tr_;

	return;
}

int NN::get_nwgts()
{
	return nwgts_;
}

vector<double> NN::get_wgts()
{
	vector<double> wgts;
	for( int l=0; l<L_ - 1; ++l )
		for( int i=1; i<N_[l+1]; ++i )
			for( int j=0; j<N_[l]; ++j )
				wgts.push_back(w_[l][i][j]);

	return wgts;
}

vector<int> NN::get_nnodes()
{
	vector<int> nnodes;
	for( vector<int>::iterator it = N_.begin(); it != N_.end(); ++it )
		nnodes.push_back(*it);
	return nnodes;
}

double NN::sigP()
{
	return sigP_;
}

double NN::poslim()
{ 
	return poslim_;
}

double NN::maxPos()
{
	return maxPos_;
}

double NN::costMult()
{
	return costMult_;
}

int NN::lotSize()
{
	return lotSize_;
}

void NN::save()
{
	wOld_ = w_;
	return;
}

void NN::restore()
{
	w_ = wOld_;
	return;
}

void NN::scale_learn(double f)
{
	learn_ *= f;
	cout << "NN::scale_learn " << f << endl;
	return;
}

ostream& operator <<(ostream& os, NN& obj)
{
	vector<int> nnodes = obj.get_nnodes();
	vector<double> wgts = obj.get_wgts();
	int nlayers = nnodes.size();

	os << nlayers << "\t";
	copy( nnodes.begin(), nnodes.end(), ostream_iterator<int>(os, "\t"));
	os << endl;

	char buf[200];
	for( int i=0; i<wgts.size(); ++i )
	{
		sprintf( buf, "%15.10f\t", wgts[i] );
		os << buf;
	}
	os << endl;

	os << obj.sigP() << "\t" << obj.poslim() << "\t" << obj.maxPos() << "\t" << obj.costMult() << "\t" << obj.lotSize();
	os << endl;

	return os;
}
