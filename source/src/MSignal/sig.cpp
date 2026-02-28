#include <MSignal/sig.h>
#include <gtlib_model/mFtns.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/jlutil_tickdata.h>
#include <MFramework.h>
#include <numeric>
#include <omp.h>
#include <boost/filesystem.hpp>
using namespace std;
using namespace hff;
using namespace gtlib;

namespace sig {

unordered_map<string, string> get_uid_map(const vector<string>& markets, int idate, const set<string>& uids)
{
	unordered_map<string, string> mTickerUid;

	for( auto itm = markets.begin(); itm != markets.end(); ++itm )
	{
		string market = *itm;
		bool dayok = mto::dataOK(market, idate);

		if( dayok )
		{
			int idate_p = prevClose(market, idate);
			char cmd[1000];
			sprintf(cmd, "select %s, uniqueID from stockcharacteristics "
					" where %s and uniqueID is not null ",
					mto::compTicker(market).c_str(),
					mto::selChara(market, idate_p).c_str());
			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			for( auto it = vv.begin(); it != vv.end(); ++it )
			{
				string ticker = trim((*it)[0]);
				string uid = trim((*it)[1]);
				if( uids.count(uid) && !ticker.empty() )
					mTickerUid[ticker] = uid;
			}
		}
	}
	return mTickerUid;
}

bool read_chara_data(hff::SigC& sig, const string& model, const string& market, const string& uid,
		int idate, int idate_p, int idate_pp, int idate_n)
{
	// Calculate sig.inUnivChara, sig.inUnivFit, sig.avgDlyVolume, sig.avgDlyVolat, sig.adjPrevClose, sig.medSprd, sig.prevDayVolume, sig.prevDayVolat,
	//   sig.mretIntraLag1, sig.hiloQAI, sig.hiloLag1Open, sig.hiloLag1Rat, sig.hiLag1, sig.loLag1, sig.mretIntraLag2, sig.hiloQAI2, sig.hiloLag2Open,
	//   sig.hiloLag2Rat, sig.hiLag2, sig.loLag2, sig.mretONLag1, sig.closeNextCloseRet.
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	hff::CharaInfo chara;
	hff::CharaInfo chara_p;
	hff::CharaInfo chara_pp;
	hff::CharaInfo chara_n;

	const hff::CharaContainer* ch = static_cast<const hff::CharaContainer*>(MEvent::Instance()->get("", "CharaContainer"));
	bool valid_chara = ch->getCharaInfo(market, idate, uid, chara);
	bool valid_chara_p = ch->getCharaInfo(market, idate_p, uid, chara_p);
	bool valid_chara_pp = ch->getCharaInfo(market, idate_pp, uid, chara_pp);
	bool valid_chara_n = ch->getCharaInfo(market, idate_n, uid, chara_n);

	if( valid_chara_p && valid_chara )
	{
		string model02 = model.substr(0, 2);

		sig.ticker = mto::compTicker(chara_p.ticker, market);
		sig.inUnivChara = chara_p.inUniv;
		sig.inUnivFit = chara_p.inUniv;
		sig.avgDlyVolume = chara_p.medVolume;
		sig.avgDlyVolat = chara_p.medVolatility * basis_pts_;
		sig.adjPrevClose = chara_p.close;
		sig.medSprd = chara_p.medmedSprd;
		sig.medNquotes = chara_p.medNquotes;
		sig.medNtrades = chara_p.medNtrades;
		if( sig.avgDlyVolume > 0. )
			sig.prevDayVolume = chara_p.volume / sig.avgDlyVolume;
		sig.prevDayVolat = chara_p.volatility * basis_pts_;

		if( "US" == model02 || "UF" == model02 || "UE" == model02 || "CA" == model02 )
			sig.exchangeChar = chara_p.market;

		if( "US" == model02 || "UF" == model02 || "UE" == model02 )
		{
			// Exchange.
			if( sig.exchangeChar != "Q" )
				sig.exchange = 1.;
			sig.sectype = chara_p.sectype;
			if( sig.sectype == "F" )
				sig.isSecTypeF = 1.;

			// adjust NASD volume.
			if( 0 )
			{
			}
		}

		if( "US" == model02 || "UE" == model02 )
		{
			// univ.
			if( sig.adjPrevClose > .5
					&& sig.adjPrevClose * sig.avgDlyVolume > 60000
					&& sig.avgDlyVolat > 50
					&& sig.adjPrevClose < 900
					&& sig.medSprd >= .00008
					&& sig.medSprd < .04
					&& sig.sectype != "P"
					&& sig.sectype != "F"
					&& sig.sectype != "X"
			  )
				sig.inUnivFit = 1;
			else
				sig.inUnivFit = 0;
		}

		if( chara.adjust > 0. )
			sig.adjPrevClose /= chara.adjust;

		if( mto::isInternational(market) && chara.tickValid == 0 )
			return false;

		if( sig.medSprd <= 0. )
		{
			sig.medSprd = typ_med_sprd_;
			return false;
		}
		else if( sig.medSprd <= min_med_sprd_ )
		{
			sig.medSprd = min_med_sprd_;
			return false;
		}
		else if( sig.medSprd > max_med_sprd_ )
		{
			sig.medSprd = max_med_sprd_;
			return false;
		}

		if( sig.avgDlyVolume <= 0. || sig.avgDlyVolat <= 0. || sig.adjPrevClose <= 0. )
			return false;

		if( chara_p.open > 0. && chara_p.close > 0. )
		{
			sig.mretIntraLag1 = basis_pts_ * (chara_p.close / chara_p.open - 1.);
			if( sig.mretIntraLag1 > max_ret_ )
				sig.mretIntraLag1 = max_ret_;
			else if( sig.mretIntraLag1 < - max_ret_ )
				sig.mretIntraLag1 = - max_ret_;
		}

		if( chara_p.open > 0. && chara_p.close > 0. && chara_p.low > 0. && chara_p.high > chara_p.low )
		{
			sig.hiloQAI = 2. * (chara_p.close - chara_p.low) / (chara_p.high - chara_p.low) - 1.;
			if( sig.hiloQAI > 1. )
				sig.hiloQAI = 1.;
			else if( sig.hiloQAI < - 1. )
				sig.hiloQAI = - 1.;

			sig.hiloLag1Open = 2. * (chara_p.open - chara_p.low) / (chara_p.high - chara_p.low) - 1.;
			if( sig.hiloLag1Open > 1. )
				sig.hiloLag1Open = 1.;
			else if( sig.hiloLag1Open < - 1. )
				sig.hiloLag1Open = - 1.;

			sig.hiloLag1Rat = (chara_p.high - chara_p.low) / chara_p.close;
		}

		sig.hiLag1 = chara_p.high;
		sig.loLag1 = chara_p.low;
		if( chara.adjust > 0. )
		{
			sig.hiLag1 /= chara.adjust;
			sig.loLag1 /= chara.adjust;
		}

		if(	valid_chara_pp )
		{
			if( chara_pp.open > 0. && chara_pp.close > 0. )
			{
				sig.mretIntraLag2 = basis_pts_ * (chara_pp.close / chara_pp.open - 1.);
				if( sig.mretIntraLag2 > max_ret_ )
					sig.mretIntraLag2 = max_ret_;
				else if( sig.mretIntraLag2 < - max_ret_ )
					sig.mretIntraLag2 = - max_ret_;
			}

			if( chara_pp.open > 0. && chara_pp.close > 0. && chara_pp.low > 0. && chara_pp.high > chara_pp.low )
			{
				sig.hiloQAI2 = 2. * (chara_pp.close - chara_pp.low) / (chara_pp.high - chara_pp.low) - 1.;
				if( sig.hiloQAI2 > 1. )
					sig.hiloQAI2 = 1.;
				else if( sig.hiloQAI2 < - 1. )
					sig.hiloQAI2 = - 1.;

				sig.hiloLag2Open = 2. * (chara_pp.open - chara_pp.low) / (chara_pp.high - chara_pp.low) - 1.;
				if( sig.hiloLag2Open > 1. )
					sig.hiloLag2Open = 1.;
				else if( sig.hiloLag2Open < - 1. )
					sig.hiloLag2Open = - 1.;

				sig.hiloLag2Rat = (chara_pp.high - chara_pp.low) / chara_pp.close;
			}

			sig.hiLag2 = chara_pp.high;
			sig.loLag2 = chara_pp.low;
			if( chara_p.adjust > 0 && chara.adjust > 0 )
			{
				float adjust = chara.adjust * chara_p.adjust;
				sig.hiLag2 /= adjust;
				sig.loLag2 /= adjust;
			}

			float chara_pp_close_adj = chara_pp.close;
			if( chara_p.adjust > 0 )
				chara_pp_close_adj /= chara_p.adjust;

			if( chara_pp_close_adj > 0. )
			{
				sig.mretONLag1 = basis_pts_ * ( chara_p.open / chara_pp_close_adj - 1. );
				if( sig.mretONLag1 > max_ret_ )
					sig.mretONLag1 = max_ret_;
				else if( sig.mretONLag1 < - max_ret_ )
					sig.mretONLag1 = - max_ret_;
			}
		}

		if( valid_chara_n && chara_n.close > 0 && chara.close > 0 && chara_n.adjust > 0 )
		{
			float chara_close_adjust = chara.close / chara_n.adjust;
			sig.closeNextCloseRet = basis_pts_ * (chara_n.close / chara_close_adjust - 1.);
			clip(sig.closeNextCloseRet, linMod.on_target_clip);

			sig.tarON = basis_pts_ * (chara_n.open / chara_close_adjust - 1.);
			clip(sig.tarON, linMod.on_target_clip);
		}
	}
	else
		return false;

	if( sig.hiLag1 <= 0 || sig.hiLag2 <= 0 || sig.loLag1 <= 0 || sig.loLag2 <= 0 )
		return false;

	return true;
}


bool read_chara_weight(hff::SigC& sig, const string& model, const string& market, const string& uid, int idate_p)
{
	// Calculate sig.inUnivChara, sig.inUnivFit, sig.avgDlyVolume, sig.avgDlyVolat, sig.adjPrevClose, sig.medSprd, sig.prevDayVolume, sig.prevDayVolat,
	//   sig.mretIntraLag1, sig.hiloQAI, sig.hiloLag1Open, sig.hiloLag1Rat, sig.hiLag1, sig.loLag1, sig.mretIntraLag2, sig.hiloQAI2, sig.hiloLag2Open,
	//   sig.hiloLag2Rat, sig.hiLag2, sig.loLag2, sig.mretONLag1, sig.closeNextCloseRet.

	hff::CharaInfo chara;
	hff::CharaInfo chara_p;

	const hff::CharaContainer* ch = static_cast<const hff::CharaContainer*>(MEvent::Instance()->get("", "CharaContainer"));
	bool valid_chara_p = ch->getCharaInfo(market, idate_p, uid, chara_p);

	if( valid_chara_p )
	{
		string model02 = model.substr(0, 2);

		sig.ticker = chara_p.ticker;
		sig.inUnivChara = chara_p.inUniv;
		sig.inUnivFit = chara_p.inUniv;
		sig.avgDlyVolume = chara_p.medVolume;
		sig.avgDlyVolat = chara_p.medVolatility * basis_pts_;
		sig.adjPrevClose = chara_p.close;
		sig.medSprd = chara_p.medmedSprd;
		if( sig.avgDlyVolume > 0 )
			sig.prevDayVolume = chara_p.volume / sig.avgDlyVolume;
		sig.prevDayVolat = chara_p.volatility * basis_pts_;

		if( "US" == model02 || "UF" == model02 || "CA" == model02 )
			sig.exchangeChar = chara_p.market;

		if( "US" == model02 || "UF" == model02 )
		{
			// Exchange.
			if( sig.exchangeChar != "Q" )
				sig.exchange = 1.;
			sig.sectype = chara_p.sectype;
			if( sig.sectype == "F" )
				sig.isSecTypeF = 1.;
		}

		if( "US" == model02 )
		{
			// adjust NASD volume.
			if( 0 )
			{
			}

			// univ.
			if( sig.adjPrevClose > .5
					&& sig.adjPrevClose * sig.avgDlyVolume > 60000
					&& sig.avgDlyVolat > 50
					&& sig.adjPrevClose < 900
					&& sig.medSprd >= .00008
					&& sig.medSprd < .04
					&& sig.sectype != "P"
					&& sig.sectype != "F"
					&& sig.sectype != "X"
			  )
				sig.inUnivFit = 1;
			else
				sig.inUnivFit = 0;
		}

		//if( valid_chara && chara.adjust > 0. )
		//	sig.adjPrevClose /= chara.adjust;

		//if( mto::isInternational(market) && chara.tickValid == 0 ) // Do not use tickValid for weight writing.
		//	return false;

		if( sig.medSprd <= 0. )
			sig.medSprd = typ_med_sprd_;
		else if( sig.medSprd <= min_med_sprd_ )
			sig.medSprd = min_med_sprd_;
		else if( sig.medSprd > max_med_sprd_ )
			sig.medSprd = max_med_sprd_;
	}
	else
		return false;

	//if( sig.hiLag1 <= 0 || sig.hiLag2 <= 0 || sig.loLag1 <= 0 || sig.loLag2 <= 0 )
	//	return false;

	return true;
}

void get_trade_stateinfo(hff::SigC& sig, const vector<TradeInfo>* trades)
{
	// Calculate sI.ltrade, sI.sig10[4], sI.sig1[3], sI.tsincet, sI.validt, sI.tlastt, sI.relativeHiLo, sI.hilo900, sI.hiloLag1, sI.hiloLag2
	// Use sig.hiLag1, sig.loLag1, sig.hiLag2, sig.loLag2.

	int nTrades = trades->size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	int L15 = 900 / linMod.gridInterval;
	const int& gpts = linMod.gpts;
	vector<hff::StateInfo>& sI = sig.sI;

	sI[0].sig10[4] = 0.; // hilo
	sI[0].sig1[3] = 0.;

	vector<double> trdVolu1s(linMod.n1sec);
	vector<double> sumTrdPrc1s(linMod.n1sec);
	vector<double> nTrds1s(linMod.n1sec);
	vector<double> sumVolTrdPrc1s(linMod.n1sec);

	// Loop trades.
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
			sumVolTrdPrc1s[secIndx] += trade.price * trade.qty;
		}

