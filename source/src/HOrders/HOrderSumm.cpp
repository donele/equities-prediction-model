#include <HOrders.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <optionlibs/TickData.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <TFile.h>
using namespace std;

HOrderSumm::HOrderSumm(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
verbose_(0),
writeDB_(true),
dailyJob_(true),
dest_("primary"),
basedir_(""),
fakeMsecsMax_(3),
q2iMax_(1000),
msecDirect_(1000),
histMax_(10000),
side_('\0'), // (B)uy, (S)ell.
execType_('B'), // (B)oth, (L), (A).
orderSchedType_(0) // 0: all. 1: scheduled. 2: asymm.
{
	if( conf.count("writeDB") )
		writeDB_ = conf.find("writeDB")->second == "true";
	if( conf.count("dailyJob") )
		dailyJob_ = conf.find("dailyJob")->second == "true";
	if( conf.count("basedir") )
		basedir_ = conf.find("basedir")->second;
	if( conf.count("dest") )
		dest_ = conf.find("dest")->second;
	if( conf.count("side") )
		side_ = conf.find("side")->second[0];
	if( conf.count("execType") )
		execType_ = conf.find("execType")->second[0];
	if( conf.count("orderSchedType") )
		orderSchedType_ = atoi( conf.find("orderSchedType")->second.c_str() );
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );

	if( !dailyJob_ )
		writeDB_ = false;
}

HOrderSumm::~HOrderSumm()
{}

void HOrderSumm::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	if( !dailyJob_ )
	{
		TFile* f = HEnv::Instance()->outfile();
		f->mkdir(moduleName_.c_str());
	}

	return;
}

void HOrderSumm::beginMarket()
{
	string market = HEnv::Instance()->market();

	region_ = mto::region_long(market);

	// Override slow/fast markets.
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
	else if( "U" == mto::region(market) )
	{
		q2iMax_ = 100;
		msecDirect_ = 100;
		histMax_ = 2000;
		fakeMsecsMax_ = 1;
	}

	char name[200];
	char title[200];
	double histMin = -histMax_ / 10.0;
	int nbins = ceil( histMax_ + fabs(histMin) - 0.5 );
	if( nbins > 5500 * 5 )
		nbins = 5500;

	// match.
	sprintf(name, "h_quoteMatch");
	sprintf(title, "Quote Match Match %s", market.c_str());
	h_quoteMatch_ = TH1F(name, title, 11, 0, 11);

	// as function of time
	sprintf(name, "p_q2o");
	sprintf(title, "q2o vs. Time of day %s", market.c_str());
	p_q2o_ = TProfile(name, title, 100, 28800000, 59400000, -2000, 2000);

	sprintf(name, "p_fillrat");
	sprintf(title, "fillrat vs. Time of day %s", market.c_str());
	p_fillrat_ = TProfile(name, title, 40, 28800000, 59400000);

	sprintf(name, "h_norders");
	sprintf(title, "norders vs. Time of day %s", market.c_str());
	h_norders_ = TH1F(name, title, 40, 28800000, 59400000);

	// Time by day.
	sprintf(name, "h_q2i");
	sprintf(title, "q2i, %s", market.c_str());
	h_q2i_ = TH1F(name, title, nbins, histMin, histMax_ );


	sprintf(name, "h_o2e_filled");
	sprintf(title, "o2e, Filled, %s", market.c_str());
	h_o2e_filled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_o2v_filled");
	sprintf(title, "o2v, Filled after missing, %s", market.c_str());
	h_o2v_filled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2d_filled");
	sprintf(title, "q2d, Filled, %s", market.c_str());
	h_q2d_filled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2t_filled");
	sprintf(title, "q2t, Filled, %s", market.c_str());
	h_q2t_filled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2d_direct_filled");
	sprintf(title, "q2d, Direct Filled, %s", market.c_str());
	h_q2d_direct_filled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2o_filled");
	sprintf(title, "q2o, Filled, %s", market.c_str());
	h_q2o_filled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2i_filled");
	sprintf(title, "q2i, Filled, %s", market.c_str());
	h_q2i_filled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_i2x_filled");
	sprintf(title, "i2x, Filled, %s", market.c_str());
	h_i2x_filled_ = TH1F(name, title, nbins, histMin, histMax_ );


	sprintf(name, "h_o2e_notFilled");
	sprintf(title, "o2e, Not filled, %s", market.c_str());
	h_o2e_notFilled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_o2v_notFilled");
	sprintf(title, "o2v, Not filled afeter missing, %s", market.c_str());
	h_o2v_notFilled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2d_notFilled");
	sprintf(title, "q2d, Not filled, %s", market.c_str());
	h_q2d_notFilled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2t_notFilled");
	sprintf(title, "q2t, Not filled, %s", market.c_str());
	h_q2t_notFilled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2o_notFilled");
	sprintf(title, "q2o, Not filled, %s", market.c_str());
	h_q2o_notFilled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_q2i_notFilled");
	sprintf(title, "q2i, Not filled, %s", market.c_str());
	h_q2i_notFilled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_i2x_notFilled");
	sprintf(title, "i2x, Not filled, %s", market.c_str());
	h_i2x_notFilled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "h_d2i_notFilled");
	sprintf(title, "d2i, Not filled, %s", market.c_str());
	h_d2i_notFilled_ = TH1F(name, title, nbins, histMin, histMax_ );

	sprintf(name, "p_fillrat_f2o");
	sprintf(title, "fillrat vs f2o, %s", market.c_str());
	p_fillrat_f2o_ = TProfile(name, title, nbins, histMin, histMax_ );

	reset();

	return;
}

