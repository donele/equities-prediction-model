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

HOrderSummRead_test::HOrderSummRead_test(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
quotedDest_(false),
verbose_(0),
nticker_(0),
targetMsecs_(12*60*60*1000),
latency_offset_(0),
driftMax_(250),
forwardMatchMax_(1000),
q2oExtra_(0),
q2tMax_(7),
tickSource_("quote"),
routing_("")
{
	if( conf.count("quotedDest") )
		quotedDest_ = conf.find("quotedDest")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("q2oExtra") )
		q2oExtra_ = atoi( conf.find("q2oExtra")->second.c_str() );
	if( conf.count("targetMsecs") )
		targetMsecs_ = atoi( conf.find("targetMsecs")->second.c_str() );
	if( conf.count("nticker") )
		nticker_ = atoi( conf.find("nticker")->second.c_str() );
	if( conf.count("tickSource") )
		tickSource_ = conf.find("tickSource")->second;
	if( conf.count("dest") )
		dest_ = conf.find("dest")->second;
	if( conf.count("routing") )
		routing_ = conf.find("routing")->second;

	typedef multimap<string, string>::const_iterator mmit;
	if( conf.count("univ") )
	{
		pair<mmit, mmit> univs = conf.equal_range("univ");
		for( mmit mi = univs.first; mi != univs.second; ++mi )
		{
			string m = mi->second;
			vector<string> vm = split(m);
			for( vector<string>::iterator it = vm.begin(); it != vm.end(); ++it )
				vUniv_.push_back(*it);
		}
	}
}

HOrderSummRead_test::~HOrderSummRead_test()
{}

void HOrderSummRead_test::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	m_us_dest_routing_['P'] = "ADD1"; // UP
	m_us_dest_routing_['Q'] = "NQDD"; // UQ
	m_us_dest_routing_['N'] = "NYD1"; // UN
	m_us_dest_routing_['C'] = "BATS"; // UC
	m_us_dest_routing_['D'] = "EDGX"; // UD
	m_us_dest_routing_['B'] = "BEXD"; // UB
	m_us_dest_routing_['J'] = "EDGA"; // UJ
	m_us_dest_routing_['X'] = "PSXD"; // UX
	m_us_dest_routing_['Y'] = "BYXD"; // UY

	m_us_dbdest_['P'] = 80; // UP
	m_us_dbdest_['Q'] = 81; // UQ
	m_us_dbdest_['N'] = 78; // UN
	m_us_dbdest_['C'] = 90; // UC
	m_us_dbdest_['D'] = 75; // UD
	m_us_dbdest_['B'] = 66; // UB
	m_us_dbdest_['J'] = 74; // UJ
	m_us_dbdest_['X'] = 88; // UX
	m_us_dbdest_['Y'] = 89; // UY

	return;
}

void HOrderSummRead_test::beginMarket()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	msecClose_ = mto::msecClose(market);

	if( HEnv::Instance()->loopingOrder() == "dmt" )
		beginMarketDay(market, idate);
	return;
}

void HOrderSummRead_test::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	if( HEnv::Instance()->loopingOrder() == "mdt" )
		beginMarketDay(market, idate);
	return;
}

void HOrderSummRead_test::beginMarketDay(string market, int idate)
{
	// TickAccess
	taQ_.Clear();
	taO_.Clear();
	taT_.Clear();
	vector<string> quote_dirs = mto::bindirs(market, idate);
	vector<string> book_dirs = mto::bindirsBook(market, idate);
	vector<string> nbbo_dirs = mto::nbbodirs(market, idate);
	if( dest_ != "all" )
	{
		if(isECN(dest_) )
		{
			if( "EG" == dest_ )
			{
				string dir = (string)"\\\\smrc-nas09\\gf1\\tickC\\eu\\binLogL2\\" + mto::code(dest_);
				if ( idate >= 20111213 )
					dir = (string)"\\\\smrc-nas09\\gf1\\tickC\\eu\\binDirect\\" + mto::code(dest_);
				quote_dirs = vector<string>(1, dir);
				book_dirs = quote_dirs;
			}
			else if ( "EH" == dest_ )
			{
				string dir = (string)"\\\\smrc-nas09\\gf1\\tickC\\eu\\binLogL2\\" + mto::code(dest_);
				if ( idate >= 20110823 )
					dir = (string)"\\\\smrc-nas09\\gf1\\tickC\\eu\\binDirect\\" + mto::code(dest_);
				quote_dirs = vector<string>(1, dir);
				book_dirs = quote_dirs;
			}
			else
			{
				quote_dirs = mto::bindirs(dest_, idate);
				book_dirs = mto::bindirsBook(dest_, idate);
			}
		}
	}
	for( vector<string>::iterator it = quote_dirs.begin(); it != quote_dirs.end(); ++it )
		taQ_.AddRoot(*it, mto::longTicker(market));
	for( vector<string>::iterator it = book_dirs.begin(); it != book_dirs.end(); ++it )
		taO_.AddRoot(*it, mto::longTicker(market));
	if( mto::region(market) == "U" )
	{
		for( vector<string>::iterator it = nbbo_dirs.begin(); it != nbbo_dirs.end(); ++it )
			taT_.AddRoot(*it, mto::longTicker(market));
	}
	else
	{
		for( vector<string>::iterator it = quote_dirs.begin(); it != quote_dirs.end(); ++it )
			taT_.AddRoot(*it, mto::longTicker(market));
	}

	set_ticker_list(idate, market);
	read_orders(idate, market);
	set_latency_offset(idate, market);

	return;
}

