#include <MOrders/MOrderAna.h>
#include <MFramework.h>
#include <optionlibs/TickData.h>
#include <jl_lib/GFee.h>
#include <jl_lib.h>
#include <cnpy.h>
#include <map>
#include <string>
using namespace std;

MOrderAna::MOrderAna(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
verbose_(0),
insert_check_(true),
write_hist_(true),
write_hist_total_(true),
exper_univ_(false),
profitable_orders_only_(false),
pos_sprd_(false),
composite_state_only_(false),
dest_("primary"),
fakeMsecsMax_(3),
matchMin_(2),
matchMax_(100),
priceMin_(0.),
priceMax_(max_double_),
q2iMax_(1000),
q2tMax_(1000),
msecDirect_(1000),
histMax_(10000),
exchrat_(0.),
side_('\0'), // (B)uy, (S)ell.
execType_('B'), // (B)oth, (L), (A).
orderSchedType_(0) // 0: all. 1: scheduled. 2: asymm.
{
	if( conf.count("insertCheck") )
		insert_check_ = conf.find("insertCheck")->second == "true";
	if( conf.count("writeHist") )
		write_hist_ = conf.find("writeHist")->second == "true";
	if( conf.count("experUniv") )
		exper_univ_ = conf.find("experUniv")->second == "true";
	if( conf.count("profitableOrdersOnly") )
		profitable_orders_only_ = conf.find("profitableOrdersOnly")->second == "true";
	if( conf.count("posSprd") )
		pos_sprd_ = conf.find("posSprd")->second == "true";
	if( conf.count("compositeStateOnly") )
		composite_state_only_ = conf.find("compositeStateOnly")->second == "true";
	if( conf.count("dest") )
		dest_ = conf.find("dest")->second;
	if( conf.count("side") )
		side_ = conf.find("side")->second[0];
	if( conf.count("execType") )
		execType_ = conf.find("execType")->second[0];
	if( conf.count("orderSchedType") )
		orderSchedType_ = atoi( conf.find("orderSchedType")->second.c_str() );
	if( conf.count("matchMin") )
		matchMin_ = atoi( conf.find("matchMin")->second.c_str() );
	if( conf.count("matchMax") )
		matchMax_ = atoi( conf.find("matchMax")->second.c_str() );
	if( conf.count("priceMin") )
		priceMin_ = atof( conf.find("priceMin")->second.c_str() );
	if( conf.count("priceMax") )
		priceMax_ = atof( conf.find("priceMax")->second.c_str() );
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
}

MOrderAna::~MOrderAna()
{}

void MOrderAna::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	histTotal_.init(histMax_, "All", fakeMsecsMax_);
	return;
}

void MOrderAna::beginMarket()
{
	string market = MEnv::Instance()->market;
	exchange_ = mto::code(market);
	destCode_ = dest_.substr(1,1);
	if( "primary" == dest_ )
		destCode_ = exchange_;
	string region = mto::region(market);

	if( region == "U" )
	{
		q2iMax_ = 10;
		msecDirect_ = 10;
		histMax_ = 2000;
		fakeMsecsMax_ = 0;
	}
	else if( region == "A" )
	{
		insert_check_ = false; // The information is not available from the database.

		if( "AT" == market )
		{
			const vector<int>& idates = MEnv::Instance()->idates;
			if( !idates.empty() )
			{
				int idate0 = idates[0];
				if( idate0 < 2010000 )
				{
					q2iMax_ = 10000;
					msecDirect_ = 10000;
					histMax_ = 100000;
				}
			}
		}
		else if( "AH" == market )
		{
			q2iMax_ = 3000;
			msecDirect_ = 10000;
		}
		else if( "AS" == market )
		{
			q2iMax_ = 1500;
			msecDirect_ = 1500;
		}
		else if( "AK" == market || "AQ" == market )
		{
			q2iMax_ = 10000;
			msecDirect_ = 10000;
			histMax_ = 100000;
		}
		else if( "AW" == market )
		{
			q2iMax_ = 10000;
			msecDirect_ = 10000;
			histMax_ = 500000;
		}
	}
	else if( "SS" == market )
	{
		insert_check_ = false;
		q2iMax_ = 150;
		fakeMsecsMax_ = 5;
	}

	perfSetMarket_.reset();
	perfMarket_.reset();
	histMarket_.init(histMax_, market, fakeMsecsMax_);

	return;
}