void HOrderSumm::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	if( verbose_ == 2 )
	{
		char ofn[200];
		sprintf( ofn, "%s_%s_%d.txt", moduleName_.c_str(), market.c_str(), idate );
		ofstream ofs(ofn);
	}

	if( dailyJob_ )
		reset();

	GODBC::Instance()->close_all();

	return;
}

void HOrderSumm::beginTicker(const string& ticker)
{
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

		if( verbose_ > 0 )
		{
			for( vector<MercatorOrder>::const_iterator it = orders.begin(); it != orders.end(); ++it )
			{
				int q2o = 24*60*60*1000;
				int o2e = 24*60*60*1000;
				int q2d = 24*60*60*1000;
				int q2t = 24*60*60*1000;
				int q2i = 24*60*60*1000;
				int i2x = 24*60*60*1000;
				if( it->orderMsecs > 0 && it->quoteMsecs > 0 )
					q2o = it->orderMsecs - it->quoteMsecs;
				if( it->eventMsecs > 0 && it->orderMsecs > 0 )
					o2e = it->eventMsecs - it->orderMsecs;
				if( it->detMsecs > 0 && it->quoteMsecs > 0 )
					q2d = it->detMsecs - it->quoteMsecs;
				if( it->trdMsecs > 0 && it->quoteMsecs > 0 )
					q2t = it->trdMsecs - it->quoteMsecs;
				if( it->insertMsecs > 0 && it->quoteMsecs > 0 )
					q2i = it->insertMsecs - it->quoteMsecs;
				if( it->executeMsecs > 0 && it->insertMsecs > 0 )
					i2x = it->executeMsecs - it->insertMsecs;

				if( verbose_ == 1 )
				{
					char buf[400];
					sprintf( buf, "%10s %d %d %d o2e %6d q2d %8d q2i %6d i2x %6d\n",
						ticker.c_str(), it->orderMsecs, it->quoteMatch, it->isFilledIncl(),
						o2e, q2d, q2i, i2x);
					cout << buf;
				}
				else if( verbose_ == 2 )
				{
					char buf[400];
					sprintf( buf, "%10s %d %d %d o2e %6d q2d %8d q2i %6d i2x %6d\n",
						ticker.c_str(), it->orderMsecs, it->quoteMatch, it->isFilledIncl(),
						o2e, q2d, q2i, i2x);

					boost::mutex::scoped_lock lock(vio_mutex_);
					char ofn[200];
					sprintf( ofn, "%s_%s_%d.txt", moduleName_.c_str(), market.c_str(), idate );
					ofstream ofs(ofn, ios::app);
					ofs << buf;
				}
				else if( verbose_ == 3 )
				{
					char buf[400];
					sprintf( buf, "%10s %d %d %d q2o %5d o2e %6d q2d %8d q2i %6d i2x %6d\n",
						ticker.c_str(), it->orderMsecs, it->quoteMatch, it->isFilledIncl(),
						q2o, o2e, q2d, q2i, i2x);
					cout << buf;
				}
				else if( verbose_ == 4 && it->qtyExec > 0 )
				{
					char buf[400];
					sprintf( buf, "%10s %d %d %.2f %5d %5d %8.2f\n",
						ticker.c_str(), it->orderMsecs, it->quoteMatch, it->price, it->qty, it->qtyExec, it->gain);
					cout << buf;
				}
			}
		}

		if( !orders.empty() )
		{
			fill_hist( orders, ticker );
			update_fillrat( orders );
			update_nOrders( orders );
		}
	}

	return;
}

void HOrderSumm::endTicker(const string& ticker)
{
	return;
}

