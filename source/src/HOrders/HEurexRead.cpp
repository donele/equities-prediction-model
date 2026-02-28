#include <HOrders.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <optionlibs/TickData.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/GODBC.h>
#include <map>
#include <string>
#include <TFile.h>
using namespace std;

HEurexRead::HEurexRead(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
verbose_(0),
n_pnl_bins_(100),
half_pnl_bins_(n_pnl_bins_ / 2),
maxQty_(50),
max_o2q_(50),
min_t2e_(5),
nTot_(0),
nTradeTickMatch_(0),
nTradetickMatchUniq_(0),
nAddTradeTickMatch_(0),
nAddTradetickMatchUniq_(0),
db_name_("futures_eu"),
tickSource_("quote")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("tickSource") )
		tickSource_ = conf.find("tickSource")->second;
}

HEurexRead::~HEurexRead()
{}

void HEurexRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	TFile* f = HEnv::Instance()->outfile();
	f->mkdir(moduleName_.c_str());
}

void HEurexRead::beginMarket()
{
	string market = HEnv::Instance()->market();
	string region = mto::region(market);

	v_rpnl_o_ms_ = vector<double>(n_pnl_bins_, 0.);
	v_rpnl_s_ = vector<double>(n_pnl_bins_, 0.);
	v_rpnl_ms_ = vector<double>(n_pnl_bins_, 0.);

	char name[100];
	char title[200];

	sprintf(name, "hQty");
	sprintf(title, "Quantity");
	hQty_ = TH1F(name, title, maxQty_, 0, maxQty_ );

	sprintf(name, "h_o2e");
	sprintf(title, "o2e");
	h_o2e_ = TH1F(name, title, max_o2q_, 0, max_o2q_);

	sprintf(name, "h_o2t");
	sprintf(title, "o2t");
	h_o2t_ = TH1F(name, title, 25, -50, 100);

	sprintf(name, "h_o2t_best");
	sprintf(title, "o2t best");
	h_o2t_best_ = TH1F(name, title, 25, -50, 100);

	sprintf(name, "h_o2t_uniq");
	sprintf(title, "o2t Unique");
	h_o2t_uniq_ = TH1F(name, title, 25, -50, 100);

	sprintf(name, "h_o2e_o2t");
	sprintf(title, "o2t vs. o2e");
	h_o2e_o2t_ = TH2F(name, title, max_o2q_, 0, max_o2q_, max_o2q_, 0, max_o2q_ );

	sprintf(name, "hCand");
	sprintf(title, "Number of Candidate trades (market taking)");
	hCand_ = TH1F(name, title, 40, 0, 40);

	sprintf(name, "hCandA");
	sprintf(title, "Number of Candidate trades (market making)");
	hCandA_ = TH1F(name, title, 40, 0, 40);

	sprintf(name, "hCandDrift");
	sprintf(title, "Number of candidates vs. Max drift");
	hCandDrift_ = TProfile(name, title, max_o2q_, 0, max_o2q_);

	sprintf(name, "hCandQty");
	sprintf(title, "Number of candidates vs. Qty");
	hCandQty_ = TProfile(name, title, maxQty_, 0, maxQty_);

	sprintf(name, "hCand2d");
	sprintf(title, "Number of candidates vs. Max drift and Qty");
	hCand2d_ = TProfile2D(name, title, max_o2q_, 0, max_o2q_, maxQty_, 0, maxQty_ );

	sprintf(name, "h_t2e");
	sprintf(title, "t2e");
	h_t2e_ = TH1F(name, title, 22, -10, 100);

	sprintf(name, "h_t2e_best");
	sprintf(title, "t2e best");
	h_t2e_best_ = TH1F(name, title, 22, -10, 100);

	sprintf(name, "h_t2e_uniq");
	sprintf(title, "t2e Unique");
	h_t2e_uniq_ = TH1F(name, title, 22, -10, 100);

	sprintf(name, "h_rpnl_o_ms");
	sprintf(title, "Pnl (wrt. Market Making Order) vs. latency");
	h_rpnl_o_ms_ = TH1F(name, title, n_pnl_bins_, -half_pnl_bins_, half_pnl_bins_);

	sprintf(name, "h_rpnl_s");
	sprintf(title, "RPnl vs. latency");
	h_rpnl_s_ = TH1F(name, title, n_pnl_bins_, -half_pnl_bins_, half_pnl_bins_);

	sprintf(name, "h_rpnl_ms");
	sprintf(title, "RPnl vs. latency");
	h_rpnl_ms_ = TH1F(name, title, n_pnl_bins_, -half_pnl_bins_, half_pnl_bins_);

	sprintf(name, "p_rpnl");
	sprintf(title, "RPnl vs t2e");
	p_rpnl_ = TProfile(name, title, 10, 0, 80);

	sprintf(name, "h_dTicksize");
	sprintf(title, "#Delta ticksize");
	h_dTicksize_ = TH1F(name, title, 12, -3, 3);

	sprintf(name, "h_rpnl_dTicksize");
	sprintf(title, "Relative Pnl vs. #Delta ticksize");
	h_rpnl_dTicksize_ = TH1F(name, title, 12, -3, 3);

	sprintf(name, "p_dTicksize_t2e");
	sprintf(title, "Average #Delta ticksize vs t2e");
	p_dTicksize_t2e_ = TProfile(name, title, 10, 0, 80);

	sprintf(name, "h_nLooser_t2e");
	sprintf(title, "#Loosing trades vs t2e");
	h_nLooser_t2e_ = TH1F(name, title, 10, 0, 80);
}

