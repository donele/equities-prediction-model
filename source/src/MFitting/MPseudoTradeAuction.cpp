#include <MFitting/MPseudoTradeAuction.h>
#include <MFitting.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <map>
#include <string>
#include <numeric>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MPseudoTradeAuction::MPseudoTradeAuction(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
getAuctionFromBook_(false),
requireBookComplete_(false),
impact_(true),
smart_(false),
verbose_(0),
maxShrChg_(10),
maxShrChgAuc_(1000000),
restoreMax_(0.),
hold_(0),
addON_(false),
ONmult_(1),
minMsecs_(0),
maxMsecs_(86400000),
minMsecsON_(0),
maxMsecsON_(86400000),
thres_(0.),
thresON_(0.),
ntrdfail_(0),
fee_(0.),
maxPos_(0),
maxPosTicker_(0),
previous_position_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("getAuctionFromBook") )
		getAuctionFromBook_ = conf.find("getAuctionFromBook")->second == "true";
	if( conf.count("requireBookComplete") )
		requireBookComplete_ = conf.find("requireBookComplete")->second == "true";
	if( conf.count("impact") )
		impact_ = conf.find("impact")->second == "true";
	if( conf.count("smart") )
		smart_ = conf.find("smart")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());
	if( conf.count("thresON") )
		thresON_ = atof(conf.find("thresON")->second.c_str());
	if( conf.count("restoreMax") )
		restoreMax_ = atof(conf.find("restoreMax")->second.c_str());
	if( conf.count("addON") )
		addON_ = conf.find("addON")->second == "true";
	if( conf.count("predName") )
		predName_ = conf.find("predName")->second.c_str();
	if( conf.count("maxPos") )
		maxPos_ = atoi(conf.find("maxPos")->second.c_str());
	if( conf.count("hold") )
		hold_ = atoi(conf.find("hold")->second.c_str());
	if( conf.count("maxPosTicker") )
		maxPosTicker_ = atoi(conf.find("maxPosTicker")->second.c_str());
	if( conf.count("ONmult") )
		ONmult_ = atoi(conf.find("ONmult")->second.c_str());
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	if( conf.count("minMsecsON") )
		minMsecsON_ = atoi(conf.find("minMsecsON")->second.c_str());
	if( conf.count("maxMsecsON") )
		maxMsecsON_ = atoi(conf.find("maxMsecsON")->second.c_str());
}

MPseudoTradeAuction::~MPseudoTradeAuction()
{}

void MPseudoTradeAuction::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;

	fee_ = mto::fee_bpt(market_);

	string onLabel;
	if( impact_ )
		onLabel += "_impact";
	if( requireBookComplete_ )
		onLabel += "_complete";
	if( smart_ )
		onLabel += "_smart";
	if( !impact_ && !smart_ && !getAuctionFromBook_ )
		onLabel += "_auctrd";

	string onDesc = "";
	if( addON_ )
		onDesc = (string)"_ON" + itos(ONmult_);

	vector<int> idates = MEnv::Instance()->idates;
	int d1 = idates[0];
	int d2 = idates[idates.size() - 1];
	char buf[1000];
	sprintf(buf, "%s_r%.0f_h%d_%d_%d%s%s", predName_.c_str(), restoreMax_, hold_, d1, d2, onDesc.c_str(), onLabel.c_str());
	dir_ = buf;
	mkd(dir_);

	sprintf(buf, "%s\\%d_%d_%.1f", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_/*, thresON_*/);
	filename_ = xpf(buf);

	ofstream ofs((filename_ + ".txt").c_str());
	ofs << "idate     intra    clop     clcl     total    cumintra cumclop  cumclcl  cumtotal nt  dv      pos     apos    tpos   tposd  ntmaxpos ntrdfail\n";

	if( debug_ )
		ofstream( (filename_ + "_debug.txt").c_str() );
}

void MPseudoTradeAuction::beginDay()
{
	int idate = MEnv::Instance()->idate;
	nMin_ = (mto::msecClose(market_, idate) - mto::msecOpen(market_, idate)) / 1000 / 60;
	vPositionMin_ = vector<double>(nMin_, previous_position_);
	GCurr::Instance()->set_idate(idate);
	ntrdfail_ = 0;
}

