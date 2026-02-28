#include <MOrders/MTradeAna.h>
#include "optionlibs/TickData.h"
#include <MFramework/MEvent.h>
#include <MFramework/MEnv.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GEX.h>
#include <jl_lib/GFee.h>
#include <jl_lib/jlutil.h>
#include <gtlib/util.h>
#include <jl_lib/mto.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <numeric>
#include <string>
using namespace std;
using namespace gtlib;

MTradeAna::MTradeAna(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName, true),
	debug_(false),
	verbose_(0),
	execType_({'L'}),
	schedType_({1, 2, 4}),
	sign_({-1, 1}),
	fee_bpt_(0.),
	iQuantile_(-1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("execType") )
		execType_ = getConfigValuesChar(conf, "execType");
	if( conf.count("schedType") )
		schedType_ = getConfigValuesInt(conf, "schedType");
	if( conf.count("sign") )
		sign_ = getConfigValuesInt(conf, "sign");

	if( conf.count("condVar") && conf.count("iQuantile") )
	{
		condVar_ = conf.find("condVar")->second;
		iQuantile_ = atoi(conf.find("iQuantile")->second.c_str());
	}
}

MTradeAna::~MTradeAna()
{}

void MTradeAna::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	return;
}

void MTradeAna::beginMarket()
{
	string market = MEnv::Instance()->market;
	vector<int> idates = MEnv::Instance()->idates;
	fee_bpt_ = mto::fee_bpt(market);
	mPs_.clear();
	ps_ = PnlSummary();

	return;
}

void MTradeAna::beginDay()
{
	psumDay_ = PnlSum();

	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	int idate_next = nextClose(market, idate);

	GCurr::Instance()->set_idate(idate);
	readClosePricePosition(mClose_, idate);
	readClosePricePosition(mCloseNext_, idate_next);

	GODBC::Instance()->close_all();

	return;
}

void MTradeAna::beginMarketDay()
{
	tqi_ = static_cast<const TradeQuantileInfo*>(MEvent::Instance()->get("", "quantileInfo"));
}

void MTradeAna::beginTicker(const string& ticker)
{
	const vector<MercatorTrade>* pvmt = static_cast<const vector<MercatorTrade>*>(MEvent::Instance()->get(ticker, "mtrades"));
	intra_day_stat(pvmt, ticker);
	return;
}

void MTradeAna::endTicker(const string& ticker)
{
	return;
}

void MTradeAna::endDay()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	double exchrat = GCurr::Instance()->exchrat("US", market);
	psumDay_.adjust_exchrat(exchrat);

	double bpsIntra = (psumDay_.dv > 0)?(psumDay_.pnlIntra / psumDay_.dv * basis_pts_):0;
	double bpsNofillIntra = (psumDay_.dvNofill > 0)?(psumDay_.pnlNofillIntra / psumDay_.dvNofill * basis_pts_):0;
	double bpsClcl = (psumDay_.dv > 0)?(psumDay_.pnlClcl / psumDay_.dv * basis_pts_):0;
	double bpsTotal1 = (psumDay_.dv > 0)?((psumDay_.pnlIntra + psumDay_.pnlClcl) / psumDay_.dv * basis_pts_):0;
	double bpsInter = (psumDay_.dv > 0)?(psumDay_.pnlInter / psumDay_.dv * basis_pts_):0;
	double bpsHori = (psumDay_.dv > 0)?(psumDay_.pnlHori / psumDay_.dv * basis_pts_):0;
	double bpsNofillHori = (psumDay_.dvNofill > 0)?(psumDay_.pnlNofillHori / psumDay_.dvNofill * basis_pts_):0;
	double bpsClop = (psumDay_.dv > 0)?(psumDay_.pnlClop / psumDay_.dv * basis_pts_):0;
	double bpsTotal2 = (psumDay_.dv > 0)?((psumDay_.pnlInter + psumDay_.pnlClop) / psumDay_.dv * basis_pts_):0;
	double ratBuy = (psumDay_.nTrades > 0)?(psumDay_.nTradesBuy / psumDay_.nTrades):0;
	double fillRat = psumDay_.nTrades / (psumDay_.nTrades + psumDay_.nTradesNofill) * 100.;
	if(verbose_ > 0)
	{
		boost::mutex::scoped_lock lock(MEnv::Instance()->io_mutex);
		printf(" %s nTickers %d nTrd %d (Buy %4.1f %%) nShr %d dv %.1f fill %.1f%%\n",
				moduleName_.c_str(), (int)psumDay_.nTickers, (int)psumDay_.nTrades, ratBuy*100.0,
				(int)psumDay_.nShares, psumDay_.dv, fillRat);
		printf(" pH %9.1f (%5.1f) pNH %8.1f (%5.1f) pCN %8.1f pCC %8.1f (%5.1f) pT  %8.1f (%5.1f)\n",
				psumDay_.pnlHori, bpsHori, // pH
				psumDay_.pnlNofillHori, bpsNofillHori, // pNH
				psumDay_.pnlClclNext, // pCN
				psumDay_.pnlClcl, bpsClcl, // pCc
				psumDay_.pnlIntra + psumDay_.pnlClcl, bpsTotal1); // pT

		printf(" pI %9.1f (%5.1f) pNI %8.1f (%5.1f) pTN %8.1f pCo %8.1f (%5.1f) pT' %8.1f (%5.1f)\n",
				psumDay_.pnlIntra, bpsIntra, // pI
				psumDay_.pnlNofillIntra, bpsNofillIntra, // pNI
				psumDay_.pnlIntra + psumDay_.pnlClclNext, // pTN
				psumDay_.pnlClop, bpsClop, // pCo
				psumDay_.pnlInter + psumDay_.pnlClop, bpsTotal2); // pT'
		cout << flush;
	}

	update_summary(ps_, psumDay_);
	if(verbose_ == 4)
		printf("sum.pnlHori %.1f summary.accPnlHori %.1f\n", psumDay_.pnlHori, ps_.accPnlHori.sum);
	return;
}

