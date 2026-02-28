#include <MSignal/sig_at.h>
#include <MSignal/sig.h>
#include <gtlib_model/mFtns.h>
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/jlutil_tickdata.h>
#include <numeric>
#include <omp.h>
#include <boost/filesystem.hpp>
using namespace std;
using namespace hff;
using namespace gtlib;

namespace sig {

void get_trade_stateinfo_at(hff::SigC& sig, const vector<TradeInfo>* trades, const vector<int>& vTindx1s)
{
	// Calculate sI.ltrade, sI.tsincet, sI.validt, sI.tlastt, sI.relativeHiLo, sI.hilo900, sI.hiloLag1, sI.hiloLag2
	// Use sig.hiLag1, sig.loLag1, sig.hiLag2, sig.loLag2.

	int nTrades = trades->size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	const int Npts = sI.size();

	// First trade.
	int timeIdx = 0;
	while( timeIdx < nTrades && (*trades)[timeIdx].msecs <= linMod.openMsecs )
		++timeIdx;

	if( timeIdx < nTrades )
	{
		float high = (*trades)[timeIdx].price;
		float low = (*trades)[timeIdx].price;
		float high900 = high;
		float low900 = low;
		float hiLoD900 = 0.;
		int idxStart = timeIdx;

		// hilo 900.
		int ntrades = trades->size();
		for( int idx = idxStart; idx < ntrades; ++idx )
		{
			int msso = (*trades)[idx].msecs - linMod.openMsecs;
			if( msso > 900000 )
				break;

			if( high900 < (*trades)[idx].price )
				high900 = (*trades)[idx].price;

			if( low900 > (*trades)[idx].price )
				low900 = (*trades)[idx].price;
		}
		if( high900 + low900 > 0 )
			hiLoD900 = basis_pts_ * (high900 - low900) / (high900 + low900);

		// Loop grid points.
		for( int i = 0; i < Npts; ++i )
		{
			int sec = sI[i].msso / 1000;

			int newTrades = 0;
			while( timeIdx < nTrades && ( (*trades)[timeIdx].msecs - linMod.openMsecs ) <= sI[i].msso )
			{
				const float& trade_price = (*trades)[timeIdx].price;
				const int& trade_qty = (*trades)[timeIdx].qty;

				if( trade_price > high )
					high = trade_price;
				if( trade_price < low )
					low = trade_price;

				++newTrades;
				++timeIdx;
			}

			if( newTrades > 0 )
			{
				const float& trade_price_p = (*trades)[timeIdx - 1].price;
				const int& trade_msecs_p = (*trades)[timeIdx - 1].msecs;

				sI[i].ltrade = trade_price_p;
				sI[i].tsincet = sI[i].msso - (trade_msecs_p - linMod.openMsecs);
				sI[i].tlastt = trade_msecs_p - linMod.openMsecs;

				double hiLoD = 0;
				if(high + low > 0)
					hiLoD = basis_pts_ * (high - low) / (high + low);
				if(hiLoD > min_hiloD_)
					sI[i].relativeHiLo = (2 * trade_price_p - high - low) / (high - low);

				double hiLoDL1 = 0;
				if(sig.hiLag1 + sig.loLag1 > 0)
					hiLoDL1 = basis_pts_ * (sig.hiLag1 - sig.loLag1) / (sig.hiLag1 + sig.loLag1);
				if(hiLoDL1 > min_hiloD_)
					sI[i].hiloLag1 = (2 * trade_price_p - sig.hiLag1 - sig.loLag1) / (sig.hiLag1 - sig.loLag1);

				double hiLoDL2 = 0;
				if(sig.hiLag2 + sig.loLag2 > 0)
					hiLoDL2 = basis_pts_ * (sig.hiLag2 - sig.loLag2) / (sig.hiLag2 + sig.loLag2);
				if(hiLoDL2 > min_hiloD_)
					sI[i].hiloLag2 = (2 * trade_price_p - sig.hiLag2 - sig.loLag2) / (sig.hiLag2 - sig.loLag2);

				if(sec >= 900 && hiLoD900 > min_hiloD_)
					sI[i].hilo900 = (2 * trade_price_p - high900 - low900) / (high900 - low900);

				sI[i].validt = 1;
			}
			else if( i > 0 )
			{
				sI[i].ltrade = sI[i - 1].ltrade;
				sI[i].tsincet = sI[i - 1].tsincet + (sI[i].msso - sI[i - 1].msso);
				sI[i].validt = sI[i - 1].validt;
				sI[i].tlastt = sI[i - 1].tlastt;
				sI[i].relativeHiLo = sI[i - 1].relativeHiLo;
				sI[i].hilo900 = sI[i - 1].hilo900;
				sI[i].hiloLag1 = sI[i - 1].hiloLag1;
				sI[i].hiloLag2 = sI[i - 1].hiloLag2;
			}
		}

		// 1s.
		vector<double> trdVolu1s(linMod.n1sec);
		vector<double> sumTrdPrc1s(linMod.n1sec);
		vector<double> nTrds1s(linMod.n1sec);
		{
			int timeIdx = 0;
			while( timeIdx < nTrades && (*trades)[timeIdx].msecs <= linMod.openMsecs )
				++timeIdx;
			while( timeIdx < nTrades )
			{
				const TradeInfo& trade = (*trades)[timeIdx];
				int secIndx = (trade.msecs - linMod.openMsecs ) / 1000 + 1;

				if( secIndx > 0 && secIndx < linMod.n1sec )
				{
					trdVolu1s[secIndx] += trade.qty;
					++nTrds1s[secIndx];
					sumTrdPrc1s[secIndx] += trade.price;
				}

				++timeIdx;
			}
		}

		// 1s cum.
		vector<double> trdVolu1sCum;
		vector<double> sumTrdPrc1sCum;
		vector<double> nTrds1sCum;
		get_cum(trdVolu1sCum, trdVolu1s);
		get_cum(sumTrdPrc1sCum, sumTrdPrc1s);
		get_cum(nTrds1sCum, nTrds1s);

		// vm and vwap.
		float fVolM600 = float(linMod.n1sec - 1) / tm_volu_mom_lb_1_;
		float fVolM3600 = float(linMod.n1sec - 1) / tm_volu_mom_lb_2_;
		for( int i = 0; i < Npts; ++i )
		{
			int sec = sI[i].msso / 1000;
			int msso = sI[i].msso;
			int msecs = msso + linMod.openMsecs;

			sI[i].voluMom600 = get_voluMom( msecs, linMod.openMsecs, 600, trdVolu1sCum, trades, vTindx1s );
			sI[i].voluMom3600 = get_voluMom( msecs, linMod.openMsecs, 3600, trdVolu1sCum, trades, vTindx1s );
			sI[i].intraVoluMom = get_voluMom( msecs, linMod.openMsecs, 0, trdVolu1sCum, trades, vTindx1s );

			// Normalize volume momentum signals.
			if(sig.avgDlyVolume > 0)
			{
				sI[i].voluMom600  = sI[i].voluMom600 * fVolM600 / sig.avgDlyVolume;
				sI[i].voluMom3600 = sI[i].voluMom3600 * fVolM3600 / sig.avgDlyVolume;
				if( sec > 0 )
				{
					float scaleFact = float((linMod.n1sec - 1)) / sec;
					sI[i].intraVoluMom = sI[i].intraVoluMom * scaleFact / sig.avgDlyVolume;
				}
			}

			sI[i].bollinger300 = get_vwap_sig( msecs, linMod.openMsecs, 300, sumTrdPrc1sCum, nTrds1sCum, trades, vTindx1s, 9 );
			sI[i].bollinger900 = get_vwap_sig( msecs, linMod.openMsecs, 900, sumTrdPrc1sCum, nTrds1sCum, trades, vTindx1s, 9 );
			sI[i].bollinger1800 = get_vwap_sig( msecs, linMod.openMsecs, 1800, sumTrdPrc1sCum, nTrds1sCum, trades, vTindx1s, 9 );
			sI[i].bollinger3600 = get_vwap_sig( msecs, linMod.openMsecs, 3600, sumTrdPrc1sCum, nTrds1sCum, trades, vTindx1s, 9 );
		}
	}
}

void get_quote_stateinfo_at(hff::SigC& sig, const vector<QuoteInfo>* quotes, const vector<int>& vQindx1s)
{
	// Calculate sig.firstMidQt, sig.sprdOpen.
	// Calculate sI.mqut, sI.sprd, sI.bid, sI.ask, sI.bsize, sI.asize, sI.tsinceq, sI.tlastq, sI.validq, sI.mret5, sI.mret15, sI.mret30,
	//   sI.mret60, sI.mret120, sI.mret300, sI.mret600, sI.mret1200, sI.mret2400, sI.mret4800, sI.maxbsize, sI.maxasize, sI.maxbsize2, sI.maxasize2.
	// Use sig.firstMidQt.
	// sI.maxbsize: maximum bid size in the last 200 seconds.
	// sI.maxbsize2: maximum bis size in the last 1200 seconds.

	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	const int Npts = sI.size();

	vector<double> vMid1s(linMod.n1sec);
	get_mid_series(vMid1s, quotes, linMod.openMsecs, linMod.closeMsecs, 0, false);

	// overnight return.
	//int secON = 900;
	double retON = 0.;
	if(sig.adjPrevClose > 0.)
		retON = basis_pts_ * (vMid1s[sec_on_] / sig.adjPrevClose - 1.);

	int firstQtMsec = 0;
	{
		int timeIdx = 0;
		while( timeIdx < nQuotes && (*quotes)[timeIdx].msecs <= linMod.openMsecs )
			++timeIdx;
		while( timeIdx < nQuotes )
		{
			const QuoteInfo& quote = (*quotes)[timeIdx];
			if (quote.ask > 0 && quote.bid > 0 )
			{
				float midqt = .5 * (quote.ask + quote.bid);
				float sprd = basis_pts_ * (quote.ask - quote.bid) / midqt;
				if(fabs(sprd) <= skip_qt_ && sprd >= 0) // require a reasonably tight spread
				{
					sig.firstMidQt = .5 * (quote.ask + quote.bid);
					firstQtMsec = quote.msecs;
					sig.sprdOpen = sprd;
					break;
				}
			}
			++timeIdx;
		}
	}

	int timeIdx = 0;
	while( timeIdx < nQuotes && (*quotes)[timeIdx].msecs <= linMod.openMsecs )
		++timeIdx;

	vector<int> vMaxBidSize1s(linMod.n1sec);
	vector<int> vMaxAskSize1s(linMod.n1sec);
	for(int i = 0; i < Npts; ++i)
	{
		int newQuotes = 0;
		while( timeIdx < nQuotes && (((*quotes)[timeIdx].msecs - linMod.openMsecs) < sI[i].msso)) // should be "<= sI["?
		{
			// update the max size
			const QuoteInfo& quote = (*quotes)[timeIdx];
			int mindx = (quote.msecs - linMod.openMsecs) / 1000 + 1;
			if( quote.bidSize > vMaxBidSize1s[mindx] )
				vMaxBidSize1s[mindx] = quote.bidSize;
			if( quote.askSize > vMaxAskSize1s[mindx] )
				vMaxAskSize1s[mindx] = quote.askSize;

			++newQuotes;
			++timeIdx;
		}

		if( newQuotes > 0 )
		{
			const QuoteInfo& quote_p = (*quotes)[timeIdx - 1];

			sI[i].mqut = .5 * (quote_p.ask + quote_p.bid);

			if(sI[i].mqut > 0)
				sI[i].sprd = basis_pts_ * (quote_p.ask - quote_p.bid) / sI[i].mqut;

			sI[i].ask = quote_p.ask;
			sI[i].bid = quote_p.bid;

			sI[i].bsize = quote_p.bidSize;
			sI[i].asize = quote_p.askSize;

			sI[i].tsinceq =	sI[i].msso - (quote_p.msecs - linMod.openMsecs);
			sI[i].tlastq = (quote_p.msecs - linMod.openMsecs);
			sI[i].validq = 1;
		}
		else if( i > 0 )
		{
			sI[i].mqut = sI[i-1].mqut;
			sI[i].sprd = sI[i-1].sprd;

			sI[i].bid = sI[i-1].bid;
			sI[i].ask = sI[i-1].ask;

			sI[i].bsize = sI[i-1].bsize;
			sI[i].asize = sI[i-1].asize;

			sI[i].tsinceq = sI[i-1].tsinceq + (sI[i].msso - sI[i - 1].msso);
			sI[i].tlastq = sI[i-1].tlastq;
			sI[i].validq = sI[i-1].validq;
		}

		if( (sI[i].bsize <= 0 || sI[i].asize <= 0)   )    // 4/25/2005
			sI[i].validq = 0;
		if( fabs(sI[i].sprd) > skip_qt_ || sI[i].mqut <= min_price_ || sig.firstMidQt == 0)  // spread needs to be narrow and price needs to be positive 
			sI[i].validq = 0;

		int msecs = sI[i].msso + linMod.openMsecs;
		if(sI[i].validq > 0 && msecs > firstQtMsec)
		{
			sI[i].mret1 = get_mret_exact(msecs, linMod.openMsecs, 1000, firstQtMsec, sig.firstMidQt, quotes, vQindx1s, sI[i].mqut, 0, true);
			sI[i].mret5 = get_mret_exact(msecs, linMod.openMsecs, 5000, firstQtMsec, sig.firstMidQt, quotes, vQindx1s, sI[i].mqut, 0, true);
			sI[i].mret15 = get_mret_exact(msecs, linMod.openMsecs, 15000, firstQtMsec, sig.firstMidQt, quotes, vQindx1s, sI[i].mqut, 0, true);
			sI[i].mret30 = get_mret_exact(msecs, linMod.openMsecs, 30000, firstQtMsec, sig.firstMidQt, quotes, vQindx1s, sI[i].mqut, 0, true);
			sI[i].mret60 = get_mret_exact(msecs, linMod.openMsecs, 60000, firstQtMsec, sig.firstMidQt, quotes, vQindx1s, sI[i].mqut, 0, true);

			sI[i].mret120 = get_mret_1s(msecs, linMod.openMsecs, 120000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut);
			sI[i].mret300 = get_mret_1s(msecs, linMod.openMsecs, 300000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut);
			sI[i].mret600 = get_mret_1s(msecs, linMod.openMsecs, 600000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut);
			sI[i].mret1200 = get_mret_1s(msecs, linMod.openMsecs, 1200000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut);
			sI[i].mret2400 = get_mret_1s(msecs, linMod.openMsecs, 2400000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut);
			sI[i].mret4800 = get_mret_1s(msecs, linMod.openMsecs, 4800000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut);
			sI[i].mret300L = get_mret_1s(msecs, linMod.openMsecs, 300000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut, 300000);
			sI[i].mret600L = get_mret_1s(msecs, linMod.openMsecs, 600000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut, 600000);
			sI[i].mret1200L = get_mret_1s(msecs, linMod.openMsecs, 1200000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut, 1200000);
			sI[i].mret2400L = get_mret_1s(msecs, linMod.openMsecs, 2400000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut, 2400000);
			sI[i].mret4800L = get_mret_1s(msecs, linMod.openMsecs, 4800000, firstQtMsec, sig.firstMidQt, vMid1s, sI[i].mqut, 4800000);
		}

		if(fabs(sI[i].mret60) > om_max_ret_)
			sI[i].validq = 0;

		if(sI[i].validq > 0 )
		{
			// overnight return.
			int sec = sI[i].msso / 1000;
			if( sec >= sec_on_ )
				sI[i].sig10[7] = retON;
			else if( sig.adjPrevClose > 0 )
				sI[i].sig10[7] = basis_pts_ * ( sI[i].mqut / sig.adjPrevClose - 1.);
			if( fabs(sI[i].sig10[7]) > max_ret_)
				sI[i].sig10[7] = max_ret_ * sI[i].sig10[7] / fabs(sI[i].sig10[7]);

			// max sizes.
			sI[i].maxbsize = get_max_bidSize(msecs, linMod.openMsecs, vMaxBidSize1s, 200, quotes, vQindx1s);
			sI[i].maxasize = get_max_askSize(msecs, linMod.openMsecs, vMaxAskSize1s, 200, quotes, vQindx1s);
			sI[i].maxbsize2 = get_max_bidSize(msecs, linMod.openMsecs, vMaxBidSize1s, 1200, quotes, vQindx1s);
			sI[i].maxasize2 = get_max_askSize(msecs, linMod.openMsecs, vMaxAskSize1s, 1200, quotes, vQindx1s);
		}
	}
}

void get_filImb_stateinfo_at(hff::SigC& sig, const vector<TradeInfo>* trades, const vector<QuoteInfo>* quotes,
		const vector<int>& vTindx1s, const vector<int>& vQindx1s)
{
	// Calculate sI.sig1[1].
	// Use sI.tsincet, sI.validt, sI.validq, sI.tlastt, sI.ltrade.

	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	for(int i = 0; i < Npts; ++i)
	{
		int msecs = sI[i].msso + linMod.openMsecs;
		int sec = sI[i].msso / 1000;
		int sec_search_begin = sec + 1;
		if( sec_search_begin >= linMod.n1sec )
			sec_search_begin = linMod.n1sec - 1;

		int trade_index = -1;
		for( int indx = vTindx1s[sec_search_begin]; indx >= 0; --indx )
		{
			const TradeInfo& trade = (*trades)[indx];
			if( trade.msecs <= msecs - 60000 )
				break;

			if( trade.msecs <= msecs )
			{
				trade_index = indx;
				break;
			}
		}

		if( trade_index >= 0 )
		{
			const TradeInfo& trade = (*trades)[trade_index];

			for( int indx = vQindx1s[sec_search_begin]; indx >= 0; --indx )
			{
				const QuoteInfo& quote = (*quotes)[indx];
				if( quote.ask > quote.bid && quote.msecs < trade.msecs )
				{
					if( valid_quote(quote) )
						sI[i].sig1[1] = 2. * (trade.price - get_mid(quote)) / (quote.ask - quote.bid);
					break;
				}
			}
		}
		clip(sI[i].sig1[1], om_fill_imb_clip_);
	}
}

void get_targets_stateinfo_at(hff::SigC& sig, const vector<QuoteInfo>* quotes)
{
	// Calculate sI.target60Intra, sI.targetClose, sI.targetNextClose.
	// Update sI.validq, sI.validt.
	// Use sig.closeNextCloseRet.

	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const int& n1sec = linMod.n1sec;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	vector<double> vMid1d(linMod.n1sec);
	get_mid_series(vMid1d, quotes, linMod.openMsecs, linMod.closeMsecs, 0, false);

	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const vector<pair<int, int> >& vHorizonLag = nonLinMod.vHorizonLag;
	for( int i = 0; i < Npts; ++i )
	{
		int secNow = (sI[i].msso - 1) / 1000 + 1;

		// Loop horizons.
		int nT = vHorizonLag.size();
		for( int iT = 0; iT < nT; ++iT )
		{
			int horizon = vHorizonLag[iT].first;
			int lag = vHorizonLag[iT].second;
			int secFrom = min(secNow + lag, n1sec - 1);
			int secTo = min(secNow + lag + horizon, n1sec - 1);
			if( secFrom >= linMod.n1sec - 1 )
				secFrom = linMod.n1sec - 1;
			if( secTo >= linMod.n1sec - 1 )
				secTo = linMod.n1sec - 1;
			if(vMid1d[secFrom] > 0. && vMid1d[secTo] > 0.)
			{
				sI[i].target[iT] = basis_pts_ * (vMid1d[secTo] / vMid1d[secFrom] - 1.);

				// clip
				if( horizon >= 60 && horizon < 600 )
					clip(sI[i].target[iT], linMod.om_target_clip);
				else if( horizon >= 600 && horizon < 3600 )
					clip(sI[i].target[iT], linMod.tm_target_clip);
				else if( horizon >= 3600 )
					clip(sI[i].target[iT], linMod.tm_target_60_clip);

				// Copy. Originals will be hedged.
				sI[i].targetUH[iT] = sI[i].target[iT];
			}
		}

		// maxHorizon to close.
		//{
			//int maxHorizon = nonLinMod.maxHorizon;
			//int secFrom = min(secNow + maxHorizon, n1sec - 1);
			//int secTo = n1sec - 1;
			//if( secFrom >= linMod.n1sec - 1 )
				//secFrom = linMod.n1sec - 1;
			//if( secTo >= linMod.n1sec - 1 )
				//secTo = linMod.n1sec - 1;
			//if(vMid1d[secFrom] > 0. && vMid1d[secTo] > 0.)
				//sI[i].target60Intra = basis_pts_ * (vMid1d[secTo] / vMid1d[secFrom] - 1.);
		//}

		// 1 min to close.
		{
			int secFrom = secNow + 60;
			int secTo = n1sec - 1;
			if( secFrom >= linMod.n1sec - 1 )
				secFrom = linMod.n1sec - 1;
			if( secTo >= linMod.n1sec - 1 )
				secTo = linMod.n1sec - 1;
			if(vMid1d[secFrom] > 0. && vMid1d[secTo] > 0.)
			{
				sI[i].targetClose = basis_pts_ * (vMid1d[secTo] / vMid1d[secFrom] - 1.);
				sI[i].targetNextClose = sI[i].targetClose + sig.closeNextCloseRet;
			}
		}

		// clip.
		//clip(sI[i].target60Intra, tm_target_60_clip_);
		clip(sI[i].targetClose, linMod.on_target_clip);
		clip(sI[i].targetNextClose, linMod.on_target_clip);

		// Copy. Originals will be hedged.
		//sI[i].target60IntraUH = sI[i].target60Intra;
		sI[i].targetCloseUH = sI[i].targetClose;
		sI[i].targetNextCloseUH = sI[i].targetNextClose;
	}

	// make the last grid point not valid.
	if(sI[Npts - 1].msso / 1000 >= n1sec - 1)
	{
		sI[Npts - 1].validq = 0;
		sI[Npts - 1].validt = 0;
	}

	// gptOK.
	for(int i = 0; i < Npts; i++)
	{
		if( sI[i].validq && sI[i].validt /*&& sig.dayok*/ )
			sI[i].gptOK = 1;
	}
}

void get_tob_stateinfo_at(TickTobAcc* tta, hff::SigC& sig, int idate, const string& ticker)
{
	// Calculate sI.sig1[8], sI.sig1[9], sI.sig1[10], sI.sig1[11], sI.tobOK.
	// Use sI.validq, sI.validt, sI.bid, sI.ask, sI.askSize, sI.bidSize.

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	tta->LoadData(ticker, idate);

	for( int i = 0; i < Npts; ++i )
	{
		int msecs = sI[i].msso + linMod.openMsecs;
		tta->RollForward(msecs - 1);
		tta->MakeBook(INT_MAX);

		if( sI[i].validq + sI[i].validt < 2 )
			continue;

		vector<const QuoteInfo*> bidSide(max_book_levels_);
		vector<const QuoteInfo*> askSide(max_book_levels_);
		int nBidSide, nAskSide;
		tta->BidSide(&bidSide[0], &nBidSide, max_book_levels_);
		tta->AskSide(&askSide[0], &nAskSide, max_book_levels_);
		//after this, you can rely on nBidSide==nAskSide

		vector<float> inputs(2 * max_book_levels_ - 1);

		if(nBidSide > 0)
		{
			// if NBBO not close to inside TOB continue 
			if( fabs( askSide[0]->ask - sI[i].ask) > min_tob_nbbo_dif_ )
				continue;
			if( fabs( bidSide[0]->bid - sI[i].bid) > min_tob_nbbo_dif_ )
				continue;

			// if inside TOB sprd is too wide, continue 
			double midPrice = 0.5 * (askSide[0]->ask + bidSide[0]->bid);

			double spreadOff = (askSide[0]->ask - bidSide[0]->bid);
			if( spreadOff < minSprdOff_ )
				spreadOff = minSprdOff_;
			double spreadper = spreadOff;
			if(midPrice > 0)
				spreadper = basis_pts_ * spreadOff / midPrice;
			if( fabs(spreadper) > skip_qt_ ||  midPrice <= min_price_)  // added on 1/6/2005
				continue;

			// loop over three levels
			for(int x = 0; x < nBidSide; ++x)
			{
				double qtSize = askSide[x]->askSize + bidSide[x]->bidSize;
				double midX = 0.5 * (askSide[x]->ask + bidSide[x]->bid);
				double spreadX = askSide[x]->ask - bidSide[x]->bid;
				double spreadOffX = (spreadX > minSprdOff_) ? spreadX : minSprdOff_;
				double spreadOffXper = basis_pts_ * spreadOffX / midPrice;

				// if the sprd is too wide continue (slightly redundant)
				if(spreadOffXper > skip_qt_ || midX <= min_price_)
					continue;

				if(askSide[x]->askSize > 0 && bidSide[x]->bidSize > 0)
				{
					double quimX = (bidSide[x]->bidSize - askSide[x]->askSize) / qtSize;
					inputs[2 * x] = quimX; //quote imbalance
					if(x > 0 && spreadX > 0)
						inputs[2 * x - 1] = (midX / midPrice - 1.) * (spreadOff / spreadOffX);
				}
			}
		}

		inputs[1] = basis_pts_ * inputs[1];
		inputs[3] = basis_pts_ * inputs[3];

		sI[i].sig1[8]  = sI[i].sig10[8] = inputs[2];
		sI[i].sig1[9]  = sI[i].sig10[9] = inputs[4];
		sI[i].sig1[10] = sI[i].sig10[10] = inputs[1];	
		sI[i].sig1[11] = sI[i].sig10[11] = inputs[3];
		sI[i].tobOK = 1; // added on 11/26/07
	}
}

void get_book_stateinfo_at(map<string, OrderBkAcc<OrderData> >& obaMap, hff::SigC& sig,
		const string& market, int idate, const string& ticker, const vector<string>& okECNs)
{
	// Calculate sI.sigBook[0] - sI.sigBook[7], sI.sigBook[8] - sI.sigBook[27].
	// Use sI.validq, sI.validt

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	vector<string> mkts = okECNs;
	if( "US" != market )
		mkts.push_back(market);
	int nbooks = int(mkts.size());
	for(int s = 0; s < nbooks; ++s)
		obaMap[mkts[s]].LoadData(ticker, idate);

	const int maxLevels = 1000;
	const unsigned nVolFrac = 6;
	const unsigned nSprdBins = 7;
	const double volumeFrac[nVolFrac] = {0.01, 0.02, 0.04, 0.08, 0.16, 0.32};
	const double sprdBins[nSprdBins] = {0.25, 0.5, 1., 2., 4., 8., 16.};
	vector<double> bookDepthMO(nVolFrac);
	vector<double> bookDepthQI(nSprdBins);
	vector<double> vBookDepthQI(nSprdBins);

	for( int i = 0; i < Npts; ++i )
	{
		if(sI[i].validq + sI[i].validt < 2)
			continue; 
		if(sI[i].tobOK == 0)
			continue; 

		//memset((void*)&bookDepthMO[0], 0., nVolFrac * sizeof(double));
		//memset((void*)&bookDepthQI[0], 0., nSprdBins * sizeof(double));
		//memset((void*)&vBookDepthQI[0], 0., nSprdBins * sizeof(double));
		std::fill(bookDepthMO.begin(), bookDepthMO.end(), 0.);
		std::fill(bookDepthQI.begin(), bookDepthQI.end(), 0.);
		std::fill(vBookDepthQI.begin(), vBookDepthQI.end(), 0.);

		int msecs = sI[i].msso + linMod.openMsecs;
		for(int s = 0; s < nbooks; ++s)
			obaMap[mkts[s]].RollForward(msecs - 1);

		OrderBk<OrderData> obk;
		if( "US" != market )
		{
			obk = OrderBk<OrderData>(obaMap[mkts[0]]);
			for(int s = 1; s < nbooks; ++s)
				obk.MergeIn(obaMap[mkts[s]]);
		}
		else
		{
			for(int s = 0; s < nbooks; ++s)
				obk.MergeIn(obaMap[mkts[s]]);
		}

		vector<const OrderData*> bidSide(maxLevels);
		vector<const OrderData*> askSide(maxLevels);
		int nBidSide = 0,nAskSide = 0;
		obk.GetBidSideBook(maxLevels, &bidSide[0], &nBidSide);
		obk.GetAskSideBook(maxLevels, &askSide[0], &nAskSide);     

		int nLevels = nBidSide < nAskSide ? nBidSide : nAskSide; //only consider the lesser number of price levels on either side
		nLevels = min(nLevels, maxLevels); 
		if(nLevels < 2)
			continue; 
		// if NBBO not close to inside TOB continue 
		if( fabs( askSide[0]->RealPrice() - sI[i].ask) > min_tob_nbbo_dif_ ) 
			continue;
		if( fabs( bidSide[0]->RealPrice() - sI[i].bid) > min_tob_nbbo_dif_ ) 
			continue;

		vector<double> inputs(hff::max_book_sigs_);
		vector<double> sprdi(max_book_levels_);
		double sizeratMax = 20;

		// if inside sprd is too wide, continue 
		double midPrice = 0.5 * (askSide[0]->RealPrice() + bidSide[0]->RealPrice());

		double spreadOff = askSide[0]->RealPrice() - bidSide[0]->RealPrice();
		if( spreadOff < minSprdOff_ )
			spreadOff = minSprdOff_;

		double bbboSize = askSide[0]->size + bidSide[0]->size;
		if(bbboSize <= 0)
			continue; 
		double spreadper = spreadOff;
		if(midPrice > 0)
			spreadper = basis_pts_ * spreadOff / midPrice;
		if( fabs(spreadper) > skip_qt_ ||  midPrice <= min_price_)  // added on 1/6/2005 
			continue;

		float pmedmedSprd = sig.medSprd * midPrice;
		calcBookDepthSigsMO(nLevels, &bidSide[0], nLevels, &askSide[0],
				midPrice, spreadOff, sig.avgDlyVolume, nVolFrac, volumeFrac, bookDepthMO);
		calcBookDepthSigsQI(nLevels, &bidSide[0], nLevels, &askSide[0],
				midPrice, pmedmedSprd, nSprdBins, sprdBins, bookDepthQI, vBookDepthQI, sig.avgDlyVolume);

		sI[i].sigBook[8] = bookDepthMO[0];
		sI[i].sigBook[14] = bookDepthMO[1];
		sI[i].sigBook[15] = bookDepthMO[2];
		sI[i].sigBook[16] = bookDepthMO[3];
		sI[i].sigBook[17] = bookDepthMO[4];
		sI[i].sigBook[18] = bookDepthMO[5];

		sI[i].sigBook[19] = bookDepthQI[0];
		sI[i].sigBook[20] = bookDepthQI[1];
		sI[i].sigBook[21] = bookDepthQI[2];
		sI[i].sigBook[22] = bookDepthQI[3];
		sI[i].sigBook[23] = bookDepthQI[4];
		sI[i].sigBook[24] = bookDepthQI[5];
		sI[i].sigBook[25] = bookDepthQI[6];

		sI[i].sigBook[9] = vBookDepthQI[2];
		sI[i].sigBook[10] = vBookDepthQI[3];
		sI[i].sigBook[11] = vBookDepthQI[4];
		sI[i].sigBook[12] = vBookDepthQI[5];
		sI[i].sigBook[13] = vBookDepthQI[6];
		sI[i].sigBook[26] = vBookDepthQI[0];
		sI[i].sigBook[27] = vBookDepthQI[1];

		// loop over levels 
		int nLevelsUsed = min(nLevels, max_book_levels_);
		for(int x = 0; x < nLevelsUsed; ++x)
		{	
			double qtSize = askSide[x]->size + bidSide[x]->size;
			double midX = 0.5 * (askSide[x]->RealPrice() + bidSide[x]->RealPrice());
			double spreadX = askSide[x]->RealPrice() - bidSide[x]->RealPrice();
			double spreadOffX = (spreadX > minSprdOff_)? spreadX: minSprdOff_;

			double spreadOffXper = basis_pts_ * spreadOffX / midPrice;
			// if the sprd is too wide continue (slightly redundant)
			if(spreadOffXper > skip_qt_ || midX <= min_price_)
				continue; 

			if(askSide[x]->size > 0 && bidSide[x]->size > 0)
			{	
				double quimX = ((double)bidSide[x]->size - (double)askSide[x]->size) / qtSize;
				inputs[4 * x] = quimX;//quote imbalance
				if(x > 0 && spreadX > 0)
				{
					inputs[4 * x - 3] = basis_pts_ * (midX / midPrice-1.) * (spreadOff / spreadOffX); // offset
					inputs[4 * x - 2] = qtSize / bbboSize;   // qutRat
					if(sizeratMax > 0.0 && inputs[4 * x - 2] >  sizeratMax) inputs[4 * x - 2] = sizeratMax;
					inputs[4 * x - 1] = spreadOff / spreadX;
				}
			}
		}

		sI[i].sigBook[0] = inputs[2];	 //  qutRat
		sI[i].sigBook[1] = inputs[6];    //  qutRat
		sI[i].sigBook[2] = inputs[3];	 //  sprdRat
		sI[i].sigBook[3] = inputs[7];    //  sprdRat
		sI[i].sigBook[4] = inputs[4];    //  qutImb   
		sI[i].sigBook[5] = inputs[8];	 //  qutImb
		sI[i].sigBook[6] = inputs[1];	 //  offset
		sI[i].sigBook[7] = inputs[5];    //  offset
	}
}

void get_signals_at(hff::SigC& sig, Sessions& sessions)
{
	// Calculate sig.logVolu, sig.logPrice, sig.logMedSprd, sig.logDolVolu, sig.logDolPrice, sig.prevDayVolume, sI.valid,
	//   sI.sig10[2], sI.sig1[14], sI.sig10[3], sI.sig1[15], sI.sig10[5], sI.sig10[6], sI.mret2400L, sI.mret4800L, sI.mretOpen, sI.sig10[0],
	//   sI.sig10[1], sI.quimMax2, sI.sig10[4], sI.sig10[7], sI[k].voluMom600, sI.voluMom3600, sI.intraVoluMom,
	//   sI.sig1[0], sI.sig1[3], sI.sig1[2], sI.sig1[4], sI.absSprd.
	// Use sI.validq, sig.adjPrevClose, sI.mqut, sig.avgDlyVolume, sig.adjPrevClose, sig.medSprd, sig.fxRate, sig.prevDayVolume, sig.avgDlyVolat,
	//   sig.dayok, sig.inUnivFit
	// Update sI.validq, sI.validt, sI.bsize, sI.asize
	// sI.sig1[0] = quoteImb.
	// sI.sig1[1] = quoteImb at last trade within 60 seconds.
	// sI.sig1[2] = sI.mret60.
	// sI.sig1[3] = sI.relativeHiLo.
	// sI.sig1[4] = gsig.
	// sI.sig1[5] = gsig. (opt)
	// sI.sig1[6] = gsig. (opt)
	// sI.sig1[7] = gsig. (opt) Alt mret30.
	// sI.sig1[8] = tob sig.
	// sI.sig1[9] = tob sig.
	// sI.sig1[10] = tob sig.
	// sI.sig1[11] = tob sig.
	// sI.sig1[12] = gsig. (opt) Alt mret15.
	// sI.sig1[13] = gsig. (opt) Alt mret5.
	// sI.sig1[14] = mret300.
	// sI.sig1[15] = mret300L.
	// sI.sig10[7] = overnight return.

	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	double qutSizeAdj = .01; // using the new TSX nbbo, starting 10/7/09

	//if(sig.dayok)
	{
		sig.logVolu = log10(sig.avgDlyVolume);
		sig.logPrice = log10(sig.adjPrevClose);
		sig.logMedSprd = log10(sig.medSprd);

		//if(sig.avgDlyVolume > 0)
		//	sig.prevDayVolume  = sig.prevDayVolume / sig.avgDlyVolume;
	}

	for( int k = 0; k < Npts; ++k )
	{
		StateInfo& sIk = sI[k];
		int msecs = sIk.msso + linMod.openMsecs;
		int sec = sIk.msso / 1000;

		if(sig.avgDlyVolume <= 0 || sig.avgDlyVolat <= 0 )
		{
			sIk.validq = 0;  
			sIk.validt = 0;
		}

		// Don't allow negative sprds outside North America.
		//if( model.substr(0, 2) != "US" && model.substr(0, 2) != "UF" && model.substr(0, 2) != "CA" && model.substr(0, 1) != "E" )
		if( !linMod.allowCross )
			if( sIk.bid >= sIk.ask )
				sIk.validq = 0;

		sIk.bsize *= qutSizeAdj;
		sIk.asize *= qutSizeAdj;

		if(sIk.validq && sIk.validt && /*sig.dayok &&*/ sig.inUnivFit)
			sIk.valid = 1;

		if( false )
		{
			sIk.sig1[13] = sIk.mret5;
			sIk.sig1[12] = sIk.mret15;
			sIk.sig1[7] = sIk.mret30;
		}

		if( !sessions.inSessionStrict(msecs) )
			sIk.valid = sIk.validq = sIk.validt = 0;

		// mret300 clip
		sIk.sig10[2] = sIk.mret300;
		clip(sIk.sig10[2], linMod.clip_ret300);

		sIk.sig1[14] = sIk.sig10[2];

		// mret300L 
		sIk.sig10[3] = sIk.mret300L;
		clip(sIk.sig10[3], linMod.clip_ret300);

		sIk.sig1[15] = sIk.sig10[3];			

		// mret 600 lagged 600 
		sIk.sig10[5] = sIk.mret600L; // ok since, unlike, sig[2], no nonlinear transform
		if(fabs(sIk.sig10[5]) > max_ret_)
			sIk.sig10[5] = max_ret_ * sIk.sig10[5] / fabs(sIk.sig10[5]);

		// mret 1200 lagged 1200
		sIk.sig10[6] = sIk.mret1200L; 
		clip(sIk.sig10[6], max_ret_);

		// mret 2400 lagged 2400
		clip(sIk.mret2400L, max_ret_);

		// mret 4800 lagged 4800
		clip(sIk.mret4800L, max_ret_);

		// mretOpen
		if(sig.firstMidQt > 0)
			sIk.mretOpen = basis_pts_ * (sIk.mqut / sig.firstMidQt - 1);
		if(fabs(sIk.mretOpen) > max_ret_)
			sIk.mretOpen = max_ret_ * sIk.mretOpen / fabs(sIk.mretOpen); 

		// qutImbWt
		if(sIk.validq && sIk.tsinceq  < 1000 * max_qtwt_lag_)
		{
			sIk.sig10[0] = ((double)(sIk.bsize - sIk.asize)) / (sIk.bsize + sIk.asize);
			sIk.sig10[0] *= sqrt(max(sIk.bsize, sIk.asize) / sig.avgDlyVolume);
		}

		// qutImbMax
		if(sIk.validq && sIk.tsinceq  < 1000 * max_qtmax_lag_ && sIk.maxbsize > 0 && sIk.maxasize > 0)
			sIk.sig10[1] = ((double)(sIk.maxbsize - sIk.maxasize)) / (sIk.maxbsize + sIk.maxasize);

		// qutImbMax2
		if(sIk.validq && sIk.tsinceq  < 1000 * max_qtmax_lag2_ && sIk.maxbsize2 > 0 && sIk.maxasize2 > 0)
			sIk.quimMax2 = ((double)(sIk.maxbsize2 - sIk.maxasize2)) / (sIk.maxbsize2 + sIk.maxasize2);

		//scale hilo by vol: this should not be used anywhere 
		sIk.sig10[4] = sIk.relativeHiLo;

		// if data is not valid, set signals and target to zero; (so nonsense does not enter hedge)
		if( sIk.validq + sIk.validt < 2 )
		{
			for(int n = 0; n < hff::tm_num_basic_sig_; ++n)
				sIk.sig10[n] = 0.;

			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
				sIk.target[iT] = 0.;
		}

		// one minute signals 
		if (sIk.validq && sIk.tsinceq  < 1000 * om_stale_qut_)
			sIk.sig1[0] = ((float)(sIk.bsize - sIk.asize)) / (sIk.bsize + sIk.asize);

		// fillImb (sig1[1]) and hilo (sig1[3]) done earlier
		// signals mret300 (sig1[14]) and mret300Lag (sig1[15]) are calculated above
		sIk.sig1[3] = sIk.relativeHiLo;

		if(sIk.validq)
			sIk.sig1[2] = sIk.mret60;
		clip(sIk.sig1[2], linMod.clip_ret60);

		// gSigs
		if( 0 )
		{
			//int fIndx = 4;
			vector<int> vIndx;
			vIndx.push_back(4);
			vIndx.push_back(5);
			vIndx.push_back(6);
			vIndx.push_back(7);
			vIndx.push_back(12);
			vIndx.push_back(13);
			int indxIndx = 0;
			const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
			for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end() && indxIndx < 6; ++itf, ++indxIndx )
			{
				const hff::IndexFilter& filter = *itf;
				string predName = filter.title() + "_pred";
				const vector<float>* vPred = static_cast<const vector<float>*>(MEvent::Instance()->get("", predName));
				sIk.sig1[vIndx[indxIndx]] = (*vPred)[sec];
			}
		}
		//if(sig.dayok)
		{
			sIk.absSprd = fabs(sIk.sprd);
			clip(sIk.absSprd, multi_lin_sprd_clip_);
		}
	}	// end of loop over k
}