void HOrderSummRead_test::beginTicker(const string& ticker)
{
	if( binary_search(tickers_.begin(), tickers_.end(), ticker) )
	{
		int idate = HEnv::Instance()->idate();
		string market = HEnv::Instance()->market();

		vector<MercatorOrder> vMOrderBuy;
		vector<MercatorOrder> vMOrderSell;
		read_orders(vMOrderBuy, vMOrderSell, market, ticker, idate);
		if( !vMOrderBuy.empty() || !vMOrderSell.empty() )
		{
			TickSeries<QuoteInfo> tsQ;
			TickSeries<OrderData> tsO;
			TickSeries<TradeInfo> tsT;

			boost::mutex::scoped_lock lock(tick_mutex_);
			{
				if( "quote" == tickSource_ )
					taQ_.GetTickSeries( ticker, idate, &tsQ );
				else if( "order" == tickSource_ )
				{
					taO_.GetTickSeries( ticker, idate, &tsO );
					boost::mutex::scoped_lock lock(quote_mutex_);
					order2quote( tsO, tsQ, mto::lotSize(market, idate) );
				}
				taT_.GetTickSeries( ticker, idate, &tsT );
			}

			read_quote_info( vMOrderBuy, market, ticker, "buy", tsQ, tsO, tsT );
			read_quote_info( vMOrderSell, market, ticker, "sell", tsQ, tsO, tsT );

			HEvent::Instance()->add<vector<MercatorOrder> >(ticker, (string)"vMOrderBuy_" + dest_, vMOrderBuy);
			HEvent::Instance()->add<vector<MercatorOrder> >(ticker, (string)"vMOrderSell_" + dest_, vMOrderSell);
		}
	}

	return;
}

void HOrderSummRead_test::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<MercatorOrder> >(ticker, (string)"vMOrderBuy_" + dest_);
	HEvent::Instance()->remove<vector<MercatorOrder> >(ticker, (string)"vMOrderSell_" + dest_);

	return;
}

void HOrderSummRead_test::endDay()
{
	vvo_.clear();
	return;
}

void HOrderSummRead_test::endMarket()
{
	string market = HEnv::Instance()->market();

	return;
}

void HOrderSummRead_test::endJob()
{
	return;
}

void HOrderSummRead_test::set_ticker_list(int idate, string market)
{
	string sExch = mto::code(market);

	string selMarket = "";
	if( mto::isInternational(market) && !isECN(market) )
		selMarket = " and exchange = '" + sExch + "' ";

	string dbSymbol = "symbol";
	if( mto::isInternational(market) )
		dbSymbol = "exchange + ':' + symbol";

	vector<string> selectedTickers;
	if( !vUniv_.empty() )
	{
		string univs = (string)"(" + vUniv_[0];
		int nUniv = vUniv_.size();
		for( int i = 1; i < nUniv; ++i )
		{
			univs += (string)"," + vUniv_[i];
		}
		univs += ")";

		char cmd[400];
		sprintf(cmd, "select %s from hfuniverse where idate = %d and inuniverse in %s %s ",
			dbSymbol.c_str(), idate, univs.c_str(), selMarket.c_str());

		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			selectedTickers.push_back(trim((*it)[0]));
	}
	sort(selectedTickers.begin(), selectedTickers.end());

	string cmd = (string)" select distinct (" + dbSymbol + ") "
		+ " from hfOrders o "
		+ " where idate = " + itos(idate)
		+ selMarket;
	vector<vector<string> > vvs;
	GODBC::Instance()->get(mto::hfo(market, idate))->ReadTable(cmd, &vvs);

	tickers_.clear();
	int cnt = 0;
	for( vector<vector<string> >::iterator it = vvs.begin(); it != vvs.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
		{
			if( vUniv_.empty() || binary_search(selectedTickers.begin(), selectedTickers.end(), symbol) )
			{
				tickers_.push_back(symbol);
				if( nticker_ > 0 && ++cnt >= nticker_ )
					break;
			}
		}
	}
	sort(tickers_.begin(), tickers_.end());
	HEnv::Instance()->tickers(tickers_);

	return;
}