void MTradeAna::endMarket()
{
	// End of market summary.
	// DV: dollar volume
	// NT: number of trades
	// pI: intra day pnl of traded shares, to the close
	// pI': intra day pnl of whole balance, to the close
	// pCc: Close(prev) to close(today) of actual balance from hfClosePosition
	// pCD: Close(today) to close(next) of traded shares
	// pCN: 
	// pCo: Close(prev) to close(today) of actual balance from hfClosePosition
	string market = MEnv::Instance()->market;
	cout << "\nendMarket " << market << ' ' << moduleName_ << " USD/day\n";
	printf(" DV:    %8.1f +- %8.1f     DVNofill:    %8.1f +- %8.1f\n",
			ps_.accDv.mean(), ps_.accDv.stdev(),
			ps_.accDvNofill.mean(), ps_.accDvNofill.stdev());
	printf(" NTrd:  %8.1f +- %8.1f     NTrdNofill:  %8.1f +- %8.1f\n",
			ps_.accNTrades.mean(), ps_.accNTrades.stdev(),
			ps_.accNTradesNofill.mean(), ps_.accNTradesNofill.stdev());

	printf(" pH  %8.1f +- %8.1f (%5.1f)  pNH %8.1f +- %8.1f (%5.1f)\n",
			ps_.accPnlHori.mean(),
			ps_.accPnlHori.stdev()/sqrt(ps_.accPnlHori.n),
			ps_.bpsHori,
			ps_.accPnlNofillHori.mean(),
			ps_.accPnlNofillHori.stdev()/sqrt(ps_.accPnlNofillHori.n),
			ps_.bpsNofillHori);
	printf(" pI  %8.1f +- %8.1f (%5.1f)  pNI %8.1f +- %8.1f (%5.1f)\n",
			ps_.accPnlIntra.mean(),
			ps_.accPnlIntra.stdev()/sqrt(ps_.accPnlIntra.n),
			ps_.bpsIntra,
			ps_.accPnlNofillIntra.mean(),
			ps_.accPnlNofillIntra.stdev()/sqrt(ps_.accPnlNofillIntra.n),
			ps_.bpsNofillIntra);
	printf(" pCN %8.1f +- %8.1f (%5.1f)  pTN %8.1f\n",
			ps_.accPnlClclNext.mean(),
			ps_.accPnlClclNext.stdev()/sqrt(ps_.accPnlClclNext.n),
			ps_.bpsClclNext,
			ps_.accPnlIntra.mean() + ps_.accPnlClclNext.mean());

	printf(" pCc %8.1f +- %8.1f (%5.1f)  pT  %8.1f +- %8.1f (%5.1f)\n",
			ps_.accPnlClcl.mean(),
			ps_.accPnlClcl.stdev()/sqrt(ps_.accPnlClcl.n),
			ps_.bpsClcl,
			ps_.accPnlTotal1.mean(),
			ps_.accPnlTotal1.stdev()/sqrt(ps_.accPnlTotal1.n),
			ps_.bpsTotal1);

	printf(" pCo %8.1f +- %8.1f (%5.1f)  pT' %8.1f +- %8.1f (%5.1f)\n",
			ps_.accPnlClop.mean(),
			ps_.accPnlClop.stdev()/sqrt(ps_.accPnlClop.n),
			ps_.bpsClop,
			ps_.accPnlTotal2.mean(),
			ps_.accPnlTotal2.stdev()/sqrt(ps_.accPnlTotal2.n),
			ps_.bpsTotal2);

	cout << flush;
	return;
}

