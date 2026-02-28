#include <HOrders.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <optionlibs/TickData.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <map>
#include <string>
#include <TFile.h>
using namespace std;

HFillTime::HFillTime(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
verbose_(0),
procName_(""),
orderSchedType_(2), // 1: scheduled. 2: asymm.
latency_offset_(0),
q2oMax_(1000),
driftMax_(250),
forwardMatchMax_(1000),
fakeMsecsMax_(3),
tickSource_("quote"),
dest_(0)
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("procName") )
		procName_ = conf.find("procName")->second.c_str();
	if( conf.count("orderSchedType") )
		orderSchedType_ = atoi( conf.find("orderSchedType")->second.c_str() );
	if( conf.count("tickSource") )
		tickSource_ = conf.find("tickSource")->second.c_str();
	if( conf.count("dest") )
		dest_ = conf.find("dest")->second.c_str()[0];

	if( orderSchedType_ == 1 )
	{
		q2oMax_ = 6000000;
	}
}

HFillTime::~HFillTime()
{}

void HFillTime::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void HFillTime::beginMarket()
{
	string market = HEnv::Instance()->market();
	return;
}

void HFillTime::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	set_ticker_list(idate, market);
	read_orders(idate, market);
	set_latency_offset(idate, market);
	return;
}

void HFillTime::beginTicker(const string& ticker)
{
	if( binary_search(tickers_.begin(), tickers_.end(), ticker) )
	{
		int idate = HEnv::Instance()->idate();
		string market = HEnv::Instance()->market();

		vector<MercatorOrder> vMOrderBuy;
		vector<MercatorOrder> vMOrderSell;

		read_orders(vMOrderBuy, vMOrderSell, market, ticker, idate);

		TickSeries<QuoteInfo> tsQ;
		TickSeries<OrderData> tsO;
		if( "quote" == tickSource_ )
			tsQ = *(static_cast<const TickSeries<QuoteInfo>*>(HEvent::Instance()->get(ticker, "quotes")));
		else if( "order" == tickSource_ )
		{
			tsO = *(static_cast<const TickSeries<OrderData>*>(HEvent::Instance()->get(ticker, "orders")));
			boost::mutex::scoped_lock lock(quote_mutex_);
			order2quote( tsO, tsQ, mto::lotSize(market, idate) );
		}

		read_quote_info( vMOrderBuy, market, ticker, "buy", tsQ, tsO );
		read_quote_info( vMOrderSell, market, ticker, "sell", tsQ, tsO );

		HEvent::Instance()->add<vector<MercatorOrder> >(ticker, (string)"vMOrderBuy" + procName_, vMOrderBuy);
		HEvent::Instance()->add<vector<MercatorOrder> >(ticker, (string)"vMOrderSell" + procName_, vMOrderSell);
	}

	return;
}

void HFillTime::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<MercatorOrder> >(ticker, (string)"vMOrderBuy" + procName_);
	HEvent::Instance()->remove<vector<MercatorOrder> >(ticker, (string)"vMOrderSell" + procName_);

	return;
}

void HFillTime::endDay()
{
	vvo_.clear();
	return;
}

void HFillTime::endMarket()
{
	string market = HEnv::Instance()->market();

	return;
}

void HFillTime::endJob()
{
	return;
}

void HFillTime::set_ticker_list(int idate, string market)
{
	vector<string> tickersTickRead = HEnv::Instance()->tickers();
	sort(tickersTickRead.begin(), tickersTickRead.end());

	tickers_.clear();
	{
		string selMarket = "";
		if( mto::isInternational(market) )
			selMarket = (string)" and dest = " + quote(itos( mto::code(market)[0] ));
		else if( "U" == mto::region(market) )
			selMarket = (string)" and dest = " + itos( dest_ );

		string dbSymbol = "symbol";
		if( mto::isInternational(market) )
			dbSymbol = "exchange + ':' + symbol";

		string cmd = (string)" select distinct (" + dbSymbol + ") "
			+ " from hfOrders "
			+ " where idate = " + itos(idate)
			+ " and orderSchedType = " + itos(orderSchedType_)
			+ " and execType = 'L' " // market taking.
			+ selMarket;
		vector<vector<string> > vvs;
		GODBC::Instance()->get(mto::hfo(market, idate))->ReadTable(cmd, &vvs);
		for( vector<vector<string> >::iterator it = vvs.begin(); it != vvs.end(); ++it )
		{
			string symbol = trim((*it)[0]);
			if( !symbol.empty() && binary_search(tickersTickRead.begin(), tickersTickRead.end(), symbol) )
				tickers_.push_back(symbol);
		}
	}
	sort(tickers_.begin(), tickers_.end());

	return;
}

void HFillTime::read_orders(int idate, string market)
{
	// Dollarvol-fillRat
	{
	}

	// Read Orders.
	{
		string selMarket = "";
		if( mto::isInternational(market) )
			selMarket = (string)" and o.dest = " + quote(itos( mto::code(market)[0] ));
		else if( "U" == mto::region(market) )
			selMarket = (string)" and dest = " + itos( dest_ );


		string dbSymbol = "o.symbol";
		if( mto::isInternational(market) )
			dbSymbol = "o.exchange + ':' + o.symbol";

		string cmd = (string)" select " + dbSymbol + ", o.side, o.price, o.qty, sum(e.qty), o.feedMsecs, o.orderMsecs, min(e.eventMsecs), "
			+ " o.bidEx, o.bidsize, o.bid, o.ask, o.asksize, o.askEx "
			+ " from hfOrders o inner join hfOrderEvents e "
			+ " on o.orderID = e.orderID "
			+ " where o.idate = " + itos(idate)
			+ " and e.idate = " + itos(idate)
			+ " and o.orderSchedType = " + itos(orderSchedType_)
			+ " and o.execType = 'L' " // market taking.
			+ " and e.eventType > 0 " // 0: reject 1: cancel 2: fill.
			+ selMarket
			+ " group by o.exchange, o.symbol, o.orderID, o.side, o.price, o.qty, o.orderMsecs, o.bidEx, o.bidsize, o.bid, o.ask, o.asksize, o.askEx "
			+ " order by o.orderMsecs, o.feedMsecs ";
		GODBC::Instance()->get(mto::hfo(market, idate))->ReadTable(cmd, &vvo_);
	}

	return;
}