void get_MI_signals_at(SigC& sig, const vector<OrderQty>* trades)
{
	if( trades == 0 || trades->empty() )
		return;

	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	double f600 = float(linMod.n1sec - 1) / 600;
	double f3600 = float(linMod.n1sec - 1) / 3600;

	vector<int> tradeVol(linMod.n1sec, 0);
	for( vector<OrderQty>::const_iterator it = trades->begin(); it != trades->end(); ++it )
	{
		int bin = (it->msecs - linMod.openMsecs - 1) / 1000 + 1;
		if( bin >= 0 && bin < linMod.n1sec )
			tradeVol[bin] += it->signedQty;
	}

	vector<int> tradeVolCum(linMod.n1sec, 0);
	get_cum(tradeVolCum, tradeVol);
	float normFac = (sig.avgDlyVolume > 0) ? sig.avgDlyVolume : 1.0;

	for( int i = 0; i < Npts; ++i )
	{
		int sec = sI[i].msso / 1000;
		float scaleFac = (float)(linMod.n1sec - 1) / sec;

		//sI[i].mI5 = get_volQty(sec, 5, tradeVolCum) / normFac;
		//sI[i].mI15 = get_volQty(sec, 15, tradeVolCum) / normFac;
		//sI[i].mI30 = get_volQty(sec, 30, tradeVolCum) / normFac;
		//sI[i].mI60 = get_volQty(sec, 60, tradeVolCum) / normFac;
		//sI[i].mI120 = get_volQty(sec, 120, tradeVolCum) / normFac;
		sI[i].mI600 = f600 * get_volQty(sec, 600, tradeVolCum) / normFac;
		sI[i].mI3600 = f3600 * get_volQty(sec, 3600, tradeVolCum) / normFac;
		sI[i].mIIntra = scaleFac * get_volQty(sec, 0, tradeVolCum) / normFac;
	}
}