void MOrderAna::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;

	if( "U" == mto::region(market) )
	{
		if( idate < 20110401 )
			q2tMax_ = 5;
		else
			q2tMax_ = 2;
	}

	countDay_.reset();
	perfDay_.reset();

	GCurr::Instance()->set_idate(idate);
	exchrat_ = GCurr::Instance()->exchrat("US", market);
	//exchrat_ = 1.;

	GODBC::Instance()->close_all();

	return;
}

void MOrderAna::beginTicker(const string& ticker)
{
	if( exper_univ_ && !sTickers_.count(ticker) )
		return;

	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;

	const vector<MercatorOrder>* vMOrderBuy
		= static_cast<const vector<MercatorOrder>*>(MEvent::Instance()->get(ticker, (string)"vMOrderBuy_" + dest_));
	const vector<MercatorOrder>* vMOrderSell
		= static_cast<const vector<MercatorOrder>*>(MEvent::Instance()->get(ticker, (string)"vMOrderSell_" + dest_));

	if( vMOrderBuy != 0 || vMOrderSell != 0 )
	{
		vector<MercatorOrder> orders;
		select_orders( orders, vMOrderBuy, ticker );
		select_orders( orders, vMOrderSell, ticker );

		if( !orders.empty() )
		{
			fill_hist( histMarket_, orders, ticker );
			if( write_hist_total_ )
				fill_hist( histTotal_, orders, ticker );
			update_fillrat( orders );
			update_nOrders( orders, ticker );
		}
	}

	return;
}

void MOrderAna::endTicker(const string& ticker)
{
	return;
}

void MOrderAna::endDay()
{
	int idate = MEnv::Instance()->idate;
	if( verbose_ == 2 )
	{
		cout << moduleName_ << "\t" << idate << endl;
		perfDay_.print();
	}

	histMarket_.add_count(countDay_);
	histTotal_.add_count(countDay_);

	perfSetMarket_.add(idate, perfDay_);
	perfSetTotal_.add(idate, perfDay_);
	perfMarket_ += perfDay_;
	perfTotal_ += perfDay_;

	return;
}

void MOrderAna::endMarket()
{
	cout << "------------------------------------------------" << moduleName_ << "\n";
	cout << " > dest = " << destCode_ << "\n";
	cout << " > exchange = " << exchange_ << "\n";
	cout << " > exectype = " << execType_ << "\n";
	cout << " > schedtype = " << orderSchedType_ << "\n";

	perfSetMarket_.print();
	//perfMarket_.print();
	//histMarket_.print_summary();
	if( write_hist_ )
		histMarket_.write_hist(moduleName_, dest_, execType_, orderSchedType_);

	return;
}

void MOrderAna::endJob()
{
	if( MEnv::Instance()->markets.size() > 1 )
	{
		cout << "------------------------------------------------\n";
		cout << " > dest = " << dest_ << "\n";
		cout << " > exectype = " << execType_ << "\n";
		cout << " > schedtype = " << orderSchedType_ << "\n";

		perfSetTotal_.print();
		//perfTotal_.print();
		//histTotal_.print_summary();
		if( write_hist_total_ )
			histTotal_.write_hist(moduleName_, dest_, execType_, orderSchedType_);
	}
}

void MOrderAna::select_orders( vector<MercatorOrder>& orders, const vector<MercatorOrder>* vMOrder, const string& ticker )
{
	string& market = MEnv::Instance()->market;
	for( vector<MercatorOrder>::const_iterator it = vMOrder->begin(); it != vMOrder->end(); ++it )
	{
		bool select = false;
		if( execType_ == 'B' || execType_ == it->execType )
		{
			if( orderSchedType_ == 0 || orderSchedType_ == it->orderSchedType )
			{
				if( side_ == '\0' || side_ == it->side )
				{
					if( priceMin_ <= it->price && it->price <= priceMax_ )
					{
						double fee = GFee::Instance().feeBpt(market, ExecFeesPrimex(market, ticker), it->price);
						if( !profitable_orders_only_
							|| (it->side == 'B' && (it->pred1 + it->pred10 - fee * (it->execType == 'L' ? 1. : 0.)) > 0)
						|| (it->side == 'S' && (-it->pred1 - it->pred10 - fee * (it->execType == 'L' ? 1. : 0.)) > 0.)
						)
						{
							if( !pos_sprd_ || it->ask > it->bid )
							{
								if( !composite_state_only_ || it->bidEx != it->askEx )
								{
									select = true;
								}
							}
						}
					}
				}
			}
		}
		if( select )
			orders.push_back(*it);
	}
	return;
}