void HOrderSumm::endDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	if( dailyJob_ )
	{
		string exchange = mto::code(market);
		string destCode = dest_.substr(1,1);
		if( "primary" == dest_ )
			destCode = exchange;

		double o2efilled = 0.;
		double q2dfilled = 0.;
		double q2tfilled = 0.;
		double q2dnotfilled = 0.;
		double q2tnotfilled = 0.;
		double d2inotfilled = 0.;
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
		get_summary(o2efilled, q2dfilled, q2tfilled, q2dnotfilled, q2tnotfilled, d2inotfilled, q2i, nfakeNotFilled, nfakeFilled, ncompetition,
			fillratshr, fillratincl, fillratinclmatched, fillratinclunmatched, fillratnonfake,
			ratdirectfill, ratmatched, ratfake, ratcomp);

		double gpt = 0.;
		double gpt_filled = 0.;
		double gpt_notfilled = 0.;
		double gpt_missed = 0.;
		if( dv_ > 0 )
			gpt = profit_ / dv_;
		if( dv_filled_ > 0 )
			gpt_filled = profit_filled_ / dv_filled_;
		if( dv_notfilled_ > 0 )
			gpt_notfilled = profit_notfilled_ / dv_notfilled_;
		if( dv_missed_ > 0 )
			gpt_missed = profit_missed_ / dv_missed_;

		if( writeDB_ )
		{
			string cmd = (string)" select * from hfordersumm "
				+ " where idate = " + itos(idate)
				+ " and dest = " + quote(destCode)
				+ " and exchange = " + quote(exchange)
				+ " and exectype = " + quote(string(1, execType_))
				+ " and schedtype = " + itos(orderSchedType_);
			vector<vector<string> > vv;
			//GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
			GODBC::Instance()->read(mto::hf(market), cmd, vv);
			if( vv.empty() )
			{
				string cmd = (string)" insert into hfordersumm "
					+ " (idate, dest, exchange, exectype, schedtype, fillratincl, fillratnonfake, ratdirectfill, ratmatched, ratfake, ratcomp, "
					+ " norder, nfillfull, nfillincl, dvorder, dvfill, nmatched, ndirectfill, ninsertfill, ninsertcancel, "
					+ "norderfake, nfillfake, nordernonfake, nfillnonfake, ncomp, o2efilled, q2dfilled, q2dnotfilled, gpt) "
					+ " values ("
					+ itos(idate) + ", "
					+ quote(destCode) + ", "
					+ quoteN(exchange) + ", "
					+ quote(string(1, execType_)) + ", "
					+ itos(orderSchedType_) + ", "

					+ dtos(fillratincl, 3) + ", "
					+ dtos(fillratnonfake, 3) + ", "
					+ dtos(ratdirectfill, 3) + ", "
					+ dtos(ratmatched, 3) + ", "
					+ dtos(ratfake, 3) + ", "
					+ dtos(ratcomp, 3) + ", "

					+ itos(nTotal_) + ", "
					+ itos(nFilledFull_) + ", "
					+ itos(nFilledIncl_) + ", "
					+ dtos(dvTotal_, 1) + ", "
					+ dtos(dvFilled_, 1) + ", "
					+ itos(nOrd_matched_) + ", "
					+ itos(nOrd_directFill_) + ", "
					+ itos(nOrd_insertNFill_) + ", "
					+ itos(nOrd_insertNCancel_) + ", "
					+ itos(nTotalOnFake_) + ", "
					+ itos(nFilledOnFake_) + ", "
					+ itos(nTotalOnNonFake_) + ", "
					+ itos(nFilledOnNonFake_) + ", "
					+ itos(ncompetition) + ", "
					+ dtos(o2efilled, 1) + ", "
					+ dtos(q2dfilled, 1) + ", "
					+ dtos(q2dnotfilled, 1) + ", "
					+ dtos(gpt, 2)
					+ ")";
				//GODBC::Instance()->get(mto::hf(market))->ExecDirect(cmd.c_str());
				GODBC::Instance()->exec(mto::hf(market), cmd);
			}
			else
			{
				string cmd = (string)" update hfordersumm "
					+ " set fillratincl = " + dtos(fillratincl, 3) + ", "
					+ " fillratnonfake = " + dtos(fillratnonfake, 3) + ", "
					+ " ratdirectfill = " + dtos(ratdirectfill, 3) + ", "
					+ " ratmatched = " + dtos(ratmatched, 3) + ", "
					+ " ratfake = " + dtos(ratfake, 3) + ", "
					+ " ratcomp = " + dtos(ratcomp, 3) + ", "

					+ " norder = " + itos(nTotal_) + ", "
					+ " nfillfull = " + itos(nFilledFull_) + ", "
					+ " nfillincl = " + itos(nFilledIncl_) + ", "
					+ " dvorder = " + dtos(dvTotal_, 1) + ", "
					+ " dvfill = " + dtos(dvFilled_, 1) + ", "
					+ " nmatched = " + itos(nOrd_matched_) + ", "
					+ " ndirectfill = " + itos(nOrd_directFill_) + ", "
					+ " ninsertfill = " + itos(nOrd_insertNFill_) + ", "
					+ " ninsertcancel = " + itos(nOrd_insertNCancel_) + ", "
					+ " norderfake = " + itos(nTotalOnFake_) + ", "
					+ " nfillfake = " + itos(nFilledOnFake_) + ", "
					+ " nordernonfake = " + itos(nTotalOnNonFake_) + ", "
					+ " nfillnonfake = " + itos(nFilledOnNonFake_) + ", "
					+ " ncomp = " + itos(ncompetition) + ", "
					+ " o2efilled = " + dtos(o2efilled, 1) + ", "
					+ " q2dfilled = " + dtos(q2dfilled, 1) + ", "
					+ " q2dnotfilled = " + dtos(q2dnotfilled, 1) + ", "
					+ " gpt = " + dtos(gpt, 2)

					+ " where idate = " + itos(idate)
					+ " and dest = " + quote(destCode)
					+ " and exchange = " + quote(exchange)
					+ " and exectype = " + quote(string(1, execType_))
					+ " and schedtype = " + itos(orderSchedType_);
				//GODBC::Instance()->get(mto::hf(market))->ExecDirect(cmd.c_str());
				GODBC::Instance()->exec(mto::hf(market), cmd);
			}
		}

		cout << "------------------------------------------------\n";
		cout << " > idate = " << idate << "\n";
		cout << " > dest = " << destCode << "\n";
		cout << " > exchange = " << exchange << "\n";
		cout << " > exectype = " << execType_ << "\n";
		cout << " > schedtype = " << orderSchedType_ << "\n";

		cout << " gpt = " << dtos(gpt, 3) << "\n";
		cout << " gptfilled = " << dtos(gpt_filled , 3) << "\n";
		cout << " gptnotfilled = " << dtos(gpt_notfilled , 3) << "\n";
		cout << " gptmissed = " << dtos(gpt_missed, 3) << "\n";
		cout << " fillratshr = " << dtos(fillratshr, 3) << "\n";
		cout << " fillratincl = " << dtos(fillratincl, 3) << "\n";
		cout << " fillratinclmatched = " << dtos(fillratinclmatched, 3) << "\n";
		cout << " fillratinclunmatched = " << dtos(fillratinclunmatched, 3) << "\n";
		cout << " fillratnonfake = " << dtos(fillratnonfake, 3) << "\n";
		cout << " ratdirectfill = " << dtos(ratdirectfill, 3) << "\n";
		cout << " ratmatched = " << dtos(ratmatched, 3) << "\n";
		cout << " ratfake = " << dtos(ratfake, 3) << "\n";
		cout << " ratcomp = " << dtos(ratcomp, 3) << "\n";

		cout << " nshr = " << itos(nShr_total_) << "\n";
		cout << " nshrfill = " << itos(nShr_filled_) << "\n";
		cout << " norder = " << itos(nTotal_) << "\n";
		cout << " nfillfull = " << itos(nFilledFull_) << "\n";
		cout << " nfillincl = " << itos(nFilledIncl_) << "\n";
		cout << " dvorder = " << dtos(dvTotal_, 1) << "\n";
		cout << " dvfill = " << dtos(dvFilled_, 1) << "\n";
		cout << " nmatched = " << itos(nOrd_matched_) << "\n";
		cout << " ndirectfill = " << itos(nOrd_directFill_) << "\n";
		cout << " ninsertfill = " << itos(nOrd_insertNFill_) << "\n";
		cout << " ninsertcancel = " << itos(nOrd_insertNCancel_) << "\n";
		cout << " norderfake = " << itos(nTotalOnFake_) << "\n";
		cout << " nfillfake = " << itos(nFilledOnFake_) << "\n";
		cout << " nordernonfake = " << itos(nTotalOnNonFake_) << "\n";
		cout << " nfillnonfake = " << itos(nFilledOnNonFake_) << "\n";
		cout << " ncomp = " << itos(ncompetition) << "\n";
		cout << " o2efilled = " << dtos(o2efilled, 3) << "\n";
		cout << " q2dfilled = " << dtos(q2dfilled, 3) << "\n";
		cout << " q2tfilled = " << dtos(q2tfilled, 3) << "\n";
		cout << " q2dnotfilled = " << dtos(q2dnotfilled, 3) << "\n";
		cout << " q2tnotfilled = " << dtos(q2tnotfilled, 3) << "\n";
		cout << " d2inotfilled = " << dtos(d2inotfilled, 3) << "\n";
		cout << " q2i = " << dtos(q2i, 3) << endl;

		if( !basedir_.empty() )
		{
			char filename[400];
			sprintf(filename, "%s\\%s\\%s\\%s\\%d\\ordersumm_%c_%d_%d.root",
				basedir_.c_str(), region_.c_str(), dest_.c_str(), market.c_str(), idate/10000, execType_, orderSchedType_, idate);
			ForceDirectory(filename);
			TFile outfile(xpf(filename).c_str(), "recreate");
			outfile.cd();

			p_q2o_.Write();
			p_fillrat_.Write();
			h_norders_.Write();

			h_quoteMatch_.Write();

			h_q2i_.Write();

			h_o2e_filled_.Write();
			h_o2v_filled_.Write();
			h_q2d_filled_.Write();
			h_q2t_filled_.Write();
			h_q2d_direct_filled_.Write();
			h_q2o_filled_.Write();
			h_q2i_filled_.Write();
			h_i2x_filled_.Write();
			h_q2t_filled_.Write();

			h_o2e_notFilled_.Write();
			h_o2v_notFilled_.Write();
			h_q2d_notFilled_.Write();
			h_q2t_notFilled_.Write();
			h_q2o_notFilled_.Write();
			h_q2i_notFilled_.Write();
			h_i2x_notFilled_.Write();
			h_d2i_notFilled_.Write();
			h_q2t_notFilled_.Write();

			p_fillrat_f2o_.Write();
		}
	}

	return;
}

