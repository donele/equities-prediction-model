#include <HOrders.h>
//#include <HMod/MercatorOrder.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <optionlibs/TickData.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <map>
#include <string>
#include <TFile.h>
using namespace std;

HFillTimeHist::HFillTimeHist(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
fakeMsecsMax_(3),
procName_("")
{
	if( conf.count("procName") )
		procName_ = conf.find("procName")->second.c_str();
}

HFillTimeHist::~HFillTimeHist()
{}

void HFillTimeHist::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->mkdir(moduleName_.c_str());
	return;
}

void HFillTimeHist::beginMarket()
{
	string market = HEnv::Instance()->market();
	int ndays = HEnv::Instance()->nDates();

	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->mkdir(market.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->mkdir("timeOfDay");
	gDirectory->mkdir("match");
	gDirectory->mkdir("vsDay");
	gDirectory->mkdir("matchVsDay");
	gDirectory->mkdir("nOrders");
	gDirectory->mkdir("fillrat");
	gDirectory->mkdir("fillratbymatch");

	char name[200];
	char title[200];

	// Match.
	sprintf(name, "h_quoteMatch_%s", market.c_str());
	sprintf(title, "Quote Match %s", market.c_str());
	h_quoteMatch_ = TH1F(name, title, 10,  0, 10);


	// Distributions.
	sprintf(name, "h_o2e_filled_%s", market.c_str());
	sprintf(title, "Order to event, Filled, %s", market.c_str());
	h_o2e_filled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_o2v_filled_%s", market.c_str());
	sprintf(title, "Order to event, Filled after missing, %s", market.c_str());
	h_o2v_filled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_q2d_filled_%s", market.c_str());
	sprintf(title, "Quote to deterioration, Filled, %s", market.c_str());
	h_q2d_filled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_q2o_filled_%s", market.c_str());
	sprintf(title, "Quote to Order, Filled, %s", market.c_str());
	h_q2o_filled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_q2i_filled_%s", market.c_str());
	sprintf(title, "Quote to Insert, Filled, %s", market.c_str());
	h_q2i_filled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_i2x_filled_%s", market.c_str());
	sprintf(title, "Insert to Execution, Filled, %s", market.c_str());
	h_i2x_filled_ = TH1F(name, title, 5500,  -1000, 10000);


	sprintf(name, "h_o2e_notFilled_%s", market.c_str());
	sprintf(title, "Order to event, Not Filled, %s", market.c_str());
	h_o2e_notFilled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_o2v_notFilled_%s", market.c_str());
	sprintf(title, "Order to event, Not Filled after missing, %s", market.c_str());
	h_o2v_notFilled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_q2d_notFilled_%s", market.c_str());
	sprintf(title, "Quote to deterioration, Not Filled, %s", market.c_str());
	h_q2d_notFilled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_q2o_notFilled_%s", market.c_str());
	sprintf(title, "Quote to Order, Not Filled, %s", market.c_str());
	h_q2o_notFilled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_q2i_notFilled_%s", market.c_str());
	sprintf(title, "Quote to Insert, Not Filled, %s", market.c_str());
	h_q2i_notFilled_ = TH1F(name, title, 5500,  -1000, 10000);

	sprintf(name, "h_i2x_notFilled_%s", market.c_str());
	sprintf(title, "Insert to Execution, Not Filled, %s", market.c_str());
	h_i2x_notFilled_ = TH1F(name, title, 5500,  -1000, 10000);


	// q2o.
	sprintf(name, "h_q2o_alph_%s", market.c_str());
	sprintf(title, "q2o vs. Alphabet %s", market.c_str());
	p_q2o_alph_ = TProfile(name, title, 26, 'A', 'Z');


	// Time by day.
	sprintf(name, "h_o2e_filled_day_%s", market.c_str());
	sprintf(title, "o2e vs. nDay, Filled, %s", market.c_str());
	h_o2e_filled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_o2e_filled_day_);

	sprintf(name, "h_o2v_filled_day_%s", market.c_str());
	sprintf(title, "o2v vs. nDay, Filled after missing, %s", market.c_str());
	h_o2v_filled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_o2v_filled_day_);

	sprintf(name, "h_q2d_filled_day_%s", market.c_str());
	sprintf(title, "q2d vs. nDay, Filled, %s", market.c_str());
	h_q2d_filled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_q2d_filled_day_);

	sprintf(name, "h_q2o_filled_day_%s", market.c_str());
	sprintf(title, "q2o vs. nDay, Filled, %s", market.c_str());
	h_q2o_filled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_q2o_filled_day_);

	sprintf(name, "h_q2i_filled_day_%s", market.c_str());
	sprintf(title, "q2i vs. nDay, Filled, %s", market.c_str());
	h_q2i_filled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_q2i_filled_day_);

	sprintf(name, "h_i2x_filled_day_%s", market.c_str());
	sprintf(title, "i2x vs. nDay, Filled, %s", market.c_str());
	h_i2x_filled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_i2x_filled_day_);

	sprintf(name, "h_fakeRat_filled_day_%s", market.c_str());
	sprintf(title, "Fake Order Rat, Filled, %s", market.c_str());
	h_fakeRat_filled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_fakeRat_filled_day_);


	sprintf(name, "h_o2e_notFilled_day_%s", market.c_str());
	sprintf(title, "o2e vs. nDay, Not filled, %s", market.c_str());
	h_o2e_notFilled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_o2e_notFilled_day_);

	sprintf(name, "h_o2v_notFilled_day_%s", market.c_str());
	sprintf(title, "o2v vs. nDay, Not filled after missing, %s", market.c_str());
	h_o2v_notFilled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_o2v_notFilled_day_);

	sprintf(name, "h_q2d_notFilled_day_%s", market.c_str());
	sprintf(title, "q2d vs. nDay, Not filled, %s", market.c_str());
	h_q2d_notFilled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_q2d_notFilled_day_);

	sprintf(name, "h_q2o_notFilled_day_%s", market.c_str());
	sprintf(title, "q2o vs. nDay, Not filled, %s", market.c_str());
	h_q2o_notFilled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_q2o_notFilled_day_);

	sprintf(name, "h_q2i_notFilled_day_%s", market.c_str());
	sprintf(title, "q2i vs. nDay, Not filled, %s", market.c_str());
	h_q2i_notFilled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_q2i_notFilled_day_);

	sprintf(name, "h_i2x_notFilled_day_%s", market.c_str());
	sprintf(title, "i2x vs. nDay, Not filled, %s", market.c_str());
	h_i2x_notFilled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_i2x_notFilled_day_);

	sprintf(name, "h_fakeRat_notFilled_day_%s", market.c_str());
	sprintf(title, "Fake Order Rat, Not filled, %s", market.c_str());
	h_fakeRat_notFilled_day_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_fakeRat_notFilled_day_);


	// Orders by day.
	sprintf(name, "h_nOrd_total_%s", market.c_str());
	sprintf(title, "Number of orders, Total, %s", market.c_str());
	h_nOrd_total_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_nOrd_total_);

	sprintf(name, "h_nOrd_matched_%s", market.c_str());
	sprintf(title, "Number of orders, matched, %s", market.c_str());
	h_nOrd_matched_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_nOrd_matched_);

	sprintf(name, "h_nOrd_unmatched_%s", market.c_str());
	sprintf(title, "Number of orders, unmatched, %s", market.c_str());
	h_nOrd_unmatched_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_nOrd_unmatched_);

	sprintf(name, "h_nOrd_directFill_%s", market.c_str());
	sprintf(title, "Number of orders, direct fill, %s", market.c_str());
	h_nOrd_directFill_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_nOrd_directFill_);

	sprintf(name, "h_nOrd_insertNFill_%s", market.c_str());
	sprintf(title, "Number of orders, insert and fill, %s", market.c_str());
	h_nOrd_insertNFill_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_nOrd_insertNFill_);

	sprintf(name, "h_nOrd_insertNCancel_%s", market.c_str());
	sprintf(title, "Number of orders, insert and cancel, %s", market.c_str());
	h_nOrd_insertNCancel_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_nOrd_insertNCancel_);

	sprintf(name, "h_nOrd_unidFill_%s", market.c_str());
	sprintf(title, "Number of orders, unidentified fill, %s", market.c_str());
	h_nOrd_unidFill_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_nOrd_unidFill_);

	sprintf(name, "h_nOrd_unidCancel_%s", market.c_str());
	sprintf(title, "Number of orders, unidentified cancel, %s", market.c_str());
	h_nOrd_unidCancel_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_nOrd_unidCancel_);


	sprintf(name, "h_ratOrd_matched_%s", market.c_str());
	sprintf(title, "Orders, matched, %s", market.c_str());
	h_ratOrd_matched_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_ratOrd_matched_);

	sprintf(name, "h_ratOrd_unmatched_%s", market.c_str());
	sprintf(title, "Orders, unmatched, %s", market.c_str());
	h_ratOrd_unmatched_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_ratOrd_unmatched_);

	sprintf(name, "h_ratOrd_directFill_%s", market.c_str());
	sprintf(title, "Orders, direct fill, %s", market.c_str());
	h_ratOrd_directFill_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_ratOrd_directFill_);

	sprintf(name, "h_ratOrd_insertNFill_%s", market.c_str());
	sprintf(title, "Orders, insert and fill, %s", market.c_str());
	h_ratOrd_insertNFill_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_ratOrd_insertNFill_);

	sprintf(name, "h_ratOrd_insertNCancel_%s", market.c_str());
	sprintf(title, "Orders, insert and cancel, %s", market.c_str());
	h_ratOrd_insertNCancel_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_ratOrd_insertNCancel_);

	sprintf(name, "h_ratOrd_unidFill_%s", market.c_str());
	sprintf(title, "Orders, unidentified fill, %s", market.c_str());
	h_ratOrd_unidFill_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_ratOrd_unidFill_);

	sprintf(name, "h_ratOrd_unidCancel_%s", market.c_str());
	sprintf(title, "Orders, unidentified cancel, %s", market.c_str());
	h_ratOrd_unidCancel_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_ratOrd_unidCancel_);

	sprintf(name, "h_ratOrd_fillWhenInserted_%s", market.c_str());
	sprintf(title, "Orders, fill when inserted, %s", market.c_str());
	h_ratOrd_fillWhenInserted_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_ratOrd_fillWhenInserted_);

	// Fill Ratio by match.
	sprintf(name, "h_fillrat_full_%s", market.c_str());
	sprintf(title, "Fill Ratio, Full, %s", market.c_str());
	h_fillrat_full_ = TH2F(name, title, ndays, 1, ndays + 1, 10, 1, 11);
	set_axis_title(h_fillrat_full_);

	sprintf(name, "h_fillrat_incl_%s", market.c_str());
	sprintf(title, "Fill Ratio, Inclusive, %s", market.c_str());
	h_fillrat_incl_ = TH2F(name, title, ndays, 1, ndays + 1, 10, 1, 11);
	set_axis_title(h_fillrat_incl_);

	sprintf(name, "h_fillrat_dv_%s", market.c_str());
	sprintf(title, "Fill Ratio, Dollar Volume, %s", market.c_str());
	h_fillrat_dv_ = TH2F(name, title, ndays, 1, ndays + 1, 10, 1, 11);
	set_axis_title(h_fillrat_dv_);

	// Fill ratio.
	sprintf(name, "h_rat_full_%s", market.c_str());
	sprintf(title, "Fill Ratio, Full, %s", market.c_str());
	h_rat_full_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_rat_full_);

	sprintf(name, "h_rat_incl_%s", market.c_str());
	sprintf(title, "Fill Ratio, Incl, %s", market.c_str());
	h_rat_incl_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_rat_incl_);

	sprintf(name, "h_rat_dv_%s", market.c_str());
	sprintf(title, "Fill Ratio, Dollarvol, %s", market.c_str());
	h_rat_dv_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_rat_dv_);

	sprintf(name, "h_rat_fake_%s", market.c_str());
	sprintf(title, "Fill Ratio, Against Fake Orders, %s", market.c_str());
	h_rat_fillAgainstFakeOrder_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_rat_fillAgainstFakeOrder_);

	sprintf(name, "h_rat_nonfake_%s", market.c_str());
	sprintf(title, "Fill Ratio, Against NonFake Orders, %s", market.c_str());
	h_rat_fillAgainstNonFakeOrder_ = TH1F(name, title, ndays, 1, ndays + 1);
	set_axis_title(h_rat_fillAgainstNonFakeOrder_);

	nDay_ = 0;

	return;
}

