#include <MOrders/MOrderSummRead.h>
#include <MFramework.h>
#include <optionlibs/TickData.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/GODBC.h>
#include <map>
#include <string>
using namespace std;

MOrderSummRead::MOrderSummRead(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
quotedDest_(false),
tokyo_colo_chicago_logged_(false),
use_test_data_(false),
verbose_(0),
nticker_(0),
targetMsecs_(12*60*60*1000),
latency_offset_(0),
driftMax_(50),
q2oMin_(0),
q2oMax_(0),
q2dMin_(0),
q2dMax_(800),
q2oExtra_(500),
tickSource_("quote"),
routing_("")
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("quotedDest") )
		quotedDest_ = conf.find("quotedDest")->second == "true";
	if( conf.count("TokyoColoChicagoLogged") )
		tokyo_colo_chicago_logged_ = conf.find("TokyoColoChicagoLogged")->second == "true";
	if( conf.count("useTestData") )
		use_test_data_ = conf.find("useTestData")->second == "true";
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

MOrderSummRead::~MOrderSummRead()
{}

void MOrderSummRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	m_us_dest_routing_['P'] = {"ADD1", "ASD1"}; // UP
	m_us_dest_routing_['Q'] = {"NQDD", "NQDS"}; // UQ
	m_us_dest_routing_['N'] = {"NYD1"}; // UN
	m_us_dest_routing_['C'] = {"BATS"}; // UC
	m_us_dest_routing_['D'] = {"EDGX"}; // UD
	m_us_dest_routing_['B'] = {"BEXD"}; // UB
	m_us_dest_routing_['J'] = {"EDGA"}; // UJ
	m_us_dest_routing_['X'] = {"PSXD"}; // UX
	//m_us_dest_routing_['Y'] = {"BYXD"}; // UY

	m_us_dbdest_['P'] = 80; // UP
	m_us_dbdest_['Q'] = 81; // UQ
	m_us_dbdest_['N'] = 78; // UN
	m_us_dbdest_['C'] = 90; // UC 'C'68 -> 'Z'90
	m_us_dbdest_['D'] = 75; // UD 'D'69 -> 'K'75
	m_us_dbdest_['B'] = 66; // UB
	m_us_dbdest_['J'] = 74; // UJ
	m_us_dbdest_['X'] = 88; // UX
	//m_us_dbdest_['Y'] = 89; // UY
}

void MOrderSummRead::beginMarket()
{
	string market = MEnv::Instance()->market;
	region_ = mto::region(market);
	msecClose_ = mto::msecClose(market);

	if( MEnv::Instance()->loopingOrder == "dmt" )
	{
		int idate = MEnv::Instance()->idate;
		beginMarketDay(market, idate);
	}
}