void HOrderSumm::endMarket()
{
	string market = HEnv::Instance()->market();
	vector<int> idates = HEnv::Instance()->idates();

	if( !dailyJob_ )
	{
		{
			string exchange = mto::code(market);
			string destCode = dest_.substr(1,1);
			if( "primary" == dest_ )
				destCode = exchange;

			double o2efilled = 0.;
			double q2dfilled = 0.;
			double q2tfilled = 0.;
			double q2dnotfilled = 0.;
			double q2tnotfilled = 0.;
			double d2inotfilled = 0.;
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
			get_summary(o2efilled, q2dfilled, q2tfilled, q2dnotfilled, q2tnotfilled, d2inotfilled, q2i, nfakeNotFilled, nfakeFilled, ncompetition,
				fillratshr, fillratincl, fillratinclmatched, fillratinclunmatched, fillratnonfake,
				ratdirectfill, ratmatched, ratfake, ratcomp);

			double gpt = 0.;
			double gpt_filled = 0.;
			double gpt_notfilled = 0.;
			double gpt_missed = 0.;
			if( dv_ > 0. )
				gpt = profit_ / dv_;
			if( dv_filled_ > 0 )
				gpt_filled = profit_filled_ / dv_filled_;
			if( dv_notfilled_ > 0 )
				gpt_notfilled = profit_notfilled_ / dv_notfilled_;
			if( dv_missed_ > 0 )
				gpt_missed = profit_missed_ / dv_missed_;

			cout << "------------------------------------------------\n";
			cout << " > dest = " << destCode << "\n";
			cout << " > exchange = " << exchange << "\n";
			cout << " > exectype = " << execType_ << "\n";
			cout << " > schedtype = " << orderSchedType_ << "\n";

			cout << " gpt = " << dtos(gpt, 3) << "\n";
			cout << " gptfilled = " << dtos(gpt_filled, 3) << "\n";
			cout << " gptnotfilled = " << dtos(gpt_notfilled, 3) << "\n";
			cout << " gptmissed = " << dtos(gpt_missed, 3) << "\n";
			cout << " fillratshr = " << dtos(fillratshr, 3) << "\n";
			cout << " fillratincl = " << dtos(fillratincl, 3) << "\n";
			cout << " fillratinclmatched = " << dtos(fillratinclmatched, 3) << "\n";
			cout << " fillratinclunmatched = " << dtos(fillratinclunmatched, 3) << "\n";
			cout << " fillratnonfake = " << dtos(fillratnonfake, 3) << "\n";
			cout << " ratdirectfill = " << dtos(ratdirectfill, 3) << "\n";
			cout << " ratmatched = " << dtos(ratmatched, 3) << "\n";
			cout << " ratfake = " << dtos(ratfake, 3) << "\n";
			cout << " ratcomp = " << dtos(ratcomp, 3) << "\n";

			cout << " nshr = " << itos(nShr_total_) << "\n";
			cout << " nshrfill = " << itos(nShr_filled_) << "\n";
			cout << " norder = " << itos(nTotal_) << "\n";
			cout << " nfillfull = " << itos(nFilledFull_) << "\n";
			cout << " nfillincl = " << itos(nFilledIncl_) << "\n";
			cout << " dvorder = " << dtos(dvTotal_, 1) << "\n";
			cout << " dvfill = " << dtos(dvFilled_, 1) << "\n";
			cout << " nmatched = " << itos(nOrd_matched_) << "\n";
			cout << " ndirectfill = " << itos(nOrd_directFill_) << "\n";
			cout << " ninsertfill = " << itos(nOrd_insertNFill_) << "\n";
			cout << " ninsertcancel = " << itos(nOrd_insertNCancel_) << "\n";
			cout << " norderfake = " << itos(nTotalOnFake_) << "\n";
			cout << " nfillfake = " << itos(nFilledOnFake_) << "\n";
			cout << " nordernonfake = " << itos(nTotalOnNonFake_) << "\n";
			cout << " nfillnonfake = " << itos(nFilledOnNonFake_) << "\n";
			cout << " ncomp = " << itos(ncompetition) << "\n";
			cout << " o2efilled = " << dtos(o2efilled, 3) << "\n";
			cout << " q2dfilled = " << dtos(q2dfilled, 3) << "\n";
			cout << " q2tfilled = " << dtos(q2tfilled, 3) << "\n";
			cout << " q2dnotfilled = " << dtos(q2dnotfilled, 3) << "\n";
			cout << " q2tnotfilled = " << dtos(q2tnotfilled, 3) << "\n";
			cout << " d2inotfilled = " << dtos(d2inotfilled, 3) << "\n";
			cout << " q2i = " << dtos(q2i, 3) << endl;
		}

		int n = idates.size();
		int idate1 = idates[0];
		int idate2 = idates[n - 1];

		TFile* f = HEnv::Instance()->outfile();
		f->cd(moduleName_.c_str());

		char dir[400];
		sprintf(dir, "m%s_d%s_%c_%d",
			market.c_str(), dest_.c_str(), execType_, orderSchedType_, idate1, idate2);
		gDirectory->mkdir(dir);
		gDirectory->cd(dir);

		p_q2o_.Write();
		p_fillrat_.Write();
		h_norders_.Write();
		h_quoteMatch_.Write();

		h_q2i_.Write();

		h_o2e_filled_.Write();
		h_o2v_filled_.Write();
		h_q2d_filled_.Write();
		h_q2t_filled_.Write();
		h_q2d_direct_filled_.Write();
		h_q2o_filled_.Write();
		h_q2i_filled_.Write();
		h_i2x_filled_.Write();

		h_o2e_notFilled_.Write();
		h_o2v_notFilled_.Write();
		h_q2d_notFilled_.Write();
		h_q2t_notFilled_.Write();
		h_q2o_notFilled_.Write();
		h_q2i_notFilled_.Write();
		h_i2x_notFilled_.Write();
		h_d2i_notFilled_.Write();

		p_fillrat_f2o_.Write();
	}

	return;
}

