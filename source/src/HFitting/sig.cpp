#include <HFitting/sig.h>
#include <numeric>
#include <omp.h>
using namespace std;
using namespace hff;

namespace sig {

HedgeInfo::HedgeInfo(string path, int gpts)
{
	int N;
	int NV;
	ifstream ifs(path.c_str());
	if( ifs.is_open() )
	{
		ifs >> N >> NV;

		vMean = vector<vector<float> >(gpts, vector<float>(NV));
		mean60Intra = vector<float>(gpts);
		meanClose = vector<float>(gpts);
		meanNextClose = vector<float>(gpts);

		for( int k = 0; k < gpts; ++k )
		{
			for( int i = 0; i < NV; ++i )
				ifs >> vMean[k][i];
			ifs >> mean60Intra[k];
			ifs >> meanClose[k];
			ifs >> meanNextClose[k];
		}
	}
}

bool HedgeInfo::valid()
{
	for( vector<vector<float> >::iterator it1 = vMean.begin(); it1 != vMean.end(); ++it1 )
		for( vector<float>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2 )
			if( *it2 != 0 )
				return true;
	return false;
}

string get_hedge_path(string model, int idate)
{
	char outdir[400];
	sprintf( outdir, "%s\\%s\\hedge", HEnv::Instance()->baseDir().c_str(), model.c_str() );
	ForceDirectory(outdir);

	string filename = "hedgeMean" + itos(idate) + ".txt";
	string path = (string)outdir + "\\" + filename;
	return path;
}

bool valid_quote(const QuoteInfo& quote)
{
	double midqt = .5 * (quote.ask + quote.bid);
	double sprd = basis_pts_ * (quote.ask - quote.bid) / midqt;
	bool valid = quote.bidSize > 0 && quote.askSize > 0 && fabs(sprd) <= skip_qt_ && midqt > min_price_;
	return valid;
}

bool read_chara_data(hff::SigC& sig, string market, string uid, int idate, int idate_p, int idate_pp, int idate_n)
{
	// Calculate sig.inUnivChara, sig.inUnivFit, sig.avgDlyVolume, sig.avgDlyVolat, sig.adjPrevClose, sig.medSprd, sig.prevDayVolume, sig.prevDayVolat,
	//   sig.mretIntraLag1, sig.hiloQAI, sig.hiloLag1Open, sig.hiloLag1Rat, sig.hiLag1, sig.loLag1, sig.mretIntraLag2, sig.hiloQAI2, sig.hiloLag2Open,
	//   sig.hiloLag2Rat, sig.hiLag2, sig.loLag2, sig.mretONLag1, sig.closeNextCloseRet.

	hff::CharaInfo chara;
	hff::CharaInfo chara_p;
	hff::CharaInfo chara_pp;
	hff::CharaInfo chara_n;

	const hff::CharaContainer* ch = static_cast<const hff::CharaContainer*>(HEvent::Instance()->get("", "CharaContainer"));
	bool valid_chara = ch->getCharaInfo(market, idate, uid, chara);
	bool valid_chara_p = ch->getCharaInfo(market, idate_p, uid, chara_p);
	bool valid_chara_pp = ch->getCharaInfo(market, idate_pp, uid, chara_pp);
	bool valid_chara_n = ch->getCharaInfo(market, idate_n, uid, chara_n);

	if( valid_chara_p )
	{
		sig.inUnivChara = chara_p.inUniv;
		sig.inUnivFit = chara_p.inUniv;
		sig.avgDlyVolume = chara_p.medVolume;
		sig.avgDlyVolat = chara_p.medVolatility * basis_pts_;
		sig.adjPrevClose = chara_p.close;
		sig.medSprd = chara_p.medmedSprd;
		sig.prevDayVolume = chara_p.volume;
		sig.prevDayVolat = chara_p.volatility * basis_pts_;

		if( chara.adjust > 0. )
			sig.adjPrevClose /= chara.adjust;

		if( chara.tickValid == 0 )
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
				float adjust = chara_p.adjust * chara_pp.adjust;
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
			if( sig.closeNextCloseRet > onret_sig_clip_ )
				sig.closeNextCloseRet = onret_sig_clip_;
			else if( sig.closeNextCloseRet < - onret_sig_clip_ )
				sig.closeNextCloseRet = - onret_sig_clip_;
		}
	}
	else
		return false;

	if( sig.hiLag1 <= 0 || sig.hiLag2 <= 0 || sig.loLag1 <= 0 || sig.loLag2 <= 0 )
		return false;

	return true;
}

