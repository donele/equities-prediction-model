#include <HOrders/HTradeAnaProfile.h>
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

HTradeAnaProfile::HTradeAnaProfile(const string& moduleName, const multimap<string, string>& conf)
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
	if( conf.count("condVar") )
		condVar_ = conf.find("condVar")->second;
}

HTradeAnaProfile::~HTradeAnaProfile()
{}

void HTradeAnaProfile::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	const double* pCostMult = static_cast<const double*>(HEvent::Instance()->get("", tradeModuleName_ + "_costMult"));
	if( pCostMult != 0 )
		costMult_ = *pCostMult;

	return;
}

void HTradeAnaProfile::beginMarket()
{
	string market = HEnv::Instance()->market();
	idate0_ = HEnv::Instance()->idates()[0];
	fee_bpt_ = mto::fee_bpt(market);

	ps_.resize(nP_);
	psumDay_.resize(nP_);
	for( int i = 0; i < nP_; ++i )
		ps_[i] = PnlSummary();

	iDay_ = 0;
	int msecOpen = mto::msecOpen(market, idate0_);
	int msecClose = mto::msecClose(market, idate0_);

	int nd = HEnv::Instance()->nDates();
	nHours_ = (msecClose - msecOpen)/1000.0/60.0/60.0 * nd;

	return;
}

void HTradeAnaProfile::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	for( int i = 0; i < nP_; ++i )
		psumDay_[i] = PnlSum();

	readClosePricePosition();
	charaProfile_.read(market, idate, condVar_, nP_, "ticker");

	GODBC::Instance()->close_all();

	return;
}

void HTradeAnaProfile::readClosePricePosition()
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
	vector<vector<string> > vvclN;
	{
		string cmd = (string)" select " + mto::compTicker(market) + ", open_, close_ "
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

void HTradeAnaProfile::beginTicker(const string& ticker)
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

void HTradeAnaProfile::intra_day_stat(const vector<double>* msecT, const vector<double>* prcT, const vector<double>* volT,
     const vector<double>* signT, const vector<double>* midT, const vector<double>* msecQ, const vector<double>* midQ, string ticker)
{
	int idate = HEnv::Instance()->idate();
	PnlSum psum;

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
	if( openPrice > 0 )
	{
		bool corpAction = lastClosePrice / openPrice > 1.2 || openPrice / lastClosePrice < 0.8;
		if( !corpAction )
		{
			double pnlClop = lastClosePosition * (openPrice - lastClosePrice);
			psum.pnlClop = pnlClop;
		}
	}

	// Pnl close to close
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
					double pnlClcl = cl.pos * (cl.nextClose - cl.close);
					psum.pnlClcl = pnlClcl;
				}
				else // Use pnlOpcl
				{
					double pnlOpcl = cl.pos * (cl.nextClose - openPrice);
					psum.pnlClcl = pnlOpcl;
				}
			}
		}
	}

	string market = HEnv::Instance()->market();
	int nd = HEnv::Instance()->nDates();
	double lastPos = lastClosePosition;
	int NT = msecT->size(); 
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
		double pnlIntra = iSignT * (closePrice - iPrcT) * iVolT - fee;

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

		lastPos += iSignT * iVolT;
	}

	// Add the pnl from the last trade to the end of the day.
	double pnlTrcl = 0;
	if( NT > 0 )
		pnlTrcl = (closePrice - (*midT)[NT - 1]) * lastPos;
	else
		pnlTrcl = (closePrice - openPrice) * lastPos;
	psum.pnlInter += pnlTrcl;

	// Update the sum for the day.
	{
		int iBin = charaProfile_.get_indx(base_name(ticker));
		boost::mutex::scoped_lock lock(ps_mutex_);
		psumDay_[iBin].pnlIntra += psum.pnlIntra;
		psumDay_[iBin].pnlClcl += psum.pnlClcl;
		psumDay_[iBin].pnlInter += psum.pnlInter;
		psumDay_[iBin].pnlClop += psum.pnlClop;
		psumDay_[iBin].nTrades += psum.nTrades;
		psumDay_[iBin].nTradesBuy += psum.nTradesBuy;
		psumDay_[iBin].nTradesSell += psum.nTradesSell;
		psumDay_[iBin].nShares += psum.nShares;
		psumDay_[iBin].dollarvol += psum.dollarvol;
	}

	return;
}