void HOrderSumm::endJob()
{
}

void HOrderSumm::reset()
{
	nOrd_total_ = 0;
	nOrd_matched_ = 0;
	nOrd_unmatched_ = 0;
	nOrd_directFill_ = 0;
	nOrd_insertNFill_ = 0;
	nOrd_insertNCancel_ = 0;
	nOrd_unidFill_ = 0;
	nOrd_unidCancel_ = 0;

	nShr_total_ = 0;
	nShr_filled_ = 0;

	nFilledFull_ = 0;
	nFilledIncl_ = 0;
	nTotal_ = 0;
	dvFilled_ = 0;
	dvTotal_ = 0;
	nFilledOnFake_ = 0;
	nTotalOnFake_ = 0;
	nFilledOnNonFake_ = 0;
	nTotalOnNonFake_ = 0;

	dv_ = 0;
	dv_filled_ = 0;
	dv_notfilled_ = 0;
	dv_missed_ = 0;
	profit_ = 0;
	profit_filled_ = 0;
	profit_notfilled_ = 0;
	profit_missed_ = 0;

	h_quoteMatch_.Reset();
	p_q2o_.Reset();
	p_fillrat_.Reset();
	h_norders_.Reset();
	h_q2i_.Reset();

	h_o2e_filled_.Reset();
	h_o2v_filled_.Reset();
	h_q2d_filled_.Reset();
	h_q2t_filled_.Reset();
	h_q2d_direct_filled_.Reset();
	h_q2o_filled_.Reset();
	h_q2i_filled_.Reset();
	h_i2x_filled_.Reset();

	h_o2e_notFilled_.Reset();
	h_o2v_notFilled_.Reset();
	h_q2d_notFilled_.Reset();
	h_q2t_notFilled_.Reset();
	h_q2o_notFilled_.Reset();
	h_q2i_notFilled_.Reset();
	h_i2x_notFilled_.Reset();
	h_d2i_notFilled_.Reset();

	p_fillrat_f2o_.Reset();

	return;
}