void MTradeAna::endJob()
{
	return;
}

void MTradeAna::readClosePricePosition(map<string, CloseInfo>& mClose, int idate)
{
	mClose.clear();
	string market = MEnv::Instance()->market;
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
			if(nextOpen < ltmb_ & nextClose > ltmb_)
				nextOpen = nextClose;
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

	return;
}

void MTradeAna::intra_day_stat(const vector<MercatorTrade>* pvmt, const string& ticker)
{
	string& market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
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

	int daypos = 0;
	double lastMidPrice = openPrice;
	vector<int> tradeIndex = selectTradeIndex(pvmt);
	double lastPos = lastClosePosition;
	for(int ti = 0; ti < tradeIndex.size(); ++ti)
	{
		int i = tradeIndex[ti];
		const MercatorTrade& mt = (*pvmt)[i];
		if(mt.qty > 0)
		{
			double fee = GFee::Instance().feeBpt(market, ExecFeesPrimex(market, ticker), mt.price) / basis_pts_ * mt.price * mt.qty;

			// PNL inter sample.
			double pnlInter0 = ( mt.mid - lastMidPrice ) * lastPos;
			double cost = mt.sign * ( mt.mid - mt.price ) * mt.qty;
			double pnlInter = pnlInter0 + cost - fee;

			double pnlHori = mt.gain61 / basis_pts_ * mt.price * mt.qty - fee;
			double pnlIntra = mt.sign * (closePrice - mt.price) * mt.qty - fee;
			double pnlClclNext = mt.sign * retClclNext * closePrice * mt.qty;

			// Update the sum for the tickerday.
			psum.pnlHori += pnlHori;
			psum.pnlIntra += pnlIntra;
			psum.pnlInter += pnlInter;
			++psum.nTrades;
			if( ceil(mt.sign - 0.5) > 0 )
				++psum.nTradesBuy;
			else if( ceil(mt.sign - 0.5) < 0 )
				++psum.nTradesSell;
			psum.nShares += mt.qty;
			psum.dv += mt.qty * mt.price;
			psum.pnlClclNext += pnlClclNext;

			daypos += mt.sign * mt.qty;
			lastPos += mt.sign * mt.qty;
			lastMidPrice = mt.mid;
			if(verbose_ == 5)
			{
				printf("%s %d gain61 %.1f plH %.1f plI %.1f mid %.1f prc %.1f\n",
						ticker.c_str(), mt.msecs, mt.gain61, pnlHori, pnlIntra, mt.mid, mt.price);
			}
		}
		else
		{
			++psum.nTradesNofill;
			double feeNofill = fee_bpt_ / basis_pts_ * mt.price * mt.oqty;
			double pnlNofillHori = mt.gain61 / basis_pts_ * mt.price * mt.oqty - feeNofill;
			double pnlNofillIntra = mt.sign * (closePrice - mt.price) * mt.oqty - feeNofill;
			psum.pnlNofillHori += pnlNofillHori;
			psum.pnlNofillIntra += pnlNofillIntra;
			psum.dvNofill += mt.oqty * mt.price;
			if(verbose_ == 3)
			{
				printf("%s %d gain61 %.1f plH %.1f plI %.1f\n",
						ticker.c_str(), mt.msecs, mt.gain61, pnlNofillHori, pnlNofillIntra);
			}
		}
	}

	// Add the pnl from the last trade to the end of the day.
	double pnlTrcl = (closePrice - lastMidPrice) * lastPos;
	psum.pnlInter += pnlTrcl;

	// Pnl close to next close, using the single day sum of position change.
	if( closePrice > ltmb_ && nextCloseAdj > ltmb_ )
		double pnlClcl = daypos * (nextCloseAdj - closePrice);

	// Update histograms with the tickerday.
	double ratBuy = 0;
	if( psum.nTrades > 0 )
		ratBuy = psum.nTradesBuy / psum.nTrades;

	// Update the summ for the day.
	{
		boost::mutex::scoped_lock lock(ps_mutex_);
		if( psum.nTrades > 0 )
			++psumDay_.nTickers;
		psumDay_.pnlHori += psum.pnlHori;
		psumDay_.pnlIntra += psum.pnlIntra;
		psumDay_.pnlClcl += psum.pnlClcl;
		psumDay_.pnlClclNext += psum.pnlClclNext;
		psumDay_.pnlNofillHori += psum.pnlNofillHori;
		psumDay_.pnlNofillIntra += psum.pnlNofillIntra;
		psumDay_.pnlOpcl += psum.pnlOpcl;
		psumDay_.pnlClopNext += psum.pnlClopNext;
		psumDay_.pnlInter += psum.pnlInter;
		psumDay_.pnlClop += psum.pnlClop;
		psumDay_.dv += psum.dv;
		psumDay_.nTrades += psum.nTrades;
		psumDay_.nTradesBuy += psum.nTradesBuy;
		psumDay_.nTradesSell += psum.nTradesSell;
		psumDay_.nShares += psum.nShares;
		psumDay_.dvNofill += psum.dvNofill;
		psumDay_.nTradesNofill += psum.nTradesNofill;

		PnlSummary& ps = mPs_[ticker];
		update_summary(ps, psum);
	}

	if(verbose_ == 2)
	{
		printf("%s %8.2f %8.2f s:%8.2f %8.2f %8.2f s':%8.2f D:%8.2f\n", ticker.c_str(),
				psum.pnlIntra, psum.pnlClcl, psum.pnlIntra + psum.pnlClcl,
				psum.pnlInter, psum.pnlClop, psum.pnlInter + psum.pnlClop,
				psum.pnlInter + psum.pnlClop - (psum.pnlIntra + psum.pnlClcl));
	}

	return;
}

