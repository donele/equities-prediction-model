#include <MFitting/MPseudoTradePrep.h>
#include <MFitting.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <MFramework.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GFee.h>
#include <map>
#include <string>
#include <numeric>
using namespace std;
using namespace gtlib;

MPseudoTradePrep::MPseudoTradePrep(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
do_auction_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("doAuction") )
		do_auction_ = conf.find("doAuction")->second == "true";
	if( conf.count("order_dir") )
	{
		pair<mmit, mmit> dirs = conf.equal_range("order_dir");
		for( mmit mi = dirs.first; mi != dirs.second; ++mi )
		{
			string dir = mi->second;
			bookDirs_.push_back(dir);
		}
	}
}

MPseudoTradePrep::~MPseudoTradePrep()
{}

void MPseudoTradePrep::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;
	tSources_.read(market_);

	if( mto::isInternational(market_) )
	{
		selMarket_ = " and (";
		vector<string> markets = MEnv::Instance()->markets;
		for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
		{
			string code = (*it).substr(1, 1);
			if( it == markets.begin() )
				selMarket_ += " market = '" + code + "' ";
			else
				selMarket_ += " or market = '" + code + "' ";
		}
		selMarket_ += ")";
	}
}

void MPseudoTradePrep::beginDay()
{
	mMsecQuote_.clear();
	mMid_.clear();
	mTicker2Uid_.clear();
	mUid2Ticker_.clear();

	int idate = MEnv::Instance()->idate;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	char cmd[1000];
	sprintf(cmd, "select %s, uniqueID from stockcharacteristics "
		" where idate = %d "
		" %s "
		" and uniqueID is not null ",
		mto::compTicker(market_).c_str(),
		idate,
		selMarket_.c_str());
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market_), cmd, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		string uid = trim((*it)[1]);
		mTicker2Uid_[ticker] = uid;
		mUid2Ticker_[uid] = ticker;
	}
}

void MPseudoTradePrep::beginTicker(const string& ticker)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
	if( quotes == 0 )
		quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "quote"));
	vector<double> vMid1s(linMod.n1sec);
	get_mid_series(vMid1s, quotes, linMod.openMsecs, linMod.closeMsecs, 0, true);

	map<int, QuoteInfo> m;
	for( vector<QuoteInfo>::const_iterator it = quotes->begin(); it != quotes->end(); ++it )
		m[it->msecs] = *it;

	{
		boost::mutex::scoped_lock lock(priv_mutex_);
		string uid = mTicker2Uid_[ticker];
		mMsecQuote_[uid] = m;
		mMid_[uid] = vMid1s;
	}
}

void MPseudoTradePrep::endTicker(const string& ticker)
{
}

void MPseudoTradePrep::endDay()
{
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;

	mClose_.clear();
	get_close_info(mClose_, markets, idate);
	MEvent::Instance()->add<map<string, CloseInfo> >("", "mClose", &mClose_);

	if( do_auction_ )
	{
		mAuc_.clear();
		mCAmsecs_.clear();
		mOAmsecs_.clear();
		get_auction_info(mAuc_, markets, idate);
		MEvent::Instance()->add<map<string, AuctionInfo> >("", "mAuc", &mAuc_);

		mBookClose_.clear();
		get_book_close(mBookClose_, markets, idate);
		MEvent::Instance()->add<map<string, Book> >("", "mBookClose", &mBookClose_);

		mBookOpen_.clear();
		get_book_nextopen(mBookOpen_, markets, idate);
		MEvent::Instance()->add<map<string, Book> >("", "mBookOpen", &mBookOpen_);

		if( debug_ )
		{
			char buf[2000];
			map<string, Book>::iterator mBookClose_end = mBookClose_.end();
			map<string, Book>::iterator mBookOpen_end = mBookOpen_.end();
			for( map<string, AuctionInfo>::iterator it = mAuc_.begin(); it != mAuc_.end(); ++it )
			{
				string uid = it->first;

				double ca_price = 0.;
				int ca_size = 0;
				double oa_price = 0.;
				int oa_size = 0;

				char completeClose = 'F';
				map<string, Book>::iterator itc = mBookClose_.find(uid);
				if( itc != mBookClose_end )
				{
					itc->second.get_auction(ca_price, ca_size);
					if( book_complete(itc->second) )
						completeClose = 'T';
				}

				char completeOpen = 'F';
				map<string, Book>::iterator ito = mBookOpen_.find(uid);
				if( ito != mBookOpen_end )
				{
					ito->second.get_auction(oa_price, oa_size);
					if( book_complete(ito->second) )
						completeOpen = 'T';
				}

				sprintf(buf, "%12s %7.2f %6d %7.2f %6d %c %7.2f %6d %c %7.2f %6d %6.2f %6.2f\n",
					uid.c_str(), it->second.CA_price, it->second.CA_size, it->second.OA_price, it->second.OA_size,
					completeClose, ca_price, ca_size, completeOpen, oa_price, oa_size,
					ca_price - it->second.CA_price, oa_price - it->second.OA_price);
				cout << buf;
			}
		}
	}

	MEvent::Instance()->add<map<string, string> >("", "mTicker2Uid", &mTicker2Uid_);
	MEvent::Instance()->add<map<string, string> >("", "mUid2Ticker", &mUid2Ticker_);
	MEvent::Instance()->add<map<string, map<int, QuoteInfo> > >("", "mMsecQuote", &mMsecQuote_);
	MEvent::Instance()->add<map<string, vector<double> > >("", "mMid", &mMid_);
}