void HFillTimeHist::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	mMatchNOrderFilledFull_.clear();
	mMatchNOrderFilledIncl_.clear();
	mMatchNOrderTotal_.clear();
	mMatchDollarvolExec_.clear();
	mMatchDollarvol_.clear();

	nOrd_total_ = 0;
	nOrd_matched_ = 0;
	nOrd_unmatched_ = 0;
	nOrd_directFill_ = 0;
	nOrd_insertNFill_ = 0;
	nOrd_insertNCancel_ = 0;
	nOrd_unidFill_ = 0;
	nOrd_unidCancel_ = 0;

	nFilledFull_ = 0;
	nFilledIncl_ = 0;
	nTotal_ = 0;
	dvFilled_ = 0;
	dvTotal_ = 0;
	nFilledOnFake_ = 0;
	nTotalOnFake_ = 0;
	nFilledOnNonFake_ = 0;
	nTotalOnNonFake_ = 0;

	char name[200];
	char title[200];

	// match.
	sprintf(name, "h_quoteMatch_%s_%d", market.c_str(), idate);
	sprintf(title, "Quote Match Match %s %d", market.c_str(), idate);
	h_quoteMatch_day_ = TH1F(name, title, 10, 0, 10);

	// q2o.
	sprintf(name, "p_q2o_o_%s_%d", market.c_str(), idate);
	sprintf(title, "q2o vs. Time of day %s %d", market.c_str(), idate);
	p_q2o_o_ = TProfile(name, title, 100, 28800000, 59400000);


	// Time by day.
	sprintf(name, "h_o2e_filled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "o2e, Filled, %s %d", market.c_str(), idate);
	h_o2e_filled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_o2v_filled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "o2v, Filled after missing, %s %d", market.c_str(), idate);
	h_o2v_filled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_q2d_filled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "q2d, Filled, %s %d", market.c_str(), idate);
	h_q2d_filled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_q2o_filled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "q2o, Filled, %s %d", market.c_str(), idate);
	h_q2o_filled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_q2i_filled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "q2i, Filled, %s %d", market.c_str(), idate);
	h_q2i_filled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_i2x_filled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "i2x, Filled, %s %d", market.c_str(), idate);
	h_i2x_filled_today_ = TH1F(name, title, 5500, -1000, 10000 );


	sprintf(name, "h_o2e_notFilled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "o2e, Not filled, %s %d", market.c_str(), idate);
	h_o2e_notFilled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_o2v_notFilled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "o2v, Not filled afeter missing, %s %d", market.c_str(), idate);
	h_o2v_notFilled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_q2d_notFilled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "q2d, Not filled, %s %d", market.c_str(), idate);
	h_q2d_notFilled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_q2o_notFilled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "q2o, Not filled, %s %d", market.c_str(), idate);
	h_q2o_notFilled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_q2i_notFilled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "q2i, Not filled, %s %d", market.c_str(), idate);
	h_q2i_notFilled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	sprintf(name, "h_i2x_notFilled_today_%s_%d", market.c_str(), idate);
	sprintf(title, "i2x, Not filled, %s %d", market.c_str(), idate);
	h_i2x_notFilled_today_ = TH1F(name, title, 5500, -1000, 10000 );

	++nDay_;

	return;
}

