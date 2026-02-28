#include <HOrders/HTradeAna.h>
#include "optionlibs/TickData.h"
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GEX.h>
#include "TFile.h"
#include "TGraphErrors.h"
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <numeric>
#include <string>
using namespace std;

/* -----------------------------------------------------------------------
Directories: tickerDayDist, PnlCumDaily, PnlSig, PnlLags.

tickerDayDist (traded ticker only) (1)
  hPnlIntra
  hNTrades
  hRatBuy
  hNShares
  hDollarvol
  hNFlips (proposed)

DailyDist (proposed)
  for each idate
    hPnlIntra: Distribution of intra day pnl of each ticker.
	hPnlClcl

PnlCumDaily (2)
  hdPnlIntra
  hdPnlClcl
  hdPnlClclNext
  hdPnlClclDaypos
  hdPnlInter
  hdPnlClop
  hdPnlTotal
  hdCumPnlIntra (p)
  hdCumPnlClcl (p)
  hdCumPnlInter (p)
  hdCumPnlClop (p)
  hdCumPnlTotal (p)

Trade Activity directories: Pnl, PosVsTime, tradeActivity.

Pnl (3)
  hPnlIntra (?)
  hPnlInter (?)
  hPnlIntraTimeofday (p)
  hPnlInterTimeofday (p)
  hPnlPrcPos (x)
  hCumPnlIntra
  hCumPnlInter
  hCumPnlClclNext
  hCumPnlPrcPos (x)

PosVsTime (3)
  hSharePos
  hDollarPos
  hSharePosAbs
  hDollarPosAbs
  hPrice
  hReturn

tradeActivity (3)
  hNTrades
  hNShares
  hDollarvol
  hSignedNTrades
  hSignedNShares (p)
  hSignedDollarvol (p)

(1) private member that udpates daily.
(2) Created end of day.
(3) Created end of day, from TradeActivityAgg.

----------------------------------------------------------------------- */

HTradeAna::HTradeAna(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
do_root_(false),
verbose_(0),
	id_("M"),
idate0_(0),
fee_bpt_(0.),
nBinsPerDay_(40),
nTickerDetailMax_(40),
minCondVar_(-max_float_),
maxCondVar_(max_double_)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("doRoot") )
		do_root_ = conf.find("doRoot")->second == "true";

	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());

	if( conf.count("nBinsPerDay") )
		nBinsPerDay_ = atoi(conf.find("nBinsPerDay")->second.c_str());

	if( conf.count("nTickerDetailMax") )
		nTickerDetailMax_ = atoi(conf.find("nTickerDetailMax")->second.c_str());

	if( conf.count("condVar") && conf.count("minCondVar") && conf.count("maxCondVar") )
	{
		condVar_ = conf.find("condVar")->second;
		minCondVar_ = atof(conf.find("minCondVar")->second.c_str());
		maxCondVar_ = atof(conf.find("maxCondVar")->second.c_str());
	}
}

HTradeAna::~HTradeAna()
{}

void HTradeAna::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	// Create the output file.
	if( do_root_ )
	{
		TFile* f = HEnv::Instance()->outfile();
		f->mkdir(moduleName_.c_str());
	}

	return;
}

