#include <gtlib_signal/sigFtns.h>
#include <gtlib_signal/haltedTickers.h>
#include <gtlib_model/hff.h>
#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GODBC.h>
#include <optionlibs/TickData.h>
using namespace std;
using namespace hff;

namespace gtlib {

unordered_map<string, string> get_uid_map(const vector<string>& markets, int idate, const set<string>& uids)
{
	unordered_map<string, string> mTickerUid;

	for( auto itm = markets.begin(); itm != markets.end(); ++itm )
	{
		string market = *itm;
		bool dayok = mto::dataOK(market, idate);

		//if( dayok )
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

bool read_chara_data(const CharaContainer* ch, SigC& sig, const string& model, const string& market,
		const string& uid, float on_target_clip, bool removeHardToBorrow, bool requireTickValid,
		int idate, int idate_p, int idate_p2, int idate_p3, int idate_p4, int idate_p5,
		int idate_p6, int idate_p7, int idate_p8, int idate_n)
{
	// Calculate sig.inUnivChara, sig.inUnivFit, sig.avgDlyVolume, sig.avgDlyVolat, sig.adjPrevClose, sig.medSprd, sig.prevDayVolume, sig.prevDayVolat,
	//   sig.mretIntraLag1, sig.hiloQAI, sig.hiloLag1Open, sig.hiloLag1Rat, sig.hiLag1, sig.loLag1, sig.mretIntraLag2, sig.hiloQAI2, sig.hiloLag2Open,
	//   sig.hiloLag2Rat, sig.hiLag2, sig.loLag2, sig.mretONLag1, sig.closeNextCloseRet.

	CharaInfo chara;
	CharaInfo chara_p;
	CharaInfo chara_p2;
	CharaInfo chara_p3;
	CharaInfo chara_p4;
	CharaInfo chara_p5;
	CharaInfo chara_p6;
	CharaInfo chara_p7;
	CharaInfo chara_p8;
	CharaInfo chara_n;

	bool valid_chara = ch->getCharaInfo(market, idate, uid, chara);
	bool valid_chara_p = ch->getCharaInfo(market, idate_p, uid, chara_p);
	bool valid_chara_p2 = ch->getCharaInfo(market, idate_p2, uid, chara_p2);
	bool valid_chara_p3 = ch->getCharaInfo(market, idate_p3, uid, chara_p3);
	bool valid_chara_p4 = ch->getCharaInfo(market, idate_p4, uid, chara_p4);
	bool valid_chara_p5 = ch->getCharaInfo(market, idate_p5, uid, chara_p5);
	bool valid_chara_p6 = ch->getCharaInfo(market, idate_p6, uid, chara_p6);
	bool valid_chara_p7 = ch->getCharaInfo(market, idate_p7, uid, chara_p7);
	bool valid_chara_p8 = ch->getCharaInfo(market, idate_p8, uid, chara_p8);
	bool valid_chara_n = ch->getCharaInfo(market, idate_n, uid, chara_n);

	float shareout = get_shareout(chara_p, chara_p2, chara_p3); // in case the information is missing.

	if( valid_chara_p && valid_chara )
	{
		string model02 = model.substr(0, 2);

		if( !calculate_chara_p(sig, chara, chara_p, shareout, model02, market, removeHardToBorrow, requireTickValid) )
			return false;

		if( valid_chara_p2 )
			calculate_chara_p2(sig, chara, chara_p, chara_p2, shareout);

		if( valid_chara_p2 && valid_chara_p3 )
			calculate_chara_p3(sig, chara, chara_p2, chara_p3);

		if( valid_chara_p3 && valid_chara_p4 )
			calculate_chara_p4(sig, chara, chara_p3, chara_p4);

		if( valid_chara_p4 && valid_chara_p5 )
			calculate_chara_p5(sig, chara, chara_p4, chara_p5);

		if( valid_chara_p5 && valid_chara_p6 )
			calculate_chara_p6(sig, chara, chara_p5, chara_p6);

		if( valid_chara_p6 && valid_chara_p7 )
			calculate_chara_p7(sig, chara, chara_p6, chara_p7);

		if( valid_chara_p7 && valid_chara_p8 )
			calculate_chara_p8(sig, chara, chara_p7, chara_p8);

		if( valid_chara_n && chara_n.close > 0 && chara.close > 0 && chara_n.adjust > 0 )
			calculate_chara_n(sig, chara, chara_n, on_target_clip);
	}
	else
		return false;

	if( sig.hiLag1 <= 0 || sig.hiLag2 <= 0 || sig.loLag1 <= 0 || sig.loLag2 <= 0 )
		return false;

	return true;
}

float get_shareout(CharaInfo& chara_p, CharaInfo& chara_p2, CharaInfo& chara_p3)
{
	if(chara_p.shareout > 0.)
		return chara_p.shareout;
	else if(chara_p2.shareout > 0.)
		return chara_p2.shareout;
	else if(chara_p3.shareout > 0.)
		return chara_p3.shareout;
	return 0.;
}

bool calculate_chara_p(SigC& sig, CharaInfo& chara, CharaInfo& chara_p, float shareout,
		const string& model02, const string& market, bool removeHardToBorrow, bool requireTickValid)
{
	sig.ticker = mto::compTicker(chara_p.ticker, market);
	sig.inUnivChara = chara_p.inUniv;
	sig.inUnivFit = chara_p.inUniv;
	sig.avgDlyVolume = chara_p.medVolume;
	sig.avgDlyVolat = chara_p.medVolatility * basis_pts_;
	sig.adjPrevClose = chara_p.close;
	sig.close = chara.close;
	sig.medSprd = chara_p.medmedSprd;
	sig.medNquotes = chara_p.medNquotes;
	sig.medNtrades = chara_p.medNtrades;
	if( sig.avgDlyVolume > 0. )
		sig.prevDayVolume = chara_p.volume / sig.avgDlyVolume;
	sig.prevDayVolat = chara_p.volatility * basis_pts_;
	sig.northBP = chara_p.nffundbp;
	sig.northRST = chara_p.nffundrst;
	sig.northTRD = chara_p.nffundtrd;
	double marketCap = shareout * chara_p.close;
	sig.logCap = marketCap > 1. ? log(marketCap) : 0.;
	sig.logShr = shareout > 1. ? log(shareout) : 0.;

	if( "US" == model02 || "UF" == model02 || "CA" == model02 )
		sig.exchangeChar = chara_p.market;
	else
		sig.exchangeChar = market.substr(1, 1);

	if( "US" == model02 || "UF" == model02 )
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

	if(false/*usUnivOverride*/)
	{
		if( "US" == model02 || "UF" == model02 )
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
	}

	if(removeHardToBorrow)
	{
		if(chara.positiveMaxPos < 1)
			sig.inUnivFit = 0;
	}

	if( chara.adjust > 0. )
		sig.adjPrevClose /= chara.adjust;

	if( mto::isInternational(market) && requireTickValid && chara.tickValid == 0 )
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

	sig.mretIntraLag1 = getMretIntraLag(chara_p);
	sig.hiloQAI = getHiloQAI(chara_p);
	sig.hiloLag1Open = getHiloLagOpen(chara_p);
	sig.hiloLag1Rat = getHiloLagRat(chara_p);

	sig.cwretIntraLag1 = sig.mretIntraLag1 * sig.logCap;

	sig.hiLag1 = chara_p.high;
	sig.loLag1 = chara_p.low;
	if( chara.adjust > 0. )
	{
		sig.hiLag1 /= chara.adjust;
		sig.loLag1 /= chara.adjust;
	}

	return true;
}

void calculate_chara_p2(SigC& sig, CharaInfo& chara, CharaInfo& chara_p, CharaInfo& chara_p2, float shareout)
{
	double marketCap = shareout * chara_p2.close;
	double logCap = marketCap > 1. ? log(marketCap) : 0.;
	double logShr = shareout > 1. ? log(shareout) : 0.;

	sig.mretIntraLag2 = getMretIntraLag(chara_p2);
	sig.hiloQAI2 = getHiloQAI(chara_p2);
	sig.hiloLag2Open = getHiloLagOpen(chara_p2);
	sig.hiloLag2Rat = getHiloLagRat(chara_p2);
	sig.mretONLag1 = getMretONLag(chara_p, chara_p2);

	sig.cwretIntraLag2 = sig.mretIntraLag2 * logCap;
	sig.cwretONLag1 = sig.mretONLag1 * logCap;

	sig.hiLag2 = chara_p2.high;
	sig.loLag2 = chara_p2.low;
	if( chara_p.adjust > 0 && chara.adjust > 0 )
	{
		float adjust = chara.adjust * chara_p.adjust;
		sig.hiLag2 /= adjust;
		sig.loLag2 /= adjust;
	}
}

void calculate_chara_p3(SigC& sig, CharaInfo& chara, CharaInfo& chara_p2, CharaInfo& chara_p3)
{
	sig.mretIntraLag3 = getMretIntraLag(chara_p3);
	sig.hiloQAI3 = getHiloQAI(chara_p3);
	sig.hiloLag3Open = getHiloLagOpen(chara_p3);
	sig.hiloLag3Rat = getHiloLagRat(chara_p3);
	sig.mretONLag2 = getMretONLag(chara_p2, chara_p3);
}

void calculate_chara_p4(SigC& sig, CharaInfo& chara, CharaInfo& chara_p3, CharaInfo& chara_p4)
{
	sig.mretIntraLag4 = getMretIntraLag(chara_p4);
	sig.hiloQAI4 = getHiloQAI(chara_p4);
	sig.hiloLag4Open = getHiloLagOpen(chara_p4);
	sig.hiloLag4Rat = getHiloLagRat(chara_p4);
	sig.mretONLag3 = getMretONLag(chara_p3, chara_p4);
}

void calculate_chara_p5(SigC& sig, CharaInfo& chara, CharaInfo& chara_p4, CharaInfo& chara_p5)
{
	sig.mretIntraLag5 = getMretIntraLag(chara_p5);
	sig.hiloQAI5 = getHiloQAI(chara_p5);
	sig.hiloLag5Open = getHiloLagOpen(chara_p5);
	sig.hiloLag5Rat = getHiloLagRat(chara_p5);
	sig.mretONLag4 = getMretONLag(chara_p4, chara_p5);
}

void calculate_chara_p6(SigC& sig, CharaInfo& chara, CharaInfo& chara_p5, CharaInfo& chara_p6)
{
	sig.mretIntraLag6 = getMretIntraLag(chara_p6);
	sig.hiloQAI6 = getHiloQAI(chara_p6);
	sig.hiloLag6Open = getHiloLagOpen(chara_p6);
	sig.hiloLag6Rat = getHiloLagRat(chara_p6);
	sig.mretONLag5 = getMretONLag(chara_p5, chara_p6);
}

void calculate_chara_p7(SigC& sig, CharaInfo& chara, CharaInfo& chara_p6, CharaInfo& chara_p7)
{
	sig.mretIntraLag7 = getMretIntraLag(chara_p7);
	sig.hiloQAI7 = getHiloQAI(chara_p7);
	sig.hiloLag7Open = getHiloLagOpen(chara_p7);
	sig.hiloLag7Rat = getHiloLagRat(chara_p7);
	sig.mretONLag6 = getMretONLag(chara_p6, chara_p7);
}

void calculate_chara_p8(SigC& sig, CharaInfo& chara, CharaInfo& chara_p7, CharaInfo& chara_p8)
{
	sig.mretIntraLag8 = getMretIntraLag(chara_p8);
	sig.hiloQAI8 = getHiloQAI(chara_p8);
	sig.hiloLag8Open = getHiloLagOpen(chara_p8);
	sig.hiloLag8Rat = getHiloLagRat(chara_p8);
	sig.mretONLag7 = getMretONLag(chara_p7, chara_p8);
}

float getMretIntraLag(CharaInfo& chara)
{
	float ret = 0.;
	if( chara.open > 0. && chara.close > 0. )
	{
		ret = basis_pts_ * (chara.close / chara.open - 1.);
		if( ret > max_ret_ )
			ret = max_ret_;
		else if( ret < - max_ret_ )
			ret = - max_ret_;
	}
	return ret;
}

float getMretONLag(CharaInfo& chara_p, CharaInfo& chara_p2)
{
	float ret = 0.;
	float chara_p2_close_adj = chara_p2.close;
	if( chara_p.adjust > 0 )
		chara_p2_close_adj /= chara_p.adjust;
	if( chara_p2_close_adj > 0. )
	{
		ret = basis_pts_ * ( chara_p.open / chara_p2_close_adj - 1. );
		if( ret > max_ret_ )
			ret = max_ret_;
		else if( ret < - max_ret_ )
			ret = - max_ret_;
	}
	return ret;
}

float getHiloQAI(CharaInfo& chara_p)
{
	float ret = 0.;
	if( chara_p.open > 0. && chara_p.close > 0. && chara_p.low > 0. && chara_p.high > chara_p.low )
	{
		ret = 2. * (chara_p.close - chara_p.low) / (chara_p.high - chara_p.low) - 1.;
		if( ret > 1. )
			ret = 1.;
		else if( ret < - 1. )
			ret = - 1.;
	}
	return ret;
}

float getHiloLagOpen(CharaInfo& chara_p)
{
	float ret = 0.;
	if( chara_p.open > 0. && chara_p.close > 0. && chara_p.low > 0. && chara_p.high > chara_p.low )
	{
		ret = 2. * (chara_p.open - chara_p.low) / (chara_p.high - chara_p.low) - 1.;
		if( ret > 1. )
			ret = 1.;
		else if( ret < - 1. )
			ret = - 1.;
	}
	return ret;
}

float getHiloLagRat(CharaInfo& chara_p)
{
	float ret = 0.;
	if( chara_p.open > 0. && chara_p.close > 0. && chara_p.low > 0. && chara_p.high > chara_p.low )
		ret = (chara_p.high - chara_p.low) / chara_p.close;
	return ret;
}

void calculate_chara_n(SigC& sig, CharaInfo& chara, CharaInfo& chara_n, float on_target_clip)
{
	float chara_close_adjust = chara.close / chara_n.adjust;
	sig.closeNextCloseRet = basis_pts_ * (chara_n.close / chara_close_adjust - 1.);
	clip(sig.closeNextCloseRet, on_target_clip);

	sig.tarON = basis_pts_ * (chara_n.open / chara_close_adjust - 1.);
	clip(sig.tarON, on_target_clip);

	sig.tarClcl = basis_pts_ * (chara_n.close / chara_close_adjust - 1.);
	clip(sig.tarClcl, on_target_clip);
}

void get_cum(vector<int>& vCum, const vector<int>& v)
{
	int vsize = v.size();
	vCum = vector<int>(vsize, 0.);
	vCum[0] = v[0];
	for( int i = 1; i < vsize; ++i )
		vCum[i] = vCum[i - 1] + v[i];
}

double get_voluMom(int openMsecs, int msecs, int length, const vector<TradeInfo>& trades, float avgVolu, float n1sec)
{
	if(avgVolu <= 0.)
		return 0.;

	double qsum = 0.;
	int msecsFrom = (length == 0) ? 0 : msecs - length * 1000;
	for( auto it = trades.rbegin(); it != trades.rend(); ++it )
	{
		if( it->msecs < msecsFrom )
			break;
		if( it->msecs <= msecs )
			qsum += it->qty;
	}

	int sec = (length == 0) ? (msecs - openMsecs)/1000 : length;
	float scaleFac = (n1sec - 1.) / sec;

	double vm = qsum * scaleFac / avgVolu;
	return vm;
}

double get_voluMomPartial(int openMsecs, int msecs, int length, const vector<TradeInfo>& trades)
{
	double vm = 0.;
	//if(length == 0 || (msecs - openMsecs)/1000 > length)
	{
		int msecsFrom = (length == 0) ? 0 : msecs - length * 1000;
		for( auto it = trades.rbegin(); it != trades.rend(); ++it )
		{
			if( it->msecs < msecsFrom )
				break;
			if( it->msecs <= msecs )
				vm += it->qty;
		}
	}
	return vm;
}

double get_voluMom(int openMsecs, int msecs, int length, const vector<TradeInfo>& trades)
{
	double vm = 0.;
	if(length == 0 || (msecs - openMsecs)/1000 > length)
	{
		int msecsFrom = (length == 0) ? 0 : msecs - length * 1000;
		for( auto it = trades.rbegin(); it != trades.rend(); ++it )
		{
			if( it->msecs < msecsFrom )
				break;
			if( it->msecs <= msecs )
				vm += it->qty;
		}
	}
	return vm;
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

double get_logBlockVol(int msecs, int length, const map<int, float>& m)
{
	double vol = 0.;
	for(auto& kv : m)
	{
		if(kv.first < msecs && (msecs - kv.first)/1000 <= length)
			vol += kv.second;
	}
	double ret = 0.;
	if(vol > 0.)
		ret = log(vol);
	return log(vol);
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

int get_max_bidSize(int msecs, int length, const vector<QuoteDataMicro>& quotes)
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

int get_max_askSize(int msecs, int length, const vector<QuoteDataMicro>& quotes)
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

bool get_persistent(const QuoteInfo& qFuture, StateInfo& sI)
{
	bool persistent = fabs(sI.bid - qFuture.bid) < 1e-4 && fabs(sI.ask - qFuture.ask) < 1e-4;
	return persistent;
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

void calcBookDepthSigsQIMicro(int nBidLevels, vector<const pair<const int, PriceLevelData>*>& bidSide, int nAskLevels, vector<const pair<const int, PriceLevelData>*>& askSide,
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
		for(; bidLevel < nBidLevels && bidSide[bidLevel]->first*OrderDataMicro::tickSize >= bidLmt; ++bidLevel)
			totBidSize += bidSide[bidLevel]->second.totsize;
		for(; askLevel < nAskLevels && askSide[askLevel]->first*OrderDataMicro::tickSize <= askLmt; ++askLevel)
			totAskSize += askSide[askLevel]->second.totsize;
		if(totBidSize + totAskSize > 0)
		{
			signals[sig] = (totBidSize - totAskSize) / (double)(totBidSize + totAskSize);
			if(dailyVolume > 0)
				vsignals[sig] = signals[sig] * (max(totBidSize, totAskSize) / dailyVolume);
		}
	}
}

void calcBookDepthSigsMOMicro(int nBidLevels, vector<const pair<const int, PriceLevelData>*>& bidSide, int nAskLevels, vector<const pair<const int, PriceLevelData>*>& askSide, 
		double midPrice, double spreadOff, double dailyVolume, unsigned nSigs, const double* volumeFrac, vector<double>& signals)
{
	if(nSigs == 0)
		return;

	int totBidSize = 0, totAskSize = 0, bidLevel = 0, askLevel = 0;
	for(unsigned sig = 0; sig < nSigs; ++sig)
	{
		int ms = (int)(volumeFrac[sig] * dailyVolume);
		for(; bidLevel < nBidLevels && totBidSize < ms; ++bidLevel)
			totBidSize += bidSide[bidLevel]->second.totsize;
		for(; askLevel < nAskLevels && totAskSize < ms; ++askLevel)
			totAskSize += askSide[askLevel]->second.totsize;

		if(bidLevel >= nBidLevels || askLevel >= nAskLevels)
			break;

		double bid1 = bidSide[bidLevel]->first * OrderDataMicro::tickSize;
		double ask1 = askSide[askLevel]->first * OrderDataMicro::tickSize;
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

}; // namespace gtlib