void HFillTimeHist::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	const vector<MercatorOrder>* vMOrderBuy
		= static_cast<const vector<MercatorOrder>*>(HEvent::Instance()->get(ticker, (string)"vMOrderBuy" + procName_));
	const vector<MercatorOrder>* vMOrderSell
		= static_cast<const vector<MercatorOrder>*>(HEvent::Instance()->get(ticker, (string)"vMOrderSell" + procName_));

	if( vMOrderBuy != 0 && vMOrderSell != 0 )
	{
		fill_hist( vMOrderBuy, ticker );
		fill_hist( vMOrderSell, ticker );
		update_fillrat( vMOrderBuy );
		update_fillrat( vMOrderSell );
		update_fillrat_bymatch( vMOrderBuy );
		update_fillrat_bymatch( vMOrderSell );
		update_nOrders( vMOrderBuy );
		update_nOrders( vMOrderSell );
	}

	return;
}

void HFillTimeHist::endTicker(const string& ticker)
{
	return;
}

void HFillTimeHist::endDay()
{
	string market = HEnv::Instance()->market();

	// daily values.
	h_o2e_filled_day_.SetBinContent(nDay_, get_top5bin_avg(h_o2e_filled_today_));
	h_o2v_filled_day_.SetBinContent(nDay_, get_top5bin_avg(h_o2v_filled_today_));
	h_q2d_filled_day_.SetBinContent(nDay_, get_top5bin_avg(h_q2d_filled_today_, fakeMsecsMax_));
	h_q2o_filled_day_.SetBinContent(nDay_, get_top5bin_avg(h_q2o_filled_today_));
	h_q2i_filled_day_.SetBinContent(nDay_, get_top5bin_avg(h_q2i_filled_today_));
	h_i2x_filled_day_.SetBinContent(nDay_, get_top5bin_avg(h_i2x_filled_today_));
	h_fakeRat_filled_day_.SetBinContent(nDay_, get_fakeRat(h_q2d_filled_today_));

	h_o2e_notFilled_day_.SetBinContent(nDay_, get_top5bin_avg(h_o2e_notFilled_today_));
	h_o2v_notFilled_day_.SetBinContent(nDay_, get_top5bin_avg(h_o2v_notFilled_today_));
	h_q2d_notFilled_day_.SetBinContent(nDay_, get_top5bin_avg(h_q2d_notFilled_today_, fakeMsecsMax_));
	h_q2o_notFilled_day_.SetBinContent(nDay_, get_top5bin_avg(h_q2o_notFilled_today_));
	h_q2i_notFilled_day_.SetBinContent(nDay_, get_top5bin_avg(h_q2i_notFilled_today_));
	h_i2x_notFilled_day_.SetBinContent(nDay_, get_top5bin_avg(h_i2x_notFilled_today_));
	h_fakeRat_notFilled_day_.SetBinContent(nDay_, get_fakeRat(h_q2d_notFilled_today_));

	// Fill ratio.
	for( int match = 0; match <= 10; ++match )
	{
		double ratFull = 0;
		double ratIncl = 0;
		double ratDV = 0;
		double errFull = 0;
		double errIncl = 0;
		double errDV = 0;
		double nOrdFull = mMatchNOrderFilledFull_[match];
		double nOrdIncl = mMatchNOrderFilledIncl_[match];
		double nOrdTot = mMatchNOrderTotal_[match];
		double dvExec = mMatchDollarvolExec_[match];
		double dv = mMatchDollarvol_[match];
		if( nOrdTot > 0 )
		{
			ratFull = nOrdFull / nOrdTot;
			ratIncl = nOrdIncl / nOrdTot;
			errFull = sqrt( ratFull * (1.0 - ratFull) / nOrdTot );
			errIncl = sqrt( ratIncl * (1.0 - ratIncl) / nOrdTot );
		}
		if( dv > 0 )
		{
			ratDV = dvExec / dv;
			errDV = sqrt( ratDV * (1.0 - ratDV) / dv );
		}
		h_fillrat_full_.SetBinContent(nDay_, match, ratFull);
		h_fillrat_incl_.SetBinContent(nDay_, match, ratIncl);
		h_fillrat_dv_.SetBinContent(nDay_, match, ratDV);
		h_fillrat_full_.SetBinError(nDay_, match, errFull);
		h_fillrat_incl_.SetBinError(nDay_, match, errIncl);
		h_fillrat_dv_.SetBinError(nDay_, match, errDV);
	}

	// Orders.
	h_nOrd_total_.SetBinContent(nDay_, nOrd_total_);
	h_nOrd_matched_.SetBinContent(nDay_, nOrd_matched_);
	h_nOrd_unmatched_.SetBinContent(nDay_, nOrd_unmatched_);
	h_nOrd_directFill_.SetBinContent(nDay_, nOrd_directFill_);
	h_nOrd_insertNFill_.SetBinContent(nDay_, nOrd_insertNFill_);
	h_nOrd_insertNCancel_.SetBinContent(nDay_, nOrd_insertNCancel_);
	h_nOrd_unidFill_.SetBinContent(nDay_, nOrd_unidFill_);
	h_nOrd_unidCancel_.SetBinContent(nDay_, nOrd_unidCancel_);

	double ratMatched = 0;
	double ratUnmatched = 0;
	double ratDirectFill = 0;
	double ratInsertNFill = 0;
	double ratInsertNCancel = 0;
	double ratUnidfill = 0;
	double ratUnidCancel = 0;
	double ratFillWhenInserted = 0;

	double errMatched = 0;
	double errUnmatched = 0;
	double errDirectFill = 0;
	double errInsertNFill = 0;
	double errInsertNCancel = 0;
	double errUnidfill = 0;
	double errUnidCancel = 0;
	double errFillWhenInserted = 0;

	if( nOrd_total_ > 0 )
	{
		ratMatched = nOrd_matched_ / nOrd_total_;
		ratUnmatched = nOrd_unmatched_ / nOrd_total_;
		ratDirectFill = nOrd_directFill_ / nOrd_total_;
		ratInsertNFill = nOrd_insertNFill_ / nOrd_total_;
		ratInsertNCancel = nOrd_insertNCancel_ / nOrd_total_;
		ratUnidfill = nOrd_unidFill_ / nOrd_total_;
		ratUnidCancel = nOrd_unidCancel_ / nOrd_total_;
		if( nOrd_insertNFill_ + nOrd_insertNCancel_ > 0 )
			ratFillWhenInserted = nOrd_insertNFill_ / (nOrd_insertNFill_ + nOrd_insertNCancel_);

		errMatched = sqrt( ratMatched * (1.0 - ratMatched) / nOrd_total_ );
		errUnmatched = sqrt( ratMatched * (1.0 - ratUnmatched) / nOrd_total_ );
		errDirectFill = sqrt( ratMatched * (1.0 - ratDirectFill) / nOrd_total_ );
		errInsertNFill = sqrt( ratMatched * (1.0 - ratInsertNFill) / nOrd_total_ );
		errInsertNCancel = sqrt( ratMatched * (1.0 - ratInsertNCancel) / nOrd_total_ );
		errUnidfill = sqrt( ratMatched * (1.0 - ratUnidfill) / nOrd_total_ );
		errUnidCancel = sqrt( ratMatched * (1.0 - ratUnidCancel) / nOrd_total_ );
		if( nOrd_insertNFill_ + nOrd_insertNCancel_ > 0 )
			errFillWhenInserted = sqrt( ratFillWhenInserted * (1.0 - ratFillWhenInserted) / (nOrd_insertNFill_ + nOrd_insertNCancel_) );
	}

	h_ratOrd_matched_.SetBinContent(nDay_, ratMatched);
	h_ratOrd_unmatched_.SetBinContent(nDay_, ratUnmatched);
	h_ratOrd_directFill_.SetBinContent(nDay_, ratDirectFill);
	h_ratOrd_insertNFill_.SetBinContent(nDay_, ratInsertNFill);
	h_ratOrd_insertNCancel_.SetBinContent(nDay_, ratInsertNCancel);
	h_ratOrd_unidFill_.SetBinContent(nDay_, ratUnidfill);
	h_ratOrd_unidCancel_.SetBinContent(nDay_, ratUnidCancel);
	h_ratOrd_fillWhenInserted_.SetBinContent(nDay_, ratFillWhenInserted);

	h_ratOrd_matched_.SetBinError(nDay_, errMatched);
	h_ratOrd_unmatched_.SetBinError(nDay_, errUnmatched);
	h_ratOrd_directFill_.SetBinError(nDay_, errDirectFill);
	h_ratOrd_insertNFill_.SetBinError(nDay_, errInsertNFill);
	h_ratOrd_insertNCancel_.SetBinError(nDay_, errInsertNCancel);
	h_ratOrd_unidFill_.SetBinError(nDay_, errUnidfill);
	h_ratOrd_unidCancel_.SetBinError(nDay_, errUnidCancel);
	h_ratOrd_fillWhenInserted_.SetBinError(nDay_, errFillWhenInserted);

	// Fill ratio.
	double ratFull = 0;
	double ratIncl = 0;
	double ratDV = 0;
	double ratFake = 0;
	double ratNonFake = 0;

	double errFull = 0;
	double errIncl = 0;
	double errDV = 0;
	double errFake = 0;
	double errNonFake = 0;

	if( nTotal_ > 0 )
	{
		ratFull = nFilledFull_ / nTotal_;
		ratIncl = nFilledIncl_ / nTotal_;
		if( dvTotal_ > 0 )
			ratDV = dvFilled_ / dvTotal_;
		if( nTotalOnFake_ > 0 )
			ratFake = nFilledOnFake_ / nTotalOnFake_;
		if( nTotalOnNonFake_ > 0 )
			ratNonFake = nFilledOnNonFake_ / nTotalOnNonFake_;

		h_rat_full_.SetBinContent(nDay_, ratFull);
		h_rat_incl_.SetBinContent(nDay_, ratIncl);
		h_rat_dv_.SetBinContent(nDay_, ratDV);
		h_rat_fillAgainstFakeOrder_.SetBinContent(nDay_, ratFake);
		h_rat_fillAgainstNonFakeOrder_.SetBinContent(nDay_, ratNonFake);

		h_rat_full_.SetBinError(nDay_, errFull);
		h_rat_incl_.SetBinError(nDay_, errIncl);
		h_rat_dv_.SetBinError(nDay_, errDV);
		h_rat_fillAgainstFakeOrder_.SetBinError(nDay_, errFake);
		h_rat_fillAgainstNonFakeOrder_.SetBinError(nDay_, errNonFake);
	}

	// q2o.
	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd("timeOfDay");

	p_q2o_o_.Write();

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd("match");

	h_quoteMatch_day_.Write();

	return;
}