void HTradeAnaProfile::endTicker(const string& ticker)
{
	return;
}

void HTradeAnaProfile::endDay()
{
	if( 1 )
	{
		int idate = HEnv::Instance()->idate();
		cout << endl << "endDay " << idate << " " << moduleName_ << endl;
		for( int i = 0; i < nP_; ++i )
		{
			cout << i << " of " << nP_ << "\n";
			double bpsIntra = (psumDay_[i].dollarvol > 0)?(psumDay_[i].pnlIntra / psumDay_[i].dollarvol * 10000):0;
			double bpsClcl = (psumDay_[i].dollarvol > 0)?(psumDay_[i].pnlClcl / psumDay_[i].dollarvol * 10000):0;
			double bpsTotal1 = (psumDay_[i].dollarvol > 0)?((psumDay_[i].pnlIntra + psumDay_[i].pnlClcl) / psumDay_[i].dollarvol * 10000):0;
			double bpsInter = (psumDay_[i].dollarvol > 0)?(psumDay_[i].pnlInter / psumDay_[i].dollarvol * 10000):0;
			double bpsClop = (psumDay_[i].dollarvol > 0)?(psumDay_[i].pnlClop / psumDay_[i].dollarvol * 10000):0;
			double bpsTotal2 = (psumDay_[i].dollarvol > 0)?((psumDay_[i].pnlInter + psumDay_[i].pnlClop) / psumDay_[i].dollarvol * 10000):0;
			double ratBuy = (psumDay_[i].nTrades > 0)?(psumDay_[i].nTradesBuy / psumDay_[i].nTrades):0;
			printf(" nTrades %d (bought %5.1f %%) nShares %d dollarvol %.1f\n",
				(int)psumDay_[i].nTrades, ratBuy*100.0, (int)psumDay_[i].nShares, psumDay_[i].dollarvol);
			printf(" pI %10.1f (%5.1f) pCc %8.1f (%5.1f) pTot %10.1f (%5.1f)\n",
				psumDay_[i].pnlIntra, bpsIntra, psumDay_[i].pnlClcl, bpsClcl, psumDay_[i].pnlIntra + psumDay_[i].pnlClcl, bpsTotal1);
			printf(" pI'%10.1f (%5.1f) pCo %8.1f (%5.1f) pTot %10.1f (%5.1f)\n",
				psumDay_[i].pnlInter, bpsInter, psumDay_[i].pnlClop, bpsClop, psumDay_[i].pnlInter + psumDay_[i].pnlClop, bpsTotal2);
			cout << flush;

			if( verbose_ == 2 )
			{
				double t1 = psumDay_[i].pnlIntra + psumDay_[i].pnlClcl;
				double t2 = psumDay_[i].pnlInter + psumDay_[i].pnlClop;
				printf("%.1f %.1f\n", t1, t2);
			}

			// Update the summary for the day
			update_summary(ps_[i], psumDay_[i]);
		}
	}

	++iDay_;

	return;
}

void HTradeAnaProfile::update_summary(PnlSummary& summary, PnlSum& sum)
{
	summary.accPnlIntra.add(sum.pnlIntra);
	summary.accPnlClcl.add(sum.pnlClcl);
	summary.accPnlTotal1.add(sum.pnlIntra + sum.pnlClcl);
	summary.accPnlInter.add(sum.pnlInter);
	summary.accPnlClop.add(sum.pnlClop);
	summary.accPnlTotal2.add(sum.pnlInter + sum.pnlClop);
	summary.accDollarvol.add(sum.dollarvol);
	summary.accNTrades.add(sum.nTrades);
	summary.accBpsIntra.add( (sum.dollarvol > 0)?(sum.pnlIntra / sum.dollarvol * 10000):0 );
	summary.accBpsClcl.add( (sum.dollarvol > 0)?(sum.pnlClcl / sum.dollarvol * 10000):0 );
	summary.accBpsTotal1.add( (sum.dollarvol > 0)?((sum.pnlIntra + sum.pnlClcl) / sum.dollarvol * 10000):0 );
	summary.accBpsInter.add( (sum.dollarvol > 0)?(sum.pnlInter / sum.dollarvol * 10000):0 );
	summary.accBpsClop.add( (sum.dollarvol > 0)?(sum.pnlClop / sum.dollarvol * 10000):0 );
	summary.accBpsTotal2.add( (sum.dollarvol > 0)?((sum.pnlInter + sum.pnlClop) / sum.dollarvol * 10000):0 );
	return;
}

