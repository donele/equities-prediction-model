#include <HOrders/HCloseToCloseAna.h>
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

// Overnight position is calculated by FIFO.

HCloseToCloseAna::HCloseToCloseAna(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
verbose_(0),
iday_(0),
openMsecs_(0),
closeMsecs_(0),
nBinsPerDay_(1),
secsPerBin_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";

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
			schedName_ = (string)"sched" + id_;
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
			schedName_ = tradeModuleName_ + "_sched";
			midName_ = tradeModuleName_ + "_mid";
			posName_ = tradeModuleName_ + "_mLastClosePos";
			sampleOpenName_ = tradeModuleName_ + "_sampleOpen";
		}
	}
}

HCloseToCloseAna::~HCloseToCloseAna()
{}

void HCloseToCloseAna::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	// Create the output file.
	TFile* f = HEnv::Instance()->outfile();
	f->mkdir(moduleName_.c_str());

	return;
}

void HCloseToCloseAna::beginMarket()
{
	string market = HEnv::Instance()->market();

	int idate0 = HEnv::Instance()->idates()[0];
	int ndays = HEnv::Instance()->idates().size();
	openMsecs_ = mto::msecOpen(market, idate0);
	closeMsecs_ = mto::msecClose(market, idate0);
	secsPerBin_ = (closeMsecs_ - openMsecs_) / 1000. / nBinsPerDay_;

	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->mkdir(market.c_str());

	int nbins = nBinsPerDay_ * ndays;

	hNTickers_ = TH1F("hNTickers", "Number of tickers", nbins, 0, ndays);
	hNShares_ = TH1F("hNShares", "Number of shares", nbins, 0, ndays);
	hDollarvol_ = TH1F("hDollarvol", "Dollar volume", nbins, 0, ndays);
	hNSharesAbs_ = TH1F("hNSharesAbs", "Sum of  abs shares", nbins, 0, ndays);
	hDollarvolAbs_ = TH1F("hDollarvolAbs", "Sum of abs Dollar volume", nbins, 0, ndays);

	hPnlSumClcl_ = TH1F("hPnlSumClcl", "SumPnl Close-to-nextClose", nbins, 0, ndays);
	hPnlSumClop_ = TH1F("hPnlSumClop", "SumPnl Close-to-nextOpen", nbins, 0, ndays);
	hPnlSumOpcl_ = TH1F("hPnlSumOpcl", "SumPnl NextOpen-to-nextClose", nbins, 0, ndays);

	pPnlClcl_ = TProfile("pPnlClcl", "AvgPnl Close-to-nextClose", nbins, 0, ndays);
	pPnlClop_ = TProfile("pPnlClop", "AvgPnl Close-to-nextOpen", nbins, 0, ndays);
	pPnlOpcl_ = TProfile("pPnlOpcl", "AvgPnl NextOpen-to-nextClose", nbins, 0, ndays);

	pRtnClcl_ = TProfile("pRtnClcl", "AvgRtn Close-to-nextClose", nbins, 0, ndays);
	pRtnClop_ = TProfile("pRtnClop", "AvgRtn Close-to-nextOpen", nbins, 0, ndays);
	pRtnOpcl_ = TProfile("pRtnOpcl", "AvgRtn NextOpen-to-nextClose", nbins, 0, ndays);

	return;
}

void HCloseToCloseAna::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	readClosePricePosition(mCloseNext_, idate);

	GODBC::Instance()->close_all();

	return;
}

void HCloseToCloseAna::beginTicker(const string& ticker)
{
	const vector<double>* msecT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, msecName_));
	const vector<double>* volT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, volName_));
	const vector<double>* signT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, signName_));

	bool trade_ok = (0 != msecT && 0 != volT && 0 != signT && !msecT->empty() && !volT->empty() && !signT->empty());
	if( trade_ok )
	{
		boost::mutex::scoped_lock lock(ptrades_mutex_);
		//list<PastTrade>& ptrades = mTickerTrade_[ticker];
		list<PastTrade> ptrades;
		int NT = msecT->size();
		for( int i = 0; i < NT; ++i )
		{
			int sign = (*signT)[i];
			int msecs = (*msecT)[i];
			int qty = (*volT)[i];

			PastTrade newTrade(msecs, iday_, sign * qty);
			add_trade(ptrades, newTrade);
		}

		// Calculate returns and pnls.
		for( list<PastTrade>::iterator it = ptrades.begin(); it != ptrades.end(); ++it )
		{
			int msecs = it->msecs;
			int pos = it->pos;
			int sign = (pos > 0) ? 1 : -1;
			double age = (iday_ - it->iday) + ((double)closeMsecs_ - msecs) / (closeMsecs_ - openMsecs_);

			double closePrice = 0.;
			double nextOpenAdj = 0.;
			double nextCloseAdj = 0.;
			map<string, CloseInfo>::iterator itcn = mCloseNext_.find(ticker);
			if( itcn != mCloseNext_.end() )
			{
				CloseInfo& cl = itcn->second;
				closePrice = cl.close;
				nextOpenAdj = cl.nextOpen * cl.nextAdj;
				nextCloseAdj = cl.nextClose * cl.nextAdj;
			}

			if( closePrice > 0. && nextOpenAdj > 0. && nextCloseAdj > 0. )
			{
				double pclcl = pos * (nextCloseAdj - closePrice);
				double pclop = pos * (nextOpenAdj - closePrice);
				double popcl = pos * (nextCloseAdj - nextOpenAdj);

				double rclcl = sign * (nextCloseAdj / closePrice - 1.);
				double rclop = sign * (nextOpenAdj / closePrice - 1.);
				double ropcl = sign * (nextCloseAdj / nextOpenAdj - 1.);

				boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
				if( it == ptrades.begin() )
					hNTickers_.Fill(age, 1.);
				hNShares_.Fill(age, pos);
				hDollarvol_.Fill(age, pos * closePrice);
				hNSharesAbs_.Fill(age, abs(pos));
				hDollarvolAbs_.Fill(age, abs(pos * closePrice));
				hPnlSumClcl_.Fill(age, pclcl);
				hPnlSumClop_.Fill(age, pclop);
				hPnlSumOpcl_.Fill(age, popcl);
				pPnlClcl_.Fill(age, pclcl);
				pPnlClop_.Fill(age, pclop);
				pPnlOpcl_.Fill(age, popcl);
				pRtnClcl_.Fill(age, rclcl);
				pRtnClop_.Fill(age, rclop);
				pRtnOpcl_.Fill(age, ropcl);
			}
		}

	}
	return;
}