void HTradeAna::beginMarket()
{
	string market = HEnv::Instance()->market();
	vector<int> idates = HEnv::Instance()->idates();
	if( !idates.empty() )
		idate0_ = idates[0];
	fee_bpt_ = mto::fee_bpt(market);

	mPs_.clear();
	ps_ = PnlSummary();

	iDay_ = 0;
	int msecOpen = mto::msecOpen(market, idate0_);
	int msecClose = mto::msecClose(market, idate0_);

	nDay_ = HEnv::Instance()->nDates();
	trdActAgg_ = TradeActivityAgg(market, idate0_, nDay_, nBinsPerDay_);
	nHours_ = (msecClose - msecOpen)/1000.0/60.0/60.0 * nDay_;

	if( do_root_ )
	{
		TFile* f = HEnv::Instance()->outfile();
		f->cd(moduleName_.c_str());

		gDirectory->mkdir(market.c_str());
		gDirectory->cd(market.c_str());

		gDirectory->mkdir("Tickers");
		gDirectory->mkdir("MostTraded");
		gDirectory->mkdir("MedTraded");
		gDirectory->mkdir("LeastTraded");
		gDirectory->mkdir("all");
		gDirectory->cd("all");

		gDirectory->mkdir("tickerDayDist");
		gDirectory->mkdir("PnlCumDaily");

		hPnlIntra_ = TH1F("hPnlIntraDist", "Pnl Intra by TickerDay", 100, -1e-4, 1e-4);
		hPnlClcl_ = TH1F("hPnlClclDist", "Pnl PrevClose-to-close by TickerDay", 100, -1e-4, 1e-4);
		hPnlClclNext_ = TH1F("hPnlClclNextDist", "Pnl Close-to-nextClose by TickerDay", 100, -1e-4, 1e-4);
		hPnlClclDaypos_ = TH1F("hPnlClclDayposDist", "Pnl Clcl SingleDayPos by TickerDay", 100, -1e-4, 1e-4);
		hNTrades_ = TH1F("hNtradesDist", "Number of Trades by TickerDay", 100, 0, 1);
		hRatBuy_ = TH1F("hRatBuyDist", "Buy Trades ratio by TickerDay", 100, 0, 1);
		hNShares_ = TH1F("hSharesDist", "Number of Shares traded by TickerDay", 100, 0, 1);
		hDollarvol_ = TH1F("hDollarvolDist", "Dollar volume by TickerDay", 100, 0, 1);
		hOpcl_ = TH1F("hOpcl", "Pnl Open-to-close by TickerDay", 100, 0, 1);
		hClop_ = TH1F("hClop", "Pnl Close-to-Open by TickerDay", 100, 0, 1);
		pIntraOpcl1_ = TProfile("pIntraOpcl1", "Open-to-close vs. Intra", 100, -10000, 10000);
		pIntraOpcl2_ = TProfile("pIntraOpcl2", "Open-to-close vs. Intra", 100, -1000, 1000);
		pIntraOpclNext1_ = TProfile("pIntraOpclNext1", "Open-to-close Next vs. Intra", 100, -10000, 10000);
		pIntraOpclNext2_ = TProfile("pIntraOpclNext2", "Open-to-close Next vs. Intra", 100, -1000, 1000);
		pIntraClop1_ = TProfile("pIntraClop1", "Close-to_open vs. Intra", 100, -10000, 10000);
		pIntraClop2_ = TProfile("pIntraClop2", "Close-to_open vs. Intra", 100, -1000, 1000);
		pIntraClopNext1_ = TProfile("pIntraClopNext1", "Close-to_open Next vs. Intra", 100, -10000, 10000);
		pIntraClopNext2_ = TProfile("pIntraClopNext2", "Close-to_open Next vs. Intra", 100, -1000, 1000);
		pIntraDolpos_ = TProfile("pIntraPos", "PnlIntra vs. lastDollarPos", 100, 0, 1);
		pChgposLastpos_ = TProfile("pChgposLastpos", "#DeltaPosition vs. LastPosition", 100, 0, 1);

		hPnlIntra_.SetCanExtend(TH1::kAllAxes);
		hPnlClcl_.SetCanExtend(TH1::kAllAxes);
		hPnlClclNext_.SetCanExtend(TH1::kAllAxes);
		hPnlClclDaypos_.SetCanExtend(TH1::kAllAxes);
		hNTrades_.SetCanExtend(TH1::kAllAxes);
		hNShares_.SetCanExtend(TH1::kAllAxes);
		hDollarvol_.SetCanExtend(TH1::kAllAxes);
		hOpcl_.SetCanExtend(TH1::kAllAxes);
		hClop_.SetCanExtend(TH1::kAllAxes);
		pIntraDolpos_.SetCanExtend(TH1::kAllAxes);
		pChgposLastpos_.SetCanExtend(TH1::kAllAxes);
	}

	if( condVar_.empty() && !idates.empty() )
	{
		int idate1 = idates[0];
		int idate2 = idates[idates.size() - 1];
		string selMarket;

		if( mto::isInternational(market) )
			selMarket = " and exchange = '" + market.substr(1, 1) + "' ";

		char cmd[1000];
		sprintf(cmd, "select symbol, count(*) as cnt from hforderevents "
			" where idate >= %d and idate <= %d and qty > 0 %s "
			" group by symbol order by cnt ",
			idate1, idate2, selMarket.c_str());
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hfo(market, idate1), cmd, vv);

		int N = vv.size();
		for( int i = 0; i < N / 3; ++i )
		{
			if( i < nTickerDetailMax_ / 3 )
			{
				string ticker1 = trim(vv[i][0]);
				vLeastTraded_.push_back(ticker1);

				string ticker2 = trim(vv[N - 1 - i][0]);
				vMostTraded_.push_back(ticker2);

				string ticker3 = trim(vv[N / 2 - nTickerDetailMax_ / 6 + i][0]);
				vMedTraded_.push_back(ticker3);
			}
		}
		sort(vMostTraded_.begin(), vMostTraded_.end());
		sort(vMedTraded_.begin(), vMedTraded_.end());
		sort(vLeastTraded_.begin(), vLeastTraded_.end());
	}

	return;
}

void HTradeAna::beginDay()
{
	psumDay_ = PnlSum();

	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	int idate_next = nextClose(market, idate);

	if( !condVar_.empty() )
	{
		vCondTickers_.clear();

		char cmd[1000];
		sprintf(cmd, "select %s from stockcharacteristics "
			" where %s "
			" and %s >= %f and %s < %f ",
			mto::compTicker(market).c_str(), mto::selChara(market, idate).c_str(), condVar_.c_str(), minCondVar_, condVar_.c_str(), maxCondVar_);

		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = (*it)[0];
			if( !ticker.empty() )
				vCondTickers_.push_back(ticker);
		}

		sort(vCondTickers_.begin(), vCondTickers_.end());
	}

	GCurr::Instance()->set_idate(idate);
	readClosePricePosition(mClose_, idate);
	readClosePricePosition(mCloseNext_, idate_next);

	GODBC::Instance()->close_all();

	return;
}

void HTradeAna::readClosePricePosition(map<string, CloseInfo>& mClose, int idate)
{
	mClose.clear();
	string market = HEnv::Instance()->market();
	int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();

	// Read close prices of previous trading day.
	vector<vector<string> > vvcl;
	{
		string cmd = (string)" select " + mto::compTicker(market) + ", close_ "
			+ " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate_prev);
		GODBC::Instance()->read(mto::hf(market), cmd, vvcl);
	}
	for( vector<vector<string> >::iterator it = vvcl.begin(); it != vvcl.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
		{
			double close = atof((*it)[1].c_str());
			mClose.insert( make_pair(symbol, CloseInfo(close)) );
		}
	}

	// Read close prices of today
	vector<vector<string> > vvclN;
	{
		string cmd = (string)" select " + mto::compTicker(market) + ", open_, close_, adjust "
			+ " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate)
			+ " order by " + mto::ts(market);
		GODBC::Instance()->read(mto::hf(market),cmd, vvclN);
	}
	for( vector<vector<string> >::iterator it = vvclN.begin(); it != vvclN.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
		{
			double nextOpen = atof((*it)[1].c_str());
			double nextClose = atof((*it)[2].c_str());
			double nextAdj = atof((*it)[3].c_str());
			map<string, CloseInfo>::iterator itm = mClose.find(symbol);
			if( itm != mClose.end() )
			{
				itm->second.nextOpen = nextOpen;
				itm->second.nextClose = nextClose;
				itm->second.nextAdj = nextAdj;
			}
		}
	}

	// Position on previous trading day.
	if( "M" == id_ )
	{
		vector<vector<string> > vvp;
		{
			string selMarket = "";
			if( mto::isInternational(market) )
				selMarket = " and exchange = " + quote(mto::code(market));
			string cmd = (string)" select symbol, pos "
				+ " from hfClosePosition "
				+ " where idate = " + itos(idate_prev)
				+ selMarket;
			GODBC::Instance()->read(mto::hfo(market, idate_prev), cmd, vvp);
		}
		for( vector<vector<string> >::iterator it = vvp.begin(); it != vvp.end(); ++it )
		{
			string temp_symbol = trim((*it)[0]);
			if( !temp_symbol.empty() )
			{
				string symbol = mto::compTicker(temp_symbol, market);
				int pos = atoi((*it)[1].c_str());
				map<string, CloseInfo>::iterator itm = mClose.find(symbol);
				if( itm != mClose.end() )
					itm->second.pos = pos;
			}
		}
	}
	else if( "" == id_ )
	{
		const map<string, int>* mPos = static_cast<const map<string, int>*>(HEvent::Instance()->get("", posName_));
		for( map<string, int>::const_iterator it = mPos->begin(); it != mPos->end(); ++it )
		{
			string symbol = it->first;
			int pos = it->second;

			map<string, CloseInfo>::iterator itm = mClose.find(symbol);
			if( itm != mClose.end() )
				itm->second.pos = pos;
		}
	}

	return;
}