void MOrderSummRead::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;

	if( MEnv::Instance()->loopingOrder == "mdt" )
		beginMarketDay(market, idate);

	if( region_ == "U" && idate >= 20130114 )
		driftMax_ = 3;
	else
		driftMax_ = 50;

	if( region_ == "U" )
	{
		q2oMin_ = 0;
		q2oMax_ = 4;
		q2dMin_ = 0;
	}
	else if( region_ == "C" )
	{
		q2oMin_ = 0;
		q2oMax_ = 4;
		q2dMin_ = 0;
	}
	else if( region_ == "E" )
	{
		q2oMin_ = 0;
		q2oMax_ = 4;
		q2dMin_ = 0;
	}
	else if( region_ == "A" ) // Tickdata timestamps are received timestamps.
	{
		if( idate >= 20130424 ) // Tokyo Colo starts.
		{
			if( idate < 20130522 ) // Tickdata logged in Chicago while trading from Tokyo.
			{
				q2oMin_ = -240;
				q2oMax_ = -140;
			}
			//else if( idate == 20130925 && market == "AT" ) // Direct QH trading, IDC data logged.
			//{
			//	q2oMin_ = -70;
			//	q2oMax_ = 0;
			//}

			if( market == "AT" )
			{
				if( dest_ == "primary" )
				{
					if( idate < 20130711 )
						q2dMin_ = 65;
					else if( idate >= 20130711 ) // DMA order entry starts.
						q2dMin_ = 55;

					if( idate >= 20130925 && use_test_data_ )
						q2dMin_ = 7;
				}
				else if( dest_ == "AC" || dest_ == "all" )
					q2dMin_ = 12;
			}
			else if( market == "AH" )
				q2dMin_ = 72;
			else if( market == "AS" )
				q2dMin_ = 230;
			else if( market == "AK" )
				q2dMin_ = 150;
			else if( market == "AQ" )
				q2dMin_ = 150;
		}
		else if( market == "AT" )
		{
			q2oMin_ = 0; // Since q (quoteMsecs) is received time, q2o is <= 1 msec.
			q2oMax_ = 0;
			q2dMin_ = 320;
		}
		else if( market == "AH" )
		{
			q2oMin_ = 0;
			q2oMax_ = 0;
			q2dMin_ = 330;
		}
		else if( market == "AS" )
		{
			q2oMin_ = 0;
			q2oMax_ = 0;
			q2dMin_ = 480;
		}
		else if( market == "AK" || market == "AQ" )
		{
			q2oMin_ = 0;
			q2oMax_ = 0;
			q2dMin_ = 400;
		}
	}
	else if( region_ == "M" )
	{
	}
	else if( region_ == "S" )
	{
		q2dMin_ = 130;
	}
}