void HEurexRead::beginDay()
{
}

void HEurexRead::beginMarketDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	string region = mto::region(market);

	set_ticker_list(idate, market);
	read_orders(idate, market);
	read_all_tickdata(idate);
}

void HEurexRead::beginTicker(const string& ticker)
{
	if( binary_search(tickers_.begin(), tickers_.end(), ticker) )
	{
		int idate = HEnv::Instance()->idate();
		string market = HEnv::Instance()->market();
		char exch = market[1];

		vector<EurexOrder> vMOrderBuy;
		vector<EurexOrder> vMOrderSell;
		read_orders(vMOrderBuy, vMOrderSell, market, ticker, idate);
		if( !vMOrderBuy.empty() || !vMOrderSell.empty() )
		{
			read_quote_info( vMOrderBuy, market, ticker, "buy" );
			read_quote_info( vMOrderSell, market, ticker, "sell" );

			HEvent::Instance()->add<vector<EurexOrder> >(ticker, (string)"vMOrderBuy_", vMOrderBuy);
			HEvent::Instance()->add<vector<EurexOrder> >(ticker, (string)"vMOrderSell_", vMOrderSell);
		}
	}
}

void HEurexRead::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<EurexOrder> >(ticker, (string)"vMOrderBuy_");
	HEvent::Instance()->remove<vector<EurexOrder> >(ticker, (string)"vMOrderSell_");
}

void HEurexRead::endDay()
{
	int idate = HEnv::Instance()->idate();
	vvoL_.clear();
	vvoA_.clear();
}

void HEurexRead::endMarket()
{
	string market = HEnv::Instance()->market();
	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());

	hCandDrift_.Write();
	hCandQty_.Write();
	hCand2d_.Write();
	hQty_.Write();
	h_o2e_.Write();
	h_o2t_best_.Write();
	h_o2t_uniq_.Write();
	h_o2t_.Write();
	h_t2e_.Write();
	h_t2e_best_.Write();
	h_t2e_uniq_.Write();
	hCand_.Write();
	hCandA_.Write();
	p_rpnl_.Write();
	h_dTicksize_.Write();
	h_rpnl_dTicksize_.Write();
	p_dTicksize_t2e_.Write();
	h_nLooser_t2e_.Write();

	int nd = HEnv::Instance()->idates().size();

	double pnl_scale = 1000. / nd;
	for( int i = 0; i < n_pnl_bins_; ++i )
		h_rpnl_o_ms_.SetBinContent(i + 1, v_rpnl_o_ms_[i] * pnl_scale);
	h_rpnl_o_ms_.Write();

	for( int i = 0; i < n_pnl_bins_; ++i )
		h_rpnl_s_.SetBinContent(i + 1, v_rpnl_s_[i] * pnl_scale);
	h_rpnl_s_.Write();

	for( int i = 0; i < n_pnl_bins_; ++i )
		h_rpnl_ms_.SetBinContent(i + 1, v_rpnl_ms_[i] * pnl_scale);
	h_rpnl_ms_.Write();
}

