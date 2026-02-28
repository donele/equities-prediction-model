#include <HOrders/HTradeAnaVolume.h>
#include "optionlibs/TickData.h"
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GCurr.h>
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

HTradeAnaVolume::HTradeAnaVolume(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
verbose_(0),
idate0_(0),
nP_(10),
fee_bpt_(0.),
costMult_(1),
tradeModuleName_(""),
id_(""),
msecName_(""),
prcName_(""),
volName_(""),
signName_(""),
midName_("")
{
	if( conf.count("debug") )
		if( conf.find("debug")->second == "true" )
			debug_ = true;

	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());

	if( conf.count("id") )
	{
		id_ = conf.find("id")->second;
		if( !id_.empty() )
		{
			msecName_ = (string)"msec" + id_;
			prcName_ = (string)"prc" + id_;
			volName_ = (string)"vol" + id_;
			signName_ = (string)"sign" + id_;
			midName_ = (string)"mid" + id_;
		}
	}
	else if( conf.count("tradeModuleName") )
	{
		tradeModuleName_ = conf.find("tradeModuleName")->second;
		if( !tradeModuleName_.empty() )
		{
			msecName_ = tradeModuleName_ + "_msec";
			signName_ = tradeModuleName_ + "_sign";
			prcName_ = tradeModuleName_ + "_prc";
			volName_ = tradeModuleName_ + "_vol";
			midName_ = tradeModuleName_ + "_mid";
			posName_ = tradeModuleName_ + "_mLastClosePos";
			sampleOpenName_ = tradeModuleName_ + "_sampleOpen";
		}
	}
}

HTradeAnaVolume::~HTradeAnaVolume()
{}

void HTradeAnaVolume::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	const double* pCostMult = static_cast<const double*>(HEvent::Instance()->get("", tradeModuleName_ + "_costMult"));
	if( pCostMult != 0 )
		costMult_ = *pCostMult;

	int idate = 0;
	ifstream ie(xpf("..\\expr_dates.txt").c_str());
	while( ie >> idate )
		edates_.insert(idate);
	ifstream ic(xpf("..\\cond_dates.txt").c_str());
	while( ic >> idate )
		cdates_.insert(idate);

	return;
}

void HTradeAnaVolume::beginMarket()
{
	string market = HEnv::Instance()->market();
	idate0_ = HEnv::Instance()->idates()[0];
	fee_bpt_ = mto::fee_bpt(market);

	iDay_ = 0;
	int msecOpen = mto::msecOpen(market, idate0_);
	int msecClose = mto::msecClose(market, idate0_);

	int nd = HEnv::Instance()->nDates();
	nHours_ = (msecClose - msecOpen)/1000.0/60.0/60.0 * nd;

	return;
}

void HTradeAnaVolume::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	GCurr::Instance()->set_idate(idate);
	exchrat_ = GCurr::Instance()->exchrat("US", market);
	//exchrat_ = 1.;

	readClosePricePosition();

	if( edates_.count(idate) )
		isExpr_ = true;
	else
		isExpr_ = false;

	return;
}