double get_voluMom( int msecs, int length, const vector<TradeInfo>* trades, int idxStart )
{
	double vm = 0.;
	int N = trades->size();
	int msecsFrom = msecs - length * 1000;
	if( msecsFrom > 0 || length == 0 )
	{
		if( length == 0 )
			msecsFrom = 0;

		for( int idx = idxStart; idx < N; ++idx )
		{
			int trade_msecs = (*trades)[idx].msecs;
			if( msecs < trade_msecs )
				break;

			if( msecsFrom < trade_msecs )
				vm += (*trades)[idx].qty;
		}
	}
	return vm;
}

double get_voluMom( int msecs, int openMsecs, int length,
		const vector<double>& trdVolu1sCum, const vector<TradeInfo>* trades, const vector<int>& vTindx1s )
{
	double vm = 0.;
	int sec = (msecs - openMsecs) / 1000;

	int secFrom = sec - length + 1;
	if( secFrom > 0 || length == 0 )
	{
		if( length == 0 )
			secFrom = 0; // intraVoluMom.

		// From 1s sample.
		vm = trdVolu1sCum[sec] - trdVolu1sCum[secFrom];

		// Add trades since last sample.
		int N = trades->size();
		for( int idx = vTindx1s[sec] + 1; idx < N; ++idx )
		{
			int trade_msecs = (*trades)[idx].msecs;
			if( msecs < trade_msecs )
				break;

			vm += (*trades)[idx].qty;
		}
	}

	return vm;
}