void HEurexRead::endJob()
{
}

void HEurexRead::read_all_tickdata(int idate)
{
	string market = HEnv::Instance()->market();
	mTickerVtrade_.clear();
	mTickerVquote_.clear();

	string quote_dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\eurex\\binLogL2\\");
	vector<string> dirs(1, quote_dir);

	for( vector<string>::iterator it = tickers_.begin(); it != tickers_.end(); ++it )
	{
		string ticker = *it;

		vector<TradeInfo> vt;
		read_tickdata<TradeInfo>(vt, market, idate, ticker, dirs);
		mTickerVtrade_[ticker] = vt;

		vector<QuoteInfo> vq;
		if( 0 )
		{
			TickAccessMulti<OrderData> taO;
			TickSeries<OrderData> tsO;
			taO.AddRoot(quote_dir, true);
			taO.GetTickSeries(ticker, idate, &tsO);
			TickSeries<QuoteInfo> tsQ;
			order2quote(tsO, tsQ, 1, false, false, 'R');
			int ntq = tsQ.NTicks();
			QuoteInfo quote;
			tsQ.StartRead();;
			for(int n = 0; n < ntq; ++n )
			{
				tsQ.Read(&quote);
				vq.push_back(quote);
			}
		}
		else
			read_tickdata<QuoteInfo>(vq, market, idate, ticker, dirs);

		mTickerVquote_[ticker] = vq;
	}
}

void HEurexRead::set_ticker_list(int idate, string market)
{
	string dbSymbol = "symbol";

	string cmd = (string)" select distinct symbol "
		+ " from hfOrders o "
		+ " where idate = " + itos(idate);
	vector<vector<string> > vvs;
	GODBC::Instance()->get(db_name_)->ReadTable(cmd, &vvs);

	tickers_.clear();
	for( vector<vector<string> >::iterator it = vvs.begin(); it != vvs.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
			tickers_.push_back(symbol);
	}
	sort(tickers_.begin(), tickers_.end());
	HEnv::Instance()->tickers(tickers_);
}

void HEurexRead::read_orders(int idate, string market)
{
	string cmdL = (string)" select o.symbol, o.side, o.price, o.qty, o.orderMsecs, e.eventMsecs, "
		+ " o.bidsize, o.bid, o.ask, o.asksize "
		+ " from hfOrders o inner join hfOrderEvents e "
		+ " on o.orderID = e.orderID "
		+ " where o.idate = " + itos(idate)
		+ " and e.idate = " + itos(idate)
		+ " and e.eventType = 2 " // 0: reject 1: cancel 2: fill.
		+ " and o.execType = 'L' and o.qty = e.qty order by o.orderMsecs ";
	GODBC::Instance()->get(db_name_)->ReadTable(cmdL, &vvoL_);

	string cmdA = (string)" select o.symbol, o.side, o.price, o.qty, o.orderMsecs, e.eventMsecs, "
		+ " o.bidsize, o.bid, o.ask, o.asksize "
		+ " from hfOrders o inner join hfOrderEvents e "
		+ " on o.orderID = e.orderID "
		+ " where o.idate = " + itos(idate)
		+ " and e.idate = " + itos(idate)
		+ " and e.eventType = 2 " // 0: reject 1: cancel 2: fill.
		+ " and o.execType = 'A' and o.qty = e.qty order by e.eventMsecs ";
	GODBC::Instance()->get(db_name_)->ReadTable(cmdA, &vvoA_);
}