void HTradeAna::beginTicker(const string& ticker)
{
	if( condVar_.empty() || binary_search(vCondTickers_.begin(), vCondTickers_.end(), ticker) )
		;
	else
		return;

	const vector<int>* msecQ = static_cast<const vector<int>*>(HEvent::Instance()->get(ticker, "msecQ"));
	const vector<double>* midQ = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ"));
	const vector<MercatorTrade>* pvmt = static_cast<const vector<MercatorTrade>*>(HEvent::Instance()->get(ticker, "mtrades"));
	intra_day_stat2(pvmt, msecQ, midQ, ticker);

	return;
}

void HTradeAna::intra_day_stat2(const vector<MercatorTrade>* pvmt, const vector<int>* msecQ, const vector<double>* midQ, const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	PnlSum psum;

	// Close prices.
	int lastClosePosition = 0;
	double lastClosePrice = 0.;
	double openPrice = 0.;
	double closePrice = 0.;
	double openPriceAdj = 0.;
	double closePriceAdj = 0.;
	map<string, CloseInfo>::iterator itm1 = mClose_.find(ticker);
	if( itm1 != mClose_.end() ) // get from stockcharacteristics
	{
		lastClosePosition = itm1->second.pos;

		if( itm1->second.close > ltmb_ )
			lastClosePrice = itm1->second.close;
		if( itm1->second.nextClose > ltmb_ )
		{
			closePrice = itm1->second.nextClose;
			closePriceAdj = itm1->second.nextClose * itm1->second.nextAdj;
		}
		if( itm1->second.nextOpen > ltmb_ )
		{
			openPrice = itm1->second.nextOpen;
			openPriceAdj = itm1->second.nextOpen * itm1->second.nextAdj;
		}
	}

	// Pnl prev close to open
	if( openPriceAdj > 0. && lastClosePrice > 0. )
		psum.pnlClop = lastClosePosition * (openPriceAdj - lastClosePrice);

	// Pnl prev close to close
	if( closePriceAdj > 0. && lastClosePrice > 0. )
		psum.pnlClcl = lastClosePosition * (closePriceAdj - lastClosePrice);

	// Pnl open to close.
	if( openPrice > 0. && closePrice > 0. )
		psum.pnlOpcl = lastClosePosition * (closePriceAdj - openPriceAdj);

	// Pnl close to next close.
	double retClclNext = 0.;
	double nextOpenAdj = 0.;
	double nextCloseAdj = 0.;
	int closePosition = lastClosePosition;
	double nextOpen = 0.;
	double nextClose = 0.;
	map<string, CloseInfo>::iterator itcn = mCloseNext_.find(ticker);
	if( itcn != mCloseNext_.end() )
	{
		CloseInfo& cl = itcn->second;
		closePosition = cl.pos;
		nextOpen = cl.nextOpen;
		nextClose = cl.nextClose;
		nextOpenAdj = cl.nextOpen * cl.nextAdj;
		nextCloseAdj = cl.nextClose * cl.nextAdj;
		if( abs(closePosition) > 0 && cl.close > ltmb_ )
		{
			// Close to close pnl from close to the next close.
			if( nextCloseAdj > ltmb_ )
			{
				double pnlClcl = closePosition * (nextCloseAdj - cl.close);
				psum.pnlClclNext = pnlClcl;
				retClclNext = nextCloseAdj / cl.close - 1.;
			}
		}
	}

	// Pnl next open to next close.
	if( nextClose > 0. && nextOpen > 0. )
		psum.pnlOpclNext = closePosition * (nextCloseAdj - nextOpenAdj);

	// Pnl close to next open.
	if( nextOpenAdj > 0. && closePrice > 0. )
		psum.pnlClopNext = closePosition * (nextOpenAdj - closePrice);

	string market = HEnv::Instance()->market();
	int nd = HEnv::Instance()->nDates();
	TradeActivity trdAct(market, idate0_, nBinsPerDay_);
	double lastPos = lastClosePosition;
	int NT = (pvmt == nullptr) ? 0 : pvmt->size();
	for( int i = 0; i < NT; ++i )
	{
		const MercatorTrade& mt = (*pvmt)[i];

		double iMidT = mt.mid;
		double iMidT_p = (i > 0) ? (*pvmt)[i - 1].mid : 0.;
		double iPrcT = mt.price;
		double iVolT = mt.qty;
		double iSignT = mt.sign;
		double iMsecT = mt.msecs;
		double fee = fee_bpt_ / 10000. * iPrcT * iVolT;

		// PNL inter sample.
		double pnlInter0 = 0;
		if( i == 0 )
			pnlInter0 = ( iMidT - openPrice ) * lastPos;
		else
			pnlInter0 = ( iMidT - iMidT_p ) * lastPos;
		double cost = iSignT * ( iMidT - iPrcT ) * iVolT;
		double pnlInter = pnlInter0 + cost - fee;

		// PNL intraday.
		double pnlIntra = iSignT * (closePrice - iPrcT) * iVolT - fee;

		// PNL from close to next close.
		double pnlClclNext = iSignT * retClclNext * iPrcT * iVolT;
		if( verbose_ == 1 )
			printf("%10.3f\n", pnlClclNext);

		// Update the sum for the tickerday.
		psum.pnlIntra += pnlIntra;
		psum.pnlInter += pnlInter;
		++psum.nTrades;
		if( ceil(iSignT - 0.5) > 0 )
			++psum.nTradesBuy;
		else if( ceil(iSignT - 0.5) < 0 )
			++psum.nTradesSell;
		psum.nShares += iVolT;
		psum.dollarvol += iVolT * iPrcT;

		trdAct.addTrade( iMsecT, iSignT, iVolT, iPrcT, pnlIntra, pnlInter, pnlClclNext );

		lastPos += iSignT * iVolT;
	}

	// Add the pnl from the last trade to the end of the day.
	double pnlTrcl = 0;
	if( NT > 0 )
		pnlTrcl = (closePrice - (*pvmt)[NT - 1].mid) * lastPos;
	else
		pnlTrcl = (closePrice - openPrice) * lastPos;
	psum.pnlInter += pnlTrcl;

	trdAct.endTickerDay(lastClosePosition, closePrice, pnlTrcl, msecQ, midQ);
	{
		boost::mutex::scoped_lock lock(ta_mutex_);
		trdActAgg_.addTickerDay(trdAct, iDay_);

		// Create new ticker at mTrdActAgg_.
		if( /*mTrdActAgg_.size() < nTickerDetailMax_ &&*/ mTrdActAgg_.find(ticker) == mTrdActAgg_.end()
			&& (binary_search(vMostTraded_.begin(), vMostTraded_.end(), ticker)
				|| binary_search(vMedTraded_.begin(), vMedTraded_.end(), ticker)
				|| binary_search(vLeastTraded_.begin(), vLeastTraded_.end(), ticker)) )
		{
			TradeActivityAgg tempTrdActAgg(market, idate0_, nd, nBinsPerDay_);
			mTrdActAgg_.insert(make_pair(ticker, tempTrdActAgg));
		}

		// Fill the ticker at mTrdActAgg_.
		if( mTrdActAgg_.find(ticker) != mTrdActAgg_.end() )
			mTrdActAgg_[ticker].addTickerDay(trdAct, iDay_);
	}

	// Pnl close to next close, using the single day sum of position change.
	int daypos = trdAct.getCumPosChg();
	if( closePrice > ltmb_ && nextCloseAdj > ltmb_ )
	{
		double pnlClcl = daypos * (nextCloseAdj - closePrice);
		psum.pnlClclDaypos = pnlClcl;
	}

	// Update histograms with the tickerday.
	double ratBuy = 0;
	if( psum.nTrades > 0 )
		ratBuy = psum.nTradesBuy / psum.nTrades;
	if( do_root_ && psum.nTrades > 0 )
	{
		boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
		hPnlIntra_.Fill(psum.pnlIntra);
		hPnlClcl_.Fill(psum.pnlClcl);
		hPnlClclNext_.Fill(psum.pnlClclNext);
		hPnlClclDaypos_.Fill(psum.pnlClclDaypos);
		hNTrades_.Fill(psum.nTrades);
		hRatBuy_.Fill(ratBuy);
		hNShares_.Fill(psum.nShares);
		hDollarvol_.Fill(psum.dollarvol);
		hOpcl_.Fill(psum.pnlOpcl);
		hClop_.Fill(psum.pnlClop);
		pIntraOpcl1_.Fill(psum.pnlIntra, psum.pnlOpcl);
		pIntraOpcl2_.Fill(psum.pnlIntra, psum.pnlOpcl);
		pIntraOpclNext1_.Fill(psum.pnlIntra, psum.pnlOpclNext);
		pIntraOpclNext2_.Fill(psum.pnlIntra, psum.pnlOpclNext);
		pIntraClop1_.Fill(psum.pnlIntra, psum.pnlClop);
		pIntraClop2_.Fill(psum.pnlIntra, psum.pnlClop);
		pIntraClopNext1_.Fill(psum.pnlIntra, psum.pnlClopNext);
		pIntraClopNext2_.Fill(psum.pnlIntra, psum.pnlClopNext);
		pIntraDolpos_.Fill(lastClosePosition, psum.pnlIntra);
		pChgposLastpos_.Fill(lastClosePosition, daypos);
	}

	// Update the summ for the day.
	{
		boost::mutex::scoped_lock lock(ps_mutex_);
		if( psum.nTrades > 0 )
			++psumDay_.nTickers;
		psumDay_.pnlIntra += psum.pnlIntra;
		psumDay_.pnlClcl += psum.pnlClcl;
		psumDay_.pnlClclNext += psum.pnlClclNext;
		psumDay_.pnlClclNotrade += psum.pnlClclNotrade;
		psumDay_.pnlClclDaypos += psum.pnlClclDaypos;
		psumDay_.pnlOpcl += psum.pnlOpcl;
		psumDay_.pnlClopNext += psum.pnlClopNext;
		psumDay_.pnlInter += psum.pnlInter;
		psumDay_.pnlClop += psum.pnlClop;
		psumDay_.nTrades += psum.nTrades;
		psumDay_.nTradesBuy += psum.nTradesBuy;
		psumDay_.nTradesSell += psum.nTradesSell;
		psumDay_.nShares += psum.nShares;
		psumDay_.dollarvol += psum.dollarvol;

		PnlSummary& ps = mPs_[ticker];
		update_summary(ps, psum);
	}

	//mIntra_[ticker] += psum.pnlIntra;

	return;
}