double get_voluMom(int msecs, int length, const vector<TradeInfo>& trades)
{
	double vm = 0.;
	int msecsFrom = (length == 0) ? 0 : msecs - length * 1000;
	for( auto it = trades.rbegin(); it != trades.rend(); ++it )
	{
		if( it->msecs < msecsFrom )
			break;
		if( it->msecs <= msecs )
			vm += it->qty;
	}
	return vm;
}

double get_vwap_sig( int msecs, int length, const vector<TradeInfo>* trades, int idxStart, double minVolu )
{
	double vwap = 0.;
	int N = trades->size();
	int msecsFrom = msecs - length * 1000;
	if( msecsFrom > 0 || length == 0 )
	{
		double sumVolTrdPrc = 0.;
		double trdVolu = 0.;
		double sumVolTrdPrcD = 0.;
		double trdVoluD = 0.;
		for( int idx = idxStart; idx < N; ++idx )
		{
			int trade_msecs = (*trades)[idx].msecs;
			if( msecs < trade_msecs )
				break;

			++trdVoluD;
			sumVolTrdPrcD += (*trades)[idx].price;
			if( msecsFrom < trade_msecs && trade_msecs <= msecs )
			{
				++trdVolu;
				sumVolTrdPrc += (*trades)[idx].price;
			}
		}
		if( trdVolu > 0 && trdVoluD > 0 )
		{
			double avgPrc = sumVolTrdPrc / trdVolu;
			double avgPrcD = sumVolTrdPrcD / trdVoluD;
			if( avgPrcD > 0 )
				vwap = avgPrc / avgPrcD - 1.;
		}
	}
	return vwap;
}