void get_trade_stateinfo(hff::SigC& sig, const vector<TradeInfo>* trades)
{
	// Calculate sI.ltrade, sI.sig10[4], sI.sig1[3], sI.tsincet, sI.validt, sI.tlastt, sI.relativeHiLo, sI.hilo900, sI.hiloLag1, sI.hiloLag2
	// Use sig.hiLag1, sig.loLag1, sig.hiLag2, sig.loLag2.

	int nTrades = trades->size();
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
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
		int sec = (trade.msecs - linMod.openMsecs ) / 1000;

		if( sec >= 0 && sec < linMod.n1sec )
		{
			trdVolu1s[sec] += trade.qty;
			++nTrds1s[sec];
			sumTrdPrc1s[sec] += trade.price;
			sumVolTrdPrc1s[sec] += trade.price * trade.qty;
		}

		++timeIdx;
	}

	// Loop grid points.
	timeIdx = 0;
	while( timeIdx < nTrades && (*trades)[timeIdx].msecs <= linMod.openMsecs )
		++timeIdx;

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
			const int& trade_msecs_p = (*trades)[timeIdx-1].msecs;

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

			double hiLoDL1=0;
			if(sig.hiLag1 + sig.loLag1>0)
				hiLoDL1 = basis_pts_ * (sig.hiLag1 - sig.loLag1) / (sig.hiLag1 + sig.loLag1);
			if(hiLoDL1 > min_hiloD_)
				sI[i].hiloLag1 = (2 * trade_price_p - sig.hiLag1 - sig.loLag1) / (sig.hiLag1 - sig.loLag1);

			double hiLoDL2=0;
			if(sig.hiLag2 + sig.loLag2 > 0)
				hiLoDL2 = basis_pts_ * (sig.hiLag2 - sig.loLag2) / (sig.hiLag2 + sig.loLag2);
			if(hiLoDL2 > min_hiloD_)
				sI[i].hiloLag2 = (2 * trade_price_p - sig.hiLag2 - sig.loLag2) / (sig.hiLag2 - sig.loLag2);

			if(hiLoD900 > min_hiloD_)
				sI[i].hilo900 = (2 * trade_price_p - high900 - low900) / (high900-low900);

			sI[i].validt = 1;
		}
	}

	vector<double> sumVolTrdPrc1sAcc;
	vector<double> trdVolu1sAcc;
	vector<double> sumTrdPrc1sAcc;
	vector<double> nTrds1sAcc;
	get_acc(sumVolTrdPrc1sAcc, sumVolTrdPrc1s);
	get_acc(trdVolu1sAcc, trdVolu1s);
	get_acc(sumTrdPrc1sAcc, sumTrdPrc1s);
	get_acc(nTrds1sAcc, nTrds1s);

	int nthreads, i, tid;
	int chunk = linMod.gpts / 16;
#pragma omp parallel shared(sI, nthreads, chunk) private(i, tid)
	{
		//tid = omp_get_thread_num();
		//if (tid == 0)
		//{
		//	nthreads = omp_get_num_threads();
		//	printf("Number of threads = %d\n", nthreads);
		//}
		//printf("Thread %d starting...\n", tid);

#pragma omp for schedule(dynamic, chunk)
		for( i = 1; i < linMod.gpts; ++i )
		{
			int sec = i * linMod.gridInterval;

			sI[i].voluMom600 = get_voluMom( sec, 600, trdVolu1s );
			sI[i].voluMom3600 = get_voluMom( sec, 3600, trdVolu1s );
			sI[i].intraVoluMom = get_voluMom( sec, 0, trdVolu1s );

			//sI[i].vwap60 = get_vwap( sec, 60, sumVolTrdPrc1s, trdVolu1s, sumVolTrdPrc1sAcc, trdVolu1sAcc );
			//sI[i].vwap300 = get_vwap( sec, 300, sumVolTrdPrc1s, trdVolu1s, sumVolTrdPrc1sAcc, trdVolu1sAcc );
			//sI[i].vwap900 = get_vwap( sec, 300, sumVolTrdPrc1s, trdVolu1s, sumVolTrdPrc1sAcc, trdVolu1sAcc );

			sI[i].bollinger300 = get_vwap( sec, 300, sumTrdPrc1s, nTrds1s, sumTrdPrc1sAcc, nTrds1sAcc, 9 );
			sI[i].bollinger900 = get_vwap( sec, 900, sumTrdPrc1s, nTrds1s, sumTrdPrc1sAcc, nTrds1sAcc, 9 );
			sI[i].bollinger1800 = get_vwap( sec, 1800, sumTrdPrc1s, nTrds1s, sumTrdPrc1sAcc, nTrds1sAcc, 9 );
			sI[i].bollinger3600 = get_vwap( sec, 3600, sumTrdPrc1s, nTrds1s, sumTrdPrc1sAcc, nTrds1sAcc, 9 );
		}
	}
}