void HOrderSumm::get_summary(double& o2efilled, double& q2dfilled, double& q2tfilled, double& q2dnotfilled, double& q2tnotfilled, double& d2inotfilled, double& q2i,
				double& nfakeNotFilled, double& nfakeFilled,
				double& ncompetition, double& fillratshr, double& fillratincl, double& fillratinclmatched, double& fillratinclunmatched, double& fillratnonfake,
				double& ratdirectfill, double& ratmatched, double& ratfake, double& ratcomp)
{
	o2efilled = get_top5bin_avg(h_o2e_filled_);
	q2dfilled = get_top5bin_avg(h_q2d_direct_filled_, fakeMsecsMax_);
	q2tfilled = get_top5bin_avg(h_q2t_filled_, fakeMsecsMax_);
	q2dnotfilled = get_top5bin_avg(h_q2d_notFilled_, fakeMsecsMax_);
	q2tnotfilled = get_top5bin_avg(h_q2t_notFilled_, fakeMsecsMax_);
	d2inotfilled = get_top5bin_avg(h_d2i_notFilled_, fakeMsecsMax_);
	q2i = get_top5bin_avg(h_q2i_, fakeMsecsMax_);
	nfakeNotFilled = get_nfake(h_q2d_notFilled_, fakeMsecsMax_);
	nfakeFilled = get_nfake(h_q2d_filled_, fakeMsecsMax_);
	ncompetition = get_competition(h_q2d_notFilled_, fakeMsecsMax_, q2dfilled);

	if( nShr_total_ > 0 )
		fillratshr = nShr_filled_ / nShr_total_;
	if( nTotal_ > 0 )
		fillratincl = nFilledIncl_ / nTotal_;
	if( nTotalOnNonFake_ > 0 )
		fillratnonfake = nFilledOnNonFake_ / nTotalOnNonFake_;
	if( nOrd_matched_ > 0 )
		ratdirectfill = nOrd_directFill_ / nOrd_matched_;
	if( nTotal_ > 0 )
		ratmatched = nOrd_matched_ / nTotal_;
	if( nOrd_matched_ > 0 )
		ratfake = (nfakeNotFilled + nfakeFilled) / (nOrd_matched_);
	if( nOrd_matched_ - nfakeNotFilled - nfakeFilled > 0 )
		ratcomp = ncompetition / (nOrd_matched_ - nfakeNotFilled - nfakeFilled);

	double matchedNfilled = h_q2d_filled_.GetEntries();
	double matchedNnotfilled = h_q2d_notFilled_.GetEntries();
	if( matchedNfilled + matchedNnotfilled > 0 )
		fillratinclmatched = matchedNfilled / (matchedNfilled + matchedNnotfilled);
	if( nOrd_matched_ > 0 )
		fillratinclunmatched = (nFilledIncl_ - matchedNfilled) / nOrd_matched_;

	return;
}

