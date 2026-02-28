#include <HLearnObj/BPTTpred2in.h>
#include <HLib/HEvent.h>
#include <HLearnObj/InputInfo.h>
#include <math.h>
#include <iostream>
#include <fstream>
using namespace std;

namespace hnn {
	BPTTpred2in::BPTTpred2in()
	{
		get_iPred();
	}

	BPTTpred2in::BPTTpred2in(const BPTTpred2in& rhs)
	:NNBase(rhs)
	{
		sigP_ = rhs.sigP_;
		poslim_ = rhs.poslim_;
		maxPos_ = rhs.maxPos_;
		maxPosChg_ = rhs.maxPosChg_;
		costMult_ = rhs.costMult_;
		lotSize_ = rhs.lotSize_;
		maxRet_ = rhs.maxRet_;
		noTwoTradesAtSamePrice_ = rhs.noTwoTradesAtSamePrice_;

		last_mid_ = 0;
		last_trade_price_ = 0;
		trade_aborted_ = false;
		F_ = 0;
		F_p_ = 0;
		P_ = 0;
		P_p_ = 0;
		pFpw_ = rhs.pFpw_;
		dFdw_ = rhs.dFdw_;
		p_dFdw_ = rhs.p_dFdw_;

		get_iPred();
	}

	BPTTpred2in::BPTTpred2in(vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut, double costMult,
				double sigP, double poslim, double maxPos, double maxPosChg, int lotSize, bool maxRet, bool noTwoTradesAtSamePrice)
	:NNBase(N, learn, wgtDecay, maxWgt, verbose, biasOut, true),
	sigP_(sigP),
	poslim_(poslim),
	maxPos_(maxPos),
	maxPosChg_(maxPosChg),
	costMult_(costMult),
	lotSize_(lotSize),
	maxRet_(maxRet),
	noTwoTradesAtSamePrice_(noTwoTradesAtSamePrice),
	last_mid_(0),
	trade_aborted_(false),
	F_(0),
	F_p_(0),
	P_(0),
	P_p_(0)
	{
		init_vectors();

		get_iPred();
	}