double get_vwap_sig( int msecs, int openMsecs, int length, const vector<double>& sumTrdPrc1sCum, const vector<double>& nTrds1sCum,
		const vector<TradeInfo>* trades, const vector<int>& vTindx1s, double minVolu )
{
	double vwap = 0.;
	int sec = (msecs - openMsecs) / 1000;

	// From 1s sample.
	int secFrom = sec - length + 1;
	if( secFrom < 0 )
		secFrom = 0;
	double sumTrdPrc = sumTrdPrc1sCum[sec] - sumTrdPrc1sCum[secFrom];
	double nTrds = nTrds1sCum[sec] - nTrds1sCum[secFrom];
	double sumTrdPrcD = sumTrdPrc1sCum[sec];
	double nTrdsD = nTrds1sCum[sec];

	// Add trades since last sample.
	int N = trades->size();
	for( int idx = vTindx1s[sec] + 1; idx < N; ++idx )
	{
		int trade_msecs = (*trades)[idx].msecs;
		if( msecs < trade_msecs )
			break;

		sumTrdPrc += (*trades)[idx].price;
		sumTrdPrcD += (*trades)[idx].price;
		++nTrds;
		++nTrdsD;
	}

	if( nTrds > minVolu )
	{
		double avgPrc = sumTrdPrc / nTrds;
		double avgPrcD = sumTrdPrcD / nTrdsD;
		if( avgPrcD > 0 )
			vwap = avgPrc / avgPrcD - 1.;
	}
	return vwap;
}