void MPseudoTradeAuction::endDay()
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	int closeMsecs = mto::msecClose(market, idate);

	for( map<string, TickerPosition>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
		it->second.beginDay();
	for( map<string, AuctionTrade>::iterator it = mAucTrd_.begin(); it != mAucTrd_.end(); ++it )
		it->second.beginDay();

	ofstream ofsDebug;
	if( debug_ )
		ofsDebug.open( (filename_ + "_debug.txt").c_str(), ios::app );

	//AuctionSummary aucSum_;
	DaySum daysum;
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));
	const map<string, MPseudoTradePrep::AuctionInfo>* mAuc = static_cast<const map<string, MPseudoTradePrep::AuctionInfo>*>(MEvent::Instance()->get("", "mAuc"));
	const map<string, Book>* mBookClose = static_cast<const map<string, Book>*>(MEvent::Instance()->get("", "mBookClose"));
	const map<string, Book>* mBookOpen = static_cast<const map<string, Book>*>(MEvent::Instance()->get("", "mBookOpen"));
	const map<string, vector<MReadTickerState::TickerState> >* mTickerState = static_cast<const map<string, vector<MReadTickerState::TickerState> >*>(MEvent::Instance()->get(predName_, "mTickerState"));
	map<string, vector<MReadTickerState::TickerState> >::const_iterator mTickerStateEnd = mTickerState->end();

	// Loop ticker.
	for( map<string, vector<MReadTickerState::TickerState> >::const_iterator itt = mTickerState->begin(); itt != mTickerStateEnd; ++itt )
	{
		string uid = itt->first;
		map<string, CloseInfo>::const_iterator itc = mClose->find(uid);
		if( itc == mClose->end() )
			continue;
		TickerPosition& uidPos = mPos_[uid];

		string market = uid.substr(0, 2);
		double exchrat = GCurr::Instance()->exchrat("US", market);

		// Loop sampled data.
		int N = itt->second.size();
		for( int i = 0; i < N; ++i )
		{
			const MReadTickerState::TickerState& ts = itt->second[i];

			if( ts.msecs < minMsecs_ || ts.msecs > maxMsecs_ )
				continue;

			int minmin = (ts.msecs - linMod.openMsecs) / 1000 / 60;

			double mid = 0.;
			if( ts.bid > 0. && ts.ask > 0. )
				mid = (ts.bid + ts.ask) / 2.0 * exchrat; // forex

			double marketPos = vPositionMin_[minmin];
			double restoreForce = 0.;
			if( maxPosTicker_ > 0. )
				restoreForce = - (uidPos.adjpos(ts.msecs, hold_) * mid) / maxPosTicker_ * (restoreMax_ / basis_pts_);

			double price = 0.;
			int bs = 0;
			int nshare = 0;
			if( ts.pred + restoreForce > ts.cost + thres_ / basis_pts_ ) // buy.
			{
				price = ts.ask * exchrat; // forex
				bs = 1;
				nshare = min(maxShrChg_, ts.askSize);
			}
			else if( - (ts.pred + restoreForce) > ts.cost + thres_ / basis_pts_ ) // sell.
			{
				price = ts.bid * exchrat; // forex
				bs = -1;
				nshare = min(maxShrChg_, ts.bidSize);
			}

			if( maxPosTicker_ != 0. )
			{
				int headroom = (maxPosTicker_ - bs * uidPos.position * mid) / mid;
				if( headroom < 0 )
					headroom = 0;
				nshare = min(nshare, headroom);
			}
			if( maxPos_ != 0. )
			{
				int headroom = (maxPos_ - bs * marketPos) / mid;
				if( headroom < 0 )
					headroom = 0;
				nshare = min(nshare, headroom);
			}

			if( bs != 0 && nshare == 0 )
				++ntrdfail_;

			// trade.
			if( bs != 0 && mid > 0. && nshare != 0 )
			{
				const CloseInfo& ci = itc->second;
				double posChg = bs * nshare * mid;
				double price_close = itc->second.close * exchrat; // forex

				// pnl to close.
				double pnl = posChg * (price_close / mid - 1.) - abs(posChg * ts.cost);

				// pnl to pred.
				double pnlPred = posChg * ts.pred - abs(posChg * ts.cost);

				// position.
				uidPos.add(ts.msecs, nshare * bs);
				for( int m = minmin; m < nMin_; ++m )
					vPositionMin_[m] += nshare * bs * mid;

				(bs > 0) ? ++daysum.nbuy : ++daysum.nsell;
				daysum.dv += fabs(posChg);
				daysum.intra += pnl;
				daysum.pred += pnlPred;

				//if( debug_ )
				//{
				//	char buf[1000];
				//	sprintf(buf, "%d %-8s %8d %3d %7.2f %7.2f %8.4f\n", idate, uid.c_str(), ts.msecs, nshare * bs, price, ci.close, pnl);
				//	ofsDebug << buf;
				//}
			}

			// Auction trade.
			AuctionTrade& uidAuc = mAucTrd_[uid];
			bool last_data = i == N - 1;
			if( last_data )
			{
				auction_trade(mBookClose, mBookOpen, mAuc, exchrat, ts, mid, uidAuc, uidPos, closeMsecs, ofsDebug, uid);
			}
		}
	}

	aucSum_.print(moduleName_);
	aucSum_.endDay();
	market_close(daysum, ofsDebug, idate);
}