void MOrderSummRead::beginMarketDay(string market, int idate)
{
	// TickAccess
	taQ_.Clear();
	taO_.Clear();
	taT_.Clear();
	vector<string> quote_dirs = mto::bindirs(market, idate);
	vector<string> book_dirs = mto::bindirsBook(market, idate);
	vector<string> nbbo_dirs = mto::nbbodirs(market, idate);
	if( use_test_data_ && market == "AT" && idate >= 20130925 )
	{
		quote_dirs.clear();
		book_dirs.clear();
		nbbo_dirs.clear();
		string dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\asia_test\\binLogL2\\T");
		quote_dirs.push_back(dir);
		book_dirs.push_back(dir);
		nbbo_dirs.push_back(dir);
	}
	if( dest_ != "all" )
	{
		if(isECN(dest_) || isUSDest(dest_))
		{
			if( "EG" == dest_ )
			{
				string dir = xpf("smrc-nas09\\gf1\\tickC\\eu\\binLogL2\\" + mto::code(dest_));
				if ( idate >= 20111213 )
					dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\eu\\binDirect\\" + mto::code(dest_));
				quote_dirs = vector<string>(1, dir);
				book_dirs = quote_dirs;
			}
			else if ( "EH" == dest_ )
			{
				string dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\eu\\binLogL2\\" + mto::code(dest_));
				if ( idate >= 20110823 )
					dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\eu\\binDirect\\" + mto::code(dest_));
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

	// Quote
	if( region_ == "U" )
	{
		for( vector<string>::iterator it = nbbo_dirs.begin(); it != nbbo_dirs.end(); ++it )
			taQ_.AddRoot(*it, mto::longTicker(market));
	}
	else
	{
		for( vector<string>::iterator it = quote_dirs.begin(); it != quote_dirs.end(); ++it )
			taQ_.AddRoot(*it, mto::longTicker(market));
	}

	// Order
	for( vector<string>::iterator it = book_dirs.begin(); it != book_dirs.end(); ++it )
		taO_.AddRoot(*it, mto::longTicker(market));

	// Trade
	if(region_ == "U" )
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
	if( region_ != "U" )
		read_orders(vvo_, idate, market);
	set_latency_offset(idate, market);
	get_retON(market, idate);
}

void MOrderSummRead::beginTicker(const string& ticker)
{
	if( binary_search(tickers_.begin(), tickers_.end(), ticker) )
	{
		int idate = MEnv::Instance()->idate;
		string market = MEnv::Instance()->market;
		char exch = market[1];

		vector<vector<string> > vvo;
		if( region_ == "U" )
		{
			boost::mutex::scoped_lock lock(db_mutex_);
			read_orders(vvo, idate, market, ticker);
		}

		vector<MercatorOrder> vMOrderBuy;
		vector<MercatorOrder> vMOrderSell;
		if( region_ == "U" )
			select_orders(vvo, vMOrderBuy, vMOrderSell, market, ticker, idate);
		else
			select_orders(vvo_, vMOrderBuy, vMOrderSell, market, ticker, idate);

		if( !vMOrderBuy.empty() || !vMOrderSell.empty() )
		{
			TickSeries<QuoteInfo> tsQ;
			TickSeries<OrderData> tsO;
			TickSeries<TradeInfo> tsT;

			if( "none" != tickSource_ )
			{
				boost::mutex::scoped_lock lock(tick_mutex_);
				{
					if( "quote" == tickSource_ )
						taQ_.GetTickSeries( ticker, idate, &tsQ );
					else if( "order" == tickSource_ )
					{
						taO_.GetTickSeries( ticker, idate, &tsO );
						boost::mutex::scoped_lock lock(quote_mutex_);
						order2quote( tsO, tsQ, mto::lotSize(market, idate), false, false, exch );
					}
					taT_.GetTickSeries( ticker, idate, &tsT );
				}

				read_quote_info( vMOrderBuy, market, ticker, "buy", tsQ, tsO, tsT );
				read_quote_info( vMOrderSell, market, ticker, "sell", tsQ, tsO, tsT );

				MEvent::Instance()->add<vector<MercatorOrder> >(ticker, (string)"vMOrderBuy_" + dest_, vMOrderBuy);
				MEvent::Instance()->add<vector<MercatorOrder> >(ticker, (string)"vMOrderSell_" + dest_, vMOrderSell);
			}
		}
	}
}

void MOrderSummRead::endTicker(const string& ticker)
{
	MEvent::Instance()->remove<vector<MercatorOrder> >(ticker, (string)"vMOrderBuy_" + dest_);
	MEvent::Instance()->remove<vector<MercatorOrder> >(ticker, (string)"vMOrderSell_" + dest_);
}

void MOrderSummRead::endDay()
{
}

void MOrderSummRead::endMarket()
{
}

void MOrderSummRead::endJob()
{
}

void MOrderSummRead::set_ticker_list(int idate, string market)
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
		sprintf(cmd, "select %s from hfuniverse where idate = (select max(idate) from hfuniverse where idate <= %d %s) and inuniverse in %s %s ",
			dbSymbol.c_str(), idate, selMarket.c_str(), univs.c_str(), selMarket.c_str());

		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			selectedTickers.push_back(trim((*it)[0]));
	}
	sort(selectedTickers.begin(), selectedTickers.end());

	string cmd = (string)" select distinct (" + dbSymbol + ") "
		+ " from hfOrders o "
		+ " where idate = " + itos(idate)
		+ selMarket;
	vector<vector<string> > vvs;
	GODBC::Instance()->read(mto::hfo(market, idate), cmd, vvs);

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
	MEnv::Instance()->tickers = tickers_;
}

void MOrderSummRead::get_retON(string market, int idate)
{
	// Read close prices on idate.
	map<string, double> mClose;
	get_close(mClose, market, idate);

	// Read close prices on idate_next.
	int idate_next = nextClose(market, idate);
	map<string, double> mCloseNext;
	get_close(mCloseNext, market, idate_next);

	mRetON_.clear();
	for( map<string, double>::iterator it = mClose.begin(); it != mClose.end(); ++it )
	{
		string ticker = it->first;
		double p0 = it->second;
		if( p0 != 0. )
		{
			map<string, double>::iterator it2 = mCloseNext.find(ticker);
			if( it2 != mCloseNext.end() )
			{
				double p1 = it2->second;

				double ret = p1 / p0 - 1.;
				if( ret > -0.15 && ret < 0.15 )
					mRetON_[ticker] = ret;
			}
		}
	}
}

void MOrderSummRead::read_orders(vector<vector<string> >& vvo, int idate, string market, string ticker)
{
	vvo.clear();
	string sExch = mto::code(market);

	// Dollarvol-fillRat
	{
	}

	// Read Orders.
	if( false )
	{
		string selMarket = get_selMarket(market);

		string dbSymbol = "o.symbol";
		string groupExchange = "";
		if( mto::isInternational(market) )
		{
			dbSymbol = "o.exchange + ':' + o.symbol";
			groupExchange = ", o.exchange ";
		}

		string selTicker = ticker.empty() ? "" : (string)" and " + dbSymbol + " = '" + ticker + "'";

		string cmd = (string)" select " + dbSymbol + ", o.side, o.price, o.qty, sum(e.qty), o.feedMsecs, o.orderMsecs, min(e.eventMsecs), "
			+ " o.bidEx, o.bidsize, o.bid, o.ask, o.asksize, o.askEx, o.exectype, o.orderschedtype, min(e.fillType), min(o.onempred), min(o.tenmpred) "
			+ " from hfOrders o inner join hfOrderEvents e "
			+ " on o.orderID = e.orderID "
			+ " where o.idate = " + itos(idate)
			+ " and e.idate = " + itos(idate)
			+ " and e.eventType > 0 " // 0: reject 1: cancel 2: fill.
			+ selMarket + selTicker
			+ " group by o.symbol, o.orderID, o.side, o.price, o.qty, o.orderMsecs, o.feedMsecs, o.bidEx, o.bidsize, o.bid, o.ask, o.asksize, o.askEx, "
			+ " o.orderschedtype, o.exectype "
			+ groupExchange
			+ " order by o.orderMsecs ";
		//GODBC::Instance()->get(mto::hfo(market, idate))->ReadTable(cmd, &vvo);
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, vvo);
	}
	else
	{
		string selMarket = get_selMarket(market);

		string dbSymbol = "o.symbol";
		string groupExchange = "";
		if( mto::isInternational(market) )
		{
			dbSymbol = "o.exchange + ':' + o.symbol";
			groupExchange = ", o.exchange ";
		}

		string selTicker = ticker.empty() ? "" : (string)" and " + dbSymbol + " = '" + ticker + "'";

		string cmd = (string)" select " + dbSymbol + ", o.side, o.price, o.qty, e.qty, o.feedMsecs, o.orderMsecs, e.eventMsecs, "
			+ " o.bidEx, o.bidsize, o.bid, o.ask, o.asksize, o.askEx, o.exectype, o.orderschedtype, e.fillType, o.onempred, o.tenmpred, o.orderID "
			+ " from hfOrders o inner join hfOrderEvents e "
			+ " on o.orderID = e.orderID "
			+ " where o.idate = " + itos(idate)
			+ " and e.idate = " + itos(idate)
			+ " and e.eventType > 0 " // 0: reject 1: cancel 2: fill.
			+ selMarket + selTicker
			//+ " group by o.symbol, o.orderID, o.side, o.price, o.qty, o.orderMsecs, o.feedMsecs, o.bidEx, o.bidsize, o.bid, o.ask, o.asksize, o.askEx, "
			//+ " o.orderschedtype, o.exectype "
			//+ groupExchange
			+ " order by o.orderID ";

		vector<vector<string> > temp_vvo;
		//GODBC::Instance()->get(mto::hfo(market, idate))->ReadTable(cmd, &temp_vvo);
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, temp_vvo);

		vector<string> row;
		int N = temp_vvo.size();
		for( int i = 0; i < N; ++i )
		{
			if( row.empty() )
				row = temp_vvo[i];

			if( i == N - 1 )
			{
				vvo.push_back(row);
				break;
			}
			else
			{
				string next_orderID = temp_vvo[i + 1][19];
				string this_orderID = row[19];
				if( this_orderID == next_orderID )
				{
					int this_qty = atoi(row[4].c_str());
					int next_qty = atoi(temp_vvo[i + 1][4].c_str());
					row[4] = itos(this_qty + next_qty);
				}
				else
				{
					vvo.push_back(row);
					row.clear();
				}
			}

		}
	}
}