void HEurexRead::read_orders(vector<EurexOrder>& vMOrderBuy, vector<EurexOrder>& vMOrderSell,
							string market, string ticker, int idate)
{
	EurexOrder prevOrder;
	for( vector<vector<string> >::iterator it = vvoL_.begin(); it != vvoL_.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( symbol == ticker )
		{
			int sign = 0;
			string side = trim((*it)[1]);
			if( !side.empty() )
			{
				if( side[0] == 'B' )
					sign = 1;
				else if( side[0] == 'S' )
					sign = -1;
			}

			double price = atof((*it)[2].c_str());
			int qty = atoi((*it)[3].c_str());
			int orderMsecs = atoi((*it)[4].c_str());
			int eventMsecs = atoi((*it)[5].c_str());

			EurexOrder newOrder(price, qty, side[0], orderMsecs, eventMsecs);
			if( sign == 1 )
				vMOrderBuy.push_back(newOrder);
			else if( sign == -1 )
				vMOrderSell.push_back(newOrder);
		}
	}
}

void HEurexRead::read_quote_info( vector<EurexOrder>& vMOrder, string market, string ticker, string bs)
{
	int idate = HEnv::Instance()->idate();
	double ts = 0;
	if( ticker.find("BS") != string::npos )
		ts = 0.005;
	else if( ticker.find("BM") != string::npos )
		ts = 0.01;
	else if( ticker.find("BL") != string::npos )
		ts = 0.01;
	else if( ticker.find("BX") != string::npos )
		ts = 0.02;

	QuoteInfo quote;
	for( vector<EurexOrder>::iterator itb = vMOrder.begin(); itb != vMOrder.end(); ++itb )
	{
		// Find matching market-making ('A') trade.
		int orderMsecs = itb->orderMsecs;
		vector<MMtrade> vMMtrades = get_matching_MMtrades(ticker, orderMsecs);

		if( vMMtrades.size() == 1 )
		{
			++nTot_;
			hQty_.Fill(itb->qty);
			int o2e = itb->eventMsecs - itb->orderMsecs;
			h_o2e_.Fill(o2e);

			MMtrade& mmt = vMMtrades[0];
			itb->tickerA = mmt.ticker;
			itb->priceA = mmt.price;
			itb->qtyA = mmt.qty;
			itb->orderMsecsA = mmt.orderMsecs;
			itb->eventMsecsA = mmt.eventMsecs;

			QuoteInfo quoteA;
			quoteA.bidSize = mmt.bidSize;
			quoteA.bid = mmt.bid;
			quoteA.ask = mmt.ask;
			quoteA.askSize = mmt.askSize;

			// Find matching trade tickdata for 'L' trades.
			vector<TradeInfo> vTrade = get_matching_trade(ticker, itb->orderMsecs, itb->eventMsecs, itb->price, itb->qty);
			int nT = vTrade.size();
			if( nT == 1 )
				itb->tradeMsecs = vTrade[0].msecs;
			else
			{
				int min_o2t = 10000;
				int orderPlus15 = itb->orderMsecs + 15;
				for( vector<TradeInfo>::iterator it = vTrade.begin(); it != vTrade.end(); ++it )
				{
					if( it->msecs >= orderPlus15 )
					{
						int o2t = it->msecs - itb->orderMsecs;
						if( o2t < min_o2t )
							itb->tradeMsecs = it->msecs;
					}
				}
			}

			// Find matching trade tickdata for 'A' trades.
			vector<TradeInfo> vTradeA = get_matching_trade_A(itb->tickerA, itb->orderMsecsA, itb->eventMsecsA, itb->priceA, itb->qtyA);
			int nTA = vTradeA.size();
			if( nTA == 1 )
				itb->tradeMsecsA = vTradeA[0].msecs;
			else
			{
				int eventMinus5 = itb->eventMsecsA - 5;
				for( vector<TradeInfo>::iterator it = vTradeA.begin(); it != vTradeA.end(); ++it )
				{
					if( it->msecs <= eventMinus5 )
					{
						int t2e = itb->eventMsecsA - it->msecs;
						itb->tradeMsecsA = it->msecs;
					}
					else
						break;
				}
			}

			// Debug
			if( verbose_ == 1 )
			{
				if( nT == 0 )
					printf("L %s %d %d %.3f %d\n", ticker.c_str(), idate, orderMsecs, itb->price, itb->qty);
				if( nTA == 0 )
					printf("A %s %d %d %.3f %d\n", itb->tickerA.c_str(), idate, itb->eventMsecsA, itb->priceA, itb->qtyA);
			}

			// Stats and hists.
			hCand_.Fill(nT);
			hCandA_.Fill(nTA);
			if( nTA > 0 )
			{
				++nAddTradeTickMatch_;
				if( nTA == 1 )
					++nAddTradetickMatchUniq_;

				h_t2e_best_.Fill(itb->eventMsecsA - itb->tradeMsecsA);
				if( nTA == 1 )
					h_t2e_uniq_.Fill(itb->eventMsecsA - itb->tradeMsecsA);
				for( vector<TradeInfo>::iterator it = vTradeA.begin(); it != vTradeA.end(); ++it )
				{
					int t2e = itb->eventMsecsA - it->msecs;
					h_t2e_.Fill(t2e);
				}
			}
			if( nT > 0 )
			{
				++nTradeTickMatch_;
				if( nT == 1 )
					++nTradetickMatchUniq_;

				h_o2t_best_.Fill(itb->tradeMsecs - itb->orderMsecs);
				if( nT == 1 )
					h_o2t_uniq_.Fill(itb->tradeMsecs - itb->orderMsecs);
				for( vector<TradeInfo>::iterator itt = vTrade.begin(); itt != vTrade.end(); ++itt )
				{
					int o2t = itt->msecs - itb->orderMsecs;
					h_o2t_.Fill(o2t);
					h_o2e_o2t_.Fill(o2e, o2t);
				}

				for( int md = 0; md < max_o2q_; ++md )
				{
					int cnt = 0;
					for( vector<TradeInfo>::iterator itt = vTrade.begin(); itt != vTrade.end(); ++itt )
					{
						int dt = itt->msecs - itb->orderMsecs;
						if( dt <= md )
							++cnt;
					}
					hCandDrift_.Fill(md, cnt);
					hCand2d_.Fill(md, itb->qty, cnt);
				}

				hCandQty_.Fill(itb->qty, nT);
			}

			// Relative Pnl.
			{
				QuoteInfo quoteRef = get_quote(ticker, itb->tradeMsecsA);
				double dPrice = (itb->side == 'B') ? (quoteRef.ask - itb->price) : (itb->price - quoteRef.bid);
				double dPriceCorr = ceil(dPrice * 1000 - 0.5) / 1000.;

				int t2e = itb->eventMsecsA - itb->tradeMsecsA;
				itb->rpnl = itb->qty * dPriceCorr;
				p_rpnl_.Fill(t2e, itb->rpnl);

				double dTicksize = dPriceCorr / ts;
				h_dTicksize_.Fill(dTicksize);
				h_rpnl_dTicksize_.Fill(dTicksize, itb->rpnl);
				p_dTicksize_t2e_.Fill(t2e, dTicksize);
				if( dPriceCorr < 0. )
					h_nLooser_t2e_.Fill(t2e);

				for( int i = 0; i < n_pnl_bins_; ++i )
				{
					int dt = i - half_pnl_bins_;
					double rpnl = get_rpnl(max(itb->tradeMsecsA + dt, itb->orderMsecsA), ticker, quoteRef, itb->side, itb->qty);
					v_rpnl_ms_[i] += rpnl;
					double rpnl_sec = get_rpnl(max(itb->tradeMsecsA + dt * 1000, itb->orderMsecsA), ticker, quoteRef, itb->side, itb->qty);
					v_rpnl_s_[i] += rpnl_sec;
				}

				QuoteInfo quoteRefOrder = get_quote(ticker, itb->orderMsecsA);
				for( int i = 0; i < n_pnl_bins_; ++i )
				{
					int dt = i - half_pnl_bins_;
					double rpnl_o = get_rpnl(max(itb->tradeMsecsA + dt, itb->orderMsecsA), ticker, quoteRefOrder, itb->side, itb->qty);
					v_rpnl_o_ms_[i] += rpnl_o;
				}

				if( verbose_ == 2 )
				{
					printf("%d %s %d %2f %.4f\n", idate, ticker.c_str(), itb->orderMsecs, dTicksize, itb->rpnl);
				}
			}
		}
	}
}