bool MPseudoTradeAuction::book_complete(Book& book)
{
	map<unsigned, unsigned> bidvol = book.get_bidvol();
	unsigned minBid = 0;
	if( !bidvol.empty() )
		minBid = bidvol.begin()->first;

	map<unsigned, unsigned> askvol = book.get_askvol();
	unsigned maxAsk = 0;
	if( !askvol.empty() )
		maxAsk = askvol.rbegin()->first;

	if( minBid == 0 || maxAsk == 0 || minBid > maxAsk )
		return false;
	return true;
}


void MPseudoTradeAuction::auction_trade(const map<string, Book>* mBookClose, const map<string, Book>* mBookOpen,
											const map<string, MPseudoTradePrep::AuctionInfo>* mAuc, double exchrat,
					const MReadTickerState::TickerState& ts, double mid, AuctionTrade& uidAuc, TickerPosition& uidPos, int closeMsecs,
					ofstream& ofsDebug, string uid)
{
	char buf[1000];
	//if( debug_ )
	//{
	//	sprintf(buf, "posClose %s %d %.1f\n", uid.c_str(), uidPos.position, uidPos.position * mid);
	//	ofsDebug << buf;
	//}

	// Check enough levels of the orderbook.
	if( requireBookComplete_ )
	{
		bool complete = true;
		map<string, Book>::const_iterator itbc = mBookClose->find(uid);
		if( itbc != mBookClose->end() )
		{
			Book book = itbc->second;
			complete = book_complete(book);
		}

		map<string, Book>::const_iterator itbo = mBookOpen->find(uid);
		if( itbo != mBookOpen->end() )
		{
			Book book = itbo->second;
			complete = book_complete(book);
		}
		if( !complete )
			return;
	}

	// Find auction price and size.
	double CA_price = 0.;
	int CA_size = 0;
	double OA_price = 0.;
	int OA_size = 0;
	find_auction(CA_price, CA_size, OA_price, OA_size, exchrat, uid, mBookClose, mBookOpen, mAuc);

	// closing auction trade.
	int CA_bs = 0;
	int CA_norder = 0;
	int CA_nshare = 0;
	{

		// Trade or not.
		if( ts.predON >= 0. ) // buy.
		{
			double limit_order_price = mid * (1. + ts.predON - fee_ / basis_pts_ - thresON_ / basis_pts_);

			if( smart_ )
				limit_order_price = get_smart_price(uid, 1, limit_order_price / exchrat, min(CA_norder, CA_size), exchrat, CA_price);
			if( limit_order_price > 0. )
			{
				// Determine the size.
				int headroom = 0;
				if( ts.predON >= 0. )
					headroom = (maxPosTicker_ - uidPos.position * mid) / mid;
				else if( ts.predON < 0. )
					headroom = (maxPosTicker_ + uidPos.position * mid ) / mid;
				if( headroom < 0 )
					headroom = 0;
				CA_norder = min(maxShrChgAuc_, headroom);
			}

			if( impact_ )
				get_auction_book_impact("close", uid, 1, limit_order_price / exchrat, CA_norder, exchrat, CA_price, CA_size);

			if( limit_order_price > 0. && limit_order_price >= CA_price )
			{
				CA_bs = 1;
				CA_nshare = min(CA_norder, CA_size);
			}
		}
		else if( ts.predON < 0. ) // sell.
		{
			double limit_order_price = mid * (1. + ts.predON + fee_ / basis_pts_ + thresON_ / basis_pts_);

			if( smart_ )
				limit_order_price = get_smart_price(uid, -1, limit_order_price / exchrat, min(CA_norder, CA_size), exchrat, CA_price);
			if( limit_order_price > 0. )
			{
				// Determine the size.
				int headroom = 0;
				if( ts.predON >= 0. )
					headroom = (maxPosTicker_ - uidPos.position * mid) / mid;
				else if( ts.predON < 0. )
					headroom = (maxPosTicker_ + uidPos.position * mid ) / mid;
				if( headroom < 0 )
					headroom = 0;
				CA_norder = min(maxShrChgAuc_, headroom);
			}

			if( impact_ )
				get_auction_book_impact("close", uid, -1, limit_order_price / exchrat, CA_norder, exchrat, CA_price, CA_size);

			if( limit_order_price > 0. && limit_order_price <= CA_price )
			{
				CA_bs = -1;
				CA_nshare = min(CA_norder, CA_size);
			}
		}

		if( CA_bs != 0 && CA_price > 0. && CA_nshare != 0 )
		{
			uidAuc.CA_price = CA_price;
			uidAuc.CA_dPos = CA_bs * CA_nshare;
			uidPos.add(closeMsecs, CA_nshare * CA_bs);
		}
	}

	//if( debug_ )
	//{
	//	sprintf(buf, "posMidnight %s %d %.1f %d %d\n", uid.c_str(), uidPos.position, uidPos.position * mid, CA_norder, CA_nshare);
	//	ofsDebug << buf;
	//}

	// opening auction trade.
	bool market_order = true;
	bool prefer_neutral = true;
	int OA_bs = 0;
	int OA_norder = 0;
	int OA_nshare = 0;
	{
		// Determine the size.
		int headroom = 0;
		if( ts.predON < 0. )
			headroom = (maxPosTicker_ - uidPos.position * mid) / mid;
		else if( ts.predON >= 0. )
			headroom = (maxPosTicker_ + uidPos.position * mid ) / mid;
		if( headroom < 0 )
			headroom = 0;
		OA_norder = min(maxShrChgAuc_, headroom);
		if( prefer_neutral )
			OA_norder = min(OA_norder, abs(uidPos.position));

		if( market_order )
		{
			// Trade or not.
			if( ts.predON < 0. && uidPos.position < 0 )
			{
				double limit_order_price = CA_price * 1.03;

				if( impact_ )
					get_auction_book_impact("open", uid, 1, limit_order_price / exchrat, OA_norder, exchrat, OA_price, OA_size);

				if( limit_order_price >= OA_price )
				{
					OA_bs = 1;
					OA_nshare = min(OA_norder, OA_size);
				}
			}
			else if( ts.predON >= 0. && uidPos.position > 0 )
			{
				double limit_order_price = CA_price * 0.97;

				if( impact_ )
					get_auction_book_impact("open", uid, -1, limit_order_price / exchrat, OA_norder, exchrat, OA_price, OA_size);

				if( limit_order_price <= OA_price )
				{
					OA_bs = -1;
					OA_nshare = min(OA_norder, OA_size);
				}
			}
		}
		else
		{
			// Trade or not.
			if( ts.predON < 0. && uidPos.position < 0 ) // if sold at previous closing, buy at the next opening.
			{
				double limit_order_price = CA_price * (1. - fee_ / basis_pts_ - thresON_ / basis_pts_);

				if( impact_ )
					get_auction_book_impact("open", uid, 1, limit_order_price / exchrat, OA_norder, exchrat, OA_price, OA_size);

				if( limit_order_price >= OA_price )
				{
					OA_bs = 1;
					OA_nshare = min(OA_norder, OA_size);
				}
			}
			else if( ts.predON >= 0. && uidPos.position > 0 )
			{
				double limit_order_price = CA_price * (1. + fee_ / basis_pts_ + thresON_ / basis_pts_);

				if( impact_ )
					get_auction_book_impact("open", uid, -1, limit_order_price / exchrat, OA_norder, exchrat, OA_price, OA_size);

				if( limit_order_price <= OA_price )
				{
					OA_bs = -1;
					OA_nshare = min(OA_norder, OA_size);
				}
			}
		}

		// Do trade.
		if( OA_bs != 0 && OA_price > 0. && OA_nshare != 0 )
		{
			uidAuc.OA_price = OA_price;
			uidAuc.OA_dPos = OA_bs * OA_nshare;
			uidPos.add(closeMsecs, OA_nshare * OA_bs);
		}
	}

	//if( debug_ )
	//{
	//	sprintf(buf, "posOpen %s %d %.1f %d %d\n", uid.c_str(), uidPos.position, uidPos.position * mid, OA_norder, OA_nshare);
	//	ofsDebug << buf;
	//}

	// Summary.
	++aucSum_.nTicker;
	if( CA_price > 0. )
		++aucSum_.nCA;
	if( OA_price > 0. )
		++aucSum_.nOA;
	if( OA_price > 0. && CA_price > 0. )
		++aucSum_.nBothAuc;
	aucSum_.CAShr += CA_size;
	aucSum_.OAShr += OA_size;
	aucSum_.CAVol += CA_size * CA_price;
	aucSum_.OAVol += OA_size * OA_price;
	aucSum_.CAShrOrder += CA_norder;
	aucSum_.CAShrFill += CA_nshare;
	aucSum_.OAShrOrder += OA_norder;
	aucSum_.OAShrFill += OA_nshare;
	aucSum_.CAVol += CA_size * CA_price;
	aucSum_.OAVol += OA_size * OA_price;
	aucSum_.CAVolOrder += CA_norder * mid;
	aucSum_.CAVolFill += CA_nshare * mid;
	aucSum_.OAVolOrder += OA_norder * mid;
	aucSum_.OAVolFill += OA_nshare * mid;
}