void HFillTimeHist::endMarket()
{
	string market = HEnv::Instance()->market();
	TFile* f = HEnv::Instance()->outfile();

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());

	h_quoteMatch_.Write();

	h_o2e_filled_.Write();
	h_o2v_filled_.Write();
	h_q2d_filled_.Write();
	h_q2o_filled_.Write();
	h_q2i_filled_.Write();
	h_i2x_filled_.Write();

	h_o2e_notFilled_.Write();
	h_o2v_notFilled_.Write();
	h_q2d_notFilled_.Write();
	h_q2o_notFilled_.Write();
	h_q2i_notFilled_.Write();
	h_i2x_notFilled_.Write();

	p_q2o_alph_.Write();

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd("vsDay");

	h_o2e_filled_day_.Write();
	h_o2v_filled_day_.Write();
	h_q2d_filled_day_.Write();
	h_q2o_filled_day_.Write();
	h_q2i_filled_day_.Write();
	h_i2x_filled_day_.Write();
	h_fakeRat_filled_day_.Write();

	h_o2e_notFilled_day_.Write();
	h_o2v_notFilled_day_.Write();
	h_q2d_notFilled_day_.Write();
	h_q2o_notFilled_day_.Write();
	h_q2i_notFilled_day_.Write();
	h_i2x_notFilled_day_.Write();
	h_fakeRat_notFilled_day_.Write();

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd("nOrders");

	h_nOrd_total_.Write();
	h_nOrd_matched_.Write();
	h_nOrd_unmatched_.Write();
	h_nOrd_directFill_.Write();
	h_nOrd_insertNFill_.Write();
	h_nOrd_insertNCancel_.Write();
	h_nOrd_unidFill_.Write();
	h_nOrd_unidCancel_.Write();

	h_ratOrd_matched_.Write();
	h_ratOrd_unmatched_.Write();
	h_ratOrd_directFill_.Write();
	h_ratOrd_insertNFill_.Write();
	h_ratOrd_insertNCancel_.Write();
	h_ratOrd_unidFill_.Write();
	h_ratOrd_unidCancel_.Write();
	h_ratOrd_fillWhenInserted_.Write();


	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd("fillratbymatch");

	h_fillrat_full_.Write();
	h_fillrat_incl_.Write();
	h_fillrat_dv_.Write();

	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd("fillrat");

	h_rat_full_.Write();
	h_rat_incl_.Write();
	h_rat_dv_.Write();
	h_rat_fillAgainstFakeOrder_.Write();
	h_rat_fillAgainstNonFakeOrder_.Write();

	return;
}