void HOrderSummRead_test::read_orders(int idate, string market)
{
	string sExch = mto::code(market);

	// Dollarvol-fillRat
	{
	}

	// Read Orders.
	{
		string selMarket = (dest_ == "all")? " and o.exchange = " + quote(sExch): get_selMarket(market);

		string dbSymbol = "o.symbol";
		string groupExchange = "";
		if( mto::isInternational(market) )
		{
			dbSymbol = "o.exchange + ':' + o.symbol";
			groupExchange = ", o.exchange ";
		}

		string cmd = (string)" select " + dbSymbol + ", o.side, o.price, o.qty, sum(e.qty), o.feedMsecs, o.orderMsecs, min(e.eventMsecs), "
			+ " o.bidEx, o.bidsize, o.bid, o.ask, o.asksize, o.askEx, o.exectype, o.orderschedtype, min(e.fillType), min(o.onempred), min(o.tenmpred) "
			+ " from hfOrders o inner join hfOrderEvents e "
			+ " on o.orderID = e.orderID "
			+ " where o.idate = " + itos(idate)
			+ " and e.eventType > 0 " // 0: reject 1: cancel 2: fill.
			+ selMarket
			+ " group by o.symbol, o.orderID, o.side, o.price, o.qty, o.orderMsecs, o.feedMsecs, o.bidEx, o.bidsize, o.bid, o.ask, o.asksize, o.askEx, "
			+ " o.orderschedtype, o.exectype "
			+ groupExchange
			+ " order by o.orderMsecs ";
		GODBC::Instance()->get(mto::hfo(market, idate))->ReadTable(cmd, &vvo_);
	}

	return;
}

string HOrderSummRead_test::get_selMarket(string market)
{
	string sExch = mto::code(market);
	char cDest = (dest_ == "primary")? sExch[0]: dest_[1];

	// Read Orders.
	string selMarket = "";
	if( quotedDest_ )
	{
		if( mto::isInternational(market) )
		{
			if( isECN(market) )
				selMarket = (string)" and ( (o.side = 'B' and askEx = " + quote(itos(cDest)) + ") or ( o.side != 'B' and bidEx = " + quote(itos(cDest)) + ") ) ";
			else
				selMarket = (string)" and o.exchange = " + quote(sExch) + " and ( (o.side = 'B' and askEx = " + quote(itos(cDest)) + ") or ( o.side != 'B' and bidEx = " + quote(itos(cDest)) + ") ) ";
		}
		else if( "U" == mto::region(market) )
		{
			if( "US" == market )
				selMarket = "";
			else
				selMarket = (string)" and ( (o.side = 'B' and askEx = " + quote(itos(cDest)) + ") or ( o.side != 'B' and bidEx = " + quote(itos(cDest)) + ") ) ";
		}
		else if( "C" == mto::region(market) )
		{
			selMarket = (string)" and ( (o.side = 'B' and askEx = " + quote(itos(cDest)) + ") or ( o.side != 'B' and bidEx = " + quote(itos(cDest)) + ") ) ";
		}
	}
	else
	{
		if( mto::isInternational(market) )
		{
			if( isECN(market) )
				selMarket = (string)" and dest = " + quote(itos(cDest));
			else
				selMarket = (string)" and o.exchange = " + quote(sExch) + " and dest = " + quote(itos(cDest));
		}
		else if( "U" == mto::region(market) )
		{
			if( "US" == market )
				selMarket = "";
			else if( routing_.empty() )
				selMarket = (string)" and routing = " + quote(m_us_dest_routing_[cDest]) + " and dest = " + quote(itos(m_us_dbdest_[cDest]));
		}
		else if( "C" == mto::region(market) )
		{
			if( "CJ" == market )
			{
				if( "primary" == dest_ )
					selMarket = " and (dest = '84' or dest = '86')"; // 'T' and 'V'.
				else
					selMarket = (string)" and dest = " + quote(itos(cDest));
			}
			else
			{
				selMarket = (string)" and dest = " + quote(itos(cDest));
			}
		}
	}

	if( !routing_.empty() )
	{
		selMarket += (string)" and routing = " + quote(routing_);
	}
	return selMarket;
}