void MPseudoTradeAuction::find_auction(double& CA_price, int& CA_size, double& OA_price, int& OA_size, double exchrat, string uid,
	const map<string, Book>* mBookClose, const map<string, Book>* mBookOpen, const map<string, MPseudoTradePrep::AuctionInfo>* mAuc)
{
	// Get auction price and size.
	if( getAuctionFromBook_ )
	{
		map<string, Book>::const_iterator itbc = mBookClose->find(uid);
		if( itbc != mBookClose->end() )
		{
			Book book = itbc->second;
			book.get_auction(CA_price, CA_size);
			CA_price *= exchrat;
		}

		map<string, Book>::const_iterator itbo = mBookOpen->find(uid);
		if( itbo != mBookOpen->end() )
		{
			Book book = itbo->second;
			book.get_auction(OA_price, OA_size);
			OA_price *= exchrat;
		}
	}
	else
	{
		{
		map<string, MPseudoTradePrep::AuctionInfo>::const_iterator ita = mAuc->find(uid);
		if( ita != mAuc->end() )
			CA_price = ita->second.CA_price * exchrat;
			CA_size = ita->second.CA_size;
			OA_price = ita->second.OA_price * exchrat;
			OA_size = ita->second.OA_size;
		}
	}
}

void MPseudoTradeAuction::get_auction_book_impact(string oc, string uid, int bs, double order_price, int order_size, double exchrat, double& auction_price, int& auction_size)
{
	const map<string, Book>* mBook;
	if( oc == "close" )
		mBook = static_cast<const map<string, Book>*>(MEvent::Instance()->get("", "mBookClose"));
	else if( oc == "open" )
		mBook = static_cast<const map<string, Book>*>(MEvent::Instance()->get("", "mBookOpen"));

	map<string, Book>::const_iterator itb = mBook->find(uid);
	if( itb != mBook->end() )
	{
		Book book = itb->second;

		if( auction_price == 0. || book_complete(book) )
		{
			OrderData order;
			order.price = order_price * 10000;
			order.size = order_size;
			order.type = (bs == 1 ) ? 0 : 2;
			book.update(order);

			book.get_auction(auction_price, auction_size);
			auction_price *= exchrat;
		}
	}
}