void HTradeAna::endTicker(const string& ticker)
{
	return;
}

void HTradeAna::endDay()
{
	string market = HEnv::Instance()->market();

	// End of day stat.
	int idate = HEnv::Instance()->idate();
	double exchrat = GCurr::Instance()->exchrat("US", market);
	psumDay_.adjust_exchrat(exchrat);

	double bpsIntra = (psumDay_.dollarvol > 0)?(psumDay_.pnlIntra / psumDay_.dollarvol * 10000):0;
	double bpsClcl = (psumDay_.dollarvol > 0)?(psumDay_.pnlClcl / psumDay_.dollarvol * 10000):0;
	double bpsTotal1 = (psumDay_.dollarvol > 0)?((psumDay_.pnlIntra + psumDay_.pnlClcl) / psumDay_.dollarvol * 10000):0;
	double bpsInter = (psumDay_.dollarvol > 0)?(psumDay_.pnlInter / psumDay_.dollarvol * 10000):0;
	double bpsClop = (psumDay_.dollarvol > 0)?(psumDay_.pnlClop / psumDay_.dollarvol * 10000):0;
	double bpsTotal2 = (psumDay_.dollarvol > 0)?((psumDay_.pnlInter + psumDay_.pnlClop) / psumDay_.dollarvol * 10000):0;
	double ratBuy = (psumDay_.nTrades > 0)?(psumDay_.nTradesBuy / psumDay_.nTrades):0;
	printf(" nTickers %d nTrades %d (bought %5.1f %%) nShares %d dollarvol %.1f\n",
		(int)psumDay_.nTickers, (int)psumDay_.nTrades, ratBuy*100.0, (int)psumDay_.nShares, psumDay_.dollarvol);
	printf(" pI %10.1f (%5.1f) pCc %8.1f (%5.1f) pTot %10.1f (%5.1f) pCcN %8.1f pTotN %10.1f\n",
		psumDay_.pnlIntra, bpsIntra, psumDay_.pnlClcl, bpsClcl, psumDay_.pnlIntra + psumDay_.pnlClcl, bpsTotal1,
		psumDay_.pnlClclNext, psumDay_.pnlIntra + psumDay_.pnlClclNext);
	printf(" pI'%10.1f (%5.1f) pCo %8.1f (%5.1f) pTot %10.1f (%5.1f) pCcd %8.1f pCcX %8.1f\n",
		psumDay_.pnlInter, bpsInter, psumDay_.pnlClop, bpsClop, psumDay_.pnlInter + psumDay_.pnlClop, bpsTotal2,
		psumDay_.pnlClclDaypos, psumDay_.pnlClclNotrade);
	cout << flush;

	// Vector for cum pnl plots.
	vdPnlIntra_.push_back(psumDay_.pnlIntra);
	vdPnlClcl_.push_back(psumDay_.pnlClcl);
	vdPnlClclNext_.push_back(psumDay_.pnlClclNext);
	vdPnlClclNotrade_.push_back(psumDay_.pnlClclNotrade);
	vdPnlClclDaypos_.push_back(psumDay_.pnlClclDaypos);
	vdPnlOpcl_.push_back(psumDay_.pnlOpcl);
	vdPnlClopNext_.push_back(psumDay_.pnlClopNext);
	vdPnlInter_.push_back(psumDay_.pnlInter);
	vdPnlClop_.push_back(psumDay_.pnlClop);
	vdPnlTotal_.push_back(psumDay_.pnlIntra + psumDay_.pnlClcl);

	// Update the summary for the day
	update_summary(ps_, psumDay_);

	++iDay_;

	return;
}