void MOrderAna::fill_hist( HistSet& histset, vector<MercatorOrder>& orders, string ticker )
{
	string& market = MEnv::Instance()->market;
	for( vector<MercatorOrder>::iterator it = orders.begin(); it != orders.end(); ++it )
	{
		double fee = 0.;
		if(it->execType == 'L')
			fee = GFee::Instance().feeBpt(market, ExecFeesPrimex(market, ticker), it->price);
		//double fee = fee_bpt_ * (it->execType == 'L' ? 1. : 0.);
		double gpt = it->gain - fee;
		double tradePrice = it->price * exchrat_;
		double pnl = tradePrice * it->qtyExec * (it->gain - fee) / 10000.;
		double pred = (it->side == 'B') ? it->pred1 + it->pred10 - fee : -it->pred1 - it->pred10 - fee;
		int f2o = it->orderMsecs - it->feedMsecs;
		int filled = it->isFilledIncl()?1:0;
		int sign = (it->side == 'B') ? 1 : -1;
		int matched = 0;
		if( matchMin_ <= it->quoteMatch && it->quoteMatch <= matchMax_ )
			matched = 1;

		// Matched orders
		int q2o = 24*60*60*1000;
		int o2e = 24*60*60*1000;
		int q2m = 24*60*60*1000;
		int q2d = 24*60*60*1000;
		int q2t = 24*60*60*1000;
		int q2i = 24*60*60*1000;
		int i2x = 24*60*60*1000;
		int d2i = 24*60*60*1000;
		if(matched == 1)
		{
			if( it->orderMsecs > 0 && it->quoteMsecs > 0 )
				q2o = it->orderMsecs - it->quoteMsecs;
			if( it->eventMsecs > 0 && it->orderMsecs > 0 )
				o2e = it->eventMsecs - it->orderMsecs;
			if( it->matMsecs > 0 && it->quoteMsecs > 0 )
				q2m = it->matMsecs - it->quoteMsecs;
			if( it->detMsecs > 0 && it->quoteMsecs > 0 )
				q2d = it->detMsecs - it->quoteMsecs;
			if( it->trdMsecs > 0 && it->quoteMsecs > 0 )
				q2t = it->trdMsecs - it->quoteMsecs;
			if( it->insertMsecs > 0 && it->quoteMsecs > 0 )
				q2i = it->insertMsecs - it->quoteMsecs;
			if( it->executeMsecs > 0 && it->insertMsecs > 0 )
				i2x = it->executeMsecs - it->insertMsecs;
			if( it->detMsecs > 0 && it->quoteMsecs > 0 && it->insertMsecs > 0 )
				d2i = q2i - q2d;
		}

		bool inserted = insert_check_ ? (it->fillType == 2 || it->fillType == 4) : (fakeMsecsMax_ < q2i && q2i < q2iMax_);
		int insert = inserted ? 1 : 0;
		int fill = it->isFilledIncl() ? 1 : 0;
		int directfill = fill && !insert ? 1 : 0;

		boost::mutex::scoped_lock lock(hist_mutex_);

		histset.v_matched.push_back(matched);
		histset.v_fill.push_back(fill);
		histset.v_q2d.push_back(q2d);
		histset.v_o2e.push_back(o2e);
		histset.v_gpt.push_back(gpt);
		if( 0 )
		{
			histset.v_insert.push_back(insert);
			histset.v_directfill.push_back(directfill);
			histset.v_msecs.push_back(it->orderMsecs);
			histset.v_pnl.push_back(pnl);

			// Prediction.
			histset.v_pred.push_back(it->pred1 + it->pred10);
			histset.v_pred1.push_back(it->pred1);
			histset.v_pred10.push_back(it->pred10);
			histset.v_ret.push_back(it->pred1 + it->pred10);
			histset.v_ret1.push_back(it->pred1);
			histset.v_ret10.push_back(it->pred10);
			histset.v_retT.push_back(it->gain * sign);
			histset.v_retR.push_back(it->gain * sign - (it->ret1 + it->ret10));

			histset.v_q2i.push_back(q2i);
			histset.v_q2m.push_back(q2m);
			histset.v_q2t.push_back(q2t);
			histset.v_q2o.push_back(q2o);
			histset.v_i2x.push_back(i2x);
		}
	}
	return;
}