double HEurexRead::get_rpnl(int msecs, string ticker, QuoteInfo quote0, char side, int qty)
{
	QuoteInfo quote = get_quote(ticker, msecs);
	double dPrice = (side == 'B') ? (quote0.ask - quote.ask) : (quote.bid - quote0.bid);
	double dPriceCorr = ceil(dPrice * 1000 - 0.5) / 1000.;
	return qty * dPrice;
}

vector<HEurexRead::MMtrade> HEurexRead::get_matching_MMtrades(string ticker, int orderMsecsL)
{
	vector<MMtrade> v;
	for( vector<vector<string> >::iterator it = vvoA_.begin(); it != vvoA_.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		int orderMsecs = atoi((*it)[4].c_str());
		int eventMsecs = atoi((*it)[5].c_str());
		if( eventMsecs > orderMsecsL )
			break;
		else if( symbol != ticker )
		{
			if( eventMsecs >= orderMsecsL - 3 && eventMsecs <= orderMsecsL )
			{
				char side = trim((*it)[1])[0];
				double price = atof((*it)[2].c_str());
				int qty = atoi((*it)[3].c_str());
				int bidSize = atoi((*it)[6].c_str());
				double bid = atof((*it)[7].c_str());
				double ask = atof((*it)[8].c_str());
				int askSize = atoi((*it)[9].c_str());
				v.push_back(MMtrade(symbol, orderMsecs, eventMsecs, price, qty, bidSize, bid, ask, askSize));
			}
		}
	}
	return v;
}