void HTradeAnaVolume::readClosePricePosition()
{
	mClose_.clear();
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();

	// Read close prices of previous trading day.
	vector<vector<string> > vvcl;
	{
		string cmd = (string)" select " + mto::compTicker(market) + ", close_ "
			+ " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate_prev);
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vvcl);
	}
	for( vector<vector<string> >::iterator it = vvcl.begin(); it != vvcl.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
		{
			double close = atof((*it)[1].c_str());
			mClose_.insert( make_pair(symbol, CloseInfo(close)) );
		}
	}

	// Read close prices of today
	mTickerMedvol_.clear();
	vector<vector<string> > vvclN;
	{
		string cmd = (string)" select " + mto::compTicker(market) + ", open_, close_, medVolume "
			+ " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate)
			+ " order by " + mto::ts(market);
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vvclN);
	}
	for( vector<vector<string> >::iterator it = vvclN.begin(); it != vvclN.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
		{
			double nextOpen = atof((*it)[1].c_str());
			double nextClose = atof((*it)[2].c_str());
			map<string, CloseInfo>::iterator itm = mClose_.find(symbol);
			if( itm != mClose_.end() )
			{
				itm->second.nextOpen = nextOpen;
				itm->second.nextClose = nextClose;
			}

			double medvol = atof((*it)[2].c_str()) * atof((*it)[3].c_str()) * exchrat_;
			mTickerMedvol_[symbol].add(medvol);
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
			GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vvp);
		}
		for( vector<vector<string> >::iterator it = vvp.begin(); it != vvp.end(); ++it )
		{
			string temp_symbol = trim((*it)[0]);
			if( !temp_symbol.empty() )
			{
				string symbol = mto::compTicker(temp_symbol, market);
				int pos = atoi((*it)[1].c_str());
				map<string, CloseInfo>::iterator itm = mClose_.find(symbol);
				if( itm != mClose_.end() )
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

			map<string, CloseInfo>::iterator itm = mClose_.find(symbol);
			if( itm != mClose_.end() )
				itm->second.pos = pos;
		}
	}

	return;
}

void HTradeAnaVolume::beginTicker(const string& ticker)
{
	const vector<double>* msecQ = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ"));
	const vector<double>* midQ = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ"));
	const vector<double>* msecQ1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ1s"));
	const vector<double>* midQ1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ1s"));

	const vector<double>* msecT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, msecName_));
	const vector<double>* prcT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, prcName_));
	const vector<double>* volT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, volName_));
	const vector<double>* signT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, signName_));
	const vector<double>* midT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, midName_));

	if( 0 == msecQ || 0 == midQ || 0 == msecQ1s || 0 == midQ1s ||
		0 == msecT || 0 == prcT || 0 == volT || 0 == signT ||
		msecQ->empty() || msecQ1s->empty() || midQ->empty() || midQ1s->empty() ||
		msecT->empty() || prcT->empty() || volT->empty() || signT->empty() )
		return;

	// Intra day stat.
	intra_day_stat(msecT, prcT, volT, signT, midT, msecQ, midQ, ticker);

	return;
}