void MOrderAna::update_fillrat( vector<MercatorOrder>& orders )
{
	for( vector<MercatorOrder>::const_iterator it = orders.begin(); it != orders.end(); ++it )
	{
		double tradePrice = it->price * exchrat_;
		double qty = it->qty;
		double qtyExec = it->qtyExec;

		boost::mutex::scoped_lock lock(fillrat_mutex_);

		countDay_.nShr_total += it->qty;
		countDay_.nShr_filled += it->qtyExec;

		if( it->isFilledFull() )
			++countDay_.nFilledFull;

		if( it->isFilledIncl() )
			++countDay_.nFilledIncl;

		++countDay_.nTotal;
		countDay_.dvFilled += qtyExec * tradePrice;
		countDay_.dvTotal += qty * tradePrice;

		if( matchMin_ <= it->quoteMatch && it->quoteMatch <= matchMax_ )
		{
			int q2d = 24*60*60*1000;
			if( it->detMsecs > 0 && it->quoteMsecs > 0 )
				q2d = it->detMsecs - it->quoteMsecs;

			if( q2d <= fakeMsecsMax_ )
			{
				if( it->isFilledIncl() )
					++countDay_.nFilledOnFake;
				++countDay_.nTotalOnFake;
			}
			else
			{
				if( it->isFilledIncl() )
					++countDay_.nFilledOnNonFake;
				++countDay_.nTotalOnNonFake;
			}
		}
	}
	return;
}

void MOrderAna::update_nOrders( vector<MercatorOrder>& orders, string ticker )
{
	string& market = MEnv::Instance()->market;
	for( vector<MercatorOrder>::iterator it = orders.begin(); it != orders.end(); ++it )
	{
		double tradePrice = it->price * exchrat_;

		int o2e = 24*60*60*1000;
		int q2i = 24*60*60*1000;
		int q2t = 24*60*60*1000;
		if( it->eventMsecs > 0 && it->orderMsecs > 0 )
			o2e = it->eventMsecs - it->orderMsecs;
		if( it->insertMsecs > 0 && it->quoteMsecs > 0 )
			q2i = it->insertMsecs - it->quoteMsecs;
		if( it->trdMsecs > 0 && it->quoteMsecs > 0 )
			q2t = it->trdMsecs - it->quoteMsecs;
		bool isFilled = it->isFilledIncl();
		bool inserted = insert_check_ ? (it->fillType == 2 || it->fillType == 4) : (fakeMsecsMax_ < q2i && q2i < q2iMax_);
		double fee = GFee::Instance().feeBpt(market, ExecFeesPrimex(market, ticker), it->price);
		double pnlbps = tradePrice * it->qtyExec * (it->gain - fee * (it->execType == 'L' ? 1. : 0.));

		if( verbose_ == 101 )
		{
			string market = MEnv::Instance()->market;
			printf("%c %s %d %c %.4f %d %.4f %.4f\n",
				market[1], ticker.c_str(), it->orderMsecs, it->side, tradePrice, it->qtyExec, tradePrice * it->qtyExec * it->gain / 10000., pnlbps / 10000.);
		}

		int sign = (it->side == 'B') ? 1 : -1;
		double pnl1 = tradePrice * it->qtyExec * (it->ret1 * sign);
		double pnl10 = tradePrice * it->qtyExec * (it->ret10 * sign);
		double pnlR = tradePrice * it->qtyExec * (it->retR * sign);
		double pnlON = tradePrice * it->qtyExec * (it->retON * sign);
		double npnl1 = tradePrice * (it->qty - it->qtyExec) * (it->ret1 * sign);
		double npnl10 = tradePrice * (it->qty - it->qtyExec) * (it->ret10 * sign);
		double npnlR = tradePrice * (it->qty - it->qtyExec) * (it->retR * sign);
		double npnlON = tradePrice * (it->qty - it->qtyExec) * (it->retON * sign);

		boost::mutex::scoped_lock lock(nOrd_mutex_);

		++countDay_.nOrd_total;

		perfDay_.dv_filled += tradePrice * it->qtyExec;
		perfDay_.profit_filled += pnlbps; // (bps).

		perfDay_.profit1 += pnl1; // (bps).
		perfDay_.profit10 += pnl10; // (bps).
		perfDay_.profitR += pnlR; // (bps).
		perfDay_.profitON += pnlON; // (bps).
		perfDay_.nprofit1 += npnl1; // (bps).
		perfDay_.nprofit10 += npnl10; // (bps).
		perfDay_.nprofitR += npnlR; // (bps).
		perfDay_.nprofitON += npnlON; // (bps).

		perfDay_.dv_notfilled += tradePrice * (it->qty - it->qtyExec);
		perfDay_.profit_notfilled += tradePrice * (it->qty - it->qtyExec) * it->gain;

		if( !inserted )
		{
			perfDay_.dv_dirfilled += tradePrice * it->qtyExec;
			perfDay_.profit_dirfilled += tradePrice * it->qtyExec * it->gain;
		}

		if( matchMin_ <= it->quoteMatch && it->quoteMatch <= matchMax_ )
		{
			perfDay_.dv_matfilled += tradePrice * it->qtyExec;
			perfDay_.profit_matfilled += tradePrice * it->qtyExec * it->gain;

			perfDay_.dv_matnotfilled += tradePrice * (it->qty - it->qtyExec);
			perfDay_.profit_matnotfilled += tradePrice * (it->qty - it->qtyExec) * it->gain;


			if( q2t >= 0 && q2t <= q2tMax_ && !isFilled)
			{
				perfDay_.dv_missed += tradePrice * it->qty;
				perfDay_.profit_missed += tradePrice * it->qty * it->gain;
			}
		}

		if( matchMin_ <= it->quoteMatch && it->quoteMatch <= matchMax_ )
		{
			++countDay_.nOrd_matched;

			if( isFilled && !inserted && o2e < msecDirect_ )
				++countDay_.nOrd_directFill;
			else if( q2i <= q2iMax_ )
			{
				if( isFilled )
				{
					++countDay_.nOrd_insertNFill;

					perfDay_.dv_insfilled += tradePrice * it->qty;
					perfDay_.profit_insfilled += tradePrice * it->qty * it->gain;

					if( q2t >= 0 && q2t <= q2tMax_ )
					{
						perfDay_.dv_misinsfilled += tradePrice * it->qty;
						perfDay_.profit_misinsfilled += tradePrice * it->qty * it->gain;
					}
				}
				else
					++countDay_.nOrd_insertNCancel;
			}
			else
			{
				if( isFilled )
					++countDay_.nOrd_unidFill;
				else
					++countDay_.nOrd_unidCancel;
			}
		}
		else
			++countDay_.nOrd_unmatched;
	}

	return;
}