void MPseudoTradePrep::endJob()
{
}

bool MPseudoTradePrep::book_complete(const Book& book)
{
	double price = 0.;
	int size = 0;
	book.get_auction(price, size);
	int intPrice = ceil(price * 10000 - 0.5);

	map<unsigned, unsigned> bidvol = book.get_bidvol();
	unsigned minBid = 0;
	if( !bidvol.empty() )
		minBid = bidvol.begin()->first;

	map<unsigned, unsigned> askvol = book.get_askvol();
	unsigned maxAsk = 0;
	if( !askvol.empty() )
		maxAsk = askvol.rbegin()->first;

	if( intPrice >= minBid && intPrice <= maxAsk )
		return true;
	return false;
}

void MPseudoTradePrep::get_close_info(map<string, CloseInfo>& mClose, const vector<string>& markets, int idate)
{
	char cmd[2000];
	sprintf(cmd, " select uniqueID, close_ from stockcharacteristics "
		" where idate = %d "
		" %s "
		" and uniqueID is not null ",
		idate,
		selMarket_.c_str());
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(markets[0]), cmd, vv);

	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string uid = trim((*it)[0]);
		double close = atof((*it)[1].c_str());
		mClose[uid] = CloseInfo(close, 0., 0.);
	}

	int idate_next = nextClose(markets[0], idate);
	char cmd2[2000];
	//sprintf(cmd2, " select uniqueID, open_ * adjust, close_ * adjust from stockcharacteristics "
	sprintf(cmd2, " select uniqueID, open_, close_, adjust from stockcharacteristics "
		" where idate = %d "
		" %s "
		" and uniqueID is not null ",
		idate_next,
		selMarket_.c_str());
	vector<vector<string> > vv2;
	GODBC::Instance()->read(mto::hf(markets[0]), cmd2, vv2);

	map<string, CloseInfo>::iterator itcEnd = mClose.end();
	for( auto it = vv2.begin(); it != vv2.end(); ++it )
	{
		string uid = trim((*it)[0]);
		double nextOpen = atof((*it)[1].c_str());
		double nextClose = atof((*it)[2].c_str());
		double adjust = atof((*it)[3].c_str());
		map<string, CloseInfo>::iterator itc = mClose.find(uid);
		if( itc != itcEnd && itc->second.close > 0. )
		{
			itc->second.retclop = adjust * nextOpen / itc->second.close - 1.;
			itc->second.retclcl = adjust * nextClose / itc->second.close - 1.;
			itc->second.adjust = adjust;
		}
	}
}

void MPseudoTradePrep::get_auction_info(map<string, AuctionInfo>& mAuc, const vector<string>& markets, int idate)
{
	for( auto it = markets.begin(); it != markets.end(); ++it )
	{
		// Read trades.
		string market = *it;
		int idateNext = nextClose(market, idate);
		bool longTicker = mto::longTicker(market);
		int openMsecs = mto::msecOpen(market, idate);
		int closeMsecs = mto::msecClose(market, idate);

		vector<string> dirs = tSources_.stocksdirectory(market, idate);
		TickAccessMulti<TradeInfo> ta;
		for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
			ta.AddRoot(*itd, longTicker);

		vector<string> tickers;
		ta.GetNames(idate, &tickers);

		vector<string> dirsNext = tSources_.stocksdirectory(market, idateNext);
		TickAccessMulti<TradeInfo> taNext;
		for( vector<string>::iterator itd = dirsNext.begin(); itd != dirsNext.end(); ++itd )
			taNext.AddRoot(*itd, longTicker);

		// Loop tickers
		for( vector<string>::iterator itt = tickers.begin(); itt != tickers.end(); ++itt)
		{
			string ticker = *itt;

			vector<TradeInfo> trades;
			vector<TradeInfo> tradesNext;
			get_trades(trades, ta, ticker, idate);
			get_trades(tradesNext, taNext, ticker, idateNext);

			// Get auction info.
			double dTemp = 0.;
			int iTemp = 0;
			double CA_price = 0.;
			int CA_size = 0;
			int CA_msecs = 0;
			double OA_price = 0.;
			int OA_size = 0;
			int OA_msecs = 0;
			get_auction(market, idate, ticker, trades, openMsecs, closeMsecs, dTemp, iTemp, CA_price, CA_size, iTemp, CA_msecs);
			get_auction(market, idateNext, ticker, tradesNext, openMsecs, closeMsecs, OA_price, OA_size, dTemp, iTemp, OA_msecs, iTemp);

			string uid = mTicker2Uid_[ticker];
			mAuc[uid] = AuctionInfo(CA_price, CA_size, OA_price, OA_size);
			mCAmsecs_[uid] = CA_msecs;
			mOAmsecs_[uid] = OA_msecs;
		}
	}
}