void HFillTimeHist::endJob()
{
	return;
}

void HFillTimeHist::set_axis_title(TH1& h)
{
	vector<int> idates = HEnv::Instance()->idates();
	int b = 1;
	for( vector<int>::iterator it = idates.begin(); it != idates.end(); ++it, ++b )
	{
		int idate = *it;
		h.GetXaxis()->SetBinLabel( b, itos(idate).c_str() );
	}
	return;
}

void HFillTimeHist::fill_hist( const vector<MercatorOrder>* vp, string ticker )
{
	for( vector<MercatorOrder>::const_iterator it = vp->begin(); it != vp->end(); ++it )
	{
		boost::mutex::scoped_lock lock(hist_mutex_);

		h_quoteMatch_.Fill(it->quoteMatch);
		h_quoteMatch_day_.Fill(it->quoteMatch);

		if( it->quoteMatch > 1 )
		{
			int q2o = 24*60*60*1000;
			int o2e = 24*60*60*1000;
			int q2d = 24*60*60*1000;
			int q2i = 24*60*60*1000;
			int i2x = 24*60*60*1000;
			if( it->orderMsecs > 0 && it->quoteMsecs > 0 )
				q2o = it->orderMsecs - it->quoteMsecs;
			if( it->eventMsecs > 0 && it->orderMsecs > 0 )
				o2e = it->eventMsecs - it->orderMsecs;
			if( it->detMsecs > 0 && it->quoteMsecs > 0 )
				q2d = it->detMsecs - it->quoteMsecs;
			if( it->insertMsecs > 0 && it->quoteMsecs > 0 )
				q2i = it->insertMsecs - it->quoteMsecs;
			if( it->executeMsecs > 0 && it->insertMsecs > 0 )
				i2x = it->executeMsecs - it->insertMsecs;

			char alph = ticker[0];

			p_q2o_o_.Fill(it->orderMsecs, q2o);
			p_q2o_alph_.Fill(alph, q2o);

			//bool q2dSmall = q2d < fakeMsecsMax_;
			bool inserted = fakeMsecsMax_ < q2i && q2i < 1000;

			if( it->isFilledIncl() )
			{
				if( inserted )
					h_o2v_filled_.Fill(o2e);
				else
					h_o2e_filled_.Fill(o2e);
				h_q2o_filled_.Fill(q2o);
				h_q2d_filled_.Fill(q2d);
				if( inserted )
				{
					h_q2i_filled_.Fill(q2i);
					h_i2x_filled_.Fill(i2x);
				}

				if( inserted )
					h_o2v_filled_today_.Fill(o2e);
				else
					h_o2e_filled_today_.Fill(o2e);
				h_q2d_filled_today_.Fill(q2d);
				h_q2o_filled_today_.Fill(q2o);
				if( inserted )
				{
					h_q2i_filled_today_.Fill(q2i);
					h_i2x_filled_today_.Fill(i2x);
				}
			}
			else
			{
				if( inserted )
					h_o2v_notFilled_.Fill(o2e);
				else
					h_o2e_notFilled_.Fill(o2e);
				h_q2o_notFilled_.Fill(q2o);
				h_q2d_notFilled_.Fill(q2d);
				if( inserted )
				{
					h_q2i_notFilled_.Fill(q2i);
					h_i2x_notFilled_.Fill(i2x);
				}

				if( inserted )
					h_o2v_notFilled_today_.Fill(o2e);
				else
					h_o2e_notFilled_today_.Fill(o2e);
				h_q2d_notFilled_today_.Fill(q2d);
				h_q2o_notFilled_today_.Fill(q2o);
				if( inserted )
				{
					h_q2i_notFilled_today_.Fill(q2i);
					h_i2x_notFilled_today_.Fill(i2x);
				}
			}
		}
	}
	return;
}