// ---------------------------------------------------------------------------------------------------
//
// Class Performance
//
// ---------------------------------------------------------------------------------------------------

MOrderAna::Performance::Performance()
:dv_filled(0.),
dv_notfilled(0.),
dv_matfilled(0.),
dv_matnotfilled(0.),
dv_dirfilled(0.),
dv_missed(0.),
dv_insfilled(0.),
dv_misinsfilled(0.),
profit_filled(0.),
profit_notfilled(0.),
profit_matfilled(0.),
profit_matnotfilled(0.),
profit_dirfilled(0.),
profit_missed(0.),
profit_insfilled(0.),
profit_misinsfilled(0.),
profit1(0.),
profit10(0.),
profitR(0.),
nprofit1(0.),
nprofit10(0.),
nprofitR(0.)
{
}

void MOrderAna::Performance::reset()
{
	dv_filled = 0.;
	dv_notfilled = 0.;
	dv_matfilled = 0.;
	dv_matnotfilled = 0.;
	dv_dirfilled = 0.;
	dv_missed = 0.;
	dv_insfilled = 0.;
	dv_misinsfilled = 0.;
	profit_filled = 0.;
	profit_notfilled = 0.;
	profit_matfilled = 0.;
	profit_matnotfilled = 0.;
	profit_dirfilled = 0.;
	profit_missed = 0.;
	profit_insfilled = 0.;
	profit_misinsfilled = 0.;
	profit1 = 0.;
	profit10 = 0.;
	profitR = 0.;
	nprofit1 = 0.;
	nprofit10 = 0.;
	nprofitR = 0.;
}

MOrderAna::Performance& MOrderAna::Performance::operator+=(const Performance& rhs)
{
	dv_filled += rhs.dv_filled;
	dv_notfilled += rhs.dv_notfilled;
	dv_matfilled += rhs.dv_matfilled;
	dv_matnotfilled += rhs.dv_matnotfilled;
	dv_dirfilled += rhs.dv_dirfilled;
	dv_missed += rhs.dv_missed;
	dv_insfilled += rhs.dv_insfilled;
	dv_misinsfilled += rhs.dv_misinsfilled;
	profit_filled += rhs.profit_filled;
	profit_notfilled += rhs.profit_notfilled;
	profit_matfilled += rhs.profit_matfilled;
	profit_matnotfilled += rhs.profit_matnotfilled;
	profit_dirfilled += rhs.profit_dirfilled;
	profit_missed += rhs.profit_missed;
	profit_insfilled += rhs.profit_insfilled;
	profit_misinsfilled += rhs.profit_misinsfilled;
	profit1 += rhs.profit1;
	profit10 += rhs.profit10;
	profitR += rhs.profitR;
	nprofit1 += rhs.nprofit1;
	nprofit10 += rhs.nprofit10;
	nprofitR += rhs.nprofitR;
	return *this;
}