void MPseudoTradePrep::get_book_close(map<string, Book>& mBook, const vector<string>& markets, int idate)
{
	for( auto it = markets.begin(); it != markets.end(); ++it )
	{
		// Read trades.
		string market = *it;
		bool longTicker = mto::longTicker(market);
		int closeMsecs = mto::msecClose(market, idate);

		vector<string> dirs;
		if( !bookDirs_.empty() )
			dirs = bookDirs_;
		else
			dirs = tSources_.bookdirectory(market, idate);

		TickAccessMulti<OrderData> ta;
		for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
			ta.AddRoot(*itd, longTicker);

		vector<string> tickers;
		ta.GetNames(idate, &tickers);

		// Loop tickers
		for( vector<string>::iterator itt = tickers.begin(); itt != tickers.end(); ++itt)
		{
			string ticker = *itt;
			string uid = mTicker2Uid_[ticker];
			//int stopMsecs = mCAmsecs_[uid] - 30 * 1000;
			int stopMsecs = 0;
			if( "AS" == market )
				stopMsecs = closeMsecs + 10 * 60 * 1000;
			if( stopMsecs != 0 )
			{
				vector<OrderData> orders;
				get_orders(orders, ta, ticker, idate, 0);

				// Get Book just before the closing auction.
				Book book;
				for( vector<OrderData>::iterator ito = orders.begin(); ito != orders.end(); ++ito )
				{
					OrderData& order = *ito;
					if( order.msecs < stopMsecs )
						book.update(order);
					else
						break;
				}

				mBook[uid] = book;
			}
		}
	}
}

void MPseudoTradePrep::get_book_nextopen(map<string, Book>& mBook, const vector<string>& markets, int idate)
{
	for( auto it = markets.begin(); it != markets.end(); ++it )
	{
		// Read trades.
		string market = *it;
		int idateNext = nextClose(market, idate);
		bool longTicker = mto::longTicker(market);
		int openMsecs = mto::msecOpen(market, idateNext);

		vector<string> dirs = tSources_.bookdirectory(market, idateNext);
		TickAccessMulti<OrderData> ta;
		for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
			ta.AddRoot(*itd, longTicker);

		vector<string> tickers;
		ta.GetNames(idateNext, &tickers);

		// Loop tickers
		for( vector<string>::iterator itt = tickers.begin(); itt != tickers.end(); ++itt)
		{
			string ticker = *itt;
			string uid = mTicker2Uid_[ticker];
			//int stopMsecs = mOAmsecs_[uid] - 5 * 1000;
			int stopMsecs = 0;
			if( "AS" == market )
				stopMsecs = openMsecs - 10 * 60 * 1000;
			if( stopMsecs != 0 )
			{
				vector<OrderData> orders;
				get_orders(orders, ta, ticker, idateNext, openMsecs);

				// Get Book just before the closing auction.
				Book book;
				for( vector<OrderData>::iterator ito = orders.begin(); ito != orders.end(); ++ito )
				{
					OrderData& order = *ito;
					if( order.msecs < stopMsecs )
						book.update(order);
					else
						break;
				}

				mBook[uid] = book;
			}
		}
	}
}

void MPseudoTradePrep::get_trades(vector<TradeInfo>& trades, TickAccessMulti<TradeInfo>& ta, const string& ticker, int idate)
{
	TickSeries<TradeInfo> ts(200000, 1.2);
	ta.GetTickSeries( ticker, idate, &ts );
	int nTicks = ts.NTicks();
	trades.reserve(nTicks);
	ts.StartRead();
	TradeInfo trade;
	for( int i = 0; i < nTicks; ++i )
	{
		ts.Read(&trade);
		trades.push_back(trade);
	}
}

void MPseudoTradePrep::get_orders(vector<OrderData>& orders, TickAccessMulti<OrderData>& ta, const string& ticker, int idate, int maxMsecs)
{
	TickSeries<OrderData> ts(200000, 1.2);
	ta.GetTickSeries( ticker, idate, &ts );
	int nTicks = ts.NTicks();
	ts.StartRead();
	OrderData order;
	for( int i = 0; i < nTicks; ++i )
	{
		ts.Read(&order);
		if( maxMsecs == 0 || order.msecs < maxMsecs )
			orders.push_back(order);
	}
}

bool MPseudoTradePrep::TradeLess::operator()(const Trade& lhs, const Trade& rhs) const
{
	if( lhs.msecs < rhs.msecs )
		return true;
	else if( lhs.msecs == rhs.msecs && lhs.uid < rhs.uid )
		return true;
	return false;
}
