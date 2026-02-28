#include <HLearnObj/BPTT2.h>
#include <math.h>
#include <iostream>
#include <fstream>
using namespace std;

namespace hnn {
	BPTT2::BPTT2()
	{}

	BPTT2::BPTT2(const BPTT2& rhs)
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
		//P_p_p_ = 0;
		pPpF_ = 0;
		pP_ppF_p_ = 0;
		pFpw_ = rhs.pFpw_; // vector
		dFdw_ = rhs.dFdw_; // vector
		p_dFdw_ = rhs.p_dFdw_; // vector
	}

	BPTT2::BPTT2(vector<int>& N, double learn, double wgtDecay, double maxWgt, int verbose, double biasOut, double costMult,
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
	P_p_(0),
	//P_p_p_(0)
	pPpF_(0),
	pP_ppF_p_(0)
	{
		init_vectors();
	}

	void BPTT2::init_vectors()
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

	BPTT2::~BPTT2()
	{}

	double BPTT2::costMult()
	{
		return costMult_;
	}

	void BPTT2::start_ticker(double position)
	{
		last_mid_ = 0;
		P_p_ = position;

		// p_dFdw_.
		p_dFdw_ = vector<vector<vector<double> > >(L_ - 1, vector<vector<double> >());
		for( int l = 0; l < L_ - 1; ++l )
			p_dFdw_[l] = vector<vector<double> >(N_[l + 1], vector<double>(N_[l], 0));

		return;
	}

	vector<double> BPTT2::train(const vector<float>& input, const QuoteInfo& quote, const double target)
	{
		vector<double> output = forward(input, quote);
		backprop();
		return output;
	}

	vector<double> BPTT2::trading_signal(const vector<float>& input, const QuoteInfo& quote, const double lastPos)
	{
		P_p_ = lastPos;
		vector<double> output = forward(input, quote);
		last_mid_ = (quote_.bid + quote_.ask) / 2.0;
		return output;
	}

	std::ostream& BPTT2::write(ostream& str) const
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

		str << sigP_ << "\t" << poslim_ << "\t" << maxPos_ << "\t" << costMult_ << "\t" << lotSize_
			<< "\t" << maxRet_ << "\t" << noTwoTradesAtSamePrice_;
		str << endl;

		return str;
	}

	std::istream& BPTT2::read(istream& str)
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

		str >> sigP_ >> poslim_ >> maxPos_ >> costMult_ >> lotSize_ >> maxRet_ >> noTwoTradesAtSamePrice_;

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

	vector<double> BPTT2::forward(const vector<float>& input, const QuoteInfo& quote)
	{
		quote_ = quote;
		double mid = (quote.bid + quote.ask) / 2.0;

	// Input Layer.

		// input layer bias node.
		ff_[0][0] = biasOut_;

		// input layer second node = last output.
		ff_[0][1] = P_p_ / sigP_;

		// input layer other nodes.
		int ninputs = input.size();
		for( int i = 0; i < ninputs; ++i )
			ff_[0][i + 2] = input[i];

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
		if( P_ > P_p_ + min(double(quote_.askSize * lotSize_), maxPosChg_) )
			P_ = P_p_ + min(double(quote_.askSize * lotSize_), maxPosChg_);
		else if( P_ < P_p_ - min(double(quote_.bidSize * lotSize_), maxPosChg_) )
			P_ = P_p_ - min(double(quote_.bidSize * lotSize_), maxPosChg_);
		else
			P_ = ceil( P_ - 0.5 );

		// maxPos adjust.
		if( P_ * mid > maxPos_ )
			P_ = maxPos_ / mid;
		else if( P_ * mid < -maxPos_ )
			P_ = -maxPos_ / mid;

		// noTwoTradesAtSampePrice?
		if( noTwoTradesAtSamePrice_ )
		{
			trade_aborted_ = false;
			double tradePrice = (fabs(P_ - P_p_) > 1e-10) ? ((P_ - P_p_) > 0 ? (quote.ask) : (-quote.bid)) : 0;
			if( fabs(last_trade_price_) > 1e-10 && fabs(tradePrice - last_trade_price_) < 1e-10 ) // attempt to trade at the same price.
			{
				trade_aborted_ = true;
				P_ = P_p_;
			}
			else if( fabs(last_trade_price_) > 1e-10 ) // do trade.
				last_trade_price_ = tradePrice;
		}
	// pPpF_ and pPpF_p_
		pP_ppF_p_ = pPpF_;
		pPpF_ = sigP_;
		if( F_ > P_p_ + min(double(quote_.askSize * lotSize_), maxPosChg_) )
			pPpF_ = min(double(quote_.askSize * lotSize_), maxPosChg_) / fabs(F_ - P_p_);
		else if( F_ < P_p_ - min(double(quote_.bidSize * lotSize_), maxPosChg_) )
			pPpF_ = min(double(quote_.bidSize * lotSize_), maxPosChg_) / fabs(F_ - P_p_);

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

	void BPTT2::backprop()
	{
	// Node errors.

		// Set the error of the output layer to unity. Assume only one output node.
		e_[L_ - 1][1] = 1.0;

		// errors of the hidden layers.
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

	// Calculate pF/pw.
		for( int l = L_ - 2; l >= 0; --l )
		{
			for( int j = 0; j < N_[l]; ++j ) // this layer.
			{
				for( int i = 1; i < N_[l + 1]; ++i ) // upper layer. ignore the bias node.
				{
					double val = e_[l + 1][i] * ff_[l][j];
					pFpw_[l][i][j] = val;
				}
			}
		}	

	// Calculate dF/dw.
		double pFpF_p = get_pFpF_p();

		for( int l = L_ - 2; l >= 0; --l )
		{
			for( int j = 0; j < N_[l]; ++j ) // this layer.
			{
				for( int i = 1; i < N_[l + 1]; ++i ) // upper layer. ignore the bias node.
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
		//double pPpF = get_pPpF();

		for( int l = L_ - 2; l >= 0; --l )
		{
			for( int j = 0; j < N_[l]; ++j ) // this layer.
			{
				for( int i = 1; i < N_[l + 1]; ++i ) // upper layer. ignore the bias node.
				{
					//double val = pRpP * sigP_ * dFdw_[l][i][j] + pRpP_p * sigP_ * p_dFdw_[l][i][j];
					double val = pRpP * pPpF_ * dFdw_[l][i][j] + pRpP_p * pP_ppF_p_ * p_dFdw_[l][i][j];
					dw_[l][i][j] += val;
				}
			}
		}

		p_dFdw_ = dFdw_;
		last_mid_ = (quote_.bid + quote_.ask) / 2.0;
		//P_p_p_ = P_p_;
		P_p_ = P_;

		return;
	}

	double BPTT2::get_R()
	{
		double R0 = get_R0();
		double g = get_g();
		double R = R0 - g;

		return R;
	}

	double BPTT2::get_R0()
	{
		double mid = (quote_.bid + quote_.ask) / 2.0;
		double R0 = 0;
		if( last_mid_ > 1e-6 )
			R0 = P_p_ * (mid - last_mid_) / last_mid_;
		return R0;
	}

	double BPTT2::get_g()
	{
		double g = 0;
		double D = (quote_.ask - quote_.bid) * costMult_;
		double mid = (quote_.bid + quote_.ask) / 2.0;
		if( mid > 1e-6 )
			g = fabs( P_ - P_p_ ) * D / 2.0 / mid;
		return g;
	}

	double BPTT2::get_pRpP()
	{
		double ret = - get_pgpP();
		return ret;
	}

	double BPTT2::get_pgpP()
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

	double BPTT2::get_pRpP_p()
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

	double BPTT2::get_pgpP_p()
	{
		double pgpP_p = -get_pgpP();
		return pgpP_p;
	}

	double BPTT2::get_pFpF_p()
	{
		double pFpy01 = 0;
		int l = 0; // layer.
		{
			int j = 1; // last position.
			{
				for( int i = 1; i < N_[l + 1]; ++i ) // upper layer. ignore the bias node.
				{
					double val = e_[l + 1][i] * w_[l][i][j];
					pFpy01 += val;
				}
			}
		}
		double py01pP_p = 1.0 / sigP_;
		//double pP_ppF_p = sigP_;
		//if( F_p_ > P_p_p_ + min(quote_.askSize * lotSize_, maxPosChg_) )
		//	pP_ppF_p = min(quote_.askSize * lotSize_, maxPosChg_) / fabs(F_p_ - P_p_p_);
		//else if( F_p_ < P_p_p_ - min(quote_.bidSize * lotSize_, maxPosChg_) )
		//	pP_ppF_p = min(quote_.bidSize * lotSize_, maxPosChg_) / fabs(F_p_ - P_p_p_);

		double ret = pFpy01 * py01pP_p * pP_ppF_p_;

		return ret;
	}
}