void HTradeAnaProfile::endMarket()
{
	string market = HEnv::Instance()->market();

	// End of market summary.
	cout << endl << "endMarket " << moduleName_ << endl;
	for( int i = 0; i < nP_; ++i )
	{
		cout << i << " of " << nP_ << "\n";
		printf(" DV  daily: %11.1f +- %9.1f sum: %13.1f\n",
			ps_[i].accDollarvol.mean(), ps_[i].accDollarvol.stdev(), ps_[i].accDollarvol.sum);
		printf(" NT  daily: %11.1f +- %9.1f sum: %13.1f\n",
			ps_[i].accNTrades.mean(), ps_[i].accNTrades.stdev(), ps_[i].accNTrades.sum);
		printf(" pI  daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
			ps_[i].accPnlIntra.mean(), ps_[i].accPnlIntra.stdev()/sqrt(ps_[i].accPnlIntra.n),
			ps_[i].accBpsIntra.mean(), ps_[i].accBpsIntra.stdev()/sqrt(ps_[i].accBpsIntra.n),
			ps_[i].accPnlIntra.sum, ps_[i].accDollarvol.sum>0?ps_[i].accPnlIntra.sum/ps_[i].accDollarvol.sum*10000:0);
		printf(" pI' daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
			ps_[i].accPnlInter.mean(), ps_[i].accPnlInter.stdev()/sqrt(ps_[i].accPnlInter.n),
			ps_[i].accBpsInter.mean(), ps_[i].accBpsInter.stdev()/sqrt(ps_[i].accBpsInter.n),
			ps_[i].accPnlInter.sum, ps_[i].accDollarvol.sum>0?ps_[i].accPnlInter.sum/ps_[i].accDollarvol.sum*10000:0);
		printf(" pCc daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
			ps_[i].accPnlClcl.mean(), ps_[i].accPnlClcl.stdev()/sqrt(ps_[i].accPnlClcl.n),
			ps_[i].accBpsClcl.mean(), ps_[i].accBpsClcl.stdev()/sqrt(ps_[i].accBpsClcl.n),
			ps_[i].accPnlClcl.sum, ps_[i].accDollarvol.sum>0?ps_[i].accPnlClcl.sum/ps_[i].accDollarvol.sum*10000:0);
		printf(" pCo daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
			ps_[i].accPnlClop.mean(), ps_[i].accPnlClop.stdev()/sqrt(ps_[i].accPnlClop.n),
			ps_[i].accBpsClop.mean(), ps_[i].accBpsClop.stdev()/sqrt(ps_[i].accBpsClop.n),
			ps_[i].accPnlClop.sum, ps_[i].accDollarvol.sum>0?ps_[i].accPnlClop.sum/ps_[i].accDollarvol.sum*10000:0);
		printf(" pT  daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
			ps_[i].accPnlTotal1.mean(), ps_[i].accPnlTotal1.stdev()/sqrt(ps_[i].accPnlTotal1.n),
			ps_[i].accBpsTotal1.mean(), ps_[i].accBpsTotal1.stdev()/sqrt(ps_[i].accBpsTotal1.n),
			ps_[i].accPnlTotal1.sum, ps_[i].accDollarvol.sum>0?ps_[i].accPnlTotal1.sum/ps_[i].accDollarvol.sum*10000:0);
		printf(" pT' daily: %8.1f +- %7.1f (%6.1f +- %5.1f) sum: %10.1f (%6.1f)\n",
			ps_[i].accPnlTotal2.mean(), ps_[i].accPnlTotal2.stdev()/sqrt(ps_[i].accPnlTotal2.n),
			ps_[i].accBpsTotal2.mean(), ps_[i].accBpsTotal2.stdev()/sqrt(ps_[i].accBpsTotal2.n),
			ps_[i].accPnlTotal2.sum, ps_[i].accDollarvol.sum>0?ps_[i].accPnlTotal2.sum/ps_[i].accDollarvol.sum*10000:0);
	}

	return;
}

void HTradeAnaProfile::endJob()
{
	return;
}