void HFillTimeHist::update_fillrat_bymatch( const std::vector<MercatorOrder>* vp )
{
	for( vector<MercatorOrder>::const_iterator it = vp->begin(); it != vp->end(); ++it )
	{
		int match = it->quoteMatch;
		double price = it->price;
		double qty = it->qty;
		double qtyExec = it->qtyExec;

		boost::mutex::scoped_lock lock(map_mutex_);

		if( it->isFilledFull() )
			++mMatchNOrderFilledFull_[match];
		if( it->isFilledIncl() )
			++mMatchNOrderFilledIncl_[match];
		++mMatchNOrderTotal_[match];
		mMatchDollarvolExec_[match] += qtyExec * price;
		mMatchDollarvol_[match] += qty * price;

		if( it->isFilledFull() )
			++mMatchNOrderFilledFull_[10];
		if( it->isFilledIncl() )
			++mMatchNOrderFilledIncl_[10];
		++mMatchNOrderTotal_[10];
		mMatchDollarvolExec_[10] += qtyExec * price;
		mMatchDollarvol_[10] += qty * price;
	}
	return;
}

void HFillTimeHist::update_fillrat( const std::vector<MercatorOrder>* vp )
{
	for( vector<MercatorOrder>::const_iterator it = vp->begin(); it != vp->end(); ++it )
	{
		int match = it->quoteMatch;
		double price = it->price;
		double qty = it->qty;
		double qtyExec = it->qtyExec;

		boost::mutex::scoped_lock lock(fillrat_mutex_);

		if( it->isFilledFull() )
			++nFilledFull_;
		if( it->isFilledIncl() )
			++nFilledIncl_;
		++nTotal_;
		dvFilled_ += qtyExec * price;
		dvTotal_ += qty * price;

		if( it->quoteMatch > 1 )
		{
			int q2d = 24*60*60*1000;
			if( it->detMsecs > 0 && it->quoteMsecs > 0 )
				q2d = it->detMsecs - it->quoteMsecs;

			if( q2d < fakeMsecsMax_ )
			{
				if( it->isFilledIncl() )
					++nFilledOnFake_;
				++nTotalOnFake_;
			}
			else
			{
				if( it->isFilledIncl() )
					++nFilledOnNonFake_;
				++nTotalOnNonFake_;
			}
		}
	}
	return;
}