double get_vwap_sig(int msecs, int length, const vector<TradeInfo>& trades, double minVolu)
{
	double sumTrdPrc = 0.;
	double sumTrdPrcD = 0.;
	double nTrds = 0.;
	double nTrdsD = 0.;
	for( auto it = trades.rbegin(); it != trades.rend(); ++it )
	{
		++nTrdsD;
		sumTrdPrcD += it->price;
		if( it->msecs > msecs - length * 1000 )
		{
			++nTrds;
			sumTrdPrc += it->price;
		}
	}
	double vwap = 0.;
	if( nTrds > minVolu )
	{
		double avgPrc = sumTrdPrc / nTrds;
		double avgPrcD = sumTrdPrcD / nTrdsD;
		if( avgPrcD > 0. )
			vwap = avgPrc / avgPrcD - 1.;
	}
	return vwap;
}

double get_mret_exact(int msecs, int openMsecs, int length_msec, int firstQtMsec, double firstMidQt,
		const vector<QuoteInfo>* quotes, const vector<int>& vQindx1s, double mqut, int lag, bool backCompat)
{
	int msecsFrom = msecs - length_msec - lag;
	int msecsTo = msecs - lag;
	if( msecs <= firstQtMsec )
		return 0.;

	double midFrom = 0.;
	if( msecsFrom <= firstQtMsec )
		midFrom = firstMidQt;
	else
	{
		int sec_search_begin = (msecsFrom - openMsecs) / 1000 + 1;
		if( sec_search_begin < 0 )
			sec_search_begin = 0;

		for( int indx = vQindx1s[sec_search_begin]; indx >= 0; --indx )
		{
			const QuoteInfo& quote = (*quotes)[indx];
			int quote_msecs = quote.msecs;
			if( quote_msecs <= firstQtMsec )
			{
				midFrom = firstMidQt;
				break;
			}
			else if( quote_msecs < msecsFrom || indx == 0 )
			{
				// Use zero if the quote is invalid.
				if( valid_quote(quote) )
					midFrom = get_mid(quote);
				break;
			}
		}
	}

	double midTo = mqut;
	if( lag > 0 )
	{
		int sec_search_begin = (msecsTo - openMsecs) / 1000 + 1;
		if( sec_search_begin < 0 )
			sec_search_begin = 0;

		for( int indx = vQindx1s[sec_search_begin]; indx >= 0; --indx )
		{
			const QuoteInfo& quote = (*quotes)[indx];
			int quote_msecs = quote.msecs;
			if( quote_msecs <= firstQtMsec )
			{
				midTo = firstMidQt;
				break;
			}
			else if( quote_msecs < msecsTo || indx == 0 )
			{
				// Use zero if the quote is invalid.
				if( valid_quote(quote) )
					midTo = get_mid(quote);
				break;
			}
		}
	}

	//bool require_backward_compatibility = true;
	double ret = 0.;
	if( backCompat )
	{
		bool backward_compatibility_0 = (msecsTo - 1) / 30000 + 1 >= firstQtMsec / 1000 / 30 + 1; // i >= firstQtGpt.
		bool backward_compatibility_1 = (msecsFrom - 1) / 30000 + 1 > firstQtMsec / 1000 / 30 + 1; // K > firstQtGpt.
		bool backward_compatibility_2 = msecsTo - msecsFrom >= 30000; // i > K.
		bool backward_compatibility_3 = (msecsFrom - 1) / 30000 + 1 <= firstQtMsec / 1000 / 30 + 1; // K <= firstQtGpt.
		bool backward_compatibility_4 = (msecsTo - 1) / 1000 + 1 - (firstQtMsec / 1000 + 1) >= 30; // K <= firstQtGpt && i > K.

		if( length_msec < 30000 )
		{
			if( backward_compatibility_0 )
				if( midFrom > 0. )
					ret = basis_pts_ * (midTo / midFrom - 1);
		}
		else if( length_msec >= 30000 )
		{
			if( backward_compatibility_1 && backward_compatibility_2 )
			{
				if( midFrom > 0. )
					ret = basis_pts_ * (midTo / midFrom - 1);
			}
			else if( backward_compatibility_3 && backward_compatibility_4 )
			{
				if( length_msec == 30000 && midFrom > 0. )
					ret = basis_pts_ * (midTo / midFrom - 1);
				else if ( length_msec > 30000 )
					ret = basis_pts_ * (mqut / firstMidQt - 1);
			}
		}
		return ret;
	}

	if( midFrom > 0. )
		ret = basis_pts_ * (midTo / midFrom - 1);
	return ret;
}

