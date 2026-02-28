#include <jl_learn/RmLin.h>
//#include <GTInput/GTEvent.h>
#include <cmath>
#include <iterator>
#include <vector>
#include <string>
#include <time.h>
#include <iostream>
#include <fstream>
//#define DEBUG
using namespace std;

RmLin::RmLin()
{}

RmLin::RmLin(int rewardTime, int interval, int lenSeries, int maxPos, double learn, double thres, double thresIncr,
			 double cost, double adap, double maxWgt, int verbose, string opt, bool trainOnline)
:RLmodel(interval, maxPos, learn, thres, thresIncr, cost, adap, maxWgt, verbose),
lenSeries_(lenSeries),
rewardTime_(rewardTime),
trainOnline_(trainOnline),
At_p_(0),
Bt_p_(0),
opt_(opt)
{
	wgt_ = vector<double>(lenSeries_ + 1, 0); // lags + constant term.
	initWgt();
	dWgt_ = vector<double>(lenSeries_ + 1, 0);

#ifdef DEBUG
	ofs_ = new ofstream("debug.txt");
#endif
}

RmLin::~RmLin(){}

void RmLin::beginDay()
{
#ifdef DEBUG
	int idate = GTEvent::Instance()->idate();
	(*ofs_) << idate << endl;
#endif

	//At_p_ = 0;
	//Bt_p_ = 0;

	if( !trainOnline_ )
	{
		// Update the weights.
		int nw = wgt_.size();
		for( int i=0; i<nw; ++i )
		{
			wgt_[i] += learn_ * dWgt_[i];
			if( fabs(wgt_[i]) > maxWgt_ )
				initWgt(i);
		}
		wgt_[0] = 0; // Turning off the constant term.

		// Initialize dWgt_.
		for( int i=0; i<nw; ++i )
			dWgt_[i] = 0;
	}

	return;
}

void RmLin::evaluate(RLticker& rt)
{
	if( rt.indxLastTradeUnrewarded > 0 && rt.indxLastTradeUnrewarded + rewardTime_ <= rt.indxT )
		update_wgt(rt);

	if( rt.indxLastTrade < 1 || rt.indxT >= rt.indxLastTrade + interval_ ) // Evaluate.
	{
		double F = get_F(rt); // [-1, 1]
		int chgPos = FtoInt(F); // [-maxPos_, maxPos]

		if( chgPos != 0 ) // trade
		{
			double mid = rt.vmid[rt.indxT];
			double dv = fabs(chgPos * mid);
			rt.dollarvol += dv;

			rt.trades.push_back( RLtrade(rt.indxT, chgPos, F) );
			rt.indxLastTrade = rt.indxT;
			rt.position += chgPos;
			if( rt.indxLastTradeUnrewarded == 0 )
				rt.indxLastTradeUnrewarded = rt.indxT;

			if( verbose_ >= 3 )
			{
				double pnl = rt.pnl(cost_);
				printf("%7.2f%6.2f%4d%9.2f\n", mid, F, rt.position, pnl);
			}
		}
	}

	return;
}

double RmLin::get_F(RLticker& rt)
{
	double ret = 0;
	if( rt.indxT >= lenSeries_ )
	{
		double x = get_x(rt);
		ret = tanh(x);
	}
	return ret;
}

double RmLin::get_x(RLticker& rt)
{
	// constant
	int i = 0;
	double x = wgt_[i++];
	int nw = wgt_.size();

	// other terms
	int indx = rt.indxT - lenSeries_ + 1;
	for( ; i<nw; )
		x += wgt_[i++] * rt.vrtn[indx++];

	return x;
}

void RmLin::update_wgt(RLticker& rt)
{
	bool updated = false;
	for( vector<RLtrade>::iterator it = rt.trades.begin(); it != rt.trades.end(); ++it )
	{
		if( !updated && it->indx == rt.indxLastTradeUnrewarded )
		{
			RLtrade& trade = *it;

			// Calculate dDdR.
			double Rt = get_Rt(trade, rt);
			double dDdR = 0;
			if( fabs(Bt_p_ - At_p_ * At_p_) > 1e-10 )
			{
				//if( At_p_ < 0 )
				//	dDdR = 1.0;
				//else
					dDdR = (Bt_p_ - At_p_ * Rt) / (Bt_p_ - At_p_ * At_p_) / sqrt(Bt_p_ - At_p_ * At_p_);
			}

			// Calculate dRdF.
			double dRdF_ = dRdF(trade, rt);

			// Calculate dFdw and dWgt.
			int nw = wgt_.size();
			vector<double> dWgt(nw);
			for( int k=0; k<nw; ++k )
			{
				double dFdw_ = dFdw(k, trade, rt);
				double dw = 0;
				if( opt_ == "profit" )
					dw = dRdF_ * dFdw_;
				else if( "diffShrpR" == opt_ )
					dw = dDdR * dRdF_ * dFdw_;
				dWgt[k] = dw;
				//if( k > 0 && fabs(dw) > 0.1 )
				//	cout << "dw > 0.1" << endl;
			}

#ifdef DEBUG
			ofstream& ofs = *ofs_;
			ofs_->precision(4);
			copy(wgt_.begin(), wgt_.end(), ostream_iterator<double>(ofs, "\t"));
			ofs << endl;
#endif

			if( trainOnline_ )
			{
				for( int i=0; i<nw; ++i )
				{
					wgt_[i] += learn_ * dWgt[i];
					if( fabs(wgt_[i]) > maxWgt_ )
						initWgt(i);
				}
				wgt_[0] = 0; // Turning off the constant term.
			}
			else
			{
				for( int i=0; i<nw; ++i )
					dWgt_[i] += dWgt[i];
			}

			// updates
			double At = At_p_ + adap_ * (Rt - At_p_);
			double Bt = Bt_p_ + adap_ * (Rt * Rt - Bt_p_);
			At_p_ = At;
			Bt_p_ = Bt;

			updated = true;
			rt.indxLastTradeUnrewarded = 0;
		}
		else if( updated )
		{
			rt.indxLastTradeUnrewarded = it->indx;
			break;
		}
	}

	return;
}

double RmLin::dRdF(RLtrade& trade, RLticker& rt)
{
	double ret = 0;
	if( trade.indx + rewardTime_ <= rt.indxT )
		ret = dFiF_ * get_unitRt(trade, rt);
	return ret;
}

double RmLin::dFdw(int k, RLtrade& trade, RLticker& rt)
{
	double ret = 0;
	double r = 1.0;
	if( k > 0 )
		r = rt.vrtn[trade.indx - lenSeries_ + k];
	ret = (1.0 - trade.F * trade.F) * r;
	return ret;
}

double RmLin::get_unitRt(RLtrade& trade, RLticker& rt)
{
	double ret = 0;
	if( trade.indx + rewardTime_ <= rt.indxT )
		ret = (rt.vmid[trade.indx + rewardTime_] - rt.vmid[trade.indx]) - cost_ * rt.vmid[trade.indx] * (abs(trade.shr)/trade.shr);
	return ret;
}

double RmLin::get_Rt(RLtrade& trade, RLticker& rt)
{
	double ret = get_unitRt(trade, rt) * trade.shr;

	return ret;
}