void MOrderAna::Performance::print()
{
	double gpt_filled = 0.;
	double gpt_notfilled = 0.;
	double gpt_matfilled = 0.;
	double gpt_matnotfilled = 0.;
	double gpt_dirfilled = 0.;
	double gpt_missed = 0.;
	double gpt_insfilled = 0.;
	double gpt_misinsfilled = 0.;
	double gpt1 = 0.;
	double gpt10 = 0.;
	double gptR = 0.;
	double gptON = 0.;
	double ngpt1 = 0.;
	double ngpt10 = 0.;
	double ngptR = 0.;
	double ngptON = 0.;

	if( dv_filled > 0. )
	{
		gpt_filled = profit_filled / dv_filled;
		gpt1 = profit1 / dv_filled;
		gpt10 = profit10 / dv_filled;
		gptR = profitR / dv_filled;
		gptON = profitON / dv_filled;
		ngpt1 = nprofit1 / dv_notfilled;
		ngpt10 = nprofit10 / dv_notfilled;
		ngptR = nprofitR / dv_notfilled;
		ngptON = nprofitON / dv_notfilled;
	}

	if( dv_notfilled > 0 )
		gpt_notfilled = profit_notfilled / dv_notfilled;

	if( dv_matfilled > 0 )
		gpt_matfilled = profit_matfilled / dv_matfilled;

	if( dv_matnotfilled > 0 )
		gpt_matnotfilled = profit_matnotfilled / dv_matnotfilled;

	if( dv_dirfilled > 0 )
		gpt_dirfilled = profit_dirfilled / dv_dirfilled;

	if( dv_missed > 0 )
		gpt_missed = profit_missed / dv_missed;

	if( dv_insfilled > 0 )
		gpt_insfilled = profit_insfilled / dv_insfilled;

	if( dv_misinsfilled > 0 )
		gpt_misinsfilled = profit_misinsfilled / dv_misinsfilled;

	char buf[400];
	sprintf(buf, "%13s %6.2f %7.3f %7.3f %7.3f %7.3f %7.3f\n", "filled", dv_filled / 1000000., gpt_filled, gpt1, gpt10, gptR, gptON);
	cout << buf;

	sprintf(buf, "%13s %6.2f %7.3f %7.3f %7.3f %7.3f %7.3f\n", "notfilled", dv_notfilled / 1000000., gpt_notfilled, ngpt1, ngpt10, ngptR, ngptON);
	cout << buf;

	sprintf(buf, "%13s %6.2f %7.3f\n", "matfilled", dv_matfilled / 1000000., gpt_matfilled);
	cout << buf;

	sprintf(buf, "%13s %6.2f %7.3f\n", "matnotfilled", dv_matnotfilled / 1000000., gpt_matnotfilled);
	cout << buf;

	sprintf(buf, "%13s %6.2f %7.3f\n", "dirfilled", dv_dirfilled / 1000000., gpt_dirfilled);
	cout << buf;

	sprintf(buf, "%13s %6.2f %7.3f\n", "missed", dv_missed / 1000000., gpt_missed);
	cout << buf;

	sprintf(buf, "%13s %6.2f %7.3f\n", "insfilled", dv_insfilled / 1000000., gpt_insfilled);
	cout << buf;

	sprintf(buf, "%13s %6.2f %7.3f\n", "misinsfilled", dv_misinsfilled / 1000000., gpt_misinsfilled);
	cout << buf;
}

// ---------------------------------------------------------------------------------------------------
//
// Class PerformanceSet
//
// ---------------------------------------------------------------------------------------------------

void MOrderAna::PerformanceSet::reset()
{
	mperf.clear();
}

void MOrderAna::PerformanceSet::add(int idate, const Performance& p)
{
	if( mperf.count(idate) )
		mperf[idate] += p;
	else
		mperf[idate] = p;
}