void HTradeAna::update_summary(PnlSummary& summary, PnlSum& sum)
{
	summary.accPnlIntra.add(sum.pnlIntra);
	summary.accPnlClcl.add(sum.pnlClcl);
	summary.accPnlClclDaypos.add(sum.pnlClclDaypos);
	summary.accPnlTotal1.add(sum.pnlIntra + sum.pnlClcl);
	summary.accPnlInter.add(sum.pnlInter);
	summary.accPnlClop.add(sum.pnlClop);
	summary.accPnlTotal2.add(sum.pnlInter + sum.pnlClop);
	summary.accDollarvol.add(sum.dollarvol);
	summary.accNTrades.add(sum.nTrades);
	summary.accBpsIntra.add( (sum.dollarvol > 0)?(sum.pnlIntra / sum.dollarvol * 10000):0 );
	summary.accBpsClcl.add( (sum.dollarvol > 0)?(sum.pnlClcl / sum.dollarvol * 10000):0 );
	summary.accBpsClclDaypos.add( (sum.dollarvol > 0)?(sum.pnlClclDaypos / sum.dollarvol * 10000):0 );
	summary.accBpsTotal1.add( (sum.dollarvol > 0)?((sum.pnlIntra + sum.pnlClcl) / sum.dollarvol * 10000):0 );
	summary.accBpsInter.add( (sum.dollarvol > 0)?(sum.pnlInter / sum.dollarvol * 10000):0 );
	summary.accBpsClop.add( (sum.dollarvol > 0)?(sum.pnlClop / sum.dollarvol * 10000):0 );
	summary.accBpsTotal2.add( (sum.dollarvol > 0)?((sum.pnlInter + sum.pnlClop) / sum.dollarvol * 10000):0 );
	return;
}