void HTradeAnaVolume::intra_day_stat(const vector<double>* msecT, const vector<double>* prcT, const vector<double>* volT,
     const vector<double>* signT, const vector<double>* midT, const vector<double>* msecQ, const vector<double>* midQ, string ticker)
{
	int idate = HEnv::Instance()->idate();
	//PnlSum psum;

	// Close prices.
	int lastClosePosition = 0;
	double closePrice = (*midQ)[midQ->size() - 1]; // from tick data
	double lastClosePrice = (*midQ)[0]; // from tick data
	map<string, CloseInfo>::iterator itm1 = mClose_.find(ticker);
	if( itm1 != mClose_.end() ) // from stockcharacteristics
	{
		lastClosePosition = itm1->second.pos;

		if( itm1->second.nextClose > 1e-6 )
			closePrice = itm1->second.nextClose;
		else
			itm1->second.nextClose = closePrice;

		if( itm1->second.close > 1e-6 )
			lastClosePrice = itm1->second.close;
		else
			itm1->second.close = lastClosePrice;
	}

	// Open price.
	double openPrice = (*midQ)[0]; // from tick data.
	map<string, CloseInfo>::iterator itm2 = mClose_.find(ticker);
	if( itm2 != mClose_.end() ) // from stockcharacteristics
	{
		if( itm2->second.nextOpen > 1e-6 )
			openPrice = itm2->second.nextOpen;
		else
			itm2->second.nextOpen = openPrice;
	}

	// Pnl close to open
	double pnlClop = 0.;
	if( openPrice > 0 )
	{
		bool corpAction = lastClosePrice / openPrice > 1.2 || openPrice / lastClosePrice < 0.8;
		if( !corpAction )
		{
			pnlClop = lastClosePosition * (openPrice - lastClosePrice);
			//psum.pnlClop = pnlClop;
		}
	}

	// Pnl close to close
	double pnlClcl = 0.;
	double pnlOpcl = 0.;
	map<string, CloseInfo>::iterator itc = mClose_.find(ticker);
	if( itc != mClose_.end() )
	{
		CloseInfo& cl = itc->second;
		if( abs(cl.pos) > 0 && cl.close > ltmb_ )
		{
			// Close close pnl from last close to today's close with last closing position.
			if( cl.nextClose > ltmb_ )
			{
				bool corpAction = lastClosePrice / openPrice > 1.2 || openPrice / lastClosePrice < 0.8;
				if( !corpAction )
				{
					pnlClcl = cl.pos * (cl.nextClose - cl.close);
					//psum.pnlClcl = pnlClcl;
				}
				else // Use pnlOpcl
				{
					pnlOpcl = cl.pos * (cl.nextClose - openPrice);
					//psum.pnlClcl = pnlOpcl;
				}
			}
		}
	}

	string market = HEnv::Instance()->market();
	int nd = HEnv::Instance()->nDates();
	double lastPos = lastClosePosition;
	int NT = msecT->size();
	double pnlIntra = 0.;
	for( int i = 0; i < NT; ++i )
	{
		double iMidT = (*midT)[i];
		double iMidT_p = (i > 0) ? (*midT)[i - 1] : 0;
		double iPrcT = (*prcT)[i];
		double iVolT = (*volT)[i];
		double iSignT = (*signT)[i];
		double iMsecT = (*msecT)[i];
		double fee = fee_bpt_ / 10000. * iPrcT * iVolT;

		// PNL inter sample.
		double pnlInter0 = 0;
		if( i == 0 )
			pnlInter0 = ( iMidT - openPrice ) * lastPos;
		else
			pnlInter0 = ( iMidT - iMidT_p ) * lastPos;
		double cost = fabs( iMidT - iPrcT ) * iVolT;
		double pnlInter = pnlInter0 - cost - fee;

		// PNL intraday.
		pnlIntra += iSignT * (closePrice - iPrcT) * iVolT - fee;

		lastPos += iSignT * iVolT;
	}

	// Add the pnl from the last trade to the end of the day.
	double pnlTrcl = 0;
	if( NT > 0 )
		pnlTrcl = (closePrice - (*midT)[NT - 1]) * lastPos;
	else
		pnlTrcl = (closePrice - openPrice) * lastPos;

	pnlIntra *= exchrat_;
	pnlClcl *= exchrat_;

	if( isExpr_ )
	{
		boost::mutex::scoped_lock lock(e_mutex_);
		emTickerPnl_[ticker].add(pnlIntra);
		emTickerPnlTot_[ticker].add(pnlIntra + pnlClcl);
	}
	else
	{
		boost::mutex::scoped_lock lock(c_mutex_);
		cmTickerPnl_[ticker].add(pnlIntra);
		cmTickerPnlTot_[ticker].add(pnlIntra + pnlClcl);
	}

	return;
}

void HTradeAnaVolume::endTicker(const string& ticker)
{
	return;
}

void HTradeAnaVolume::endDay()
{
	++iDay_;

	return;
}

void HTradeAnaVolume::endMarket()
{
	string market = HEnv::Instance()->market();
	ofstream ofs( (market + "_pnl.txt").c_str() );
	char buf[400];
	for( map<string, Acc>::iterator it = emTickerPnl_.begin(); it != emTickerPnl_.end(); ++it )
	{
		string ticker = it->first;
		double ePnl = it->second.mean();
		double ePnlStdev = it->second.stdev();
		double cPnl = cmTickerPnl_[ticker].mean();
		double cPnlStdev = cmTickerPnl_[ticker].stdev();
		double vol = mTickerMedvol_[ticker].mean();
		sprintf(buf, "%12s %12.1f %8.2f %8.2f %8.2f %8.2f\n", ticker.c_str(), vol, ePnl, ePnlStdev, cPnl, cPnlStdev);
		ofs << buf;
	}

	return;
}

void HTradeAnaVolume::endJob()
{
	return;
}