		++timeIdx;
	}

	// Loop grid points.
	timeIdx = 0;
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

		for( int i = 1; i < gpts; ++i )
		{
			int newTrades = 0;
			while( timeIdx < nTrades && ( (*trades)[timeIdx].msecs - linMod.openMsecs ) * 0.001 < i * linMod.gridInterval )
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

			if( i == L15 )
			{
				high900 = high;
				low900 = low;
				if( high900 + low900 > 0 )
					hiLoD900 = basis_pts_ * (high900 - low900) / (high900 + low900);
			}

			if( newTrades == 0 )
			{
				sI[i].ltrade = sI[i - 1].ltrade;
				sI[i].sig10[4] = sI[i - 1].sig10[4];
				sI[i].sig1[3] = sI[i - 1].sig1[3];
				sI[i].tsincet = sI[i - 1].tsincet + 1000 * linMod.gridInterval;
				sI[i].validt = sI[i - 1].validt;
				sI[i].tlastt = sI[i - 1].tlastt;
				sI[i].relativeHiLo = sI[i - 1].relativeHiLo;
				sI[i].hilo900 = sI[i - 1].hilo900;
				sI[i].hiloLag1 = sI[i - 1].hiloLag1;
				sI[i].hiloLag2 = sI[i - 1].hiloLag2;
			}
			else
			{
				const float& trade_price_p = (*trades)[timeIdx - 1].price;
				const int& trade_msecs_p = (*trades)[timeIdx - 1].msecs;

				sI[i].ltrade = trade_price_p;
				sI[i].tsincet = i * linMod.gridInterval * 1000 - (trade_msecs_p - linMod.openMsecs);
				sI[i].tlastt = trade_msecs_p - linMod.openMsecs;

				double hiLoD=0;
				if(high + low > 0)
					hiLoD = basis_pts_ * (high - low) / (high + low);
				if(trade_price_p >= high - om_hilo_tol_  && timeIdx - 1 != idxStart && hiLoD > min_hiloD_ ) 
				{
					sI[i].sig10[4] = 1.;
					sI[i].sig1[3] = 1.;
				}
				if(trade_price_p <= low + om_hilo_tol_ && timeIdx - 1 != idxStart && hiLoD > min_hiloD_ )   
				{
					sI[i].sig10[4] = -1.;
					sI[i].sig1[3] = -1.;
				}
				if(hiLoD > min_hiloD_)
					sI[i].relativeHiLo = (2 * trade_price_p - high - low) / (high - low);

				double hiLoDL1 = 0;
				if(sig.hiLag1 + sig.loLag1 > 0)
					hiLoDL1 = basis_pts_ * (sig.hiLag1 - sig.loLag1) / (sig.hiLag1 + sig.loLag1);
				if(hiLoDL1 > min_hiloD_)
					sI[i].hiloLag1 = (2 * trade_price_p - sig.hiLag1 - sig.loLag1) / (sig.hiLag1 - sig.loLag1);

				double hiLoDL2= 0;
				if(sig.hiLag2 + sig.loLag2 > 0)
					hiLoDL2 = basis_pts_ * (sig.hiLag2 - sig.loLag2) / (sig.hiLag2 + sig.loLag2);
				if(hiLoDL2 > min_hiloD_)
					sI[i].hiloLag2 = (2 * trade_price_p - sig.hiLag2 - sig.loLag2) / (sig.hiLag2 - sig.loLag2);

				if(hiLoD900 > min_hiloD_)
					sI[i].hilo900 = (2 * trade_price_p - high900 - low900) / (high900 - low900);

				sI[i].validt = 1;
			}
		}
	}

	vector<double> sumVolTrdPrc1sCum;
	vector<double> trdVolu1sCum;
	vector<double> sumTrdPrc1sCum;
	vector<double> nTrds1sCum;
	get_cum(sumVolTrdPrc1sCum, sumVolTrdPrc1s);
	get_cum(trdVolu1sCum, trdVolu1s);
	get_cum(sumTrdPrc1sCum, sumTrdPrc1s);
	get_cum(nTrds1sCum, nTrds1s);

	float fVolM600  = float(linMod.n1sec-1) / tm_volu_mom_lb_1_;
	float fVolM3600 = float(linMod.n1sec-1) / tm_volu_mom_lb_2_;
	for( int i = 1; i < linMod.gpts; ++i )
	{
		int sec = i * linMod.gridInterval;

		sI[i].voluMom600 = get_voluMom( sec, 600, trdVolu1sCum );
		sI[i].voluMom3600 = get_voluMom( sec, 3600, trdVolu1sCum );
		sI[i].intraVoluMom = get_voluMom( sec, 0, trdVolu1sCum );

		// Normalize volume momentum signals.
		if(sig.avgDlyVolume > 0)
		{
			sI[i].voluMom600  = sI[i].voluMom600 * fVolM600 / sig.avgDlyVolume;
			sI[i].voluMom3600 = sI[i].voluMom3600 * fVolM3600 / sig.avgDlyVolume;
			float scaleFact = float((linMod.n1sec-1)) / (i * linMod.gridInterval);
			sI[i].intraVoluMom = sI[i].intraVoluMom * scaleFact / sig.avgDlyVolume;
		}

		sI[i].bollinger300 = get_vwap_sig( sec, 300, sumTrdPrc1sCum, nTrds1sCum, 9 );
		sI[i].bollinger900 = get_vwap_sig( sec, 900, sumTrdPrc1sCum, nTrds1sCum, 9 );
		sI[i].bollinger1800 = get_vwap_sig( sec, 1800, sumTrdPrc1sCum, nTrds1sCum, 9 );
		sI[i].bollinger3600 = get_vwap_sig( sec, 3600, sumTrdPrc1sCum, nTrds1sCum, 9 );

		double cwap = get_vwap( sec, 0, sumTrdPrc1sCum, nTrds1sCum, 9 );
		double vwap = get_vwap( sec, 0, sumVolTrdPrc1sCum, trdVolu1sCum, 9 );
		double cwap600 = get_vwap( sec, 600, sumTrdPrc1sCum, nTrds1sCum, 9 );
		double vwap600 = get_vwap( sec, 600, sumVolTrdPrc1sCum, trdVolu1sCum, 9 );
		//if( cwap > 0. )
			//sI[i].midCwap = (sI[i].mqut / cwap - 1.) * basis_pts_;
		//if( vwap > 0. )
			//sI[i].midVwap = (sI[i].mqut / vwap - 1.) * basis_pts_;
		//if( cwap600 > 0. )
			//sI[i].midCwap600 = (sI[i].mqut / cwap600 - 1.) * basis_pts_;
		//if( vwap600 > 0. )
			//sI[i].midVwap600 = (sI[i].mqut / vwap600 - 1.) * basis_pts_;
	}
}