void HOrderSummRead_test::read_orders(vector<MercatorOrder>& vMOrderBuy, vector<MercatorOrder>& vMOrderSell,
							string market, string ticker, int idate)
{
	// Orders Filled (partial or full) / Orders not filled.
	MercatorOrder prevOrder;
	for( vector<vector<string> >::iterator it = vvo_.begin(); it != vvo_.end(); ++it )
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
			int qtyExec = atoi((*it)[4].c_str());
			int feedMsecs = atoi((*it)[5].c_str());
			int orderMsecs = atoi((*it)[6].c_str());
			int eventMsecs = atoi((*it)[7].c_str());
			char bidEx = atoi((*it)[8].c_str());
			int bidsize = atoi((*it)[9].c_str());
			double bid = atof((*it)[10].c_str());
			double ask = atof((*it)[11].c_str());
			int asksize = atoi((*it)[12].c_str());
			char askEx = atoi((*it)[13].c_str());
			char execType = trim((*it)[14])[0];
			int orderSchedType = atoi((*it)[15].c_str());
			int fillType = atoi((*it)[16].c_str());
			double pred1 = atof((*it)[17].c_str()) * basis_pts_;
			double pred10 = atof((*it)[17].c_str()) * basis_pts_;

			MercatorOrder newOrder(price, qty, qtyExec, side[0], feedMsecs, orderMsecs, eventMsecs,
				bidEx, bidsize, bid, ask, asksize, askEx, execType, orderSchedType, fillType, pred1, pred10);
			if( newOrder.side == prevOrder.side && newOrder.qty == prevOrder.qty && abs(newOrder.price - prevOrder.price) < 1e-4
				&& newOrder.qtyExec == 0 && prevOrder.qtyExec == 0) // remove bad orders.
				int i = 0;
			else
			{
				if( sign == 1 )
					vMOrderBuy.push_back(newOrder);
				else if( sign == -1 )
					vMOrderSell.push_back(newOrder);
			}
			prevOrder = newOrder;
		}
	}

	return;
}

void HOrderSummRead_test::set_latency_offset(int idate, string market)
{
	latency_offset_ = 0;
	if( idate >= 20090701 && idate < 20090901 ) // Trade in London, but logged the tickdata in Chicago.
	{
		if( "EA" == market || "EB" == market || "EI" == market || "EP" == market || "EF" == market || "EL" == market
			|| "ED" == market || "EM" == market || "EZ" == market || "EX" == market )
			latency_offset_ = 45;
		else if( "EO" == market )
			latency_offset_ = 310;
		else if( "EC" == market || "EW" == market || "EY" == market )
			latency_offset_ = 70;
	}
	return;
}