double get_dret(int msecs, int openMsecs, int firstQtMsec, const vector<QuoteInfo>* quotes, const vector<int>& vQindx1s,
		const vector<QuoteInfo>* sip, const vector<int>& vSindx1s)
{
	double ret = -99999.;
	if( quotes == 0 || sip == 0 )
		return ret;

	// Book
	double midFrom = 0.;
	if( msecs >firstQtMsec )
	{
		int sec_search_begin = (msecs - openMsecs) / 1000 + 1;
		if( sec_search_begin < 0 )
			sec_search_begin = 0;

		for( int indx = vQindx1s[sec_search_begin]; indx >= 0; --indx )
		{
			const QuoteInfo& quote = (*quotes)[indx];
			int quote_msecs = quote.msecs;
			if( quote_msecs < msecs || indx == 0 )
			{
				// Use zero if the quote is invalid.
				if( valid_quote(quote) )
					midFrom = get_mid(quote);
				break;
			}
		}
	}

	// SIP
	double midTo = 0.;
	{
		int sec_search_begin = (msecs - openMsecs) / 1000 + 1;
		if( sec_search_begin < 0 )
			sec_search_begin = 0;

		for( int indx = vSindx1s[sec_search_begin]; indx >= 0; --indx )
		{
			const QuoteInfo& quote = (*sip)[indx];
			int quote_msecs = quote.msecs;
			if( quote_msecs < msecs || indx == 0 )
			{
				// Use zero if the quote is invalid.
				if( valid_quote(quote) )
					midTo = get_mid(quote);
				break;
			}
		}
	}

	if( midFrom > 0. && midTo > 0. )
		ret = basis_pts_ * (midTo / midFrom - 1.);
	return ret;
}