vector<TradeInfo> HEurexRead::get_matching_trade(string ticker, int orderMsecs, int eventMsecs, double price, int qty)
{
	vector<TradeInfo> v;
	vector<TradeInfo>& vTrade = mTickerVtrade_[ticker];
	for( vector<TradeInfo>::iterator it = vTrade.begin(); it != vTrade.end(); ++it )
	{
		int t = it->msecs;
		if( t > orderMsecs - max_o2q_ && t < eventMsecs + max_o2q_ )
		{
			if( fabs(price - it->price) < ltmb_ && qty <= it->qty ) // Include aggregated trades.
				v.push_back(*it);
		}
		else if( t > eventMsecs + max_o2q_ )
			break;
	}

	return v;
}

vector<TradeInfo> HEurexRead::get_matching_trade_A(string ticker, int orderMsecs, int eventMsecs, double price, int qty)
{
	vector<TradeInfo> v;
	vector<TradeInfo>& vTrade = mTickerVtrade_[ticker];

	// Single fill.
	for( vector<TradeInfo>::iterator it = vTrade.begin(); it != vTrade.end(); ++it )
	{
		int t = it->msecs;
		if( t > orderMsecs && t < eventMsecs - min_t2e_ )
		{
			if( fabs(price - it->price) < ltmb_ && qty <= it->qty ) // Include aggregated trades.
				v.push_back(*it);
		}
		else if( t > eventMsecs + max_o2q_ )
			break;
	}

	// Consider partial fills.
	if( qty > 1 )
	{
		for( vector<TradeInfo>::iterator it0 = vTrade.begin(); it0 != vTrade.end(); ++it0 )
		{
			if( it0->msecs > orderMsecs )
			{
				int cumQty = 0;
				int t0 = 0;
				for( vector<TradeInfo>::iterator it = it0; it != vTrade.end(); ++it )
				{
					int t = it->msecs;
					if( t > orderMsecs && t < eventMsecs - min_t2e_ )
					{
						if( fabs(price - it->price) )
						{
							if( cumQty == 0 )
								t0 = t;

							cumQty += it->qty;
							if( cumQty == qty ) // match found.
							{
								if( v.empty() || t > v.rbegin()->msecs )
									v.push_back(*it);
								break;
							}
							else if( cumQty > qty )
								break;
						}
					}
					else if( t > eventMsecs - min_t2e_ )
						break;
				}
			}
			else if( it0->msecs > eventMsecs - min_t2e_ )
				break;
		}
	}

	return v;
}

QuoteInfo HEurexRead::get_quote(string ticker, int msecs)
{
	QuoteInfo qret;

	vector<QuoteInfo>& vQuote = mTickerVquote_[ticker];
	for( vector<QuoteInfo>::iterator it = vQuote.begin(); it != vQuote.end(); ++it )
	{
		if( it->msecs < msecs )
			qret = *it;
		else
			break;
	}
	return qret;
}