void HOrderSummRead_test::read_quote_info( vector<MercatorOrder>& vMOrder, string market, string ticker, string bs,
								TickSeries<QuoteInfo>& tsQ, TickSeries<OrderData>& tsO, TickSeries<TradeInfo>& tsT )
{
	int ntq = tsQ.NTicks();
	int ntt = tsT.NTicks();
	QuoteInfo quote;
	int mult = (bs == "buy") ? 1 : -1;

	for( vector<MercatorOrder>::iterator itb = vMOrder.begin(); itb != vMOrder.end(); ++itb )
	{
		int orderMsecs = itb->orderMsecs;
		int q2oMax = (itb->orderSchedType == 2) ? 1000 : 6000000;

		// Returns.
		double mid60 = itb->price;
		double mid660 = itb->price;
		double mid4260 = itb->price;
		double midTarget = itb->price;
		tsQ.StartRead();
		for( int n = 0; n < ntq; ++n )
		{
			tsQ.Read(&quote);

			double mid = get_mid(quote.bid, quote.ask);
			double sprd = 2.0 * (quote.ask - quote.bid) / (quote.ask + quote.bid);
			if( mid > 1e-6 && fabs(sprd) < 0.05 )
			{
				if( quote.msecs < orderMsecs + 60000 )
				{
					mid60 = mid;
					mid660 = mid;
					mid4260 = mid;
				}
				else if( quote.msecs < orderMsecs + 660000 )
				{
					mid660 = mid;
					mid4260 = mid;
				}
				else if( quote.msecs < orderMsecs + 4260000 )
					mid4260 = mid;

				if( quote.msecs < orderMsecs + targetMsecs_ )
					midTarget = mid;
			}

			if( quote.msecs >= orderMsecs + 4260000 && quote.msecs >= orderMsecs + targetMsecs_ )
				break;
		}

		double retTarget = mult * (midTarget / itb->price - 1.) * basis_pts_;
		itb->gain = retTarget;
		itb->ret1 = (mid60 / itb->price - 1.) * basis_pts_;
		itb->ret10 = ((mid660 / mid60 - 1.) + .5 * (mid4260 / mid660 - 1.)) * basis_pts_;

		if( verbose_ == 2 )
		{
			QuoteInfo last_quote;
			tsQ.StartRead();
			double ret = 0.;
			double mid = 0.;
			for( int n=0; n<ntq; ++n )
			{
				tsQ.Read(&quote);
				if( orderMsecs + targetMsecs_ <= quote.msecs || msecClose_ <= quote.msecs || n == ntq-1 )
				{
					double this_mid = get_mid(last_quote.bid, last_quote.ask);
					double sprd = 2.0 * (last_quote.ask - last_quote.bid) / (last_quote.ask + last_quote.bid);
					if( this_mid > 1e-6 && fabs(sprd) < 0.05 )
					{
						mid = this_mid;
						ret = mult * (mid - itb->price) / itb->price * 10000;
						break;
					}
				}
				last_quote = quote;
			}
			printf("%13s %d %.2f %.2f %.4f %.4f\n", ticker.c_str(), orderMsecs, midTarget, mid, retTarget, ret);
		}

		// Collect the matching quote candidates.
		vector<QuoteInfo> vQuoteOneSide;
		vector<QuoteInfo> vQuoteBothSides;
		vector<QuoteInfo> vQuoteOneSideFuture;
		vector<QuoteInfo> vQuoteBothSidesFuture;
		vector<QuoteInfo> vQuoteOneSidePast;
		tsQ.StartRead();
		for( int n = 0; n < ntq; ++n )
		{
			tsQ.Read(&quote);

			bool isPast = quote.msecs - latency_offset_ <= orderMsecs + driftMax_;
			bool quiteClose = isPast && orderMsecs - q2oMax - q2oExtra_ - driftMax_ < quote.msecs - latency_offset_;
			bool closeFuture = orderMsecs + driftMax_ < quote.msecs - latency_offset_
				&& quote.msecs - latency_offset_ < orderMsecs + driftMax_ + forwardMatchMax_;
			bool ourOfRange = orderMsecs + driftMax_ + forwardMatchMax_ <= quote.msecs - latency_offset_;

			bool askSideMatch = fabs(quote.ask - itb->ask) < ltmb_ && quote.askSize == itb->asksize;
			bool bidSideMatch = fabs(quote.bid - itb->bid) < ltmb_ && quote.bidSize == itb->bidsize;

			bool targetSideMatch = ("buy" == bs && askSideMatch) || ("sell" == bs && bidSideMatch);
			bool otherSideMatch = ("buy" == bs && bidSideMatch) || ("sell" == bs && askSideMatch);

			if( isPast )
			{
				if( targetSideMatch )
					vQuoteOneSidePast.push_back(quote);
			}

			if( quiteClose )
			{
				if( targetSideMatch && otherSideMatch )
					vQuoteBothSides.push_back(quote);
				else if( targetSideMatch )
					vQuoteOneSide.push_back(quote);
			}
			else if( closeFuture )
			{
				if( targetSideMatch && otherSideMatch )
					vQuoteBothSidesFuture.push_back(quote);
				else if( targetSideMatch )
					vQuoteOneSideFuture.push_back(quote);
			}
			else if( ourOfRange )
				break;
		}

		// Select the matching quote.
		QuoteInfo theQuote;
		if( !vQuoteBothSides.empty() )
		{
			theQuote = vQuoteBothSides[vQuoteBothSides.size() - 1];
			itb->quoteMatch = 5;
		}
		else if( !vQuoteOneSide.empty() )
		{
			theQuote = vQuoteOneSide[vQuoteOneSide.size() - 1];
			itb->quoteMatch = 4;
		}
		else if( !vQuoteBothSidesFuture.empty() )
		{
			theQuote = vQuoteBothSidesFuture[0];
			itb->quoteMatch = 3;
		}
		else if( !vQuoteOneSideFuture.empty() )
		{
			theQuote = vQuoteOneSideFuture[0];
			itb->quoteMatch = 2;
		}

		// If matched, find the deterioration.
		if( itb->quoteMatch > 1 )
		{
			itb->quoteMsecs = theQuote.msecs;

			// Find the price deterioration. Take the last snapshot of each time stamp.
			tsQ.StartRead();
			QuoteInfo lastQuote;
			for( int n = 0; n < ntq; ++n )
			{
				tsQ.Read(&quote);

				if( lastQuote.msecs < quote.msecs )
				{
					if( theQuote.msecs <= lastQuote.msecs )
					{
						if( "buy" == bs )
						{
							if( lastQuote.ask - theQuote.ask > ltmb_ )
							{
								itb->detMsecs = lastQuote.msecs;
								break;
							}
						}
						else if( "sell" == bs )
						{
							if( theQuote.bid - lastQuote.bid > ltmb_ )
							{
								itb->detMsecs = lastQuote.msecs;
								break;
							}
						}
					}
				}
				lastQuote = quote;
			}
		}

		// If matched, look for trade.
		if( itb->quoteMatch > 1 && itb->execType == 'L' && itb->orderSchedType == 2 )
		{
			int theMsecs = theQuote.msecs;
			tsT.StartRead();
			TradeInfo trade;
			int ntrd = 0;
			int trade_msecs = -1;
			double trade_price = 0.;
			double mid = (itb->bid + itb->ask) / 2.0;
			for( int n = 0; n < ntt; ++n )
			{
				tsT.Read(&trade);
				if( trade.msecs >= theMsecs && trade.msecs <= theMsecs + q2tMax_ )
				{
					if( (itb->side == 'B' && mid <= trade.price && trade.price < itb->price + 1e-4)
						|| (itb->side == 'B' && itb->ask <= itb->bid && trade.price < itb->price + 1e-4)
						|| (itb->side == 'S' && itb->price < trade.price + 1e-4 && itb->ask <= itb->bid)
						|| (itb->side == 'S' && itb->price < trade.price + 1e-4 && trade.price < mid) )
					{
						if( trade_msecs < 0 )
						{
							trade_msecs = trade.msecs;
							trade_price = trade.price;
						}
						++ntrd;
					}
				}
				else if( trade.msecs > theMsecs + q2tMax_ )
					break;
			}
			if( ntrd > 0 )
				itb->trdMsecs = trade_msecs;
		}

		// If matched and foundDetMsecs, find q2i and i2x.
		bool isMatched = itb->quoteMsecs > 0;
		bool foundDetMsecs = itb->detMsecs > 0;
		if( isMatched && foundDetMsecs )
		{
			if( "quote" == tickSource_ )
			{
			}
			else if( "order" == tickSource_ )
			{
				// Look for our order appearing on the book.
				bool foundInsert = false;
				double price = itb->price;
				int sizeAfterInsert = 0;
				int nto = tsO.NTicks();
				OrderData order;
				tsO.StartRead();
				for( int n = 0; n < nto; ++n )
				{
					tsO.Read(&order);

					if( theQuote.msecs /*+ fakeMsecsMax_*/ < order.msecs )
					{
						if( "buy" == bs )
						{
							if( fabs(order.price/10000.0 - price) < ltmb_ && order.size == itb->qty && order.type == 0 )
							{
								itb->insertMsecs = order.msecs;
								foundInsert = true;
								break;
							}
						}
						else if( "sell" == bs )
						{
							if( fabs(order.price/10000.0 - price) < ltmb_ && order.size == itb->qty && order.type == 2 )
							{
								itb->insertMsecs = order.msecs;
								foundInsert = true;
								break;
							}
						}
					}
				}

				// Find the price/size deterioration after insertMsecs. Take the last snapshot of each time stamp.
				if( foundInsert )
				{
					tsQ.StartRead();
					QuoteInfo lastQuote;
					for( int n = 0; n < ntq; ++n )
					{
						tsQ.Read(&quote);

						if( lastQuote.msecs < quote.msecs )
						{
							if( itb->insertMsecs == lastQuote.msecs )
							{
								if( "buy" == bs && fabs(price - quote.bid) < ltmb_ )
									sizeAfterInsert = lastQuote.bidSize;
								else if( "sell" == bs && fabs(price - quote.ask) < ltmb_ )
									sizeAfterInsert = lastQuote.askSize;
							}

							if( itb->insertMsecs <= lastQuote.msecs )
							{
								if( "buy" == bs )
								{
									if( price - lastQuote.bid > ltmb_ || lastQuote.bidSize < sizeAfterInsert )
									{
										itb->executeMsecs = lastQuote.msecs;
										break;
									}
								}
								else if( "sell" == bs )
								{
									if( lastQuote.ask - price > ltmb_ || lastQuote.askSize < sizeAfterInsert )
									{
										itb->executeMsecs = lastQuote.msecs;
										break;
									}
								}
							}
						}
						lastQuote = quote;
					}
				}
			}
		}

		// debug.
		{
			int q2o = 24*60*60*1000;
			int o2e = 24*60*60*1000;
			int q2d = 24*60*60*1000;
			int q2i = 24*60*60*1000;
			int i2x = 24*60*60*1000;
			if( itb->orderMsecs > 0 && itb->quoteMsecs > 0 )
				q2o = itb->orderMsecs - itb->quoteMsecs;
			if( itb->eventMsecs > 0 && itb->orderMsecs > 0 )
				o2e = itb->eventMsecs - itb->orderMsecs;
			if( itb->detMsecs > 0 && itb->quoteMsecs > 0 )
				q2d = itb->detMsecs - itb->quoteMsecs;
			if( itb->insertMsecs > 0 && itb->quoteMsecs > 0 )
				q2i = itb->insertMsecs - itb->quoteMsecs;
			if( itb->executeMsecs > 0 && itb->insertMsecs > 0 )
				i2x = itb->executeMsecs - itb->insertMsecs;

			if( verbose_ == 3
				|| (verbose_ == 4 && !itb->isFilledIncl() && q2d > 200)
				|| (verbose_ == 5 && !itb->isFilledIncl() && q2d < 2)
				|| (verbose_ == 6 && q2d < 2)
				|| (verbose_ == 7 && i2x < 3)
				|| (verbose_ == 8 && !itb->isFilledIncl() && q2d > 4000)
				|| (verbose_ == 9 && itb->quoteMatch > 1 )
				|| (verbose_ == 10 && itb->quoteMatch <= 1 )
				)
			{
				char buf[400];
				sprintf( buf, "%10s %d %d %d o2e %6d q2d %8d q2i %6d i2x %6d\n",
					ticker.c_str(), orderMsecs, itb->quoteMatch, itb->isFilledIncl(),
					o2e, q2d, q2i, i2x);
				cout << buf;
			}
		}
	}
	return;
}