string MOrderSummRead::get_selMarket(string market)
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
		else if( "U" == region_ )
		{
			if( "US" == market )
				selMarket = "";
			else
				selMarket = (string)" and ( (o.side = 'B' and askEx = " + quote(itos(cDest)) + ") or ( o.side != 'B' and bidEx = " + quote(itos(cDest)) + ") ) ";
		}
		else if( "C" == region_ )
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
			else if( "all" == dest_ )
				selMarket = (string)" and o.exchange = " + quote(sExch);
			else
				selMarket = (string)" and o.exchange = " + quote(sExch) + " and dest = " + quote(itos(cDest));
		}
		else if( "U" == region_ )
		{
			if( "US" == market )
				selMarket = "";
			else if( routing_.empty() )
			{
				vector<string>& rtg = m_us_dest_routing_[cDest];
				if(rtg.size() == 1)
					selMarket = (string)" and routing = " + quote(rtg[0]) + " and dest = " + quote(itos(m_us_dbdest_[cDest]));
				else if(rtg.size() == 2)
					selMarket = (string)" and (routing = " + quote(rtg[0]) + " or routing = " + quote(rtg[1]) + ") and dest = " + quote(itos(m_us_dbdest_[cDest]));
			}
		}
		else if( "C" == region_ )
		{
			if( "CJ" == market )
			{
				if( "primary" == dest_ )
					selMarket = " and (dest = '84' or dest = '86')"; // 'T' and 'V'.
				else if( "all" == dest_ )
					selMarket = "";
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

void MOrderSummRead::select_orders(vector<vector<string> >& vvo, vector<MercatorOrder>& vMOrderBuy, vector<MercatorOrder>& vMOrderSell,
							string market, string ticker, int idate)
{
	// Orders Filled (partial or full) / Orders not filled.
	MercatorOrder prevOrder;
	for( vector<vector<string> >::iterator it = vvo.begin(); it != vvo.end(); ++it )
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
}

void MOrderSummRead::set_latency_offset(int idate, string market)
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
}

void MOrderSummRead::read_quote_info( vector<MercatorOrder>& vMOrder, string market, string ticker, string bs,
								TickSeries<QuoteInfo>& tsQ, TickSeries<OrderData>& tsO, TickSeries<TradeInfo>& tsT )
{
	int ntq = tsQ.NTicks();
	int ntt = tsT.NTicks();
	QuoteInfo quote;
	int mult = (bs == "buy") ? 1 : -1;
	int closeMsecs = mto::msecClose(market);
	double reton = mRetON_[ticker];

	for( vector<MercatorOrder>::iterator itb = vMOrder.begin(); itb != vMOrder.end(); ++itb )
	{
		int orderMsecs = itb->orderMsecs;
		if( itb->orderSchedType == 1 )
			q2oMax_ = 6000000;

		calc_returns(tsQ, orderMsecs, itb, mult, closeMsecs, reton);

		QuoteInfo theQuote;
		find_match(theQuote, tsQ, orderMsecs, itb, bs);

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
				if( quote.msecs - theQuote.msecs > 500 ) // Conservative cut.
					break;

				if( lastQuote.msecs < quote.msecs )
				{
					if( theQuote.msecs <= lastQuote.msecs )
					{
						if( "buy" == bs && lastQuote.ask - theQuote.ask > ltmb_ )
							itb->detMsecs = lastQuote.msecs;
						else if( "sell" == bs && theQuote.bid - lastQuote.bid > ltmb_ )
							itb->detMsecs = lastQuote.msecs;

						if( theQuote.msecs != lastQuote.msecs ) // If q2d is zero, do not break and look for another.
							break;
					}
				}
				lastQuote = quote;
			}
		}

		if( verbose_ == 2 )
		{
			char buf[400];
			sprintf(buf, "%12s %d %c %d %c %2d %d %3d %6d %8.2f %8.2f %6d %3d \n",
				ticker.c_str(), orderMsecs, itb->execType, itb->orderSchedType, itb->side, itb->quoteMatch, itb->isFilledIncl(),
				itb->bidEx, itb->bidsize, itb->bid, itb->ask, itb->asksize, itb->askEx);
			cout << buf;
		}

		// If matched and filled, find the match.
		if( itb->quoteMatch > 1 && itb->isFilledIncl() )
		{
			itb->quoteMsecs = theQuote.msecs;

			// Find the order match. Take the last snapshot of each time stamp.
			tsQ.StartRead();
			QuoteInfo lastQuote;
			for( int n = 0; n < ntq; ++n )
			{
				tsQ.Read(&quote);
				if( quote.msecs - theQuote.msecs < q2dMin_ ) // Too early, continue.
					continue;
				else if( quote.msecs - theQuote.msecs > q2dMin_ + 500 ) // Too late, break.
					break;

				if( "buy" == bs &&
					(quote.ask - theQuote.ask > ltmb_ || theQuote.askSize - quote.askSize == itb->qty || lastQuote.askSize - quote.askSize == itb->qty) )
				{
					itb->matMsecs = quote.msecs;
					break;
				}
				else if( "sell" == bs &&
					(theQuote.bid - quote.bid > ltmb_ || theQuote.bidSize - quote.bidSize == itb->qty || lastQuote.bidSize - quote.bidSize == itb->qty) )
				{
					itb->matMsecs = quote.msecs;
					break;
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
				if( trade.msecs > theMsecs && trade.msecs <= theMsecs + q2dMax_ )
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
				else if( trade.msecs > theMsecs + q2dMax_ )
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
}

void MOrderSummRead::calc_returns(TickSeries<QuoteInfo>& tsQ, int orderMsecs, vector<MercatorOrder>::iterator& itb, const int mult, int closeMsecs, double reton)
{
	// Returns.
	double mid60 = itb->price;
	double mid660 = itb->price;
	double mid4260 = itb->price;
	double midTarget = itb->price;
	double midEnd = itb->price;
	QuoteInfo quote;
	tsQ.StartRead();
	int ntq = tsQ.NTicks();
	for( int n = 0; n < ntq; ++n )
	{
		tsQ.Read(&quote);
		if( quote.msecs > closeMsecs )
			break;

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
			midEnd = mid;
		}
	}

	double retTarget = mult * (midTarget / itb->price - 1.) * basis_pts_;
	itb->gain = retTarget;
	itb->ret1 = (mid60 / itb->price - 1.) * basis_pts_;
	itb->ret10 = (mid4260 / mid60 - 1.) * basis_pts_;
	itb->retR = (midEnd / mid4260 - 1.) * basis_pts_;
	itb->retON = reton * basis_pts_;
}

void MOrderSummRead::find_match(QuoteInfo& theQuote, TickSeries<QuoteInfo>& tsQ, int orderMsecs, vector<MercatorOrder>::iterator& itb, string bs)
{
	int matchMsecMin = orderMsecs - q2oMax_ - driftMax_ + latency_offset_;
	int matchMsecMax = orderMsecs - q2oMin_ + driftMax_ + latency_offset_;

	multimap<int, QuoteInfo> mTightMM;
	multimap<int, QuoteInfo> mTightMP;
	multimap<int, QuoteInfo> mTightM;
	multimap<int, QuoteInfo> mLooseMM;
	multimap<int, QuoteInfo> mLooseMP;
	multimap<int, QuoteInfo> mLooseM;
	QuoteInfo quote;
	tsQ.StartRead();
	int ntq = tsQ.NTicks();
	for( int n = 0; n < ntq; ++n )
	{
		tsQ.Read(&quote);

		bool tightMsecMatch = matchMsecMin < quote.msecs && quote.msecs < matchMsecMax;
		bool looseMsecMatch = matchMsecMin - q2oExtra_ < quote.msecs && quote.msecs < matchMsecMax;
		bool outOfRange = quote.msecs > matchMsecMax;
		if( tightMsecMatch || looseMsecMatch )
		{

			bool askExMatch = exchangeMatch(quote.askEx, itb->askEx);
			bool bidExMatch = exchangeMatch(quote.bidEx, itb->bidEx);
			bool askPriceMatch = askExMatch && fabs(quote.ask - itb->ask) < ltmb_;
			bool bidPriceMatch = bidExMatch && fabs(quote.bid - itb->bid) < ltmb_;
			bool askSideMatch = askPriceMatch && quote.askSize == itb->asksize;
			bool bidSideMatch = bidPriceMatch && quote.bidSize == itb->bidsize;

			bool targetMatch = ("buy" == bs && askSideMatch) || ("sell" == bs && bidSideMatch);
			bool otherMatch = ("buy" == bs && bidSideMatch) || ("sell" == bs && askSideMatch);
			bool targetPriceMatch = ("buy" == bs && askPriceMatch) || ("sell" == bs && bidPriceMatch);
			bool otherPriceMatch = ("buy" == bs && bidPriceMatch) || ("sell" == bs && askPriceMatch);

			int dmsec = abs(quote.msecs - (orderMsecs - q2oMin_ + latency_offset_));
			if( tightMsecMatch )
			{
				if( targetMatch && otherMatch )
					mTightMM.insert(make_pair(dmsec, quote));
				else if( targetMatch && otherPriceMatch )
					mTightMP.insert(make_pair(dmsec, quote));
				else if( targetMatch )
					mTightM.insert(make_pair(dmsec, quote));
			}
			else if( looseMsecMatch )
			{
				if( targetMatch && otherMatch )
					mLooseMM.insert(make_pair(dmsec, quote));
				else if( targetMatch && otherPriceMatch )
					mLooseMP.insert(make_pair(dmsec, quote));
				else if( targetMatch )
					mLooseM.insert(make_pair(dmsec, quote));
			}
		}
		else if( outOfRange )
			break;
	}

	string market = MEnv::Instance()->market;

	// Select the matching quote.
	if( !mTightMM.empty() )
	{
		theQuote = mTightMM.begin()->second;
		itb->quoteMatch = 10;
	}
	else if( !mLooseMM.empty() )
	{
		theQuote = mLooseMM.begin()->second;
		itb->quoteMatch = 9;
	}
	else if( !mTightMP.empty() )
	{
		theQuote = mTightMP.begin()->second;
		itb->quoteMatch = 8;
	}
	else if( !mLooseMP.empty() )
	{
		theQuote = mLooseMP.begin()->second;
		itb->quoteMatch = 7;
	}
	else if( !mTightM.empty() )
	{
		theQuote = mTightM.begin()->second;
		itb->quoteMatch = 6;
	}
	else if( !mLooseM.empty() )
	{
		theQuote = mLooseM.begin()->second;
		itb->quoteMatch = 5;
	}
}

bool MOrderSummRead::exchangeMatch(int tickEx, int dbEx)
{
	return true;

	bool ret = false;
	string market = MEnv::Instance()->market;
	if( region_ == "U" )
		ret = dbEx == m_us_dbdest_[tickEx];
	else
		ret = dbEx == tickEx;
	return ret;
}

bool MOrderSummRead::is_valid( QuoteInfo& quote )
{
	bool ret = quote.bid > ltmb_ && quote.ask > ltmb_ && quote.bid < 10000000 && quote.ask < 10000000
		&& quote.ask > quote.bid && 2.0*(quote.ask - quote.bid)/(quote.ask + quote.bid) < 0.1;
	return ret;
}

int MOrderSummRead::get_price_change( double last, double now )
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

bool MOrderSummRead::valid_price( double price )
{
	bool valid = price > ltmb_ && price < 10000000;
	return valid;
}

void MOrderSummRead::get_o2x( vector<MercatorOrder>& vMOrder, int m1, int m2, double price, int priceDeteriorated )
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
}