void MOrderAna::PerformanceSet::print()
{
	Acc acc_dv_f;

	Acc acc_gpt_f;
	Acc acc_gpt1_f;
	Acc acc_gpt10_f;
	Acc acc_gptR_f;
	Acc acc_gptON_f;

	Acc acc_pnl_f;
	Acc acc_pnl1_f;
	Acc acc_pnl10_f;
	Acc acc_pnlR_f;
	Acc acc_pnlON_f;

	Acc acc_dv_nf;
	Acc acc_gpt_nf;
	Acc acc_gpt1_nf;
	Acc acc_gpt10_nf;
	Acc acc_gptR_nf;
	Acc acc_gptON_nf;
	printf("%8s %7s %7s %7s %7s %7s %7s %7s\n", "idate", "dv(K)", "fillrat", "gpt", "gpt1", "gpt10", "gptR", "gptON");
	for( map<int, Performance>::iterator it = mperf.begin(); it != mperf.end(); ++it )
	{
		print_day(it->first, it->second);

		acc_dv_f.add(it->second.dv_filled);
		if( it->second.dv_filled > 0. )
		{
			acc_gpt_f.add(it->second.profit_filled / it->second.dv_filled);
			acc_gpt1_f.add(it->second.profit1 / it->second.dv_filled);
			acc_gpt10_f.add(it->second.profit10 / it->second.dv_filled);
			acc_gptR_f.add(it->second.profitR / it->second.dv_filled);
			acc_gptON_f.add(it->second.profitON / it->second.dv_filled);

			acc_pnl_f.add(it->second.profit_filled);
			acc_pnl1_f.add(it->second.profit1);
			acc_pnl10_f.add(it->second.profit10);
			acc_pnlR_f.add(it->second.profitR);
			acc_pnlON_f.add(it->second.profitON);
		}
		acc_dv_nf.add(it->second.dv_notfilled);
		if( it->second.dv_notfilled > 0. )
		{
			acc_gpt_nf.add(it->second.profit_notfilled / it->second.dv_notfilled);
			acc_gpt1_nf.add(it->second.nprofit1 / it->second.dv_notfilled);
			acc_gpt10_nf.add(it->second.nprofit10 / it->second.dv_notfilled);
			acc_gptR_nf.add(it->second.nprofitR / it->second.dv_notfilled);
			acc_gptON_nf.add(it->second.nprofitON / it->second.dv_notfilled);
		}
	}

	double gpt = acc_pnl_f.mean() / acc_dv_f.mean();
	double gpt1 = acc_pnl1_f.mean() / acc_dv_f.mean();
	double gpt10 = acc_pnl10_f.mean() / acc_dv_f.mean();
	double gptR = acc_pnlR_f.mean() / acc_dv_f.mean();
	double gptON = acc_pnlON_f.mean() / acc_dv_f.mean();

	// acc_gpt_f.stdev() is not exactly the weighted stdev of gpt, but should be close enough.
	printf("\n%25s %18s\n", "filled", "notfilled");
	printf("%6s %7.1f +- %7.1f %7.1f +- %7.1f (K USD per day)\n", "dv", acc_dv_f.mean() / 1000., acc_dv_f.stdev() / 1000., acc_dv_nf.mean() / 1000., acc_dv_nf.stdev() / 1000.);
	printf("%6s %7.3f +- %7.3f %7.3f +- %7.3f\n", "gpt", gpt, acc_gpt_f.stdev(), acc_gpt_nf.mean(),acc_gpt_nf.stdev());
	printf("%6s %7.3f +- %7.3f %7.3f +- %7.3f\n", "gpt1", gpt1, acc_gpt1_f.stdev(), acc_gpt1_nf.mean(),acc_gpt1_nf.stdev());
	printf("%6s %7.3f +- %7.3f %7.3f +- %7.3f\n", "gpt10", gpt10, acc_gpt10_f.stdev(), acc_gpt10_nf.mean(),acc_gpt10_nf.stdev());
	printf("%6s %7.3f +- %7.3f %7.3f +- %7.3f\n", "gptR", gptR, acc_gptR_f.stdev(), acc_gptR_nf.mean(),acc_gptR_nf.stdev());
	printf("%6s %7.3f +- %7.3f %7.3f +- %7.3f\n", "gptON", gptON, acc_gptON_f.stdev(), acc_gptON_nf.mean(),acc_gptON_nf.stdev());
}