void HFillTime::read_orders(vector<MercatorOrder>& vMOrderBuy, vector<MercatorOrder>& vMOrderSell,
							string market, string ticker, int idate)
{
	// Orders Filled (partial or full) / Orders not filled.
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

			if( sign == 1 )
				vMOrderBuy.push_back( MercatorOrder(price, qty, qtyExec, side[0], feedMsecs, orderMsecs, eventMsecs, bidEx, bidsize, bid, ask, asksize, askEx) );
			else if( sign == -1 )
				vMOrderSell.push_back( MercatorOrder(price, qty, qtyExec, side[0], feedMsecs, orderMsecs, eventMsecs, bidEx, bidsize, bid, ask, asksize, askEx) );
		}
	}

	return;
}

void HFillTime::set_latency_offset(int idate, string market)
{
	latency_offset_ = 0;
	if( idate >= 20090701 && idate < 20090901 )
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

void HFillTime::read_quote_info( vector<MercatorOrder>& vMOrder, string market, string ticker, string bs,
								TickSeries<QuoteInfo>& tsQ, TickSeries<OrderData>& tsO )
{
	int ntq = tsQ.NTicks();
	QuoteInfo quote;

	for( vector<MercatorOrder>::iterator itb = vMOrder.begin(); itb != vMOrder.end(); ++itb )
	{
		int orderMsecs = itb->orderMsecs;

		// Collect the matching quote candidates.
		vector<QuoteInfo> vQuoteOneSide;
		vector<QuoteInfo> vQuoteBothSides;
		vector<QuoteInfo> vQuoteOneSideFuture;
		vector<QuoteInfo> vQuoteBothSidesFuture;
		tsQ.StartRead();
		for( int n=0; n<ntq; ++n )
		{
			tsQ.Read(&quote);
			//quote.msecs -= latency_offset_;

			bool quiteClose = orderMsecs - q2oMax_ - driftMax_ < quote.msecs - latency_offset_
				&& quote.msecs - latency_offset_ <= orderMsecs + driftMax_;
			bool closeFuture = orderMsecs + driftMax_ < quote.msecs - latency_offset_
				&& quote.msecs - latency_offset_ < orderMsecs + driftMax_ + forwardMatchMax_;
			bool ourOfRange = orderMsecs + driftMax_ + forwardMatchMax_ <= quote.msecs - latency_offset_;

			if( quiteClose )
			{
				bool targetSideMatch = ("buy" == bs && fabs(quote.ask - itb->ask) < ltmb_ && quote.askSize == itb->asksize)
					|| ("sell" == bs && fabs(quote.bid - itb->bid) < ltmb_ && quote.bidSize == itb->bidsize);
				bool otherSideMatch = ("buy" == bs && fabs(quote.bid - itb->bid) < ltmb_ && quote.bidSize == itb->bidsize)
					|| ("sell" == bs && fabs(quote.ask - itb->ask) < ltmb_ && quote.askSize == itb->asksize);
				if( targetSideMatch && otherSideMatch )
					vQuoteBothSides.push_back(quote);
				else if( targetSideMatch )
					vQuoteOneSide.push_back(quote);
			}
			else if( closeFuture )
			{
				bool targetSideMatch = ("buy" == bs && fabs(quote.ask - itb->ask) < ltmb_ && quote.askSize == itb->asksize)
					|| ("sell" == bs && fabs(quote.bid - itb->bid) < ltmb_ && quote.bidSize == itb->bidsize);
				bool otherSideMatch = ("buy" == bs && fabs(quote.bid - itb->bid) < ltmb_ && quote.bidSize == itb->bidsize)
					|| ("sell" == bs && fabs(quote.ask - itb->ask) < ltmb_ && quote.askSize == itb->asksize);
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
			for( int n=0; n<ntq; ++n )
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

		// If q2d < 3, find q2i and i2x.
		if( itb->detMsecs > 0 && itb->quoteMsecs > 0 && itb->detMsecs - itb->quoteMsecs < fakeMsecsMax_ )
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
				for( int n=0; n<nto; ++n )
				{
					tsO.Read(&order);

					if( theQuote.msecs + fakeMsecsMax_ <= order.msecs )
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
					for( int n=0; n<ntq; ++n )
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

bool HFillTime::is_valid( QuoteInfo& quote )
{
	bool ret = quote.bid > ltmb_ && quote.ask > ltmb_ && quote.bid < 10000000 && quote.ask < 10000000
		&& quote.ask > quote.bid && 2.0*(quote.ask - quote.bid)/(quote.ask + quote.bid) < 0.1;
	return ret;
}

int HFillTime::get_price_change( double last, double now )
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

bool HFillTime::valid_price( double price )
{
	bool valid = price > ltmb_ && price < 10000000;
	return valid;
}

void HFillTime::get_o2x( vector<MercatorOrder>& vMOrder, int m1, int m2, double price, int priceDeteriorated )
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