double get_mret_1s(int msecs, int openMsecs, int length_msec, int firstQtMsec, double firstMidQt,
		const vector<double>& vMid1s, double mqut, int lag)
{
	// Not checking for valid_quote is a problem.

	// mTo
	double mTo = mqut;
	if( lag > 0 )
	{
		int secTo = (msecs - lag - openMsecs) / 1000;
		if( secTo > 0 )
			mTo = vMid1s[secTo];
		else
			return 0.;
	}
	else if( lag < 0 )
		return 0.;

	// secFrom and mFrom
	double mFrom = 0.;
	int secFrom = (msecs - lag - length_msec - openMsecs) / 1000;
	if( secFrom * 1000 <= firstQtMsec )
		mFrom = firstMidQt;
	else
		mFrom = vMid1s[secFrom];

	double ret = 0.;
	if( msecs > firstQtMsec && mFrom > 0. )
		ret = basis_pts_ * (mTo / mFrom - 1.);
	return ret;
}

int get_max_bidSize(int msecs, int openMsecs, const vector<int>& vBidSize1s, int lag,
		const vector<QuoteInfo>* quotes, const vector<int>& vQindx1s)
{
	int maxSize = 0;

	// From 1 second grid.
	int secFrom = (msecs - openMsecs) / 1000 - lag + 1;
	if( secFrom < 0 )
		secFrom = 0;
	int secTo = (msecs - openMsecs) / 1000;
	for( int indx = secFrom; indx <= secTo; ++indx )
	{
		if( vBidSize1s[indx] > maxSize )
			maxSize = vBidSize1s[indx];
	}

	// Since last 1 second grid.
	int N = quotes->size();
	for( int indx = vQindx1s[secTo] + 1; indx < N; ++indx )
	{
		const QuoteInfo& quote = (*quotes)[indx];
		int quote_msecs = quote.msecs;
		if( msecs < quote_msecs )
			break;

		if( quote.bidSize > maxSize )
			maxSize = quote.bidSize;
	}
	return maxSize;
}

int get_max_askSize(int msecs, int openMsecs, const vector<int>& vAskSize1s, int lag,
		const vector<QuoteInfo>* quotes, const vector<int>& vQindx1s)
{
	int maxSize = 0;

	// From 1 second grid.
	int secFrom = (msecs - openMsecs) / 1000 - lag + 1;
	if( secFrom < 0 )
		secFrom = 0;
	int secTo = (msecs - openMsecs) / 1000;
	for( int indx = secFrom; indx <= secTo; ++indx )
	{
		if( vAskSize1s[indx] > maxSize )
			maxSize = vAskSize1s[indx];
	}

	// Since last 1 second grid.
	int N = quotes->size();
	for( int indx = vQindx1s[secTo] + 1; indx < N; ++indx )
	{
		const QuoteInfo& quote = (*quotes)[indx];
		int quote_msecs = quote.msecs;
		if( msecs < quote_msecs )
			break;

		if( quote.askSize > maxSize )
			maxSize = quote.askSize;
	}
	return maxSize;
}

int get_max_bidSize(int msecs, int length, const vector<QuoteInfo>& quotes)
{
	int maxBidSize = 0;
	int prevMsecs = 0;
	for( auto it = quotes.rbegin(); it != quotes.rend(); ++it )
	{
		if( it->msecs != prevMsecs && it->bidSize > maxBidSize )
			maxBidSize = it->bidSize;
		if( it->msecs <= msecs - length * 1000 )
			break;
		prevMsecs = it->msecs;
	}
	return maxBidSize;
}

int get_max_askSize(int msecs, int length, const vector<QuoteInfo>& quotes)
{
	int maxAskSize = 0;
	int prevMsecs = 0;
	for( auto it = quotes.rbegin(); it != quotes.rend(); ++it )
	{
		if( it->msecs != prevMsecs && it->askSize > maxAskSize )
			maxAskSize = it->askSize;
		if( it->msecs <= msecs - length * 1000 )
			break;
		prevMsecs = it->msecs;
	}
	return maxAskSize;
}

} // namespace sig