void get_quote_stateinfo(hff::SigC& sig, const vector<QuoteInfo>* quotes, bool debug)
{
	// Calculate sig.firstMidQt, sig.firstQtGpt, sig.sprdOpen.
	// Calculate sI.mqut, sI.sprd, sI.bid, sI.ask, sI.bsize, sI.asize, sI.tsinceq, sI.tlastq, sI.validq, sI.mret5, sI.mret15, sI.mret30,
	//   sI.mret60, sI.mret120, sI.mret300, sI.mret600, sI.mret1200, sI.mret2400, sI.mret4800, sI.maxbsize, sI.maxasize, sI.maxbsize2, sI.maxasize2.
	// Use sig.firstMidQt, sig.firstQtGpt.

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<int> vQindx1s = get_index_1s(quotes, linMod.openMsecs, linMod.closeMsecs);

	int nQuotes = quotes->size();
	const int& gpts = linMod.gpts;
	vector<hff::StateInfo>& sI = sig.sI;

	int n = 4;
	vector<vector<int> > mba(n, vector<int>(gpts));

	int dT = max_qtmax_lag_ % linMod.gridInterval;

	int timeIdx = 0;
	while( timeIdx < nQuotes && (*quotes)[timeIdx].msecs <= linMod.openMsecs )
		++timeIdx;

	int checkFirstQt = 1;
	int firstQtMsec = 0;
	for(int i = 1; i < gpts; ++i)
	{
		int newQuotes = 0;
		int maxBid = 0;
		int maxAsk = 0;
		int dmaxBid = 0;
		int dmaxAsk =0;
		int sec = i * linMod.gridInterval;

		while((timeIdx<nQuotes) && (((*quotes)[timeIdx].msecs - linMod.openMsecs) * 0.001 < i * linMod.gridInterval))
		{
			++newQuotes;
			const QuoteInfo& quote = (*quotes)[timeIdx];

			if(checkFirstQt)
			{
				if (quote.ask > 0 && quote.bid > 0 )
				{
					float midqt = .5 * (quote.ask + quote.bid);
					float sprd = basis_pts_ * (quote.ask - quote.bid) / midqt;
					if(fabs(sprd) <= skip_qt_ && sprd >= 0) // require a reasonably tight spread
					{
						sig.firstMidQt = .5 * (quote.ask + quote.bid);
						sig.firstQtGpt = i;
						firstQtMsec = quote.msecs - linMod.openMsecs;
						checkFirstQt = 0;
						sig.sprdOpen = sprd;
					}
				}
			}

			if((quote.msecs - linMod.openMsecs) * 0.001 >= i * linMod.gridInterval - max_qtmax_lag_)
			{
				if(quote.bidSize > maxBid) maxBid = quote.bidSize;
				if(quote.askSize > maxAsk) maxAsk = quote.askSize;
				if( (quote.msecs - linMod.openMsecs) * 0.001 >= i * linMod.gridInterval - dT )
				{
					if(quote.bidSize > dmaxBid) dmaxBid = quote.bidSize;
					if(quote.askSize > dmaxAsk) dmaxAsk = quote.askSize;
				}
			}

			++timeIdx;
		}

		// two possibilities: no quotes in interval, or some quotes
		int qIndx = vQindx1s[sec];
		if(newQuotes == 0 || qIndx < 0)
		{
			sI[i].mqut = sI[i-1].mqut;
			sI[i].sprd = sI[i-1].sprd;

			sI[i].bid = sI[i-1].bid;
			sI[i].ask = sI[i-1].ask;

			sI[i].bsize = sI[i-1].bsize;
			sI[i].asize = sI[i-1].asize;

			sI[i].tsinceq = sI[i-1].tsinceq + linMod.gridInterval * 1000;
			sI[i].tlastq = sI[i-1].tlastq;
			sI[i].validq = sI[i-1].validq;
		}
		else if(timeIdx <= nQuotes) 
		{
			const QuoteInfo& quote_p = (*quotes)[qIndx];

			sI[i].mqut = .5 * (quote_p.ask + quote_p.bid);

			if(sI[i].mqut > 0)
				sI[i].sprd = basis_pts_ * (quote_p.ask - quote_p.bid) / sI[i].mqut;

			sI[i].ask = quote_p.ask;
			sI[i].bid = quote_p.bid;

			sI[i].bsize = quote_p.bidSize;
			sI[i].asize = quote_p.askSize;

			sI[i].tsinceq =	i * linMod.gridInterval * 1000 - (quote_p.msecs - linMod.openMsecs);
			sI[i].tlastq = (quote_p.msecs - linMod.openMsecs);
			sI[i].validq = 1;
		}

		mba[0][i] = maxBid;
		mba[1][i] = maxAsk;
		mba[2][i] = dmaxBid;
		mba[3][i] = dmaxAsk;

		sI[i].persistentChina = get_persistent(sec, 4, quotes, vQindx1s, sI[i]);

		if( (sI[i].bsize <= 0 || sI[i].asize <= 0)   )    // 4/25/2005
			sI[i].validq = 0;
		if( fabs(sI[i].sprd) > skip_qt_ || sI[i].mqut <= min_price_ || sig.firstMidQt == 0)  // spread needs to be narrow and price needs to be positive 
			sI[i].validq = 0;

		if(sI[i].validq > 0 && sec * 1000 > firstQtMsec)
		{
			double sprd = 0.;

			sI[i].mret1 = get_mret(sec, 1, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret5 = get_mret(sec, 5, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret15 = get_mret(sec, 15, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret30 = get_mret(sec, 30, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret60 = get_mret(sec, 60, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret120 = get_mret(sec, 120, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret300 = get_mret(sec, 300, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret600 = get_mret(sec, 600, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret1200 = get_mret(sec, 1200, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret2400 = get_mret(sec, 2400, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
			sI[i].mret4800 = get_mret(sec, 4800, firstQtMsec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt, sprd);
		}

		if( fabs(sI[i].mret60) > om_max_ret_ )  
			sI[i].validq = 0;
		if( fabs(sI[i].mret30) > om_max_ret_ )  
			sI[i].validq = 0;
		if( fabs(sI[i].mret15) > om_max_ret_ )  
			sI[i].validq = 0;
		if( fabs(sI[i].mret5) > om_max_ret_ )  
			sI[i].validq = 0;
		if( fabs(sI[i].mret1) > om_max_ret_ )  
			sI[i].validq = 0;
	}

	// Get the max in the interval
	int nInts = max_qtmax_lag_ / linMod.gridInterval;
	int nInts2 = max_qtmax_lag2_ / linMod.gridInterval;
	for(int i = 1; i < gpts; ++i)
	{
		int endj = min(nInts, i); 
		int maxb = mba[2][i - endj];
		int maxa = mba[3][i - endj];

		int endj2 = min(nInts2, i); 
		int maxb2 = 0;
		int maxa2 = 0; // because dT2 = 0

		for(int j = 0; j < endj; ++j)
		{
			if(mba[0][i - j] > maxb)
				maxb = mba[0][i - j];
			if(mba[1][i - j] > maxa)
				maxa = mba[1][i - j];	
		}
		for(int j = 0; j < endj2; ++j)
		{
			if(mba[0][i - j] > maxb2)
				maxb2 = mba[0][i - j];
			if(mba[1][i - j] > maxa2)
				maxa2 = mba[1][i - j];
		}
		sI[i].maxbsize = maxb;
		sI[i].maxasize = maxa;
		sI[i].maxbsize2 = maxb2;
		sI[i].maxasize2 = maxa2;
	}
}

void get_filImb_stateinfo(hff::SigC& sig, const vector<TradeInfo>* trades, const vector<QuoteInfo>* quotes)
{
	// Calculate sI.sig1[1].
	// Use sI.tsincet, sI.validt, sI.validq, sI.tlastt, sI.ltrade.

	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const int& gpts = linMod.gpts;
	vector<hff::StateInfo>& sI = sig.sI;

	int qtIdx = 1;
	for(int i = 1; i < gpts; ++i)
	{
		// filImb
		// was there a trade in the last 60 seconds? if yes, there is a signal: ow no signal (set signal to zero?)
		double mid, sprd;
		if( (sI[i].tsincet  < 1000 * om_stale_trd_)  &&  sI[i].validt && sI[i].validq )
		{
			// search for quote PRIORQUTIME seconds PRIOR to the trade
			while( (qtIdx < nQuotes)  && (((*quotes)[qtIdx].msecs - linMod.openMsecs) < (sI[i].tlastt - 1000 * om_priorqutime_) ) ) 
				qtIdx++;

			const QuoteInfo& quote_p = (*quotes)[qtIdx - 1];

			if( quote_p.ask > quote_p.bid && quote_p.msecs - linMod.openMsecs < (sI[i].tlastt - 1000 * om_priorqutime_ ))
			{
				mid = .5 * ( quote_p.bid + quote_p.ask);
				if( mid >= min_price_ ) 
				{
					sprd = basis_pts_ * (quote_p.ask - quote_p.bid) / mid;
					if( sprd <= skip_qt_ ) 
						sI[i].sig1[1] = 2. * (sI[i].ltrade - mid ) / (quote_p.ask - quote_p.bid);
				}
			}
		}
		// what happens if there are no quotes for some time? need to think about this...
		// what should we do in the case that this signal is bigger than one (so tha the trade price was outside of the 
		// bid ask? either set it to zero or set it to abs 1. 
		// it makes more sense to me to set it to abs 1
		//if (fabs(sI[i].sig1[1]) > om_fill_imb_clip_)
		//	sI[i].sig1[1] = om_fill_imb_clip_ * sI[i].sig1[1] / fabs(sI[i].sig1[1]);
		clip(sI[i].sig1[1], om_fill_imb_clip_);
	}
}

void get_targets_stateinfo(hff::SigC& sig, const vector<QuoteInfo>* quotes)
{
	// Calculate sI.target60Intra, sI.targetClose, sI.targetNextClose.
	// Update sI.validq, sI.validt.
	// Use sig.closeNextCloseRet.

	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const int& gpts = linMod.gpts;
	const int& n1sec = linMod.n1sec;
	vector<hff::StateInfo>& sI = sig.sI;

	// Delay 1 second. -> not any more.
	vector<double> vMid1d(linMod.n1sec);
	get_mid_series(vMid1d, quotes, linMod.openMsecs, linMod.closeMsecs, 0, false);

	int L1 = 60;
	int L5 = 300;
	int L10 = 600;
	int L60 = tm_target_60_; // 20051215 : make NASD and NY hori the same
	int L_CLOSE = linMod.n1sec - 1;

	int ki, kf;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const vector<pair<int, int> >& vHorizonLag = nonLinMod.vHorizonLag;
	for(int i = 1; i < gpts; ++i)
	{
		int sec = i * linMod.gridInterval;

		// Loop horizons.
		int nT = vHorizonLag.size();
		for( int iT = 0; iT < nT; ++iT )
		{
			int horizon = vHorizonLag[iT].first;
			int lag = vHorizonLag[iT].second;

			kf = min(sec + horizon + lag, n1sec - 1);
			ki = min(sec + lag, n1sec - 1);
			if(vMid1d[ki] > 0. && vMid1d[kf] > 0.)
			{
				sI[i].target[iT] = basis_pts_ * (vMid1d[kf] / vMid1d[ki] - 1.);

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

		// to close.
		int maxHorizon = nonLinMod.maxHorizon;
		//kf = min(sec + L_CLOSE, n1sec - 1); // maxHorizon -> Close
		//ki = min(sec + maxHorizon, n1sec - 1);
		//if(vMid1d[ki] > 0. && vMid1d[kf] > 0.)
			//sI[i].target60Intra = basis_pts_ * (vMid1d[kf] / vMid1d[ki] - 1);

		//clip(sI[i].target60Intra, tm_target_60_clip_);

		kf = min(sec + L_CLOSE, n1sec - 1); // -> to close
		ki = min(sec, n1sec - 1);
		if(vMid1d[ki] > 0. && vMid1d[kf] > 0.)
		{
			sI[i].targetClose = basis_pts_ * (vMid1d[kf] / vMid1d[ki] - 1);
			sI[i].targetNextClose = sI[i].targetClose + sig.closeNextCloseRet;
		}

		// clip.
		clip(sI[i].targetClose, linMod.on_target_clip);
		clip(sI[i].targetNextClose, linMod.on_target_clip);

		// Copy. Originals will be hedged.
		//sI[i].target60IntraUH = sI[i].target60Intra;
		sI[i].targetCloseUH = sI[i].targetClose;
		sI[i].targetNextCloseUH = sI[i].targetNextClose;
	}

	// make the last grid point not valid 
	sI[gpts-1].validq = sI[gpts-1].validt = 0;


	// gptOK.
	for(int i = 1; i < gpts; i++)
	{
		if( sI[i].validq && sI[i].validt /*&& sig.dayok*/ )
		{
			// To prevent certain wild price movement. Not sure if affects fit much.
			if( fabs(sI[i].target[0] * sI[i].mret60) < om_max_tar_ret_ && fabs(sI[i].target[0] * sI[i].mret30) < om_max_tar_ret_ )
				sI[i].gptOK = 1;
		}
	}
}

void get_tob_stateinfo(TickTobAcc* tta, hff::SigC& sig, int idate, const string& ticker)
{
	// Calculate sI.sig1[8], sI.sig1[9], sI.sig1[10], sI.sig1[11], sI.tobOK.
	// Use sI.validq, sI.validt, sI.bid, sI.ask, sI.askSize, sI.bidSize.

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<hff::StateInfo>& sI = sig.sI;

	tta->LoadData(ticker, idate);
	int msecsRoll = linMod.gridInterval * 1000;

	for( int i = 1; i < linMod.gpts; ++i )
	{
		tta->RollForward(linMod.openMsecs + i * msecsRoll - 1);
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
			if( fabs(spreadper) > skip_qt_ || midPrice <= min_price_)  // added on 1/6/2005
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

		sI[i].sig1[8]  = sI[i].sig10[8] = inputs[2]; // TOBqI2
		sI[i].sig1[9]  = sI[i].sig10[9] = inputs[4]; // TOBqI3
		sI[i].sig1[10] = sI[i].sig10[10] = inputs[1]; // TOBoff1
		sI[i].sig1[11] = sI[i].sig10[11] = inputs[3]; // TOBoff2
		sI[i].tobOK = 1; // added on 11/26/07
	}
}

void get_book_stateinfo(map<string, OrderBkAcc<OrderData> >& obaMap, hff::SigC& sig,
		const string& market, int idate, const string& ticker, const vector<string>& okECNs)
{
	// Calculate sI.sigBook[0] - sI.sigBook[7], sI.sigBook[8] - sI.sigBook[27].
	// Use sI.validq, sI.validt

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<hff::StateInfo>& sI = sig.sI;

	vector<string> mkts = okECNs;
	if( "US" != market )
		mkts.push_back(market);
	int nbooks = int(mkts.size());
	for(int s = 0; s < nbooks; ++s)
		obaMap[mkts[s]].LoadData(ticker, idate);

	int msecsRoll = linMod.gridInterval * 1000;

	const int maxLevels = 1000;
	const unsigned nVolFrac = 6;
	const unsigned nSprdBins = 7;
	const double volumeFrac[nVolFrac] = {0.01, 0.02, 0.04, 0.08, 0.16, 0.32};
	const double sprdBins[nSprdBins] = {0.25, 0.5, 1., 2., 4., 8., 16.};
	vector<double> bookDepthMO(nVolFrac);
	vector<double> bookDepthQI(nSprdBins);
	vector<double> vBookDepthQI(nSprdBins);

	for(int i = 1; i< linMod.gpts; ++i)
	{
		std::fill(bookDepthMO.begin(), bookDepthMO.end(), 0.);
		std::fill(bookDepthQI.begin(), bookDepthQI.end(), 0.);
		std::fill(vBookDepthQI.begin(), vBookDepthQI.end(), 0.);

		int msecs = linMod.openMsecs + i * msecsRoll - 1;
		for(int s = 0; s < nbooks; ++s)
			obaMap[mkts[s]].RollForward(msecs);

		if(sI[i].validq + sI[i].validt < 2)
			continue; 
		if(sI[i].tobOK == 0)
			continue;

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

		// check tobok



		vector<const OrderData*> bidSide(maxLevels);
		vector<const OrderData*> askSide(maxLevels);
		int nBidSide = 0,nAskSide = 0;
		obk.GetBidSideBook(maxLevels, &bidSide[0], &nBidSide);
		obk.GetAskSideBook(maxLevels, &askSide[0], &nAskSide);     

		int nLevels = nBidSide < nAskSide ? nBidSide : nAskSide; //only consider the lesser number of price levels on either side
		nLevels = min(nLevels, maxLevels); 
		if(nLevels < 2)
			continue; 

		vector<double> inputs(hff::max_book_sigs_);
		vector<double> sprdi(max_book_levels_);
		double sizeratMax = 20;
		// if NBBO not close to inside TOB continue 
		if( fabs( askSide[0]->RealPrice() - sI[i].ask) > min_tob_nbbo_dif_ ) 
			continue;
		if( fabs( bidSide[0]->RealPrice() - sI[i].bid) > min_tob_nbbo_dif_ ) 
			continue;

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
			spreadper = basis_pts_ * spreadOff/midPrice;
		if( fabs(spreadper) > skip_qt_ ||  midPrice <= min_price_)  // added on 1/6/2005 
			continue;

		float pmedmedSprd = sig.medSprd * midPrice;
		calcBookDepthSigsMO(nLevels, &bidSide[0], nLevels, &askSide[0],
				midPrice, spreadOff, sig.avgDlyVolume, nVolFrac, volumeFrac, bookDepthMO);
		calcBookDepthSigsQI(nLevels, &bidSide[0], nLevels, &askSide[0],
				midPrice, pmedmedSprd, nSprdBins, sprdBins, bookDepthQI, vBookDepthQI, sig.avgDlyVolume);

		sI[i].sigBook[8] = bookDepthMO[0]; // BOffmedVol.01
		sI[i].sigBook[14] = bookDepthMO[1]; // BOffmedVol.02 *
		sI[i].sigBook[15] = bookDepthMO[2]; // BOffmedVol.04 ***
		sI[i].sigBook[16] = bookDepthMO[3]; // BOffmedVol.08 ****
		sI[i].sigBook[17] = bookDepthMO[4]; // BOffmedVol.16 **
		sI[i].sigBook[18] = bookDepthMO[5]; // BOffmedVol.32

		sI[i].sigBook[19] = bookDepthQI[0]; // BmedSprdqI.25
		sI[i].sigBook[20] = bookDepthQI[1]; // BmedSprdqI.5
		sI[i].sigBook[21] = bookDepthQI[2]; // BmedSprdqI1 ****
		sI[i].sigBook[22] = bookDepthQI[3]; // BmedSprdqI2 ***
		sI[i].sigBook[23] = bookDepthQI[4]; // BmedSprdqI4 **
		sI[i].sigBook[24] = bookDepthQI[5]; // BmedSprdqI8 *
		sI[i].sigBook[25] = bookDepthQI[6]; // BmedSprdqI16

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
					inputs[4 * x - 2] = qtSize/bbboSize;   // qutRat
					if(sizeratMax > 0.0 && inputs[4 * x - 2] >  sizeratMax) inputs[4 * x - 2] = sizeratMax;
					inputs[4 * x - 1] = spreadOff / spreadX;
				}
			}
		}

		sI[i].sigBook[0] = inputs[2];	 //  qutRat
		sI[i].sigBook[1] = inputs[6];    //  qutRat
		sI[i].sigBook[2] = inputs[3];	 //  sprdRat BsRat1
		sI[i].sigBook[3] = inputs[7];    //  sprdRat BsRat2
		sI[i].sigBook[4] = inputs[4];    //  qutImb BqI2
		sI[i].sigBook[5] = inputs[8];	 //  qutImb
		sI[i].sigBook[6] = inputs[1];	 //  offset Boff1
		sI[i].sigBook[7] = inputs[5];    //  offset Boff2
	}
}

void get_signals(hff::SigC& sig, Sessions& sessions)
{
	// Calculate sig.logVolu, sig.logPrice, sig.logMedSprd, sig.prevDayVolume, sI.valid,
	//   sI.sig10[2], sI.sig1[14], sI.sig10[3], sI.sig1[15], sI.sig10[5], sI.sig10[6], sI.mret2400L, sI.mret4800L, sI.mretOpen, sI.sig10[0],
	//   sI.sig10[1], sI.quimMax2, sI.sig10[4], sI.sig10[7], sI[k].voluMom600, sI.voluMom3600, sI.intraVoluMom,
	//   sI.sig1[0], sI.sig1[3], sI.sig1[2], sI.sig1[4], sI.absSprd.
	// Use sI.validq, sig.adjPrevClose, sI.mqut, sig.avgDlyVolume, sig.adjPrevClose, sig.medSprd, sig.fxRate, sig.prevDayVolume, sig.avgDlyVolat,
	//   sig.dayok, sig.inUnivFit
	// Update sI.validq, sI.validt, sI.bsize, sI.asize
	// sI.sig1[0] = quoteImb.
	// sI.sig1[1] = fillImb at last trade within 60 seconds.
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
	// sig10[0] = qutImbWt.
	// sig10[1] = qutImbMax.
	// sig10[2] = mret300.
	// sig10[3] = mret300L.
	// sig10[4] = hilo.
	// sig10[5] = mret600L.
	// sig10[6] = mret1200L.
	// sig10[7] = onret.
	// sig10[8] = TOB.
	// sig10[9] = TOB.
	// sig10[10] = TOB.
	// sig10[11] = TOB.

	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	vector<hff::StateInfo>& sI = sig.sI;

	int ON = 900 / linMod.gridInterval;
	int L1 = 60 / linMod.gridInterval, L5 = 300 / linMod.gridInterval, L10 = 600 / linMod.gridInterval, L20 = 1200 / linMod.gridInterval;
	int L40 = 2400 / linMod.gridInterval, L60 = 3600 / linMod.gridInterval, L80 = 4800 / linMod.gridInterval, L160 = 9600 / linMod.gridInterval;
	int L120 = 7200 / linMod.gridInterval;

	double qutSizeAdj = .01; // using the new TSX nbbo, starting 10/7/09

	double onret3 = 0;
	if(sI[ON].validq && sig.adjPrevClose > 0)
		onret3 = basis_pts_ * (sI[ON].mqut / sig.adjPrevClose - 1);

	double effOpen = 0;
	if(sI[ON].validq)
		effOpen = sI[ON].mqut;

	//if(sig.dayok)
	{
		sig.logVolu = log10(sig.avgDlyVolume);
		sig.logPrice = log10(sig.adjPrevClose);
		sig.logMedSprd = log10(sig.medSprd);

		//if(sig.avgDlyVolume > 0)
		//	sig.prevDayVolume  = sig.prevDayVolume / sig.avgDlyVolume;
	}

	for(int k = 0; k < linMod.gpts; ++k)
	{
		int msecs = k * linMod.gridInterval * 1000 + linMod.openMsecs;
		int sec = k * linMod.gridInterval;

		if(sig.avgDlyVolume <= 0 || sig.avgDlyVolat <= 0 )
		{
			sI[k].validq = 0;
			sI[k].validt = 0;
		}

		// Don't allow negative sprds outside North America.
		//if( model.substr(0, 2) != "US" && model.substr(0, 2) != "UF" && model.substr(0, 2) != "CA" && model.substr(0, 1) != "E" )
		//	if( sI[k].bid >= sI[k].ask )
		//		sI[k].validq = 0;
		if( !linMod.allowCross )
			if( sI[k].bid >= sI[k].ask )
				sI[k].validq = 0;

		sI[k].bsize *= qutSizeAdj;
		sI[k].asize *= qutSizeAdj;

		if(sI[k].validq && sI[k].validt && /*sig.dayok &&*/ sig.inUnivFit)
			sI[k].valid = 1;

		if( false )
		{
			sI[k].sig1[13] = sI[k].mret5;
			sI[k].sig1[12] = sI[k].mret15;
			sI[k].sig1[7] = sI[k].mret30;
		}

		if( !sessions.inSessionStrict(msecs) )
			sI[k].valid = sI[k].validq = sI[k].validt = 0;

		// mret300 clip
		sI[k].sig10[2] = sI[k].mret300;
		clip(sI[k].sig10[2], linMod.clip_ret300);

		sI[k].sig1[14] = sI[k].sig10[2];

		// mret300L 
		if(k > L5)
			sI[k].sig10[3] = sI[k - L5].mret300;
		clip(sI[k].sig10[3], linMod.clip_ret300);

		sI[k].sig1[15] = sI[k].sig10[3];			

		// mret 600 lagged 600 
		if(k > L10)
			sI[k].sig10[5] = sI[k - L10].mret600; // ok since, unlike, sig[2], no nonlinear transform
		clip(sI[k].sig10[5], max_ret_);

		// mret 1200 lagged 1200
		if(k > L20)
			sI[k].sig10[6] = sI[k - L20].mret1200; 
		clip(sI[k].sig10[6], max_ret_);

		// mret 2400 lagged 2400
		if(k > L40)
			sI[k].mret2400L = sI[k - L40].mret2400;
		clip(sI[k].mret2400L, max_ret_);

		// mret 4800 lagged 4800
		if(k > L80)
			sI[k].mret4800L = sI[k - L80].mret4800;
		clip(sI[k].mret4800L, max_ret_);

		// mretOpen
		if(k > 0 && sig.firstMidQt > 0)
			sI[k].mretOpen = basis_pts_*(sI[k].mqut / sig.firstMidQt - 1);
		clip(sI[k].mretOpen, max_ret_);

		// qutImbWt
		if(sI[k].validq && sI[k].tsinceq  < 1000 * max_qtwt_lag_)
		{
			sI[k].sig10[0] = ((double)(sI[k].bsize - sI[k].asize)) / (sI[k].bsize + sI[k].asize);
			sI[k].sig10[0] *= sqrt(max(sI[k].bsize, sI[k].asize) / sig.avgDlyVolume);
		}

		// qutImbMax
		if(sI[k].validq && sI[k].tsinceq  < 1000 * max_qtmax_lag_ )
			sI[k].sig10[1] = ((double)(sI[k].maxbsize - sI[k].maxasize)) / (sI[k].maxbsize + sI[k].maxasize);

		// qutImbMax2
		if(sI[k].validq && sI[k].tsinceq  < 1000 * max_qtmax_lag2_ )
			sI[k].quimMax2 = ((double)(sI[k].maxbsize2 - sI[k].maxasize2)) / (sI[k].maxbsize2 + sI[k].maxasize2);

		//scale hilo by vol: this should not be used anywhere 
		sI[k].sig10[4] *= sig.avgDlyVolat;
		sI[k].sig10[4] = sI[k].relativeHiLo;

		// the on signal
		if(k < ON && sI[k].validq && sig.adjPrevClose > 0)
			sI[k].sig10[7] = basis_pts_ * ( sI[k].mqut / sig.adjPrevClose - 1);
		if(k >= ON)
			sI[k].sig10[7] = onret3;	
		clip(sI[k].sig10[7], max_ret_);

		// if data is not valid, set signals and target to zero; (so nonsense does not enter hedge)
		if( sI[k].validq + sI[k].validt < 2 )
		{
			for(int n = 0; n < hff::tm_num_basic_sig_; ++n)
				sI[k].sig10[n] = 0.;

			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
				sI[k].target[iT] = 0.;
		}

		// one minute signals 
		if (sI[k].validq && sI[k].tsinceq  < 1000 * om_stale_qut_)
			sI[k].sig1[0] = ((float)(sI[k].bsize - sI[k].asize)) / (sI[k].bsize + sI[k].asize);

		// fillImb (sig1[1]) and hilo (sig1[3]) done earlier
		// signals mret300 (sig1[14]) and mret300Lag (sig1[15]) are calculated above
		sI[k].sig1[3] = sI[k].relativeHiLo;

		if(sI[k].validq)
			sI[k].sig1[2] = sI[k].mret60;
		clip(sI[k].sig1[2], linMod.clip_ret60);

		// Do this in MMakeSample::get_prediction().
		if( 0 )
		{
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
				if( vPred != 0 )
					sI[k].sig1[vIndx[indxIndx]] = (*vPred)[sec];
			}
		}

		//if(sig.dayok)
		{
			sI[k].absSprd = fabs(sI[k].sprd);
			clip(sI[k].absSprd, multi_lin_sprd_clip_);
		}
	}	// end of loop over k
}

void get_MI_signals(SigC& sig, const vector<OrderQty>* trades)
{
	vector<hff::StateInfo>& sI = sig.sI;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	double f600 = float(linMod.n1sec - 1) / 600;
	double f3600 = float(linMod.n1sec - 1) / 3600;

	vector<int> tradeVol(linMod.n1sec, 0);
	if( trades != 0 )
	{
		for( vector<OrderQty>::const_iterator it = trades->begin(); it != trades->end(); ++it )
		{
			int bin = (it->msecs - linMod.openMsecs - 1) / 1000 + 1;
			if( bin >= 0 && bin < linMod.n1sec )
				tradeVol[bin] += it->signedQty;
		}
	}

	vector<int> tradeVolCum(linMod.n1sec, 0);
	get_cum(tradeVolCum, tradeVol);
	float normFac = (sig.avgDlyVolume > 0) ? sig.avgDlyVolume : 1.0;

	for( int i = 1; i < linMod.gpts; ++i )
	{
		int sec = i * linMod.gridInterval;
		float scaleFac = (float)(linMod.n1sec - 1) / (i * linMod.gridInterval);

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

void get_news_signals(SigC& sig, const string& ticker)
{
	vector<hff::StateInfo>& sI = sig.sI;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
/*
	const map<int, pair<int, int> >* newsmap = static_cast<const map<int, pair<int, int> >*>(MEvent::Instance()->get(ticker, "newsSentVol"));
	if( newsmap != 0 && !newsmap->empty() )
	{
		for( int i = 1; i < linMod.gpts; ++i )
		{
			int msso = i * linMod.gridInterval * 1000;
			map<int, pair<int, int> >::const_iterator it = newsmap->upper_bound(msso);
			if( it != newsmap->begin() )
				advance(it, -1);

			sI[i].newsSenti = it->second.first;
			sI[i].newsVol = it->second.second;
		}
	}
	*/
}

void calcBookDepthSigsMO(int nBidLevels, const OrderData** bidSide, int nAskLevels, const OrderData** askSide, 
		double midPrice, double spreadOff, double dailyVolume, unsigned nSigs, const double* volumeFrac, vector<double>& signals)
{
	if(nSigs == 0)
		return;
	int totBidSize = 0, totAskSize = 0, bidLevel = 0, askLevel = 0;
	for(unsigned sig = 0; sig < nSigs; ++sig)
	{
		int ms = (int)(volumeFrac[sig] * dailyVolume);
		for(; bidLevel < nBidLevels && totBidSize < ms; ++bidLevel)
			totBidSize += bidSide[bidLevel]->size;
		for(; askLevel < nAskLevels && totAskSize < ms; ++askLevel)
			totAskSize += askSide[askLevel]->size;

		if(bidLevel >= nBidLevels || askLevel >= nAskLevels)
			break;

		double bid1 = bidSide[bidLevel]->RealPrice();
		double ask1 = askSide[askLevel]->RealPrice();
		double sprd1 = ask1 - bid1;
		double mid1 = 0.5 * (ask1 + bid1);
		sprd1 = sprd1 > minSprdOff_ ? sprd1 : minSprdOff_;
		signals[sig] = (mid1 / midPrice - 1.) * (spreadOff / sprd1);
	}
}

void calcBookDepthSigsQI(int nBidLevels,const OrderData** bidSide, int nAskLevels, const OrderData** askSide,
		double midPrice, double medSprd, unsigned nSigs, const double* bins,
		std::vector<double>& signals, std::vector<double>& vsignals, double dailyVolume)
{     
	if(nSigs == 0)
		return;
	int bidLevel = 0, askLevel = 0;
	double totBidSize = 0, totAskSize = 0;
	for(unsigned sig = 0; sig < nSigs; ++sig)
	{   
		double bidLmt = midPrice - bins[sig] * medSprd;
		double askLmt = midPrice + bins[sig] * medSprd;
		for(; bidLevel < nBidLevels && bidSide[bidLevel]->RealPrice() >= bidLmt; ++bidLevel)
			totBidSize += bidSide[bidLevel]->size;
		for(; askLevel < nAskLevels && askSide[askLevel]->RealPrice() <= askLmt; ++askLevel)
			totAskSize += askSide[askLevel]->size;
		if(totBidSize + totAskSize > 0)
		{
			signals[sig] = (totBidSize - totAskSize) / (double)(totBidSize + totAskSize);
			if(dailyVolume > 0)
				vsignals[sig] = signals[sig] * (max(totBidSize, totAskSize) / dailyVolume);
		}
	}
}

double get_mret(int sec, int dsec, int firstQtMsec, const vector<QuoteInfo>* quotes,
		const vector<int>& vQindx1s, double mqut, double firstMidQt, double medSprd)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	double ret = 0.;
	if( sec * 1000 > firstQtMsec )
	{
		//int secFrom = max(sec - dsec, firstQtMsec / 1000 + 1);
		//bool backward_compatibility_1 = dsec < 30 || sec - secFrom >= 30; // i > K.
		//bool backward_compatibility_2 = secFrom / 30 > firstQtMsec / 1000 / 30; // K > firstQtGpt.

		//if( backward_compatibility_1 && backward_compatibility_2 && secFrom * 1000 > firstQtMsec && valid_quote( (*quotes)[vQindx1s[secFrom]] ) )
		//	ret = basis_pts_ * (mqut / get_mid((*quotes)[vQindx1s[secFrom]]) - 1);
		//else if( backward_compatibility_1 && !backward_compatibility_2 && secFrom * 1000 <= firstQtMsec )
		//	ret = basis_pts_ * (mqut / firstMidQt - 1);






		int secFrom = sec - dsec;
		bool backward_compatibility_0 = (sec - 1) / 30 + 1 >= firstQtMsec / 1000 / 30 + 1; // i >= firstQtGpt.
		bool backward_compatibility_1 = (secFrom - 1) / 30 + 1 > firstQtMsec / 1000 / 30 + 1; // K > firstQtGpt.
		bool backward_compatibility_2 = sec - secFrom >= 30; // i > K.
		bool backward_compatibility_3 = (secFrom - 1) / 30 + 1 <= firstQtMsec / 1000 / 30 + 1; // K <= firstQtGpt.
		bool backward_compatibility_4 = sec - (firstQtMsec / 1000 + 1) >= 30; // K <= firstQtGpt && i > K.

		QuoteInfo quoteFrom;
		if( secFrom >= 0 )
		{
			int qIndx = vQindx1s[secFrom];
			if( qIndx >= 0 )
				quoteFrom = (*quotes)[qIndx];
		}

		if( dsec < 30 )
		{
			if( backward_compatibility_0 )
			{
				//if( valid_quote(quotes, vQindx1s, secFrom) )
				//ret = basis_pts_ * (mqut / get_mid((*quotes)[vQindx1s[secFrom]]) - 1);
				if( valid_quote(quoteFrom, medSprd, linMod.minSpreadMMS, linMod.maxSpreadMMS) )
					ret = basis_pts_ * (mqut / get_mid(quoteFrom) - 1.);
			}
		}
		else if( dsec >= 30 )
		{
			if( backward_compatibility_1 && backward_compatibility_2 )
			{
				//if( valid_quote(quotes, vQindx1s, secFrom) )
				//	ret = basis_pts_ * (mqut / get_mid((*quotes)[vQindx1s[secFrom]]) - 1);
				if( valid_quote(quoteFrom, medSprd, linMod.minSpreadMMS, linMod.maxSpreadMMS) )
					ret = basis_pts_ * (mqut / get_mid(quoteFrom) - 1.);
			}
			else if( backward_compatibility_3 && backward_compatibility_4 )
			{
				//if( dsec == 30 && valid_quote(quotes, vQindx1s, secFrom) )
				//	ret = basis_pts_ * (mqut / get_mid((*quotes)[vQindx1s[secFrom]]) - 1);
				if( dsec == 30 && valid_quote(quoteFrom, medSprd, linMod.minSpreadMMS, linMod.maxSpreadMMS) )
					ret = basis_pts_ * (mqut / get_mid(quoteFrom) - 1.);
				else if ( dsec > 30 )
					ret = basis_pts_ * (mqut / firstMidQt - 1);
			}
		}
	}
	return ret;
}

bool get_persistent(const QuoteInfo& qFuture, hff::StateInfo& sI)
{
	bool persistent = fabs(sI.bid - qFuture.bid) < 1e-4 && fabs(sI.ask - qFuture.ask) < 1e-4;
	return persistent;
}

bool get_persistent(int sec, int dsec, const vector<QuoteInfo>* quotes, const vector<int>& vQindx1s, hff::StateInfo& sI)
{
	bool persistent = false;
	int secFuture = sec + dsec;
	int qIndx = vQindx1s[secFuture];
	if( qIndx >= 0 )
	{
		if( secFuture < vQindx1s.size() )
		{
			const QuoteInfo& qFuture = (*quotes)[qIndx];
			//persistent = fabs(sI.bid - qFuture.bid) < 1e-4 && fabs(sI.ask - qFuture.ask) < 1e-4;
			persistent = get_persistent(qFuture, sI);
		}
		else
			persistent = true;
	}
	return persistent;
}

double get_voluMom( int sec, int length, const vector<double>& trdVolu1sCum )
{
	double ret = 0.;
	int offset1 = sec - length;
	int offset2 = sec;
	if( offset1 > 0 || length == 0 )
	{
		if( length == 0 )
			offset1 = 0; // intraVoluMom.
		ret = trdVolu1sCum[offset2] - trdVolu1sCum[offset1];
	}

	return ret;
}

double get_volQty( int sec, int length, const vector<int>& tradeQtyCum )
{
	double ret = 0.;
	int offset1 = sec - length;
	int offset2 = sec;
	if( offset1 > 0 || length == 0 )
	{
		if( length == 0 )
			offset1 = 0; // mIIntra.
		ret = tradeQtyCum[offset2] - tradeQtyCum[offset1];
	}
	return ret;
}

double get_vwap_sig( int sec, int length, const vector<double>& sumVolTrdPrc1sCum, const vector<double>& trdVolu1sCum, double minVolu )
{
	double vwap = 0.;
	int offset1 = (length > 0) ? sec - length : 0;
	int offset2 = sec;
	if( offset1 < 0 )
		offset1 = 0;
	double sumVolTrdPrc = sumVolTrdPrc1sCum[offset2] - sumVolTrdPrc1sCum[offset1];
	double trdVolu = trdVolu1sCum[offset2] - trdVolu1sCum[offset1];

	if( trdVolu > minVolu )
	{
		double avgPrc = 0.;
		if( trdVolu > 0 )
			avgPrc = sumVolTrdPrc / trdVolu;

		double avgPrcD = 0.;
		double sumVolTrdPrcD = sumVolTrdPrc1sCum[sec];
		double trdVoluD = trdVolu1sCum[sec];
		if( trdVoluD > 0 )
			avgPrcD = sumVolTrdPrcD / trdVoluD;

		if( avgPrcD > 0 )
			vwap = avgPrc / avgPrcD - 1;
	}
	return vwap;
}

double get_vwap( int sec, int length, const vector<double>& sumVolTrdPrc1sCum, const vector<double>& trdVolu1sCum, double minVolu )
{
	double vwap = 0.;
	int offset1 = (length > 0) ? sec - length : 0;
	int offset2 = sec;
	if( offset1 < 0 )
		offset1 = 0;
	double sumVolTrdPrc = sumVolTrdPrc1sCum[offset2] - sumVolTrdPrc1sCum[offset1];
	double trdVolu = trdVolu1sCum[offset2] - trdVolu1sCum[offset1];

	double avgPrc = 0.;
	if( trdVolu > minVolu )
	{
		if( trdVolu > 0 )
			avgPrc = sumVolTrdPrc / trdVolu;

		//double avgPrcD = 0.;
		//double sumVolTrdPrcD = sumVolTrdPrc1sCum[sec];
		//double trdVoluD = trdVolu1sCum[sec];
		//if( trdVoluD > 0 )
		//	avgPrcD = sumVolTrdPrcD / trdVoluD;

		//if( avgPrcD > 0 )
		//	vwap = avgPrc / avgPrcD - 1;
	}
	return avgPrc;
}

void get_cum(vector<double>& vCum, const vector<double>& v)
{
	int vsize = v.size();
	vCum = vector<double>(vsize, 0.);
	vCum[0] = v[0];
	for( int i = 1; i < vsize; ++i )
		vCum[i] = vCum[i - 1] + v[i];
}

void get_cum(vector<int>& vCum, const vector<int>& v)
{
	int vsize = v.size();
	vCum = vector<int>(vsize, 0.);
	vCum[0] = v[0];
	for( int i = 1; i < vsize; ++i )
		vCum[i] = vCum[i - 1] + v[i];
}

} // namespace sig