void HOrderSumm::select_orders( vector<MercatorOrder>& orders, const vector<MercatorOrder>* vMOrder )
{
	for( vector<MercatorOrder>::const_iterator it = vMOrder->begin(); it != vMOrder->end(); ++it )
	{
		if( execType_ == 'B' || execType_ == it->execType )
		{
			if( orderSchedType_ == 0 || orderSchedType_ == it->orderSchedType )
			{
				if( side_ == '\0' || side_ == it->side )
				{
					orders.push_back(*it);
				}
			}
		}
	}
	return;
}

void HOrderSumm::fill_hist( vector<MercatorOrder>& orders, string ticker )
{
	for( vector<MercatorOrder>::iterator it = orders.begin(); it != orders.end(); ++it )
	{
		boost::mutex::scoped_lock lock(hist_mutex_);

		int f2o = it->orderMsecs - it->feedMsecs;
		h_quoteMatch_.Fill(it->quoteMatch);

		int filled = it->isFilledIncl()?1:0;
		p_fillrat_.Fill(it->orderMsecs, filled);
		p_fillrat_f2o_.Fill(f2o, (double)filled);
		h_norders_.Fill(it->orderMsecs, 1);

		if( it->quoteMatch > 1 )
		{
			int q2o = 24*60*60*1000;
			int o2e = 24*60*60*1000;
			int q2d = 24*60*60*1000;
			int q2t = 24*60*60*1000;
			int q2i = 24*60*60*1000;
			int i2x = 24*60*60*1000;
			int d2i = 24*60*60*1000;
			if( it->orderMsecs > 0 && it->quoteMsecs > 0 )
				q2o = it->orderMsecs - it->quoteMsecs;
			if( it->eventMsecs > 0 && it->orderMsecs > 0 )
				o2e = it->eventMsecs - it->orderMsecs;
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

			p_q2o_.Fill(it->orderMsecs, q2o);
			h_q2i_.Fill(q2i);

			bool inserted = fakeMsecsMax_ < q2i && q2i < q2iMax_;

			if( it->isFilledIncl() )
			{
				h_q2d_filled_.Fill(q2d);
				h_q2t_filled_.Fill(q2t);
				h_q2o_filled_.Fill(q2o);
				if( inserted )
				{
					h_q2i_filled_.Fill(q2i);
					h_i2x_filled_.Fill(i2x);
					h_o2v_filled_.Fill(o2e);
				}
				else
				{
					h_q2d_direct_filled_.Fill(q2d);
					h_o2e_filled_.Fill(o2e);
				}
			}
			else
			{
				h_q2d_notFilled_.Fill(q2d);
				h_q2t_notFilled_.Fill(q2t);
				h_q2o_notFilled_.Fill(q2o);
				if( inserted )
				{
					h_q2i_notFilled_.Fill(q2i);
					h_i2x_notFilled_.Fill(i2x);
					h_o2v_notFilled_.Fill(o2e);
					h_d2i_notFilled_.Fill(d2i);
				}
				else
					h_o2e_notFilled_.Fill(o2e);
			}
		}
	}
	return;
}