void MOrderAna::PerformanceSet::print_day(int idate, Performance p)
{
	double dv = p.dv_filled;
	double dvn = p.dv_notfilled;
	double gpt = (dv > 0.) ? p.profit_filled / dv : 0.;
	double gptn = (dvn > 0.) ? p.profit_notfilled / dvn : 0.;
	double gpt1 = (dv > 0.) ? p.profit1 / dv : 0.;
	double gpt10 = (dv > 0.) ? p.profit10 / dv : 0.;
	double gptR = (dv > 0.) ? p.profitR / dv : 0.;
	double gptON = (dv > 0.) ? p.profitON / dv : 0.;
	double fillrat = p.dv_filled / (p.dv_filled + p.dv_notfilled);
	printf("%8d %7.1f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f\n", idate, dv / 1000., fillrat, gpt, gpt1, gpt10, gptR, gptON);
}

// ---------------------------------------------------------------------------------------------------
//
// Class Count
//
// ---------------------------------------------------------------------------------------------------

void MOrderAna::Count::reset()
{
	nOrd_total = 0;
	nOrd_matched = 0;
	nOrd_unmatched = 0;
	nOrd_directFill = 0;
	nOrd_insertNFill = 0;
	nOrd_insertNCancel = 0;
	nOrd_unidFill = 0;
	nOrd_unidCancel = 0;

	nShr_total = 0;
	nShr_filled = 0;

	nFilledFull = 0;
	nFilledIncl = 0;
	nTotal = 0;
	dvFilled = 0;
	dvTotal = 0;
	nFilledOnFake = 0;
	nTotalOnFake = 0;
	nFilledOnNonFake = 0;
	nTotalOnNonFake = 0;
}

void MOrderAna::Count::add(const MOrderAna::Count& rhs)
{
	nOrd_total += rhs.nOrd_total;
	nOrd_matched += rhs.nOrd_matched;
	nOrd_unmatched += rhs.nOrd_unmatched;
	nOrd_directFill += rhs.nOrd_directFill;
	nOrd_insertNFill += rhs.nOrd_insertNFill;
	nOrd_insertNCancel += rhs.nOrd_insertNCancel;
	nOrd_unidFill += rhs.nOrd_unidFill;
	nOrd_unidCancel += rhs.nOrd_unidCancel;

	nShr_total += rhs.nShr_total;
	nShr_filled += rhs.nShr_filled;

	nFilledFull += rhs.nFilledFull;
	nFilledIncl += rhs.nFilledIncl;
	nTotal += rhs.nTotal;
	dvFilled += rhs.dvFilled;
	dvTotal += rhs.dvTotal;
	nFilledOnFake += rhs.nFilledOnFake;
	nTotalOnFake += rhs.nTotalOnFake;
	nFilledOnNonFake += rhs.nFilledOnNonFake;
	nTotalOnNonFake += rhs.nTotalOnNonFake;
}

// ---------------------------------------------------------------------------------------------------
//
// Class HistSet
//
// ---------------------------------------------------------------------------------------------------

void MOrderAna::HistSet::init(double histMax, string m, int fm)
{
	market = m;
	count.reset();
	corr.clear();
	corr_filled.clear();
	corr_notFilled.clear();

	v_matched.clear();
	v_msecs.clear();
	v_fill.clear();
	v_insert.clear();
	v_directfill.clear();

	v_q2i.clear();
	v_q2m.clear();
	v_q2d.clear();
	v_q2t.clear();
	v_q2o.clear();

	v_i2x.clear();
	v_o2e.clear();

	v_pnl.clear();
	v_gpt.clear();
	v_pred.clear();
	v_pred1.clear();
	v_pred10.clear();
	v_ret.clear();
	v_ret1.clear();
	v_ret10.clear();
	v_retT.clear();
	v_retR.clear();
}

void MOrderAna::HistSet::write_hist(const string& moduleName, const string& dest, char execType, int orderSchedType)
{
	char fname[400];
	sprintf(fname, "m%s_d%s_%c_%d.npz", market.c_str(), dest.c_str(), execType, orderSchedType);

	cnpy::npz_save(fname, "matched", &v_matched[0], {v_matched.size()}, "w");
	cnpy::npz_save(fname, "fill", &v_fill[0], {v_fill.size()}, "a");
	cnpy::npz_save(fname, "q2d", &v_q2d[0], {v_q2d.size()}, "a");
	cnpy::npz_save(fname, "o2e", &v_o2e[0], {v_o2e.size()}, "a");
	cnpy::npz_save(fname, "gpt", &v_gpt[0], {v_gpt.size()}, "a");
}

void MOrderAna::HistSet::add_count(const MOrderAna::Count& rhs)
{
	count.add(rhs);
}