void get_quote_stateinfo(hff::SigC& sig, const vector<QuoteInfo>* quotes)
{
	// Calculate sig.firstMidQt, sig.firstQtGpt, sig.sprdOpen.
	// Calculate sI.mqut, sI.sprd, sI.bid, sI.ask, sI.bsize, sI.asize, sI.tsinceq, sI.tlastq, sI.validq, sI.mret5, sI.mret15, sI.mret30,
	//   sI.mret60, sI.mret120, sI.mret300, sI.mret600, sI.mret1200, sI.mret2400, sI.mret4800, sI.maxbsize, sI.maxasize, sI.maxbsize2, sI.maxasize2.
	// Use sig.firstMidQt, sig.firstQtGpt.

	vector<int> vQindx1s;
	get_quote_index_1s(vQindx1s, quotes);

	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	const int& gpts = linMod.gpts;
	vector<hff::StateInfo>& sI = sig.sI;

	int n = 4;
	vector<vector<int> > mba(n, vector<int>(gpts));

	int nInts = max_qtmax_lag_ / linMod.gridInterval;
	int dT = max_qtmax_lag_ % linMod.gridInterval;

	int nInts2 = max_qtmax_lag2_ / linMod.gridInterval;
	int dT2 = max_qtmax_lag2_ % linMod.gridInterval;

	int timeIdx = 0;
	while( timeIdx < nQuotes && (*quotes)[timeIdx].msecs <= linMod.openMsecs )
		++timeIdx;

	int checkFirstQt = 1;
	for(int i = 1; i < gpts; ++i)
	{
		int newQuotes = 0;
		int maxBid = 0;
		int maxAsk = 0;
		int dmaxBid = 0;
		int dmaxAsk =0;
		int sec = i * linMod.gridInterval;
		int firstQtSec = 0;

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
						firstQtSec = i * linMod.gridInterval;
						checkFirstQt = 0;
						sig.sprdOpen = sprd;
					}
				}
			}

			if( (quote.msecs - linMod.openMsecs) * 0.001 >= i * linMod.gridInterval - max_qtmax_lag_) 
			{
				if (quote.bidSize > maxBid ) maxBid = quote.bidSize;
				if (quote.askSize > maxAsk ) maxAsk = quote.askSize;
				if( (quote.msecs - linMod.openMsecs) * 0.001 >= i * linMod.gridInterval - dT ) 
				{
					if (quote.bidSize > dmaxBid ) dmaxBid = quote.bidSize;
					if (quote.askSize > dmaxAsk ) dmaxAsk = quote.askSize;
				}
			}

			++timeIdx;
		}

		// two possibilities: no quotes in interval, or some quotes
		if(newQuotes == 0)
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
			const QuoteInfo& quote_p = (*quotes)[vQindx1s[sec]];

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

		if( (sI[i].bsize <= 0 || sI[i].asize <= 0)   )    // 4/25/2005
			sI[i].validq = 0;
		if( fabs(sI[i].sprd) > skip_qt_ || sI[i].mqut <= min_price_ || sig.firstMidQt == 0)  // spread needs to be narrow and price needs to be positive 
			sI[i].validq = 0;

		if(sI[i].validq > 0)
		{
			sI[i].mret5 = get_mret(sec, 5, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret15 = get_mret(sec, 15, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret30 = get_mret(sec, 30, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret60 = get_mret(sec, 60, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret120 = get_mret(sec, 120, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret300 = get_mret(sec, 300, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret600 = get_mret(sec, 600, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret1200 = get_mret(sec, 1200, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret2400 = get_mret(sec, 2400, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
			sI[i].mret4800 = get_mret(sec, 4800, firstQtSec, quotes, vQindx1s, sI[i].mqut, sig.firstMidQt);
		}

		if ( fabs(sI[i].mret60) > om_max_ret_ )  
			sI[i].validq = 0;
	}

	// Get the max in the interval
	int maxa,maxb;
	int endj,j;
	int maxa2,maxb2;
	int endj2;
	for(int i=1; i<gpts; i++)
	{
		endj = min(nInts, i); 
		maxb = mba[2][i - endj];
		maxa = mba[3][i - endj];

		endj2 = min(nInts2, i); 
		maxb2=0;
		maxa2=0; // because dT2 = 0

		for(j=0;j<endj;j++)
		{
			if(mba[0][i-j] > maxb)
				maxb = mba[0][i-j];
			if(mba[1][i-j] > maxa)
				maxa = mba[1][i-j];	
		}
		for(j=0;j<endj2;j++)
		{
			if(mba[0][i-j] > maxb2)
				maxb2 = mba[0][i-j];
			if(mba[1][i-j] > maxa2)
				maxa2 = mba[1][i-j];	
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
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	const int& gpts = linMod.gpts;
	vector<hff::StateInfo>& sI = sig.sI;

	int qtIdx = 1;
	for(int i=1;i<gpts;i++)
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
						sI[i].sig1[1] = 2.*(sI[i].ltrade - mid ) / (quote_p.ask - quote_p.bid);
				} 
			}
		}
		// what happens if there are no quotes for some time? need to think about this...
		// what should we do in the case that this signal is bigger than one (so tha the trade price was outside of the 
		// bid ask? either set it to zero or set it to abs 1. 
		// it makes more sense to me to set it to abs 1
		if (fabs(sI[i].sig1[1]) > om_fill_imb_clip_)
			sI[i].sig1[1] = om_fill_imb_clip_ * sI[i].sig1[1] / fabs(sI[i].sig1[1]);
	}
}

void get_targets_stateinfo(hff::SigC& sig, const vector<QuoteInfo>* quotes)
{
	// Calculate sI.target1, sI.targetF, sI.target10, sI.target60, sI.target60Intra, sI.targetClose, sI.targetNextClose,
	//   sI.target1UH, sI.target10UH, sI.target60UH.
	// Update sI.validq, sI.validt.
	// Use sig.closeNextCloseRet.

	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	const int& gpts = linMod.gpts;
	const int& n1sec = linMod.n1sec;
	vector<hff::StateInfo>& sI = sig.sI;

	// Delay 1 second.
	vector<double> vMid1s_lag1sec(linMod.n1sec);
	get_mid_series(vMid1s_lag1sec, quotes);

	int L1 = 60;
	int L5 = 300;
	int L10 = 600;
	int L60 = tm_target_60_; // 20051215 : make NASD and NY hori the same
	int L_CLOSE = linMod.n1sec - 1;

	int ki, kf;
	const hff::NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();
	const vector<pair<int, int> >& vHorizonLag = nonLinMod.vHorizonLag;
	for(int i = 1; i < gpts; i++)
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
			if(vMid1s_lag1sec[ki] > 0 && vMid1s_lag1sec[kf] > 0)
			{
				sI[i].target[iT] = basis_pts_ * (vMid1s_lag1sec[kf] / vMid1s_lag1sec[ki] - 1);

				// clip
				if( horizon >= 600 && horizon < 3600 )
				{
					if( fabs(sI[i].target[iT]) > tm_target_clip_ )
						sI[i].target[iT] = tm_target_clip_ * sI[i].target[iT] / fabs(sI[i].target[iT]);
				}
				else if( horizon >= 3600 )
				{
					if( fabs(sI[i].target[iT]) > tm_target_60_clip_ )
						sI[i].target[iT] = tm_target_60_clip_ * sI[i].target[iT] / fabs(sI[i].target[iT]);
				}

				// Copy. Originals will be hedged.
				sI[i].targetUH[iT] = sI[i].target[iT];
			}
		}

		// to close.
		int maxHorizon = nonLinMod.maxHorizon;
		kf = min(sec + L_CLOSE, n1sec - 1); // maxHorizon -> Close
		ki = min(sec + maxHorizon, n1sec - 1);
		if(vMid1s_lag1sec[ki] > 0 && vMid1s_lag1sec[kf] > 0)
			sI[i].target60Intra = basis_pts_ * (vMid1s_lag1sec[kf] / vMid1s_lag1sec[ki] - 1);

		if( fabs(sI[i].target60Intra) > tm_target_60_clip_ )
			sI[i].target60Intra = tm_target_60_clip_ * sI[i].target60Intra / fabs(sI[i].target60Intra);

		kf = min(sec + L_CLOSE, n1sec - 1); // 1 Min -> to close
		ki = min(sec + L1, n1sec - 1);
		if(vMid1s_lag1sec[ki] > 0 && vMid1s_lag1sec[kf] > 0)
		{
			sI[i].targetClose = basis_pts_ * (vMid1s_lag1sec[kf] / vMid1s_lag1sec[ki] - 1);
			sI[i].targetNextClose = sI[i].targetClose + sig.closeNextCloseRet;
		}

		// clip.
		if( fabs(sI[i].targetClose) > tm_target_60_clip_ )
			sI[i].targetClose = tm_target_60_clip_ * sI[i].targetClose / fabs(sI[i].targetClose);
		if( fabs(sI[i].targetNextClose) > tm_target_60_clip_ )
			sI[i].targetNextClose = tm_target_60_clip_ * sI[i].targetNextClose / fabs(sI[i].targetNextClose);

		// Copy. Originals will be hedged.
		sI[i].target60IntraUH = sI[i].target60Intra;
		sI[i].targetCloseUH = sI[i].targetClose;
		sI[i].targetNextCloseUH = sI[i].targetNextClose;
	}

	// make the last grid point not valid 
	sI[gpts-1].validq = sI[gpts-1].validt = 0;

	// gptOK.
	for(int i = 1; i < gpts; i++)
	{
		if( sI[i].validq && sI[i].validt /*&& sig.dayok*/ )
			sI[i].gptOK = 1;
	}
}

void get_tob_stateinfo(TickTobAcc* tta, hff::SigC& sig, int idate, string ticker)
{
	// Calculate sI.sig1[8], sI.sig1[9], sI.sig1[10], sI.sig1[11], sI.tobOK.
	// Use sI.validq, sI.validt, sI.bid, sI.ask, sI.askSize, sI.bidSize.

	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
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
		int nBidSide,nAskSide;
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
			for(int x=0;x<nBidSide;++x)
			{
				double qtSize = askSide[x]->askSize + bidSide[x]->bidSize;
				double midX = 0.5 * (askSide[x]->ask + bidSide[x]->bid);
				double spreadX = askSide[x]->ask - bidSide[x]->bid;
				double spreadOffX = (spreadX > minSprdOff_)? spreadX: minSprdOff_;
				double spreadOffXper = basis_pts_ * spreadOffX / midPrice;

				// if the sprd is too wide continue (slightly redundant)
				if(spreadOffXper > skip_qt_ || midX <= min_price_)
					continue;

				if(askSide[x]->askSize > 0 && bidSide[x]->bidSize > 0)
				{
					double quimX = (bidSide[x]->bidSize - askSide[x]->askSize) / qtSize;
					inputs[2 * x] = quimX; //quote imbalance
					if(x > 0 && spreadX > 0)
						inputs[2 * x - 1] = (midX / midPrice-1.) * (spreadOff / spreadOffX);
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

void get_book_stateinfo(map<string, OrderBkAcc<OrderData> >& obaMap, hff::SigC& sig, string market, int idate, string ticker, vector<string> okECNs)
{
	// Calculate sI.sigBook[0] - sI.sigBook[7], sI.sigBook[8] - sI.sigBook[27].
	// Use sI.validq, sI.validt

	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	vector<hff::StateInfo>& sI = sig.sI;

	vector<string> mkts = okECNs;
	mkts.push_back(market);
	int nbooks = int(mkts.size());
	for(int s=0;s<nbooks;s++)
		obaMap[mkts[s]].LoadData(ticker, idate);

	int msecsRoll = linMod.gridInterval * 1000;

	const int maxLevels = 1000;
	int maxNLevels = maxLevels;

	const unsigned nVolFrac = 6;
	const unsigned nSprdBins = 7;
	const double volumeFrac[nVolFrac] = {0.01, 0.02, 0.04, 0.08, 0.16, 0.32};
	const double sprdBins[nSprdBins] = {0.25, 0.5, 1., 2., 4., 8., 16.};

	for(int i = 1; i< linMod.gpts; i++)
	{
		vector<double> bookDepthMO(nVolFrac);
		vector<double> bookDepthQI(nSprdBins);
		vector<double> vBookDepthQI(nSprdBins);

		int msecs = linMod.openMsecs + i * msecsRoll - 1;
		for(int s = 0; s < nbooks; s++)
			obaMap[mkts[s]].RollForward(msecs);

		if(sI[i].validq + sI[i].validt < 2)
			continue; 

		OrderBk<OrderData> obk(obaMap[mkts[0]]);
		for(int s = 1; s<nbooks; s++)
			obk.MergeIn(obaMap[mkts[s]]);

		vector<const OrderData*> bidSide(maxLevels);
		vector<const OrderData*> askSide(maxLevels);
		int nBidSide = 0,nAskSide = 0;
		obk.GetBidSideBook(maxLevels, &bidSide[0], &nBidSide);
		obk.GetAskSideBook(maxLevels, &askSide[0], &nAskSide);     

		int nLevels = nBidSide < nAskSide? nBidSide: nAskSide; //only consider the lesser number of price levels on either side
		if(nLevels < 2)
			continue; 

		nLevels = min(nLevels, maxNLevels); 

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
		if(midPrice>0)
			spreadper = basis_pts_ * spreadOff/midPrice;
		if( fabs(spreadper) > skip_qt_ ||  midPrice <= min_price_)  // added on 1/6/2005 
			continue;

		float pmedmedSprd = sig.medSprd * midPrice;
		calcBookDepthSigsMO(nLevels, &bidSide[0], nLevels, &askSide[0], midPrice, spreadOff, sig.avgDlyVolume, nVolFrac, volumeFrac, bookDepthMO);
		calcBookDepthSigsQI(nLevels, &bidSide[0], nLevels, &askSide[0], midPrice, pmedmedSprd, nSprdBins, sprdBins, bookDepthQI, vBookDepthQI, sig.avgDlyVolume);

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

		sI[i].sigBook[9] = vBookDepthQI[0];
		sI[i].sigBook[10] = vBookDepthQI[1];
		sI[i].sigBook[11] = vBookDepthQI[2];
		sI[i].sigBook[12] = vBookDepthQI[3];
		sI[i].sigBook[13] = vBookDepthQI[4];
		sI[i].sigBook[26] = vBookDepthQI[5];
		sI[i].sigBook[27] = vBookDepthQI[6];

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
		sI[i].sigBook[2] = inputs[3];	 //  sprdRat
		sI[i].sigBook[3] = inputs[7];    //  sprdRat
		sI[i].sigBook[4] = inputs[4];    //  qutImb   
		sI[i].sigBook[5] = inputs[8];	 //  qutImb
		sI[i].sigBook[6] = inputs[1];	 //  offset
		sI[i].sigBook[7] = inputs[5];    //  offset
	}
}

void get_signals(hff::SigC& sig)
{
	// Calculate sig.logVolu, sig.logPrice, sig.logMedSprd, sig.logDolVolu, sig.logDolPrice, sig.prevDayVolume, sI.valid,
	//   sI.sig10[2], sI.sig1[14], sI.sig10[3], sI.sig1[15], sI.sig10[5], sI.sig10[6], sI.mret2400L, sI.mret4800L, sI.mretOpen, sI.sig10[0],
	//   sI.sig10[1], sI.quimMax2, sI.sig10[4], sI.sig10[7], sI[k].voluMom600, sI.voluMom3600, sI.intraVoluMom,
	//   sI.sig1[0], sI.sig1[3], sI.sig1[2], sI.sig1[4], sI.absSprd.
	// Use sI.validq, sig.adjPrevClose, sI.mqut, sig.avgDlyVolume, sig.adjPrevClose, sig.medSprd, sig.fxRate, sig.prevDayVolume, sig.avgDlyVolat,
	//   sig.dayok, sig.inUnivFit
	// Update sI.validq, sI.validt, sI.bsize, sI.asize, sI.target10, sI.target60, sI.targetF
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

	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	vector<hff::StateInfo>& sI = sig.sI;

	int ON = 900 / linMod.gridInterval;
	int L1 = 60 / linMod.gridInterval, L5 = 300 / linMod.gridInterval, L10 = 600 / linMod.gridInterval, L20 = 1200 / linMod.gridInterval;
	int L40 = 2400 / linMod.gridInterval, L60 = 3600 / linMod.gridInterval, L80 = 4800 / linMod.gridInterval, L160 = 9600 / linMod.gridInterval;
	int L120 = 7200 / linMod.gridInterval;

	float fVolM600  = float(linMod.n1sec-1) / tm_volu_mom_lb_1_;
	float fVolM3600 = float(linMod.n1sec-1) / tm_volu_mom_lb_2_;

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

		// testing for EU
		//sig.logDolVolu = log10(sig.avgDlyVolume * sig.fxRate * sig.adjPrevClose);
		//sig.logDolPrice = log10(sig.fxRate * sig.adjPrevClose);

		if(sig.avgDlyVolume > 0)
			sig.prevDayVolume  = sig.prevDayVolume / sig.avgDlyVolume;
	}

	for(int k=0; k<linMod.gpts; k++)
	{
		if(sig.avgDlyVolume <= 0 || sig.avgDlyVolat <= 0 )
		{
			sI[k].validq = 0;  
			sI[k].validt = 0;
		}

		sI[k].bsize *= qutSizeAdj;
		sI[k].asize *= qutSizeAdj;

		if(sI[k].validq && sI[k].validt && /*sig.dayok &&*/ sig.inUnivFit)
			sI[k].valid = 1;

		if( linMod.vHorizonLag[0].first < 60 )
		{
			sI[k].sig1[13] = sI[k].mret5;
			sI[k].sig1[12] = sI[k].mret15;
			sI[k].sig1[7] = sI[k].mret30;
		}

		//		get rid of lunch period targets
		//if( model==110 && (k*linMod.gridInterval >= 7200)  &&  (k*linMod.gridInterval <= 12600) )
		//	sI[k].valid = sI[k].validq = sI[k].validt = 0;
		//else if( (model==112 || model == 130) && (k*linMod.gridInterval >= 9000)  &&  (k*linMod.gridInterval <= 16200) )
		//	sI[k].valid = sI[k].validq = sI[k].validt = 0;
		//else if( model==118 && (k*linMod.gridInterval >= 12600)  &&  (k*linMod.gridInterval <= 18000) )
		//	sI[k].valid = sI[k].validq = sI[k].validt = 0;

		// mret300 clip
		sI[k].sig10[2] = sI[k].mret300;
		if( fabs(sI[k].sig10[2]) > linMod.clip_ret300 )
			sI[k].sig10[2] = linMod.clip_ret300 * sI[k].sig10[2] / fabs(sI[k].sig10[2]);

		sI[k].sig1[14] = sI[k].sig10[2];

		// mret300L 
		if(k>L5)
			sI[k].sig10[3] = sI[k-L5].mret300;
		if( fabs(sI[k].sig10[3]) > linMod.clip_ret300 )
			sI[k].sig10[3] = linMod.clip_ret300 * sI[k].sig10[3] / fabs(sI[k].sig10[3]);

		sI[k].sig1[15] = sI[k].sig10[3];			

		// mret 600 lagged 600 
		if(k>L10)
			sI[k].sig10[5] = sI[k-L10].mret600; // ok since, unlike, sig[2], no nonlinear transform
		if(fabs(sI[k].sig10[5]) > max_ret_)
			sI[k].sig10[5] = max_ret_*sI[k].sig10[5] / fabs(sI[k].sig10[5]); 						

		// mret 1200 lagged 1200
		if(k>L20)
			sI[k].sig10[6] = sI[k-L20].mret1200; 
		if(fabs(sI[k].sig10[6]) > max_ret_)
			sI[k].sig10[6] = max_ret_*sI[k].sig10[6] / fabs(sI[k].sig10[6]); 

		// mret 2400 lagged 2400
		if(k>L40)
			sI[k].mret2400L = sI[k-L40].mret2400;
		if(fabs(sI[k].mret2400L) > max_ret_)
			sI[k].mret2400L = max_ret_*sI[k].mret2400L / fabs(sI[k].mret2400L); 

		// mret 4800 lagged 4800
		if(k>L80)
			sI[k].mret4800L = sI[k-L80].mret4800;
		if(fabs(sI[k].mret4800L) > max_ret_)
			sI[k].mret4800L = max_ret_*sI[k].mret4800L / fabs(sI[k].mret4800L); 

		if(k > 0 && sig.firstMidQt > 0)
			sI[k].mretOpen = basis_pts_*(sI[k].mqut / sig.firstMidQt-1);
		if(fabs(sI[k].mretOpen) > max_ret_)
			sI[k].mretOpen = max_ret_*sI[k].mretOpen / fabs(sI[k].mretOpen); 

		// qutImbWt
		if(sI[k].validq && sI[k].tsinceq  < 1000 * max_qtwt_lag_)
		{
			sI[k].sig10[0] = ((double)(sI[k].bsize - sI[k].asize)) / (sI[k].bsize + sI[k].asize);
			sI[k].sig10[0] *= sqrt(max(sI[k].bsize, sI[k].asize) / sig.avgDlyVolume);
		}

		// qutImbMax
		if(sI[k].validq && sI[k].tsinceq  < 1000 * max_qtmax_lag_ )
		{
			sI[k].sig10[1] = ((double)(sI[k].maxbsize - sI[k].maxasize)) / (sI[k].maxbsize + sI[k].maxasize);
		}

		// qutImbMax2
		if(sI[k].validq && sI[k].tsinceq  < 1000 * max_qtmax_lag2_ )
		{
			sI[k].quimMax2 = ((double)(sI[k].maxbsize2 - sI[k].maxasize2)) / (sI[k].maxbsize2 + sI[k].maxasize2);
		}

		//scale hilo by vol: this should not be used anywhere 
		sI[k].sig10[4] *= sig.avgDlyVolat;
		sI[k].sig10[4]=sI[k].relativeHiLo;

		// the on signal
		if(k < ON && sI[k].validq && sig.adjPrevClose > 0)
			sI[k].sig10[7] = basis_pts_ * ( sI[k].mqut / sig.adjPrevClose - 1);
		if(k >= ON)
			sI[k].sig10[7] = onret3;	
		if( fabs(sI[k].sig10[7]) > max_ret_)
			sI[k].sig10[7] = max_ret_ * sI[k].sig10[7] / fabs(sI[k].sig10[7]);

		// the volume momentum signals
		if(k > L10)
		{
			if(sig.avgDlyVolume > 0)
				sI[k].voluMom600  = sI[k].voluMom600 * fVolM600 / sig.avgDlyVolume;
		}
		if(k>L60)
		{
			if(sig.avgDlyVolume > 0)
				sI[k].voluMom3600 = sI[k].voluMom3600 * fVolM3600 / sig.avgDlyVolume;
		}
		if(k>0)
		{
			float scaleFact = float((linMod.n1sec-1)) / (k*linMod.gridInterval);
			if(sig.avgDlyVolume > 0 )
				sI[k].intraVoluMom = sI[k].intraVoluMom * scaleFact / sig.avgDlyVolume;
		}

		// if data is not valid, set signals and target to zero; (so nonsense does not enter hedge)
		if( sI[k].validq + sI[k].validt < 2 )
		{
			for(int n=0;n<hff::tm_num_basic_sig_;n++)
				sI[k].sig10[n]=0.;
		}

		// one minute signals 
		if (sI[k].validq && sI[k].tsinceq  < 1000 * om_stale_qut_)
			sI[k].sig1[0] = ((float)(sI[k].bsize - sI[k].asize)) / (sI[k].bsize + sI[k].asize);

		// fillImb (sig1[1]) and hilo (sig1[3]) done earlier
		// signals mret300 (sig1[14]) and mret300Lag (sig1[15]) are calculated above
		sI[k].sig1[3] = sI[k].relativeHiLo;

		if(sI[k].validq)
			sI[k].sig1[2] = sI[k].mret60;
		if ( fabs(sI[k].sig1[2]) > om_max_clip_  )
			sI[k].sig1[2] = om_max_clip_ * sI[k].sig1[2] / fabs(sI[k].sig1[2]);

		// gSigs
		{
			int fIndx = 4;
			const vector<hff::IndexFilter>& filters = HEnv::Instance()->indexFilters().filters;
			for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end() && fIndx < 7; ++itf, ++fIndx )
			{
				const hff::IndexFilter& filter = *itf;
				string predName = filter.title() + "_pred";
				const vector<float>* vPred = static_cast<const vector<float>*>(HEvent::Instance()->get("", predName));
				sI[k].sig1[fIndx] = (*vPred)[k];
			}
		}
		//if(sig.dayok)
		{
			sI[k].absSprd = fabs(sI[k].sprd);
			if(sI[k].absSprd > multi_lin_sprd_clip_)
				sI[k].absSprd = multi_lin_sprd_clip_;	
		}
	}	// end of loop over k
}

void get_MI_signals(SigC& sig, const vector<OrderQty>* trades)
{
	vector<hff::StateInfo>& sI = sig.sI;
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	//int binInterval = linMod.gridInterval;
	//int nBins = linMod.gpts;

	vector<int> tradeVol(linMod.n1sec, 0);
	if( trades != 0 )
	{
		for( vector<OrderQty>::const_iterator it = trades->begin(); it != trades->end(); ++it )
		{
			int bin = (it->msecs - 1) / 1000 + 1;
			if( bin >= 0 && bin < linMod.n1sec )
				tradeVol[bin] += it->signedQty;
		}
	}

	float normFac = (sig.avgDlyVolume > 0) ? sig.avgDlyVolume : 1.0;

	int i;
	int chunk = linMod.gpts / 16;
#pragma omp parallel shared(sI, chunk) private(i)
	{
#pragma omp for schedule(dynamic, chunk)
		for( i = 1; i < linMod.gpts; ++i )
		{
			int sec = i * linMod.gridInterval;
			float scaleFac = (float)(linMod.n1sec - 1) / (i * linMod.gridInterval);

			sI[i].mI5 = get_volQty(sec, 5, tradeVol) / normFac;
			sI[i].mI15 = get_volQty(sec, 15, tradeVol) / normFac;
			sI[i].mI30 = get_volQty(sec, 30, tradeVol) / normFac;
			sI[i].mI60 = get_volQty(sec, 60, tradeVol) / normFac;
			sI[i].mI120 = get_volQty(sec, 120, tradeVol) / normFac;
			sI[i].mI600 = get_volQty(sec, 600, tradeVol) / normFac;
			sI[i].mIIntra = scaleFac * get_volQty(sec, 0, tradeVol) / normFac;
		}
	}
}

void calcBookDepthSigsMO(int nBidLevels, const OrderData** bidSide, int nAskLevels, const OrderData** askSide, 
		double midPrice, double spreadOff, double dailyVolume, unsigned nSigs, const double* volumeFrac, vector<double>& signals)
{     
	if(nSigs==0)
		return;
	int totBidSize=0,totAskSize=0,bidLevel=0,askLevel=0;
	for(unsigned sig=0;sig<nSigs;++sig)
	{
		int ms=(int)(volumeFrac[sig]*dailyVolume);
		for(;bidLevel<nBidLevels&&totBidSize<ms;++bidLevel)
			totBidSize+=bidSide[bidLevel]->size;
		for(;askLevel<nAskLevels&&totAskSize<ms;++askLevel)
			totAskSize+=askSide[askLevel]->size;

		if(bidLevel>=nBidLevels||askLevel>=nAskLevels)
			break;

		double bid1=bidSide[bidLevel]->RealPrice();
		double ask1=askSide[askLevel]->RealPrice();
		double sprd1=ask1-bid1;
		double mid1=0.5*(ask1+bid1);
		sprd1 = sprd1 > minSprdOff_? sprd1: minSprdOff_;
		signals[sig] = (mid1 / midPrice - 1.) * (spreadOff / sprd1);
	}
}

void calcBookDepthSigsQI(int nBidLevels,const OrderData** bidSide, int nAskLevels, const OrderData** askSide,
		double midPrice, double medSprd, unsigned nSigs, const double* bins, std::vector<double>& signals, std::vector<double>& vsignals, double dailyVolume)
{     
	if(nSigs==0)
		return;
	int bidLevel=0,askLevel=0;
	double totBidSize=0,totAskSize=0;
	for(unsigned sig=0;sig<nSigs;++sig)
	{   
		double bidLmt=midPrice -bins[sig]*medSprd;
		double askLmt=midPrice +bins[sig]*medSprd;
		for(;bidLevel<nBidLevels && bidSide[bidLevel]->RealPrice() >=bidLmt;++bidLevel)
			totBidSize+=bidSide[bidLevel]->size;
		for(;askLevel<nAskLevels && askSide[askLevel]->RealPrice() <=askLmt;++askLevel)
			totAskSize+=askSide[askLevel]->size;
		if(totBidSize + totAskSize>0)
		{
			signals[sig]= (totBidSize - totAskSize)/(double)(totBidSize + totAskSize);
			if(dailyVolume>0)
				vsignals[sig]= signals[sig]*(max(totBidSize,totAskSize)/dailyVolume);
		}
	}
}

void get_mid_series(vector<double>& vMid1s, const vector<QuoteInfo>* quotes, int lagMsecs)
{
	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();

	// Lag the quotes.
	vector<QuoteInfo> laggedQuotes;
	laggedQuotes.reserve(nQuotes);
	int tnQuotes = 0;   // need to drop quotes after the close
	while ( (tnQuotes < nQuotes) && (((*quotes)[tnQuotes].msecs - linMod.openMsecs) * 0.001 < linMod.n1sec - 1) )
	{
		laggedQuotes.push_back( (*quotes)[tnQuotes] );
		laggedQuotes[tnQuotes].msecs -= lagMsecs;
		++tnQuotes;
	}

	// Initialize with -1.
	vMid1s = vector<double>(linMod.n1sec, -1.);

	int timeIdx = 0;
	while( timeIdx < tnQuotes && laggedQuotes[timeIdx].msecs <= linMod.openMsecs - lagMsecs)
		++timeIdx;

	double firstMidQt = 0.;
	int firstQtGpt = 0;
	int L1 = 60;
	int checkFirstQt = 1;
	for(int i = 1; i < linMod.n1sec; i++)
	{
		int newQuotes = 0;
		while((timeIdx < tnQuotes) && ((laggedQuotes[timeIdx].msecs - linMod.openMsecs) * 0.001 < i)) 
		{
			++newQuotes;
			const QuoteInfo& quote = laggedQuotes[timeIdx];

			if(checkFirstQt)
			{
				if(quote.ask > 0 && quote.bid > 0)
				{
					double midqt = .5 * (quote.ask + quote.bid);
					double sprd = basis_pts_ * (quote.ask - quote.bid) / midqt;
					if(fabs(sprd) <= skip_qt_ && sprd >= 0) // require a reasonably tight spread
					{
						firstMidQt = .5 * (quote.ask + quote.bid);
						firstQtGpt = i;
						checkFirstQt = 0;
					}
				}
			}

			++timeIdx;
		}

		if( firstMidQt != 0. )
		{
			if (newQuotes == 0)
				vMid1s[i] = vMid1s[i - 1];
			else if (timeIdx <= tnQuotes)
			{
				const QuoteInfo& quote_p = laggedQuotes[timeIdx - 1];

				if( valid_quote(quote_p) )
					vMid1s[i] = .5 * (quote_p.ask + quote_p.bid);
			}

			double mret60 = 0.;
			int K = 0;
			if( vMid1s[i] > 0. )
			{
				K = max(i - L1, 0);
				K = max(K, firstQtGpt);

				if(i > K && K > firstQtGpt && vMid1s[K] > 0.)
					mret60 = basis_pts_ * (vMid1s[i] / vMid1s[K] - 1);
				else if(i > K && K == firstQtGpt)
					mret60 = basis_pts_ * (vMid1s[i] / firstMidQt - 1);
			}

			if( fabs(mret60) > om_max_ret_ )
				vMid1s[i] = -1.;
		}
	}
}

void get_quote_index_1s(vector<int>& vQindx1s, const vector<QuoteInfo>* quotes)
{
	int nQuotes = quotes->size();
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();

	// Initialize.
	vQindx1s = vector<int>(linMod.n1sec);

	int timeIdx = 0;
	while( timeIdx < nQuotes && (*quotes)[timeIdx].msecs <= linMod.openMsecs )
		++timeIdx;

	int L1 = 60;
	int checkFirstQt = 1;
	for(int i = 1; i < linMod.n1sec; i++)
	{
		int newQuotes = 0;
		while((timeIdx < nQuotes) && (((*quotes)[timeIdx].msecs - linMod.openMsecs) * 0.001 < i)) 
		{
			++newQuotes;
			++timeIdx;
		}

		if (newQuotes == 0)
			vQindx1s[i] = vQindx1s[i - 1];
		else if (timeIdx <= nQuotes) 
			vQindx1s[i] = timeIdx - 1;
	}
}

double get_mret(int sec, int dsec, int firstQtSec, const vector<QuoteInfo>* quotes, const vector<int>& vQindx1s, double mqut, double firstMidQt)
{
	double ret = 0.;
	int S = max(sec - dsec, 0);
	if( S > firstQtSec && valid_quote( (*quotes)[vQindx1s[S]] ) )
		ret = basis_pts_ * (mqut / get_mid((*quotes)[vQindx1s[S]]) - 1);
	else if( S == firstQtSec )
		ret = basis_pts_ * (mqut / firstMidQt - 1);
	return ret;
}

double get_voluMom( int sec, int length, const vector<double>& trdVolu1s )
{
	double ret = 0.;
	int offset1 = sec + 1 - length;
	int offset2 = sec + 1;
	if( offset1 > 0 )
	{
		vector<double>::const_iterator it1 = trdVolu1s.begin() + offset1;
		vector<double>::const_iterator it2 = trdVolu1s.begin() + offset2;
		ret = accumulate(it1, it2, 0.);
	}
	return ret;
}

double get_volQty( int sec, int length, const vector<int>& tradeQty )
{
	double ret = 0.;
	int offset1 = sec + 1 - length;
	int offset2 = sec + 1;
	if( offset1 > 0 )
	{
		vector<int>::const_iterator it1 = tradeQty.begin() + offset1;
		vector<int>::const_iterator it2 = tradeQty.begin() + offset2;
		ret = accumulate(it1, it2, 0);
	}
	return ret;
}

double get_vwap( int sec, int length, const vector<double>& sumVolTrdPrc1s, const vector<double>& trdVolu1s,
		const vector<double>& sumVolTrdPrc1sAcc, const vector<double>& trdVolu1sAcc, double minVolu )
{
	double vwap = 0.;

	int offset1 = sec + 1 - length;
	if( offset1 < 0 )
		offset1 = 0;
	int offset2 = sec + 1;

	vector<double>::const_iterator it1 = sumVolTrdPrc1s.begin() + offset1;
	vector<double>::const_iterator it2 = sumVolTrdPrc1s.begin() + offset2;
	double sumVolTrdPrc = accumulate(it1, it2, 0.);

	vector<double>::const_iterator itVol1 = trdVolu1s.begin() + offset1;
	vector<double>::const_iterator itVol2 = trdVolu1s.begin() + offset2;
	double trdVolu = accumulate(itVol1, itVol2, 0.);

	if( trdVolu > minVolu )
	{
		double avgPrc = 0.;
		if( trdVolu > 0 )
			avgPrc = sumVolTrdPrc / trdVolu;

		double avgPrcD = 0.;
		double sumVolTrdPrcD = sumVolTrdPrc1sAcc[sec];
		double trdVoluD = trdVolu1sAcc[sec];
		if( trdVoluD > 0 )
			avgPrcD = sumVolTrdPrcD / trdVoluD;

		if( avgPrcD > 0 )
			vwap = avgPrc / avgPrcD - 1;
	}
	return vwap;
}

void get_acc(vector<double>& vAcc, const vector<double>& v)
{
	int vsize = v.size();
	vAcc = vector<double>(vsize, 0.);
	vAcc[0] = v[0];
	for( int i = 1; i < vsize; ++i )
		vAcc[i] = vAcc[i - 1] + v[i];
}
}