void HTradeAna::endMarket()
{
	string market = HEnv::Instance()->market();

	// End of market summary.
	cout << endl << "endMarket " << moduleName_ << endl;
	printf(" DV  daily: %11.1f +- %9.1f sum: %13.1f\n",
		ps_.accDollarvol.mean(), ps_.accDollarvol.stdev(), ps_.accDollarvol.sum);
	printf(" NT  daily: %11.1f +- %9.1f sum: %13.1f\n",
		ps_.accNTrades.mean(), ps_.accNTrades.stdev(), ps_.accNTrades.sum);
	printf(" pI  daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
		ps_.accPnlIntra.mean(), ps_.accPnlIntra.stdev()/sqrt(ps_.accPnlIntra.n),
		ps_.accBpsIntra.mean(), ps_.accBpsIntra.stdev()/sqrt(ps_.accBpsIntra.n),
		ps_.accPnlIntra.sum, ps_.accDollarvol.sum>0 ? ps_.accPnlIntra.sum/ps_.accDollarvol.sum*10000 : 0);
	printf(" pI' daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
		ps_.accPnlInter.mean(), ps_.accPnlInter.stdev()/sqrt(ps_.accPnlInter.n),
		ps_.accBpsInter.mean(), ps_.accBpsInter.stdev()/sqrt(ps_.accBpsInter.n),
		ps_.accPnlInter.sum, ps_.accDollarvol.sum>0 ? ps_.accPnlInter.sum/ps_.accDollarvol.sum*10000 : 0);
	printf(" pCc daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
		ps_.accPnlClcl.mean(), ps_.accPnlClcl.stdev()/sqrt(ps_.accPnlClcl.n),
		ps_.accBpsClcl.mean(), ps_.accBpsClcl.stdev()/sqrt(ps_.accBpsClcl.n),
		ps_.accPnlClcl.sum, ps_.accDollarvol.sum>0 ? ps_.accPnlClcl.sum/ps_.accDollarvol.sum*10000 : 0);
	printf(" pCcD daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
		ps_.accPnlClclDaypos.mean(), ps_.accPnlClclDaypos.stdev()/sqrt(ps_.accPnlClclDaypos.n),
		ps_.accBpsClclDaypos.mean(), ps_.accBpsClclDaypos.stdev()/sqrt(ps_.accBpsClclDaypos.n),
		ps_.accPnlClclDaypos.sum, ps_.accDollarvol.sum>0 ? ps_.accPnlClclDaypos.sum/ps_.accDollarvol.sum*10000 : 0);
	printf(" pCo daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
		ps_.accPnlClop.mean(), ps_.accPnlClop.stdev()/sqrt(ps_.accPnlClop.n),
		ps_.accBpsClop.mean(), ps_.accBpsClop.stdev()/sqrt(ps_.accBpsClop.n),
		ps_.accPnlClop.sum, ps_.accDollarvol.sum>0 ? ps_.accPnlClop.sum/ps_.accDollarvol.sum*10000 : 0);
	printf(" pT  daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
		ps_.accPnlTotal1.mean(), ps_.accPnlTotal1.stdev()/sqrt(ps_.accPnlTotal1.n),
		ps_.accBpsTotal1.mean(), ps_.accBpsTotal1.stdev()/sqrt(ps_.accBpsTotal1.n),
		ps_.accPnlTotal1.sum, ps_.accDollarvol.sum>0 ? ps_.accPnlTotal1.sum/ps_.accDollarvol.sum*10000 : 0);
	printf(" pT' daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
		ps_.accPnlTotal2.mean(), ps_.accPnlTotal2.stdev()/sqrt(ps_.accPnlTotal2.n),
		ps_.accBpsTotal2.mean(), ps_.accBpsTotal2.stdev()/sqrt(ps_.accBpsTotal2.n),
		ps_.accPnlTotal2.sum, ps_.accDollarvol.sum>0 ? ps_.accPnlTotal2.sum/ps_.accDollarvol.sum*10000 : 0);

	if( do_root_ )
	{
		TFile* f = HEnv::Instance()->outfile();
		plot_tradeActivity("all", trdActAgg_, market);

		TH1F hdPnlIntra = make_hist(vdPnlIntra_, "hdPnlIntra", "Cum Pnl Intraday", "cum", nDay_);
		TH1F hdPnlClcl = make_hist(vdPnlClcl_, "hdPnlClcl", "Cum Pnl PrevClose to Close", "cum", nDay_);
		TH1F hdPnlClclNext = make_hist(vdPnlClclNext_, "hdPnlClclNext", "Cum Pnl Close to NextClose", "cum", nDay_);
		TH1F hdPnlClclNotrade = make_hist(vdPnlClclNotrade_, "hdPnlClclNotrade", "Cum Pnl Close to NextClose, NoTrade", "cum", nDay_);
		TH1F hdPnlClclDaypos = make_hist(vdPnlClclDaypos_, "hdPnlClclDaypos", "Cum Pnl Close to NextClose, SingleDayPos", "cum", nDay_);
		TH1F hdPnlOpcl = make_hist(vdPnlOpcl_, "hdPnlOpcl", "Cum Pnl Open to close", "cum", nDay_);
		TH1F hdPnlClopNext = make_hist(vdPnlClopNext_, "hdPnlClopNext", "Cum Pnl Close to NextOpen", "cum", nDay_);
		TH1F hdPnlInter = make_hist(vdPnlInter_, "hdPnlInter", "Cum Pnl Intersample", "cum", nDay_);
		TH1F hdPnlClop = make_hist(vdPnlClop_, "hdPnlClop", "Cum Pnl Close to Open", "cum", nDay_);
		TH1F hdPnlTotal = make_hist(vdPnlTotal_, "hdPnlTotal", "Cum Pnl Intraday + Close to Close", "cum", nDay_);

		for( map<string, TradeActivityAgg>::iterator it = mTrdActAgg_.begin(); it != mTrdActAgg_.end(); ++it )
		{
			string ticker = it->first;
			string ticker_dir = removeColon(ticker);
			f->cd(moduleName_.c_str());
			gDirectory->cd(market.c_str());

			string dir_ticker_dir;
			if( binary_search(vMostTraded_.begin(), vMostTraded_.end(), ticker) )
			{
				gDirectory->cd("MostTraded");
				dir_ticker_dir = (string)"MostTraded/" + ticker_dir;
			}
			else if( binary_search(vLeastTraded_.begin(), vLeastTraded_.end(), ticker) )
			{
				gDirectory->cd("LeastTraded");
				dir_ticker_dir = (string)"LeastTraded/" + ticker_dir;
			}
			else if( binary_search(vMedTraded_.begin(), vMedTraded_.end(), ticker) )
			{
				gDirectory->cd("MedTraded");
				dir_ticker_dir = (string)"MedTraded/" + ticker_dir;
			}
			else
			{
				gDirectory->cd("Tickers");
				dir_ticker_dir = (string)"Tickers/" + ticker_dir;
			}
			gDirectory->mkdir(ticker_dir.c_str());
			plot_tradeActivity(dir_ticker_dir, it->second, market);
		}

		f->cd(moduleName_.c_str());
		gDirectory->cd(market.c_str());
		gDirectory->cd("all");
		gDirectory->cd("PnlCumDaily");
		hdPnlIntra.Write();
		hdPnlClcl.Write();
		hdPnlClclNext.Write();
		hdPnlClclNotrade.Write();
		hdPnlClclDaypos.Write();
		hdPnlOpcl.Write();
		hdPnlClopNext.Write();
		hdPnlInter.Write();
		hdPnlClop.Write();
		hdPnlTotal.Write();

		f->cd(moduleName_.c_str());
		gDirectory->cd(market.c_str());
		gDirectory->cd("all");
		gDirectory->cd("tickerDayDist");
		hPnlIntra_.Write();
		hPnlClcl_.Write();
		hPnlClclNext_.Write();
		hPnlClclDaypos_.Write();
		hNTrades_.Write();
		hRatBuy_.Write();
		hNShares_.Write();
		hDollarvol_.Write();
		hOpcl_.Write();
		hClop_.Write();
		pIntraOpcl1_.Write();
		pIntraOpcl2_.Write();
		pIntraOpclNext1_.Write();
		pIntraOpclNext2_.Write();
		pIntraClop1_.Write();
		pIntraClop2_.Write();
		pIntraClopNext1_.Write();
		pIntraClopNext2_.Write();
		pIntraDolpos_.Write();
		pChgposLastpos_.Write();
	}

	return;
}

