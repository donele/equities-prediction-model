#include <HOrders.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <optionlibs/TickData.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <TFile.h>
using namespace std;

HOrderAna::HOrderAna(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
verbose_(0),
insert_check_(true),
write_hist_(true),
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
fee_bpt_(0.),
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

HOrderAna::~HOrderAna()
{}

void HOrderAna::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	if( write_hist_ )
	{
		TFile* f = HEnv::Instance()->outfile();
		f->mkdir(moduleName_.c_str());
	}

	histTotal_.init(histMax_, "All", fakeMsecsMax_);

	return;
}

void HOrderAna::beginMarket()
{
	string market = HEnv::Instance()->market();
	fee_bpt_ = mto::fee_bpt(market);
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
			vector<int> idates = HEnv::Instance()->idates();
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

void HOrderAna::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

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

void HOrderAna::beginTicker(const string& ticker)
{
	if( exper_univ_ && !sTickers_.count(ticker) )
		return;

	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	const vector<MercatorOrder>* vMOrderBuy
		= static_cast<const vector<MercatorOrder>*>(HEvent::Instance()->get(ticker, (string)"vMOrderBuy_" + dest_));
	const vector<MercatorOrder>* vMOrderSell
		= static_cast<const vector<MercatorOrder>*>(HEvent::Instance()->get(ticker, (string)"vMOrderSell_" + dest_));

	if( vMOrderBuy != 0 || vMOrderSell != 0 )
	{
		vector<MercatorOrder> orders;
		select_orders( orders, vMOrderBuy );
		select_orders( orders, vMOrderSell );

		if( !orders.empty() )
		{
			fill_hist( histMarket_, orders, ticker );
			fill_hist( histTotal_, orders, ticker );
			update_fillrat( orders );
			update_nOrders( orders, ticker );
		}
	}

	return;
}

void HOrderAna::endTicker(const string& ticker)
{
	return;
}

void HOrderAna::endDay()
{
	int idate = HEnv::Instance()->idate();
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

void HOrderAna::endMarket()
{
	cout << "------------------------------------------------" << moduleName_ << "\n";
	cout << " > dest = " << destCode_ << "\n";
	cout << " > exchange = " << exchange_ << "\n";
	cout << " > exectype = " << execType_ << "\n";
	cout << " > schedtype = " << orderSchedType_ << "\n";

	perfSetMarket_.print();
	//perfMarket_.print();
	histMarket_.print_summary();
	if( write_hist_ )
		histMarket_.write_hist(moduleName_, dest_, execType_, orderSchedType_);

	return;
}

void HOrderAna::endJob()
{
	if( HEnv::Instance()->markets().size() > 1 )
	{
		cout << "------------------------------------------------\n";
		cout << " > dest = " << dest_ << "\n";
		cout << " > exectype = " << execType_ << "\n";
		cout << " > schedtype = " << orderSchedType_ << "\n";

		perfSetTotal_.print();
		//perfTotal_.print();
		histTotal_.print_summary();
		if( write_hist_ )
			histTotal_.write_hist(moduleName_, dest_, execType_, orderSchedType_);
	}
}

void HOrderAna::select_orders( vector<MercatorOrder>& orders, const vector<MercatorOrder>* vMOrder )
{
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
						if( !profitable_orders_only_
							|| (it->side == 'B' && (it->pred1 + it->pred10 - fee_bpt_ * (it->execType == 'L' ? 1. : 0.)) > 0)
						|| (it->side == 'S' && (-it->pred1 - it->pred10 - fee_bpt_ * (it->execType == 'L' ? 1. : 0.)) > 0.)
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

void HOrderAna::fill_hist( HistSet& histset, vector<MercatorOrder>& orders, string ticker )
{
	for( vector<MercatorOrder>::iterator it = orders.begin(); it != orders.end(); ++it )
	{
		double fee = fee_bpt_ * (it->execType == 'L' ? 1. : 0.);
		double gpt = it->gain - fee;
		double tradePrice = it->price * exchrat_;
		double pnl = tradePrice * it->qtyExec * (it->gain - fee) / 10000.;
		double pred = (it->side == 'B') ? it->pred1 + it->pred10 - fee : -it->pred1 - it->pred10 - fee;

		boost::mutex::scoped_lock lock(hist_mutex_);

		int f2o = it->orderMsecs - it->feedMsecs;
		histset.h_quoteMatch.Fill(it->quoteMatch);

		int filled = it->isFilledIncl()?1:0;
		histset.p_fillrat.Fill(it->orderMsecs, filled);
		histset.p_fillrat_f2o.Fill(f2o, (double)filled);
		histset.h_norders.Fill(it->orderMsecs, 1);

		int N = histset.h_pnl_pred.GetNbinsX();
		for( int b = 1; b <= N; ++b )
		{
			if( pred > histset.h_pnl_pred.GetBinLowEdge(b) )
				histset.h_pnl_pred.Fill(histset.h_pnl_pred.GetBinLowEdge(b), pnl);
			else
				break;
		}

		// Prediction.
		int sign = (it->side == 'B') ? 1 : -1;
		histset.p_pred.Fill(it->pred1 + it->pred10, it->ret1 + it->ret10);
		histset.p_pred1.Fill(it->pred1, it->ret1);
		histset.p_pred10.Fill(it->pred10, it->ret10);
		histset.p_predT.Fill(it->pred1 + it->pred10, it->gain * sign);
		histset.p_predR.Fill(it->pred1 + it->pred10, it->gain * sign - (it->ret1 + it->ret10));
		histset.corr.add(it->pred1 + it->pred10, it->ret1 + it->ret10);
		if( it->isFilledIncl() )
		{
			histset.p_pred_filled.Fill(it->pred1 + it->pred10, it->ret1 + it->ret10);
			histset.p_pred1_filled.Fill(it->pred1, it->ret1);
			histset.p_pred10_filled.Fill(it->pred10, it->ret10);
			histset.p_predT_filled.Fill(it->pred1 + it->pred10, it->gain * sign);
			histset.p_predR_filled.Fill(it->pred1 + it->pred10, it->gain * sign - (it->ret1 + it->ret10));
			histset.corr_filled.add(it->pred1 + it->pred10, it->ret1 + it->ret10);
		}
		else if( it->qtyExec == 0 )
		{
			histset.p_pred_notFilled.Fill(it->pred1 + it->pred10, it->ret1 + it->ret10);
			histset.p_pred1_notFilled.Fill(it->pred1, it->ret1);
			histset.p_pred10_notFilled.Fill(it->pred10, it->ret10);
			histset.p_predT_notFilled.Fill(it->pred1 + it->pred10, it->gain * sign);
			histset.p_predR_notFilled.Fill(it->pred1 + it->pred10, it->gain * sign - (it->ret1 + it->ret10));
			histset.corr_notFilled.add(it->pred1 + it->pred10, it->ret1 + it->ret10);
		}

		// Histograms.
		if( matchMin_ <= it->quoteMatch && it->quoteMatch <= matchMax_ )
		{
			int q2o = 24*60*60*1000;
			int o2e = 24*60*60*1000;
			int q2m = 24*60*60*1000;
			int q2d = 24*60*60*1000;
			int q2t = 24*60*60*1000;
			int q2i = 24*60*60*1000;
			int i2x = 24*60*60*1000;
			int d2i = 24*60*60*1000;
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

			histset.p_q2o.Fill(it->orderMsecs, q2o);
			histset.h_q2i.Fill(q2i);

			bool inserted = insert_check_ ? (it->fillType == 2 || it->fillType == 4) : (fakeMsecsMax_ < q2i && q2i < q2iMax_);

			if( it->isFilledIncl() )
			{
				histset.h_q2m_filled.Fill(q2m);
				histset.h_q2d_filled.Fill(q2d);
				histset.h_q2t_filled.Fill(q2t);
				histset.h_q2o_filled.Fill(q2o);
				histset.p_gpt_q2t_filled.Fill(q2t, gpt);
				if( inserted )
				{
					histset.h_q2i_filled.Fill(q2i);
					histset.h_i2x_filled.Fill(i2x);
					histset.h_o2v_filled.Fill(o2e);
				}
				else
				{
					histset.h_q2m_direct_filled.Fill(q2m);
					histset.h_q2d_direct_filled.Fill(q2d);
					histset.h_o2e_filled.Fill(o2e);
				}
			}
			else if( it->qtyExec == 0 )
			{
				histset.h_q2d_notFilled.Fill(q2d);
				histset.h_q2t_notFilled.Fill(q2t);
				histset.h_q2o_notFilled.Fill(q2o);
				histset.h_o2e_notFilled.Fill(o2e);
				histset.p_gpt_q2t_notFilled.Fill(q2t, gpt);
			}
		}
	}
	return;
}

void HOrderAna::update_fillrat( vector<MercatorOrder>& orders )
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

void HOrderAna::update_nOrders( vector<MercatorOrder>& orders, string ticker )
{
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
		double pnlbps = tradePrice * it->qtyExec * (it->gain - fee_bpt_ * (it->execType == 'L' ? 1. : 0.));

		if( verbose_ == 101 )
		{
			string market = HEnv::Instance()->market();
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

HOrderAna::Performance::Performance()
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

void HOrderAna::Performance::reset()
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

HOrderAna::Performance& HOrderAna::Performance::operator+=(const Performance& rhs)
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

void HOrderAna::Performance::print()
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

void HOrderAna::PerformanceSet::reset()
{
	mperf.clear();
}

void HOrderAna::PerformanceSet::add(int idate, const Performance& p)
{
	if( mperf.count(idate) )
		mperf[idate] += p;
	else
		mperf[idate] = p;
}

void HOrderAna::PerformanceSet::print()
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

void HOrderAna::PerformanceSet::print_day(int idate, Performance p)
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

void HOrderAna::Count::reset()
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

void HOrderAna::Count::add(const HOrderAna::Count& rhs)
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

void HOrderAna::HistSet::init(double histMax, string m, int fm)
{
	fakeMsecsMax = fm;
	market = m;

	count.reset();

	corr.clear();
	corr_filled.clear();
	corr_notFilled.clear();

	char name[200];
	char title[200];
	double histMin = - histMax / 10.0;
	int nbins = ceil( histMax + fabs(histMin) - 0.5 );
	if( nbins > 5500 * 5 )
		nbins = 5500;

	// match.
	sprintf(name, "h_quoteMatch");
	sprintf(title, "Quote Match Match %s", market.c_str());
	h_quoteMatch = TH1F(name, title, 11, 0, 11);

	// as function of time
	sprintf(name, "p_q2o");
	sprintf(title, "q2o vs. Time of day %s", market.c_str());
	p_q2o = TProfile(name, title, 100, 28800000, 59400000, -2000, 2000);

	sprintf(name, "p_fillrat");
	sprintf(title, "fillrat vs. Time of day %s", market.c_str());
	p_fillrat = TProfile(name, title, 40, 28800000, 59400000);

	sprintf(name, "h_norders");
	sprintf(title, "norders vs. Time of day %s", market.c_str());
	h_norders = TH1F(name, title, 40, 28800000, 59400000);

	// Time by day.
	sprintf(name, "h_q2i");
	sprintf(title, "q2i, %s", market.c_str());
	h_q2i = TH1F(name, title, nbins, histMin, histMax );


	sprintf(name, "h_pnl_pred");
	sprintf(title, "pnl vs pred - cost, %s", market.c_str());
	h_pnl_pred = TH1F(name, title, 200, -100, 100 );


	sprintf(name, "h_o2e_filled");
	sprintf(title, "o2e, Filled, %s", market.c_str());
	h_o2e_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_o2v_filled");
	sprintf(title, "o2v, Filled after missing, %s", market.c_str());
	h_o2v_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2m_filled");
	sprintf(title, "q2m, Filled, %s", market.c_str());
	h_q2m_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2d_filled");
	sprintf(title, "q2d, Filled, %s", market.c_str());
	h_q2d_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2t_filled");
	sprintf(title, "q2t, Filled, %s", market.c_str());
	h_q2t_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2m_direct_filled");
	sprintf(title, "q2m, Direct Filled, %s", market.c_str());
	h_q2m_direct_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2d_direct_filled");
	sprintf(title, "q2d, Direct Filled, %s", market.c_str());
	h_q2d_direct_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2o_filled");
	sprintf(title, "q2o, Filled, %s", market.c_str());
	h_q2o_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2i_filled");
	sprintf(title, "q2i, Filled, %s", market.c_str());
	h_q2i_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_i2x_filled");
	sprintf(title, "i2x, Filled, %s", market.c_str());
	h_i2x_filled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_o2e_notFilled");
	sprintf(title, "o2e, Not filled, %s", market.c_str());
	h_o2e_notFilled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2d_notFilled");
	sprintf(title, "q2d, Not filled, %s", market.c_str());
	h_q2d_notFilled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2t_notFilled");
	sprintf(title, "q2t, Not filled, %s", market.c_str());
	h_q2t_notFilled = TH1F(name, title, nbins, histMin, histMax );

	sprintf(name, "h_q2o_notFilled");
	sprintf(title, "q2o, Not filled, %s", market.c_str());
	h_q2o_notFilled = TH1F(name, title, nbins, histMin, histMax );


	sprintf(name, "p_pred");
	sprintf(title, "ret vs pred, %s", market.c_str());
	p_pred = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_pred_filled");
	sprintf(title, "ret vs pred, Filled %s", market.c_str());
	p_pred_filled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_pred_notFilled");
	sprintf(title, "ret vs pred, Not filled %s", market.c_str());
	p_pred_notFilled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_pred1");
	sprintf(title, "ret1 vs pred1, %s", market.c_str());
	p_pred1 = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_pred1_filled");
	sprintf(title, "ret1 vs pred1, Filled %s", market.c_str());
	p_pred1_filled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_pred1_notFilled");
	sprintf(title, "ret1 vs pred1, Not filled %s", market.c_str());
	p_pred1_notFilled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_pred10");
	sprintf(title, "ret10 vs pred10, %s", market.c_str());
	p_pred10 = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_pred10_filled");
	sprintf(title, "ret10 vs pred10, Filled %s", market.c_str());
	p_pred10_filled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_pred10_notFilled");
	sprintf(title, "ret10 vs pred10, Not filled %s", market.c_str());
	p_pred10_notFilled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_predT");
	sprintf(title, "retT vs predT, %s", market.c_str());
	p_predT = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_predT_filled");
	sprintf(title, "retT vs predT, Filled %s", market.c_str());
	p_predT_filled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_predT_notFilled");
	sprintf(title, "retT vs predT, Not filled %s", market.c_str());
	p_predT_notFilled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_predR");
	sprintf(title, "retR vs predT, %s", market.c_str());
	p_predR = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_predR_filled");
	sprintf(title, "retR vs predT, Filled %s", market.c_str());
	p_predR_filled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_predR_notFilled");
	sprintf(title, "retR vs predT, Not filled %s", market.c_str());
	p_predR_notFilled = TProfile(name, title, 40, -200., 200. );

	sprintf(name, "p_fillrat_f2o");
	sprintf(title, "fillrat vs f2o, %s", market.c_str());
	p_fillrat_f2o = TProfile(name, title, nbins, histMin, histMax );

	sprintf(name, "p_gpt_q2t_filled");
	sprintf(title, "gpt vs q2t, Filled, %s", market.c_str());
	p_gpt_q2t_filled = TProfile(name, title, 500, 0, 1000 );

	sprintf(name, "p_gpt_q2t_notFilled");
	sprintf(title, "gpt vs q2t, Not Filled, %s", market.c_str());
	p_gpt_q2t_notFilled = TProfile(name, title, 500, 0, 1000 );
}

void HOrderAna::HistSet::write_hist(const string& moduleName, const string& dest, char execType, int orderSchedType)
{
	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName.c_str());

	char dir[400];
	sprintf(dir, "m%s_d%s_%c_%d",
		market.c_str(), dest.c_str(), execType, orderSchedType);
	gDirectory->mkdir(dir);
	gDirectory->cd(dir);

	p_q2o.Write();
	p_fillrat.Write();
	h_norders.Write();
	h_quoteMatch.Write();

	h_q2i.Write();
	h_pnl_pred.Write();

	h_o2e_filled.Write();
	h_o2v_filled.Write();
	h_q2m_filled.Write();
	h_q2d_filled.Write();
	h_q2t_filled.Write();
	h_q2m_direct_filled.Write();
	h_q2d_direct_filled.Write();
	h_q2o_filled.Write();
	h_q2i_filled.Write();
	h_i2x_filled.Write();

	h_o2e_notFilled.Write();
	h_q2d_notFilled.Write();
	h_q2t_notFilled.Write();
	h_q2o_notFilled.Write();

	p_pred.Write();
	p_pred_filled.Write();
	p_pred_notFilled.Write();
	p_pred1.Write();
	p_pred1_filled.Write();
	p_pred1_notFilled.Write();
	p_pred10.Write();
	p_pred10_filled.Write();
	p_pred10_notFilled.Write();
	p_predT.Write();
	p_predT_filled.Write();
	p_predT_notFilled.Write();
	p_predR.Write();
	p_predR_filled.Write();
	p_predR_notFilled.Write();
	p_fillrat_f2o.Write();
	p_gpt_q2t_filled.Write();
	p_gpt_q2t_notFilled.Write();
}

void HOrderAna::HistSet::add_count(const HOrderAna::Count& rhs)
{
	count.add(rhs);
}

void HOrderAna::HistSet::print_summary()
{
	double o2efilled = 0.;
	double q2dfilled = 0.;
	double q2tfilled = 0.;
	double q2dnotfilled = 0.;
	double q2tnotfilled = 0.;
	double q2i = 0.;
	double nfakeNotFilled = 0.;
	double nfakeFilled = 0.;
	double ncompetition = 0.;
	double fillratshr = 0.;
	double fillratincl = 0.;
	double fillratinclmatched = 0.;
	double fillratinclunmatched = 0.;
	double fillratnonfake = 0.;
	double ratdirectfill = 0.;
	double ratmatched = 0.;
	double ratfake = 0.;
	double ratcomp = 0.;

	get_summary(count, o2efilled, q2dfilled, q2tfilled, q2dnotfilled, q2tnotfilled, q2i, nfakeNotFilled, nfakeFilled, ncompetition,
		fillratshr, fillratincl, fillratinclmatched, fillratinclunmatched, fillratnonfake,
		ratdirectfill, ratmatched, ratfake, ratcomp);

	cout << " fillratshr = " << dtos(fillratshr, 3) << "\n";
	cout << " fillratincl = " << dtos(fillratincl, 3) << "\n";
	cout << " fillratinclmatched = " << dtos(fillratinclmatched, 3) << "\n";
	cout << " fillratinclunmatched = " << dtos(fillratinclunmatched, 3) << "\n";
	cout << " fillratnonfake = " << dtos(fillratnonfake, 3) << "\n";
	cout << " ratdirectfill = " << dtos(ratdirectfill, 3) << "\n";
	cout << " ratmatched = " << dtos(ratmatched, 3) << "\n";
	cout << " ratfake = " << dtos(ratfake, 3) << "\n";
	cout << " ratcomp = " << dtos(ratcomp, 3) << "\n";

	cout << " nshr = " << itos(count.nShr_total) << "\n";
	cout << " nshrfill = " << itos(count.nShr_filled) << "\n";
	cout << " norder = " << itos(count.nTotal) << "\n";
	cout << " nfillfull = " << itos(count.nFilledFull) << "\n";
	cout << " nfillincl = " << itos(count.nFilledIncl) << "\n";
	cout << " dvorder = " << dtos(count.dvTotal, 1) << "\n";
	cout << " dvfill = " << dtos(count.dvFilled, 1) << "\n";
	cout << " nmatched = " << itos(count.nOrd_matched) << "\n";
	cout << " ndirectfill = " << itos(count.nOrd_directFill) << "\n";
	cout << " ninsertfill = " << itos(count.nOrd_insertNFill) << "\n";
	cout << " ninsertcancel = " << itos(count.nOrd_insertNCancel) << "\n";
	cout << " norderfake = " << itos(count.nTotalOnFake) << "\n";
	cout << " nfillfake = " << itos(count.nFilledOnFake) << "\n";
	cout << " nordernonfake = " << itos(count.nTotalOnNonFake) << "\n";
	cout << " nfillnonfake = " << itos(count.nFilledOnNonFake) << "\n";

	cout << " ncomp = " << itos(ncompetition) << "\n";
	cout << " o2efilled = " << dtos(o2efilled, 3) << "\n";
	cout << " q2dfilled = " << dtos(q2dfilled, 3) << "\n";
	cout << " q2tfilled = " << dtos(q2tfilled, 3) << "\n";
	cout << " q2dnotfilled = " << dtos(q2dnotfilled, 3) << "\n";
	cout << " q2tnotfilled = " << dtos(q2tnotfilled, 3) << "\n";
	cout << " q2i = " << dtos(q2i, 3) << endl;

	cout << "corr " << corr.corr() << endl;
	cout << "corr filled " << corr_filled.corr() << endl;
	cout << "corr not filled " << corr_notFilled.corr() << endl;

}

void HOrderAna::HistSet::get_summary(Count& count, double& o2efilled, double& q2dfilled, double& q2tfilled, double& q2dnotfilled, double& q2tnotfilled, double& q2i,
				double& nfakeNotFilled, double& nfakeFilled,
				double& ncompetition, double& fillratshr, double& fillratincl, double& fillratinclmatched, double& fillratinclunmatched, double& fillratnonfake,
				double& ratdirectfill, double& ratmatched, double& ratfake, double& ratcomp)
{
	o2efilled = get_top5bin_avg(h_o2e_filled, 0);
	q2dfilled = get_top5bin_avg(h_q2d_direct_filled, fakeMsecsMax);
	q2tfilled = get_top5bin_avg(h_q2t_filled, fakeMsecsMax);
	q2dnotfilled = get_top5bin_avg(h_q2d_notFilled, fakeMsecsMax);
	q2tnotfilled = get_top5bin_avg(h_q2t_notFilled, fakeMsecsMax);
	q2i = get_top5bin_avg(h_q2i, fakeMsecsMax);
	nfakeNotFilled = get_nfake(h_q2d_notFilled);
	nfakeFilled = get_nfake(h_q2d_filled);
	ncompetition = get_competition(h_q2d_notFilled, q2dfilled);

	if( count.nShr_total > 0 )
		fillratshr = count.nShr_filled / count.nShr_total;
	if( count.nTotal > 0 )
		fillratincl = count.nFilledIncl / count.nTotal;
	if( count.nTotalOnNonFake > 0 )
		fillratnonfake = count.nFilledOnNonFake / count.nTotalOnNonFake;
	if( count.nOrd_matched > 0 )
		ratdirectfill = count.nOrd_directFill / count.nOrd_matched;
	if( count.nTotal > 0 )
		ratmatched = count.nOrd_matched / count.nTotal;
	if( count.nOrd_matched > 0 )
		ratfake = (nfakeNotFilled + nfakeFilled) / (count.nOrd_matched);
	if( count.nOrd_matched - nfakeNotFilled - nfakeFilled > 0 )
		ratcomp = ncompetition / (count.nOrd_matched - nfakeNotFilled - nfakeFilled);

	double matchedNfilled = h_q2d_filled.GetEntries();
	double matchedNnotfilled = h_q2d_notFilled.GetEntries();
	if( matchedNfilled + matchedNnotfilled > 0 )
		fillratinclmatched = matchedNfilled / (matchedNfilled + matchedNnotfilled);
	if( count.nOrd_matched > 0 )
		fillratinclunmatched = (count.nFilledIncl - matchedNfilled) / count.nOrd_matched;

	return;
}

double HOrderAna::HistSet::get_top5bin_avg(TH1F& h, double thresMsecs)
{
	// Locate the peak.
	double maxBinContent = 0;
	int iBinMaxContent = 0;
	int N = h.GetNbinsX();
	double binSize = h.GetXaxis()->GetBinLowEdge(2) - h.GetXaxis()->GetBinLowEdge(1);
	for( int b = 1; b <= N; ++b )
	{
		double binCont = h.GetBinContent(b);
		double binLowEdge = h.GetBinLowEdge(b);
		if( thresMsecs < binLowEdge && maxBinContent < binCont )
		{
			maxBinContent = binCont;
			iBinMaxContent = b;
		}
	}

	double sumProd = 0;
	double sumContent = 0;

	// The peak and the rhs.
	{
		int nValidBins = 0;
		double lastBinContent = maxBinContent;
		for( int b = iBinMaxContent; b <= N; ++b )
		{
			double binContent = h.GetBinContent(b);
			if( binContent > 0.5 )
				++nValidBins;

			if( b == iBinMaxContent || ( binContent > maxBinContent * 0.01 && binContent < lastBinContent && nValidBins <= 3 ) )
			{
				double binCenter = h.GetXaxis()->GetBinLowEdge(b);
				if( binSize > 1.1 )
					binCenter = (h.GetXaxis()->GetBinLowEdge(b) + h.GetXaxis()->GetBinLowEdge(b+1)) / 2.0;
				sumProd += binCenter * binContent;
				sumContent += binContent;
			}
			if( binContent < lastBinContent )
				lastBinContent = binContent;

			if( binContent > lastBinContent || binContent < maxBinContent * 0.01 || nValidBins > 3 )
				break;
		}
	}

	// lhs.
	{
		int nValidBins = 0;
		double lastBinContent = maxBinContent;
		for( int b = iBinMaxContent - 1; b > 0; --b )
		{
			double binContent = h.GetBinContent(b);
			if( binContent > 0.5 )
				++nValidBins;

			if( binContent > maxBinContent * 0.01 && binContent < lastBinContent && nValidBins <= 2 )
			{
				double binCenter = h.GetXaxis()->GetBinLowEdge(b);
				if( binSize > 1.1 )
					binCenter = (h.GetXaxis()->GetBinLowEdge(b) + h.GetXaxis()->GetBinLowEdge(b + 1)) / 2.0;
				sumProd += binCenter * binContent;
				sumContent += binContent;
			}
			if( binContent < lastBinContent )
				lastBinContent = binContent;

			if( binContent > lastBinContent || binContent < maxBinContent * 0.01 || nValidBins > 2 )
				break;
		}
	}

	double avg = 0;
	if( sumContent > 0 )
		avg = sumProd / sumContent;

	return avg;
}

double HOrderAna::HistSet::get_nfake(TH1F& h)
{
	int N = h.GetNbinsX();
	double nfake = 0;
	for( int b = 1; b <= N; ++b )
	{
		double binLowEdge = h.GetXaxis()->GetBinLowEdge(b);
		double binHighEdge = h.GetXaxis()->GetBinLowEdge(b+1);
		if( binLowEdge <= fakeMsecsMax )
		{
			double c = h.GetBinContent(b);
			nfake += c;
		}
	}
	return nfake;
}

double HOrderAna::HistSet::get_competition(TH1F& h, double q2dfilled)
{
	int N = h.GetNbinsX();
	{
		vector<double> v;
		for( int b = 1; b <= N; ++b )
			v.push_back(h.GetBinContent(b));

		sort(v.begin(), v.end());
		int vs = v.size();
		double thres = v[max(0, vs - 5)];

		double maxX = 0;
		for( int b = 1; b <= N; ++b )
		{
			double c = h.GetBinContent(b);
			if( c >= thres )
			{
				double binCenter = (h.GetXaxis()->GetBinLowEdge(b) + h.GetXaxis()->GetBinLowEdge(b + 1)) / 2.0;
				if( binCenter > fakeMsecsMax && binCenter > maxX)
				{
					maxX = binCenter;
				}
			}
		}
		q2dfilled = maxX;
	}

	double ncomp = 0;
	for( int b = 1; b <= N; ++b )
	{
		double binLowEdge = h.GetXaxis()->GetBinLowEdge(b);
		double binHighEdge = h.GetXaxis()->GetBinLowEdge(b + 1);
		if( binLowEdge > fakeMsecsMax && binHighEdge <= q2dfilled )
		{
			double c = h.GetBinContent(b);
			ncomp += c;
		}
	}
	return ncomp;
}