void HCloseToCloseAna::endTicker(const string& ticker)
{
	return;
}

void HCloseToCloseAna::endDay()
{
	++iday_;
	return;
}

void HCloseToCloseAna::endMarket()
{
	string market = HEnv::Instance()->market();
	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());

	hNTickers_.Write();
	hNShares_.Write();
	hDollarvol_.Write();
	hNSharesAbs_.Write();
	hDollarvolAbs_.Write();

	hPnlSumClcl_.Write();
	hPnlSumClop_.Write();
	hPnlSumOpcl_.Write();

	pPnlClcl_.Write();
	pPnlClop_.Write();
	pPnlOpcl_.Write();

	pRtnClcl_.Write();
	pRtnClop_.Write();
	pRtnOpcl_.Write();

	return;
}

void HCloseToCloseAna::endJob()
{
	return;
}

void HCloseToCloseAna::add_trade(list<PastTrade>& ptrades, PastTrade newTrade)
{
	if( ptrades.empty() ) // If empty, just add.
		ptrades.push_back(newTrade);
	else if( ptrades.front().pos * newTrade.pos > 0 ) // If same sign, just add new trade.
	{
		ptrades.push_back(newTrade);
	}
	else if( ptrades.front().pos * newTrade.pos < 0 ) // Not empty, opposite sign.
	{
		while( newTrade.pos != 0 )
		{
			if( ptrades.empty() ) // If empty, add and break.
			{
				ptrades.push_back(newTrade);
				newTrade.pos = 0;
				break;
			}
			
			// If new trade is equal or bigger, remove the front and repeat.
			else if( abs(ptrades.front().pos) <= abs(newTrade.pos) )
			{
				int front_size = ptrades.front().pos;
				ptrades.pop_front();
				newTrade.pos -= front_size;
			}

			// If new trade is smaller, reduce the position.
			else
			{
				ptrades.front().pos -= newTrade.pos;
				newTrade.pos = 0;
				break;
			}
		}
	}
}

void HCloseToCloseAna::readClosePricePosition(map<string, CloseInfo>& mClose, int idate)
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
		//GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vvcl);
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
		//GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vvclN);
		GODBC::Instance()->read(mto::hf(market), cmd, vvclN);
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

	//// Position on previous trading day.
	//if( "M" == id_ )
	//{
	//	vector<vector<string> > vvp;
	//	{
	//		string selMarket = "";
	//		if( mto::isInternational(market) )
	//			selMarket = " and exchange = " + quote(mto::code(market));
	//		string cmd = (string)" select symbol, pos "
	//			+ " from hfClosePosition "
	//			+ " where idate = " + itos(idate_prev)
	//			+ selMarket;
	//		GODBC::Instance()->get(mto::hfo(market, idate_prev))->ReadTable(cmd, &vvp);
	//	}
	//	for( vector<vector<string> >::iterator it = vvp.begin(); it != vvp.end(); ++it )
	//	{
	//		string temp_symbol = trim((*it)[0]);
	//		if( !temp_symbol.empty() )
	//		{
	//			string symbol = mto::compTicker(temp_symbol, market);
	//			int pos = atoi((*it)[1].c_str());
	//			map<string, CloseInfo>::iterator itm = mClose.find(symbol);
	//			if( itm != mClose.end() )
	//				itm->second.pos = pos;
	//		}
	//	}
	//}
	//else if( "" == id_ )
	//{
	//	const map<string, int>* mPos = static_cast<const map<string, int>*>(HEvent::Instance()->get("", posName_));
	//	for( map<string, int>::const_iterator it = mPos->begin(); it != mPos->end(); ++it )
	//	{
	//		string symbol = it->first;
	//		int pos = it->second;

	//		map<string, CloseInfo>::iterator itm = mClose.find(symbol);
	//		if( itm != mClose.end() )
	//			itm->second.pos = pos;
	//	}
	//}

	return;
}