void HTradeAna::plot_tradeActivity(string dir, TradeActivityAgg& taa, string market)
{
	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd(dir.c_str());
	gDirectory->mkdir("tradeActivity");
	gDirectory->mkdir("PosVsTime");
	gDirectory->mkdir("Pnl");
	gDirectory->mkdir("TimeOfDay");

	TH1F hNTrades = make_hist(taa.vNTrades, "hNTrades", "Number of Trades vs Time", "", nDay_);
	TH1F hNShares = make_hist(taa.vNShares, "hNShares", "Number of Shares vs Time", "", nDay_);
	TH1F hDollarvol = make_hist(taa.vDollarvol, "hDollarvol", "Dollar volume vs Time", "", nDay_);
	TH1F hSignedNTrades = make_hist(taa.vSignedNTrades, "hSignedNTrades", "Sum of Signed Trades vs Time", "", nDay_);

	TH1F hPnlIntra = make_hist(taa.vPnlIntra, "hPnlIntra", "Pnl Intraday", "", nDay_);
	TH1F hPnlInter = make_hist(taa.vPnlInter, "hPnlInter", "Pnl Intersample", "", nDay_);
	TH1F hCumPnlIntra = make_hist(taa.vPnlIntra, "hCumPnlIntra", "Cum Pnl Intraday", "cum", nDay_);
	TH1F hCumPnlInter = make_hist(taa.vPnlInter, "hCumPnlInter", "Cum Pnl Intersample", "cum", nDay_);
	TH1F hCumPnlClclNext = make_hist(taa.vPnlClclNext, "hCumPnlClclNext", "Cum Pnl Close to close", "cum", nDay_);

	TH1F hSharePos = make_hist(taa.vSharePos, "hSharePos", "Share Position vs Time", "", nDay_);
	TH1F hDollarPos = make_hist(taa.vDollarPos, "hDollarPos", "Dollar Position vs Time", "", nDay_);
	TH1F hSharePosAbs = make_hist(taa.vSharePosAbs, "hSharePosAbs", "Abs Share Position vs Time", "", nDay_);
	TH1F hDollarPosAbs = make_hist(taa.vDollarPosAbs, "hDollarPosAbs", "Abs Dollar Position vs Time", "", nDay_);

	TH1F hPrice = make_hist(taa.vPrc, "hPrice", "Price Series", "", nDay_);
	TH1F hReturn = make_hist(taa.vPrc, "hReturn", "Return Series", "ret", nDay_);

	TH1F hDollarvolTod = make_hist(taa.vDollarvol, "hDollarvolTod", "Dollar volume vs Time of day", "tod", 1.);
	TH1F hNTradesTod = make_hist(taa.vNTrades, "hNTradesTod", "Number of Trades vs Time of day", "tod", 1.);
	TH1F hPnlIntraTod = make_hist(taa.vPnlIntra, "hPnlIntraTod", "Pnl Intraday vs Time of day", "tod", 1.);
	TH1F hPnlClclNextTod = make_hist(taa.vPnlClclNext, "hPnlClclNextTod", "Pnl Close to close vs Time of day", "tod", 1.);

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd(dir.c_str());
	gDirectory->cd("tradeActivity");
	hNTrades.Write();
	hNShares.Write();
	hDollarvol.Write();
	hSignedNTrades.Write();

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd(dir.c_str());
	gDirectory->cd("Pnl");
	hPnlIntra.Write();
	hPnlInter.Write();
	hCumPnlIntra.Write();
	hCumPnlInter.Write();
	hCumPnlClclNext.Write();

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd(dir.c_str());
	gDirectory->cd("PosVsTime");
	hSharePos.Write();
	hDollarPos.Write();
	hSharePosAbs.Write();
	hDollarPosAbs.Write();
	hPrice.Write();
	hReturn.Write();

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd(dir.c_str());
	gDirectory->cd("TimeOfDay");
	hDollarvolTod.Write();
	hNTradesTod.Write();
	hPnlIntraTod.Write();
	hPnlClclNextTod.Write();

	return;
}

TH1F HTradeAna::make_hist(vector<double> v, string name, string title, string method, double xMax)
{
	int N = v.size();

	int nbins = N;
	if( "tod" == method )
		nbins = nBinsPerDay_;

	TH1F h(name.c_str(), title.c_str(), nbins, 0, xMax);

	double sum = 0;
	double lastVal = 0;
	for( int i=0; i<N; ++i )
	{
		if( "cum" == method )
		{
			sum += v[i];
			h.SetBinContent(i+1, sum);
		}
		else if( "ret" == method )
		{
			double ret = 0;
			if( fabs(lastVal) > 1e-10 )
				ret = (v[i] - lastVal) / lastVal;
			h.SetBinContent(i+1, ret);
			lastVal = v[i];
		}
		else if( "tod" == method )
		{
			double x = (double)(i % nBinsPerDay_) / nbins;
			h.Fill(x, v[i]);
		}
		else
			h.SetBinContent(i+1, v[i]);
	}
	return h;
}

void HTradeAna::endJob()
{
	return;
}

HTradeAna::TradeActivity::TradeActivity()
{}