bool HOrderSummRead_test::is_valid( QuoteInfo& quote )
{
	bool ret = quote.bid > ltmb_ && quote.ask > ltmb_ && quote.bid < 10000000 && quote.ask < 10000000
		&& quote.ask > quote.bid && 2.0*(quote.ask - quote.bid)/(quote.ask + quote.bid) < 0.1;
	return ret;
}

int HOrderSummRead_test::get_price_change( double last, double now )
{
	int ret = 0;
	if( last > ltmb_ && now > ltmb_ && valid_price(last) && valid_price(now) )
	{
		if( now - last > ltmb_ )
			ret = 1;
		else if( last - now > ltmb_ )
			ret = -1;
	}
	return ret;
}

bool HOrderSummRead_test::valid_price( double price )
{
	bool valid = price > ltmb_ && price < 10000000;
	return valid;
}

void HOrderSummRead_test::get_o2x( vector<MercatorOrder>& vMOrder, int m1, int m2, double price, int priceDeteriorated )
{
	vector<MercatorOrder>::iterator it = vMOrder.begin();
	vector<MercatorOrder>::iterator itl = vMOrder.end();
	for( ; it != vMOrder.end() && it->orderMsecs < m2; ++it )
	{
		if( m1 < it->orderMsecs && it->price == price )
			itl = it;
	}

	if( itl != vMOrder.end() )
		itl->detMsecs = priceDeteriorated * m2;

	return;
}