double MPseudoTradeAuction::get_smart_price(string uid, int bs, double order_price, int order_size, double exchrat, double auction_price)
{
	const map<string, Book>* mBook = static_cast<const map<string, Book>*>(MEvent::Instance()->get("", "mBookClose"));
	map<string, Book>::const_iterator itb = mBook->find(uid);
	if( itb != mBook->end() )
	{
		Book book = itb->second;

		if( book_complete(book) )
		{
			int i_auction_price = ceil(auction_price * 10000 - 0.5);
			for( int iter_price = ceil(order_price * 10000 - 0.5); iter_price >= bs * i_auction_price; iter_price -= 10)
			{
				OrderData order;
				//order.price = order_price * 10000;
				//order.size = order_size;
				order.price = iter_price;
				order.size = order_size;
				order.type = (bs == 1 ) ? 0 : 2;

				book.update(order);

				double new_price = 0.;
				int new_size = 0;
				book.get_auction(new_price, new_size);

				if( new_price <= bs * i_auction_price )
					return iter_price / 10000. * exchrat;

				order.type += 1;
				book.update(order);
			}
		}
	}
	return 0.;
}

void MPseudoTradeAuction::market_close(DaySum& daysum, ofstream& ofsDebug, int idate)
{
	// Loop ticker, do Clcl.
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	Acc accPos;
	double eodPos = 0.;
	double eodAbsPos = 0.;
	int ntmaxpos = 0;
	for( map<string, TickerPosition>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
	{
		map<string, CloseInfo>::const_iterator itc = mClose->find(it->first);
		if( itc != itcEnd )
		{
			string uid = it->first;
			string market = uid.substr(0, 2);
			double exchrat = GCurr::Instance()->exchrat("US", market);
			double price_close = itc->second.close * exchrat; // forex
			double price_open = price_close * (1. + itc->second.retclop);
			double price_next_close = price_close * (1. + itc->second.retclcl);

			// Auction trade.
			AuctionTrade& uidAuc = mAucTrd_[uid];
			double cost = (fee_ / basis_pts_) * (abs(uidAuc.CA_dPos * uidAuc.CA_price) + abs(uidAuc.OA_dPos * uidAuc.OA_price));
			double Auc_clop = uidAuc.CA_dPos * (price_open - uidAuc.CA_price) + uidAuc.OA_dPos * (price_open - uidAuc.OA_price) - cost;
			double Auc_clcl = uidAuc.CA_dPos * (price_next_close - uidAuc.CA_price) + uidAuc.OA_dPos * (price_next_close - uidAuc.OA_price) - cost;

			//cout << uid << "\t" << Auc_clop << endl;
			if( debug_ )
			{
				char buf[200];
				sprintf(buf, "%s %8.3f %8.3f %8.3f %8.3f\n", uid.c_str(), uidAuc.CA_price, uidAuc.OA_price, price_close, Auc_clop);
				ofsDebug << buf;
			}

			// Clop and Clcl.
			double clop = price_close * it->second.position * itc->second.retclop;
			double clcl = price_close * it->second.position * itc->second.retclcl;

			corrON_.add(itc->second.retclop, itc->second.retclcl - itc->second.retclop);

			daysum.clop += clop + Auc_clop;
			daysum.clcl += clcl + Auc_clcl;

			if( it->second.position != 0 )
			{
				double tickerpos = it->second.position * price_close;
				double abspos = abs(tickerpos);
				accPos.add(abspos);
				eodPos += tickerpos;
				eodAbsPos += abspos;

				if( abs(maxPosTicker_ - abspos) <= (1.1 * price_close) )
					++ntmaxpos;
			}
		}
	}
	previous_position_ = eodPos;
	
	accIntra_.add(daysum.intra);
	accClcl_.add(daysum.clcl);
	accClop_.add(daysum.clop);
	accNtrd_.add(daysum.nbuy + daysum.nsell);
	accDV_.add(daysum.dv);

	accTotal_.add(daysum.intra + daysum.clcl);

	// print.
	double gpt = 0.;
	if( daysum.dv > 0. )
		gpt = daysum.intra / daysum.dv * 10000.;

	//if( debug_ )
	//{
	//	char buf[200];
	//	for( map<string, AuctionTrade>::iterator it = mAucTrd_.begin(); it != mAucTrd_.end(); ++it )
	//	{
	//		sprintf(buf, "%s %8.3f %8.3f\n", it->first.c_str(), it->second.CA_price, it->second.OA_price);
	//		ofsDebug << buf;
	//	}
	//}

	if( verbose_ > 0 )
	{
		char buf[1000];
		sprintf(buf, "%d %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f %4d %.1f %.1f %.1f %.1f %.1f %d %d",//       pos %.1f nt %d dv %.1f gpt %.1f tpos %.1f %.1f",
			idate,
			daysum.intra, daysum.clop, daysum.clcl, daysum.intra + daysum.clcl,
			accIntra_.sum, accClop_.sum, accClcl_.sum, accIntra_.sum + accClcl_.sum,
			daysum.nbuy + daysum.nsell, daysum.dv,
			previous_position_, eodAbsPos, accPos.mean(), accPos.stdev(), ntmaxpos,
			ntrdfail_);
		ofstream ofs((filename_ + ".txt").c_str(), ios::app);
		ofs << buf << endl;
	}
}

void MPseudoTradeAuction::endJob()
{
	cout << "Corr clop opcl " << corrON_.corr() << endl;
	aucSum_.endJob(moduleName_);

	char buf[1000];
	sprintf(buf, "%s\\summ_%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
	ofstream ofs(xpf(buf).c_str());
	sprintf(buf, "maxPosTicker %.1f maxPos %.1f\n", maxPosTicker_, maxPos_);
	ofs << buf;
	sprintf(buf, "      intra     clop     clcl     total    ntrd    dv          gpt\n");
	ofs << buf;

	double gpt = 0.;
	if( accDV_.mean() > 0. )
		gpt = accIntra_.mean() / accDV_.mean() * basis_pts_;
	sprintf(buf, "mean  %7.1f %8.1f %8.1f %8.1f %7.1f %7.1f %7.1f\n",
		accIntra_.mean(), accClop_.mean(), accClcl_.mean(), accTotal_.mean(), accNtrd_.mean(), accDV_.mean(),
		gpt);
	ofs << buf;
	sprintf(buf, "stdev %7.1f %8.1f %8.1f %8.1f %7.1f %7.1f\n",
		accIntra_.stdev() / sqrt(accIntra_.n),
		accClop_.stdev() / sqrt(accClop_.n), accClcl_.stdev() / sqrt(accClcl_.n), accTotal_.stdev() / sqrt(accTotal_.n),
		accNtrd_.stdev() / sqrt(accNtrd_.n), accDV_.stdev() / sqrt(accDV_.n));
	ofs << buf;
}

//void MPseudoTradeAuction::TickerPosition::add(int msecs, int shares)
//{
//	if( (shares > 0 && position >= 0) || (shares < 0 && position <= 0) ) // accumulate the position.
//	{
//		position += shares;
//		mMsecsPos[msecs] += shares;
//	}
//	else if( (shares > 0 && position < 0) || (shares < 0 && position > 0) ) //
//	{
//		position += shares;
//		int remain = shares;
//		for( map<int, int>::iterator it = mMsecsPos.begin(); it != mMsecsPos.end() && remain != 0; )
//		{
//			if( (it->second > 0 && remain < 0) || (it->second < 0 && remain > 0) ) // opposite sign.
//			{
//				int abs_remain = abs(remain);
//				int abs_position = abs(it->second);
//				if( abs_remain >= abs_position )
//				{
//					remain += it->second;
//					mMsecsPos.erase(it++);
//				}
//				else
//				{
//					it->second += remain;
//					remain = 0;
//					++it;
//				}
//			}
//			else
//				exit(7);
//		}
//
//		if( remain != 0 )
//			mMsecsPos[msecs] = remain;
//	}
//}
//
//int MPseudoTradeAuction::TickerPosition::adjpos(int msecs, int hold)
//{
//	int pos = 0;
//	int maxMsecs = msecs - hold * 60 * 1000;
//	for( map<int, int>::iterator it = mMsecsPos.begin(); it != mMsecsPos.end(); ++it )
//	{
//		if( it->first <= maxMsecs )
//			pos += it->second;
//		else
//			break;
//	}
//	return pos;
//}
//
//void MPseudoTradeAuction::TickerPosition::beginDay()
//{
//	int lastPos = position;
//	mMsecsPos.clear();
//	if( lastPos != 0 )
//		mMsecsPos[0] = lastPos;
//}

void MPseudoTradeAuction::AuctionTrade::beginDay()
{
	CA_price = 0.;
	CA_dPos = 0.;
	OA_price = 0.;
	OA_dPos = 0.;
}

void MPseudoTradeAuction::AuctionSummary::print(string moduleName)
{
	boost::mutex::scoped_lock lock(MEnv::Instance()->io_mutex);
	char buf[1000];
	sprintf(buf, "%s nT %d nCA %d nOA %d\n", moduleName.c_str(), nTicker, nCA, nOA);
	cout << buf;
	sprintf(buf, "   %7s %7s %7s %7s %7s\n", "Vol(M)", "Ord(M)", "Fill(M)", "Fill/Vol", "Fill/Ord");
	cout << buf;
	sprintf(buf, "CA %7.1f %7.3f %7.3f %7.5f %7.3f\n", CAVol/1000000., CAVolOrder/1000000., CAVolFill/1000000., CAVolFill/CAVol, CAVolFill/CAVolOrder);
	cout << buf;
	sprintf(buf, "OA %7.1f %7.3f %7.3f %7.5f %7.3f\n", OAVol/1000000., OAVolOrder/1000000., OAVolFill/1000000., OAVolFill/OAVol, OAVolFill/OAVolOrder);
	cout << buf;
	cout << endl;
}

void MPseudoTradeAuction::AuctionSummary::endDay()
{
	intraVolTot += intraVol;
	CAVolTot += CAVol;
	OAVolTot += OAVol;
	CAVolOrderTot += CAVolOrder;
	CAVolFillTot += CAVolFill;
	OAVolOrderTot += OAVolOrder;
	OAVolFillTot += OAVolFill;

	nTicker = 0;
	nCA = 0;
	nOA = 0;
	nBothAuc = 0;
	intraShr = 0;
	CAShr = 0;
	OAShr = 0;
	intraVol = 0.;
	CAVol = 0.;
	OAVol = 0.;
	CAShrOrder = 0;
	CAShrFill = 0;
	OAShrOrder = 0;
	OAShrFill = 0;
	CAVolOrder = 0.;
	CAVolFill = 0.;
	OAVolOrder = 0.;
	OAVolFill = 0.;
}

void MPseudoTradeAuction::AuctionSummary::endJob(string moduleName)
{
	boost::mutex::scoped_lock lock(MEnv::Instance()->io_mutex);
	char buf[1000];
	sprintf(buf, "%s endJob()\n", moduleName.c_str());
	cout << buf;
	sprintf(buf, "   %7s %7s %7s %7s %7s\n", "Vol(M)", "Ord(M)", "Fill(M)", "Fill/Vol", "Fill/Ord");
	cout << buf;
	sprintf(buf, "CA %7.1f %7.3f %7.3f %7.5f %7.3f\n", CAVolTot/1000000., CAVolOrderTot/1000000., CAVolFillTot/1000000., CAVolFillTot/CAVolTot, CAVolFillTot/CAVolOrderTot);
	cout << buf;
	sprintf(buf, "OA %7.1f %7.3f %7.3f %7.5f %7.3f\n", OAVolTot/1000000., OAVolOrderTot/1000000., OAVolFillTot/1000000., OAVolFillTot/OAVolTot, OAVolFillTot/OAVolOrderTot);
	cout << buf;
	cout << endl;
}