HTradeAna::TradeActivity::TradeActivity(string market, int idate, int nBinsPerDay)
:market_(market),
nBinsPerDay_(nBinsPerDay)
{
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);

	for( int i=0; i<nBinsPerDay_; ++ i )
	{
		vNTradesBuy[i] = 0;
		vNTradesSell[i] = 0;
		vNShares[i] = 0;
		vDollarvol[i] = 0;

		vPnlIntra[i] = 0;
		vPnlInter[i] = 0;
		vPnlClclNext[i] = 0;

		vSharePos[i] = 0;
		vDollarPos[i] = 0;
		vSharePosAbs[i] = 0;
		vDollarPosAbs[i] = 0;

		vPosChg[i] = 0;
		vPrc[i] = 0;
	}
}

HTradeAna::TradeActivity::~TradeActivity()
{}

void HTradeAna::TradeActivity::addTrade(double msecs, double sign, double vol, double prc, double pnlIntra, double pnlInter, double pnlClclNext)
{
	int iBin = nBinsPerDay_ * (msecs - msecOpen_) / (msecClose_ - msecOpen_);

	if( iBin >= 0 )
	{
		if( sign > 0 )
			++vNTradesBuy[iBin];
		else if( sign < 0 )
			++vNTradesSell[iBin];
		vNShares[iBin] += vol;
		vDollarvol[iBin] += vol * prc;

		vPnlIntra[iBin] += pnlIntra;
		vPnlInter[iBin] += pnlInter;
		vPnlClclNext[iBin] += pnlClclNext;

		vPosChg[iBin] += sign * vol;
	}

	return;
}

void HTradeAna::TradeActivity::endTickerDay(int lastClosePosition, double closePrice, double pnlTrcl,
											   const vector<int>* msecQ, const vector<double>* midQ)
{
	// Fill the prices from midQ.
	if( midQ != 0 && !midQ->empty() )
	{
		int N = msecQ->size();
		for( int i=0; i<N; ++i )
		{
			int msecs = (*msecQ)[i];
			int iBin = (double)nBinsPerDay_ * (msecs - msecOpen_) / (msecClose_ - msecOpen_);
			if( iBin > 0 )
				vPrc[iBin] = (*midQ)[i];
		}
	}

	// Remove zero prices.
	double lastPrc = 0;
	{
		int lb = -1;
		for( int i=0; i<nBinsPerDay_; ++i )
		{
			if( vPrc[i] > 1e-6 )
			{
				lb = i;
				break;
			}
		}
		if( lb > -1 && lb < nBinsPerDay_ )
			lastPrc = vPrc[lb];
		else
			lastPrc = closePrice;
	}
	for( int i=0; i<nBinsPerDay_; ++i )
	{
		double prc = vPrc[i];
		if( prc > 1e-6 )
			lastPrc = prc;
		else
			vPrc[i] = lastPrc;
	}

	// Position and Exposure
	double cumPosChg = 0;
	for( int i=0; i<nBinsPerDay_; ++i )
	{
		cumPosChg += vPosChg[i];
		double posTicker = lastClosePosition + cumPosChg;
		vSharePos[i] += posTicker;
		vSharePosAbs[i] += fabs(posTicker);
		double dollarPosTicker = posTicker * vPrc[i];
		vDollarPos[i] += dollarPosTicker;
		vDollarPosAbs[i] += fabs(dollarPosTicker);
	}

	vPnlInter[nBinsPerDay_ - 1] += pnlTrcl;

	return;
}

double HTradeAna::TradeActivity::getCumPosChg()
{
	double cumPosChg = 0;
	for( int i=0; i<nBinsPerDay_; ++i )
	{
		cumPosChg += vPosChg[i];
	}
	return cumPosChg;
}

HTradeAna::TradeActivityAgg::TradeActivityAgg()
{}

HTradeAna::TradeActivityAgg::TradeActivityAgg(string market, int idate, int nd, int nBinsPerDay)
:market_(market),
nBinsPerDay_(nBinsPerDay)
{
	int nBins = nBinsPerDay_ * nd;
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);

	vNTrades = vector<double>(nBins, 0);
	vNShares = vector<double>(nBins, 0);
	vDollarvol = vector<double>(nBins, 0);
	vSignedNTrades = vector<double>(nBins, 0);

	vPnlIntra = vector<double>(nBins, 0);
	vPnlInter = vector<double>(nBins, 0);
	vPnlClclNext = vector<double>(nBins, 0);

	vSharePos = vector<double>(nBins, 0);
	vDollarPos = vector<double>(nBins, 0);
	vSharePosAbs = vector<double>(nBins, 0);
	vDollarPosAbs = vector<double>(nBins, 0);

	vPrc = vector<double>(nBins, 0);
}

HTradeAna::TradeActivityAgg::~TradeActivityAgg()
{}

void HTradeAna::TradeActivityAgg::addTickerDay(const TradeActivity& trdAct, int iDay)
{
	int msecsOffset = iDay * (msecClose_ - msecOpen_);
	int iOffset = msecsOffset / (msecClose_ - msecOpen_) * nBinsPerDay_;
	for( int i=0; i<nBinsPerDay_; ++i )
	{
		int io = i + iOffset;
		vNShares[io] += trdAct.vNShares[i];
		vDollarvol[io] += trdAct.vDollarvol[i];

		vPnlIntra[io] += trdAct.vPnlIntra[i];
		vPnlInter[io] += trdAct.vPnlInter[i];
		vPnlClclNext[io] += trdAct.vPnlClclNext[i];

		vSharePos[io] += trdAct.vSharePos[i];
		vDollarPos[io] += trdAct.vDollarPos[i];
		vSharePosAbs[io] += trdAct.vSharePosAbs[i];
		vDollarPosAbs[io] += trdAct.vDollarPosAbs[i];

		vPrc[io] += trdAct.vPrc[i];

		vNTrades[io] += trdAct.vNTradesBuy[i] + trdAct.vNTradesSell[i];
		vSignedNTrades[io] += trdAct.vNTradesBuy[i] - trdAct.vNTradesSell[i];
	}

	return;
}