vector<int> MTradeAna::selectTradeIndex(const vector<MercatorTrade>* pvmt)
{
	vector<int> tradeIndex;
	int NT = pvmt->size();
	for(int i = 0; i < NT; ++i)
	{
		const MercatorTrade& mt = (*pvmt)[i];

		bool okExec = false;
		bool okSign = false;
		bool okSched = false;
		if(find(begin(execType_), end(execType_), mt.execType) != end(execType_))
			okExec = true;
		if(find(begin(sign_), end(sign_), mt.sign) != end(sign_))
			okSign = true;
		if(find(begin(schedType_), end(schedType_), mt.schedType) != end(schedType_))
			okSched = true;

		if(okExec && okSign && okSched)
		{
			bool okQuantile = true;
			if(!condVar_.empty() && iQuantile_ > 0)
				okQuantile = tqi_->getQuantile(mt, condVar_) == iQuantile_;
			if(okQuantile)
				tradeIndex.push_back(i);
		}
	}
	return tradeIndex;
}

void MTradeAna::update_summary(PnlSummary& summary, PnlSum& sum)
{
	summary.accPnlHori.add(sum.pnlHori);
	summary.accPnlIntra.add(sum.pnlIntra);
	summary.accPnlNofillHori.add(sum.pnlNofillHori);
	summary.accPnlNofillIntra.add(sum.pnlNofillIntra);
	summary.accPnlClcl.add(sum.pnlClcl);
	summary.accPnlClclNext.add(sum.pnlClclNext);
	summary.accPnlTotal1.add(sum.pnlIntra + sum.pnlClcl);
	summary.accPnlInter.add(sum.pnlInter);
	summary.accPnlClop.add(sum.pnlClop);
	summary.accPnlTotal2.add((sum.dv > 0.)?(sum.pnlInter + sum.pnlClop):0);
	summary.accDv.add(sum.dv);
	summary.accNTrades.add(sum.nTrades);
	summary.accDvNofill.add(sum.dvNofill);
	summary.accNTradesNofill.add(sum.nTradesNofill);

	summary.bpsHori = summary.accPnlHori.sum / summary.accDv.sum * basis_pts_;
	summary.bpsIntra = summary.accPnlIntra.sum / summary.accDv.sum * basis_pts_;
	summary.bpsNofillHori = summary.accPnlNofillHori.sum / summary.accDvNofill.sum * basis_pts_;
	summary.bpsNofillIntra = summary.accPnlNofillIntra.sum / summary.accDvNofill.sum * basis_pts_;
	summary.bpsClcl = summary.accPnlClcl.sum / summary.accDv.sum * basis_pts_;
	summary.bpsClclNext = summary.accPnlClclNext.sum / summary.accDv.sum * basis_pts_;
	summary.bpsTotal1 = summary.accPnlTotal1.sum / summary.accDv.sum * basis_pts_;
	summary.bpsInter = summary.accPnlInter.sum / summary.accDv.sum * basis_pts_;
	summary.bpsClop = summary.accPnlClop.sum / summary.accDv.sum * basis_pts_;
	summary.bpsTotal2 = summary.accPnlTotal2.sum / summary.accDv.sum * basis_pts_;
	return;
}