void HOrderSumm::update_fillrat( vector<MercatorOrder>& orders )
{
	for( vector<MercatorOrder>::const_iterator it = orders.begin(); it != orders.end(); ++it )
	{
		int match = it->quoteMatch;
		double price = it->price;
		double qty = it->qty;
		double qtyExec = it->qtyExec;

		boost::mutex::scoped_lock lock(fillrat_mutex_);

		nShr_total_ += it->qty;
		nShr_filled_ += it->qtyExec;

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

void HOrderSumm::update_nOrders( vector<MercatorOrder>& orders )
{
	for( vector<MercatorOrder>::iterator it = orders.begin(); it != orders.end(); ++it )
	{
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

		boost::mutex::scoped_lock lock(nOrd_mutex_);

		++nOrd_total_;

		dv_ += it->price * it->qtyExec;
		profit_ += it->price * it->qtyExec * it->gain; // (bps).
		if( it->quoteMatch > 1 )
		{
			if( it->qtyExec > 0 )
			{
				dv_filled_ += it->price * it->qty;
				profit_filled_ += it->price * it->qty * it->gain;
			}
			else if( it->qtyExec == 0 )
			{
				dv_notfilled_ += it->price * it->qty;
				profit_notfilled_ += it->price * it->qty * it->gain;
				if( q2t >= 0 && q2t <= 5 )
				{
					dv_missed_ += it->price * it->qty;
					profit_missed_ += it->price * it->qty * it->gain;
				}
			}
		}

		if( it->quoteMatch > 1 )
		{
			++nOrd_matched_;

			if( isFilled && q2i > q2iMax_ && o2e < msecDirect_ )
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

double HOrderSumm::get_top5bin_avg(TH1F& h, double thresMsecs)
{
	// Locate the peak.
	double maxBinContent = 0;
	int iBinMaxContent = 0;
	int N = h.GetNbinsX();
	double binSize = h.GetXaxis()->GetBinLowEdge(2) - h.GetXaxis()->GetBinLowEdge(1);
	for( int b=1; b<=N; ++b )
	{
		double binCont = h.GetBinContent(b);
		double binLowEdge = h.GetBinLowEdge(b);
		if( fakeMsecsMax_ <= binLowEdge && maxBinContent < binCont )
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
					binCenter = (h.GetXaxis()->GetBinLowEdge(b) + h.GetXaxis()->GetBinLowEdge(b+1)) / 2.0;
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

double HOrderSumm::get_nfake(TH1F& h, double thresMsecs)
{
	int N = h.GetNbinsX();
	double nfake = 0;
	for( int b=1; b<=N; ++b )
	{
		double binLowEdge = h.GetXaxis()->GetBinLowEdge(b);
		double binHighEdge = h.GetXaxis()->GetBinLowEdge(b+1);
		if( binLowEdge < thresMsecs )
		{
			double c = h.GetBinContent(b);
			nfake += c;
		}
	}
	return nfake;
}

double HOrderSumm::get_competition(TH1F& h, double thresMsecs, double q2dfilled)
{
	int N = h.GetNbinsX();
	{
		vector<double> v;
		for( int b=1; b<=N; ++b )
			v.push_back(h.GetBinContent(b));

		sort(v.begin(), v.end());
		int vs = v.size();
		double thres = v[max(0, vs-5)];

		double maxX = 0;
		for( int b=1; b<=N; ++b )
		{
			double c = h.GetBinContent(b);
			if( c >= thres )
			{
				double binCenter = (h.GetXaxis()->GetBinLowEdge(b) + h.GetXaxis()->GetBinLowEdge(b+1)) / 2.0;
				if( binCenter > thresMsecs && binCenter > maxX)
				{
					maxX = binCenter;
				}
			}
		}
		q2dfilled = maxX;
	}

	double ncomp = 0;
	for( int b=1; b<=N; ++b )
	{
		double binLowEdge = h.GetXaxis()->GetBinLowEdge(b);
		double binHighEdge = h.GetXaxis()->GetBinLowEdge(b+1);
		if( binLowEdge >= thresMsecs && binHighEdge <= q2dfilled )
		{
			double c = h.GetBinContent(b);
			ncomp += c;
		}
	}
	return ncomp;
}