void HFillTimeHist::update_nOrders( const std::vector<MercatorOrder>* vp )
{
	for( vector<MercatorOrder>::const_iterator it = vp->begin(); it != vp->end(); ++it )
	{
		int o2e = 24*60*60*1000;
		int q2i = 24*60*60*1000;
		if( it->eventMsecs > 0 && it->orderMsecs > 0 )
			o2e = it->eventMsecs - it->orderMsecs;
		if( it->insertMsecs > 0 && it->quoteMsecs > 0 )
			q2i = it->insertMsecs - it->quoteMsecs;
		bool isFilled = it->isFilledIncl();

		boost::mutex::scoped_lock lock(nOrd_mutex_);

		++nOrd_total_;
		if( it->quoteMatch > 1 )
		{
			++nOrd_matched_;

			if( q2i > 1000 && o2e < 1000 )
				++nOrd_directFill_;
			else if( q2i < 1000 )
			{
				if( isFilled )
					++nOrd_insertNFill_;
				else
					++nOrd_insertNCancel_;
			}
			else
			{
				if( isFilled )
					++nOrd_unidFill_;
				else
					++nOrd_unidCancel_;
			}
		}
		else
			++nOrd_unmatched_;
	}

	return;
}

double HFillTimeHist::get_top5bin_avg(TH1F& h, double thresMsecs)
{
	vector<double> v;
	int N = h.GetNbinsX();
	for( int b=1; b<=N; ++b )
		v.push_back(h.GetBinContent(b));

	sort(v.begin(), v.end());
	int vs = v.size();
	double thres = v[max(0, vs-5)];

	double sumProd = 0;
	double sumContent = 0;
	for( int b=1; b<=N; ++b )
	{
		double c = h.GetBinContent(b);
		if( c >= thres )
		{
			double binCenter = (h.GetXaxis()->GetBinLowEdge(b) + h.GetXaxis()->GetBinLowEdge(b+1)) / 2.0;
			if( binCenter > thresMsecs )
			{
				sumProd += binCenter * c;
				sumContent += c;
			}
		}
	}

	double avg = 0;
	if( sumContent > 0 )
		avg = sumProd / sumContent;

	return avg;
}

double HFillTimeHist::get_fakeRat(TH1F& h)
{
	double nFake = 0;
	double nTotal = 0;
	int N = h.GetNbinsX();
	for( int b=1; b<=N; ++b )
	{
		double c = h.GetBinContent(b);
		double binCenter = (h.GetXaxis()->GetBinLowEdge(b) + h.GetXaxis()->GetBinLowEdge(b+1)) / 2.0;
		if( 0 < binCenter && binCenter < 1000 )
		{
			nTotal += c;
			if( binCenter < fakeMsecsMax_ )
				nFake += c;
		}
	}
	double rat = 0;
	if( nTotal > 0 )
		rat = nFake / nTotal;
	return rat;
}