	void BPTTpred2in::init_vectors()
	{
		// pFpw_.
		pFpw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			pFpw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		// dFdw_.
		dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		// p_dFdw_.
		p_dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			p_dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));
		return;
	}

	BPTTpred2in::~BPTTpred2in()
	{}

	void BPTTpred2in::beginDay()
	{
		get_iPred();
		return;
	}

	void BPTTpred2in::get_iPred()
	{
		const vector<string>* pin = static_cast<const vector<string>*>(HEvent::Instance()->get("", "inputNames"));
		int i = 0;
		if( pin != 0 )
		{
			for( vector<string>::const_iterator it = pin->begin(); it != pin->end(); ++it, ++i )
			{
				string name = *it;
				if( "pred41m" == name )
					iPred41m_ = i;
			}
		}
	}

	double BPTTpred2in::costMult()
	{
		return costMult_;
	}

	void BPTTpred2in::start_ticker(double position)
	{
		last_mid_ = 0;
		P_p_ = position;

		// p_dFdw_.
		p_dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			p_dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		return;
	}

	vector<double> BPTTpred2in::train(const vector<float>& input, const QuoteInfo& quote, const double target)
	{
		vector<double> output = forward(input, quote);
		double error = 0;

		// Prediction
		//double pred1m = input[iPred1m_];
		double pred41m = input[iPred41m_];

		// Unnormalize the prediction
		const hnn::InputInfo* pnii = static_cast<const hnn::InputInfo*>(HEvent::Instance()->get("", "input_info"));
		pred41m = pred41m * pnii->stdev[iPred41m_] + pnii->mean[iPred41m_];
		//pred40m = pred40m * pnii->stdev[iPred40m_] + pnii->mean[iPred40m_];

		// Position
		double pred = pred41m / 10000;
		double cost = (quote.ask - quote.bid) / (quote.ask + quote.bid) * costMult_;
		//double cost = 0;
		double pos = P_p_;

		if( pred - cost > 0 )
			pos = P_p_ + maxPosChg_;
		else if( pred + cost < 0 )
			pos =  P_p_ - maxPosChg_;

		// Error
		error = pos/sigP_ - F_;

		// Backprop
		backprop(error);

		// These two extra values are used only for monitoring.
		output.push_back(pos);
		output.push_back(F_ * sigP_);

		return output;
	}

	vector<double> BPTTpred2in::trading_signal(const vector<float>& input, const QuoteInfo& quote, const double lastPos)
	{
		P_p_ = lastPos;
		vector<double> output = forward(input, quote);
		last_mid_ = (quote_.bid + quote_.ask) / 2.0;
		return output;
	}

	std::ostream& BPTTpred2in::write(ostream& str) const
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
			double val = wgts[i];
			if( fabs(val) > 1e15 )
				val = 0;
			sprintf( buf, "%15.10f\t", val );
			str << buf;
		}
		str << endl;

		str << sigP_ << "\t" << poslim_ << "\t" << maxPos_ << "\t" << maxPosChg_ << "\t" << costMult_ << "\t" << lotSize_
			<< "\t" << maxRet_ << "\t" << noTwoTradesAtSamePrice_;
		str << endl;

		return str;
	}

	std::istream& BPTTpred2in::read(istream& str)
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
		nwgts_ = 0;
		for( int l = 0; l < L_ - 1; ++l )
			nwgts_ += (N_[l + 1] - 1) * (N_[l]);

		str >> sigP_ >> poslim_ >> maxPos_ >> maxPosChg_ >> costMult_ >> lotSize_ >> maxRet_ >> noTwoTradesAtSamePrice_;

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

		init_vectors();

		return str;
	}

	vector<double> BPTTpred2in::forward(const vector<float>& input, const QuoteInfo& quote)
	{
		quote_ = quote;
		double mid = (quote_.bid + quote_.ask) / 2.0;

	// Input Layer.

		// input layer bias node.
		ff_[0][0] = biasOut_;

		// input layer second node = last output.
		ff_[0][1] = P_p_ / sigP_;

		// input layer other nodes.
		int ninputs = input.size();
		for( int i = 0; i < ninputs; ++i )
			ff_[0][i+2] = input[i];

	// Hidden layers.

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

	// Output layer.

		// bias node, not used.
		ff_[L_ - 1][0] = 0;

		// other nodes.
		for( int i = 1; i < N_[L_ - 1]; ++i )
		{
			double net = 0;
			for( int j = 0; j < N_[L_ - 2]; ++j )
				net += w_[L_ - 2][i][j] * ff_[L_ - 2][j];
			ff_[L_ - 1][i] = net;
		}

	// output.
		F_ = ff_[L_ - 1][1];

	// P_ = position.
		P_ = F_ * sigP_;
		if( P_ > P_p_ + 1 )
			P_ = P_p_ + 1;
		else if( P_ < P_p_ - 1 )
			P_ = P_p_ - 1;
		else
			P_ = ceil( P_ - 0.5 );

		// maxPos adjust.
		//if( P_ * mid > maxPos_ )
		//	P_ = maxPos_ / mid;
		//else if( P_ * mid < -maxPos_ )
		//	P_ = -maxPos_ / mid;

		// noTwoTradesAtSampePrice?
		//if( noTwoTradesAtSamePrice_ )
		//{
		//	trade_aborted_ = false;
		//	double tradePrice = (fabs(P_ - P_p_) > 1e-10) ? ((P_ - P_p_) > 0 ? (quote.ask) : (-quote.bid)) : 0;
		//	if( fabs(last_trade_price_) > 1e-10 && fabs(tradePrice - last_trade_price_) < 1e-10 ) // attempt to trade at the same price.
		//	{
		//		trade_aborted_ = true;
		//		P_ = P_p_;
		//	}
		//	else if( fabs(last_trade_price_) > 1e-10 ) // do trade.
		//		last_trade_price_ = tradePrice;
		//}

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

	void BPTTpred2in::backprop(double error)
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

		last_mid_ = (quote_.bid + quote_.ask) / 2.0;
		P_p_ = P_;

		return;
	}

	double BPTTpred2in::get_R()
	{
		double R0 = get_R0();
		double g = get_g();
		double R = R0 - g;

		return R;
	}

	double BPTTpred2in::get_R0()
	{
		double mid = (quote_.bid + quote_.ask) / 2.0;
		double R0 = 0;
		if( last_mid_ > 1e-6 )
		{
			R0 = P_p_ * (mid - last_mid_) / last_mid_;
		}
		return R0;
	}

	double BPTTpred2in::get_g()
	{
		double g = 0;
		double D = (quote_.ask - quote_.bid) * costMult_;
		//double D = 0;
		double mid = (quote_.bid + quote_.ask) / 2.0;
		if( mid > 1e-6 )
			g = fabs( P_ - P_p_ ) * D / 2.0 / mid;
		return g;
	}
}