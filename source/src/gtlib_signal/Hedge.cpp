#include <gtlib_signal/Hedge.h>
#include <gtlib_model/hff.h>
#include <gtlib_model/mFtns.h>
#include <jl_lib/jlutil.h>
using namespace std;

namespace gtlib {

Hedge::Hedge()
	:nTarget_(0)
{
}

Hedge::~Hedge()
{
}

const vector<hff::SigH>* Hedge::getPvSigH()
{
	return &vSigH_;
}

void Hedge::updateTicker(const std::string ticker, int inUnivFit,
		double tarON, double tarClcl, int openMsecs, int closeMsecs,
		const TradeInfo& firstTrade, const vector<QuoteInfo>& quotes)
{
	if( firstTrade.msecs > 0 )
	{
		int n1sec = (closeMsecs - openMsecs) / 1000 + 1;
		vector<double> vMid;
		get_mid_series(vMid, &quotes, openMsecs, closeMsecs, 0, true, true);

		hff::SigH sig(ticker, inUnivFit, n1sec);
		sig.tarON = tarON;
		sig.tarClcl = tarClcl;
		int firstTradeSec = (firstTrade.msecs - openMsecs) / 1000 + 1;
		for( int sec = firstTradeSec + 1; sec < n1sec - 1; ++sec )
		{
			// future 1 second return.
			double mid0 = vMid[sec];
			double mid1 = vMid[sec + 1];
			double ret = 0.;
			if( mid0 > 0. && mid1 > 0. )
			{
				ret = basis_pts_ * (mid1 / mid0 - 1.);
				clip(ret, hff::sec_ret_clip_);
				sig.sIH[sec].target[0] = ret;
				sig.sIH[sec].gptOK = 1;
			}
		}
		boost::mutex::scoped_lock lock(hedge_mutex_);
		vSigH_.push_back(sig);
	}
}

void Hedge::updateTicker(const std::string ticker, int inUnivFit,
		double tarON, double tarClcl, int openMsecs, int closeMsecs,
		const TradeInfo& firstTrade, TCM_classic& tcmc)
{
	if( firstTrade.msecs > 0 )
	{
		int n1sec = (closeMsecs - openMsecs) / 1000 + 1;
		hff::SigH sig(ticker, inUnivFit, n1sec);
		sig.tarON = tarON;
		sig.tarClcl = tarClcl;

		int firstSec = (firstTrade.msecs - openMsecs) / 1000 + 1;
		for( int sec = firstSec; sec < n1sec - 1; ++sec )
		{
			int msec1 = sec * 1000 + openMsecs;
			int msec2 = (sec+1) * 1000 + openMsecs;
			QuoteInfo q1 = tcmc.SingleConverter()->NbboAt(msec1);
			QuoteInfo q2 = tcmc.SingleConverter()->NbboAt(msec2);

			if( valid_quote(q1) && valid_quote(q2) )
			{
				double retBpt = basis_pts_ * (get_mid(q2) / get_mid(q1) - 1.);
				clip(retBpt, hff::sec_ret_clip_);
				sig.sIH[sec].target[0] = retBpt;
				sig.sIH[sec].gptOK = 1;
			}
		}
		vSigH_.push_back(sig);
	}
}

void Hedge::updateTicker(const hff::LinearModel& linMod, const hff::NonLinearModel& nonLinMod,
		const std::string ticker, int inUnivFit, double closePrc, double tarON, double tarClcl,
		const TradeInfo& firstTrade, const vector<QuoteInfo>& quotes, bool smoothTarget)
{
	const vector<pair<int, int> >& vHorizonLag = nonLinMod.vHorizonLag;
	nTarget_ = vHorizonLag.size();
	if( firstTrade.msecs > 0 )
	{
		//int n1sec = (linMod.closeMsecs - linMod.openMsecs) / 1000 + 1;
		hff::SigH sig(ticker, inUnivFit, linMod.n1sec, nTarget_);
		sig.tarON = tarON;
		sig.tarClcl = tarClcl;

		vector<double> vMid;
		if(smoothTarget)
			get_mid_series_smooth(vMid, &quotes, linMod.openMsecs, linMod.closeMsecs, 0, false);
		else
			get_mid_series(vMid, &quotes, linMod.openMsecs, linMod.closeMsecs, 0, false);

		int firstSec = (firstTrade.msecs - linMod.openMsecs) / 1000 + 1;
		for( int sec = firstSec; sec < linMod.n1sec - 1; ++sec )
		{
			hff::StateInfoH& sI = sig.sIH[sec];
			float midQt = vMid[sec];
			if( midQt > 0. )
				sig.sIH[sec].gptOK = 1;

			// Targets with different horizons.
			for( int iT = 0; iT < nTarget_; ++iT )
			{
				int horizon = vHorizonLag[iT].first;
				int lag = vHorizonLag[iT].second;

				int secFrom = min(sec + lag, linMod.n1sec - 1);
				float midFrom = vMid[secFrom];

				int secTo = min(sec + lag + horizon, linMod.n1sec - 1);
				float midTo = vMid[secTo];

				if( midFrom > 0. && midTo > 0. )
				{
					sI.target[iT] = basis_pts_ * (midTo / midFrom - 1.);

					// clip.
					if( horizon <= 60 )
						clip(sI.target[iT], linMod.om_target_clip);
					else if( horizon <= 600 )
						clip(sI.target[iT], linMod.tm_target_clip);
					else if( horizon <= 3600 )
						clip(sI.target[iT], linMod.tm_target_60_clip);
					else if( horizon > 3600 )
						clip(sI.target[iT], linMod.tm_target_intra_clip);
				}
			}

			// Target to close.
			{
				{
					int secFrom = sec;
					if(secFrom < linMod.n1sec - 1)
					{
						float midFrom = vMid[secFrom];
						if( midFrom > 0. && closePrc > 0. )
							sI.targetClose = basis_pts_ * (closePrc / midFrom - 1.);
					}
				}
				{
					int secFrom = sec + 11*60;
					if(secFrom < linMod.n1sec - 1)
					{
						float midFrom = vMid[secFrom];
						if( midFrom > 0. && closePrc > 0. )
							sI.target11Close = basis_pts_ * (closePrc / midFrom - 1.);
					}
				}
				{
					int secFrom = sec + 71*60;
					if(secFrom < linMod.n1sec - 1)
					{
						float midFrom = vMid[secFrom];
						if( midFrom > 0. && closePrc > 0. )
							sI.target71Close = basis_pts_ * (closePrc / midFrom - 1.);
					}
				}

				clip(sI.target11Close, linMod.tm_target_intra_clip);
				clip(sI.target71Close, linMod.tm_target_intra_clip);
				//clip(sI.targetClose, linMod.tm_target_intra_clip);
			}
		}
		vSigH_.push_back(sig);
	}
}

} // namespace gtlib
