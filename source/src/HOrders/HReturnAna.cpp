#include <HOrders/HReturnAna.h>
#include "optionlibs/TickData.h"
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/GEX.h>
#include <jl_lib/GODBC.h>
#include "TFile.h"
#include "TGraphErrors.h"
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <numeric>
#include <string>
using namespace std;

HReturnAna::HReturnAna(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
debug_(false),
verbose_(0),
nBinsPerDay_(9),
secsPerBin_(0),
fee_bpt_(0.),
nLags_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";

	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());

	if( conf.count("id") )
	{
		id_ = conf.find("id")->second;
		if( !id_.empty() )
		{
			msecName_ = (string)"msec" + id_;
			prcName_ = (string)"prc" + id_;
			volName_ = (string)"vol" + id_;
			signName_ = (string)"sign" + id_;
			schedName_ = (string)"sched" + id_;
			midName_ = (string)"mid" + id_;
		}
	}
	else if( conf.count("tradeModuleName") )
	{
		tradeModuleName_ = conf.find("tradeModuleName")->second;
		if( !tradeModuleName_.empty() )
		{
			msecName_ = tradeModuleName_ + "_msec";
			signName_ = tradeModuleName_ + "_sign";
			prcName_ = tradeModuleName_ + "_prc";
			volName_ = tradeModuleName_ + "_vol";
			schedName_ = tradeModuleName_ + "_sched";
			midName_ = tradeModuleName_ + "_mid";
			posName_ = tradeModuleName_ + "_mLastClosePos";
			sampleOpenName_ = tradeModuleName_ + "_sampleOpen";
		}
	}
}

HReturnAna::~HReturnAna()
{}

void HReturnAna::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	// Create the output file.
	TFile* f = HEnv::Instance()->outfile();
	f->mkdir(moduleName_.c_str());

	return;
}

void HReturnAna::beginMarket()
{
	string market = HEnv::Instance()->market();
	fee_bpt_ = mto::fee_bpt(market);

	int idate0 = HEnv::Instance()->idates()[0];
	int msecOpen = mto::msecOpen(market, idate0);
	int msecClose = mto::msecClose(market, idate0);
	secsPerBin_ = (msecClose - msecOpen) / 1000. / nBinsPerDay_;

	// Lags for the current day.
	for( int lag = 1; lag < 60; lag *= 2 ) // 60 sec = 1min.
		lags_.push_back(lag);
	for( int lag = 60; lag < 4260; lag *= 2 ) // 4260 sec = 71 min.
		lags_.push_back(lag);
	for( int lag = 4260; lag <= (msecClose - msecOpen) / 1000 - 3600 / 2; lag += 3600 ) // 32400 sec = 9 hours, 3600 sec = 1 hour.
		lags_.push_back(lag);
	lags_.push_back( (msecClose - msecOpen) / 1000 );
	nLags_ = lags_.size();

	// Lags for the next day analysis.
	lagsNext_.push_back(0);
	for( int lag = 0; lag <= (msecClose - msecOpen) / 1000 - 3600 / 2; lag += 3600 )
		lagsNext_.push_back(lag);
	lagsNext_.push_back( (msecClose - msecOpen) / 1000 );
	nLagsNext_ = lagsNext_.size();

	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->mkdir(market.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->mkdir("SameDay");
	gDirectory->mkdir("NextDay");

	// Current day.
	int NL = lags_.size();
	for( vector<int>::iterator it = lags_.begin(); it != lags_.end(); ++it )
	{
		int lag = *it;
		lag_sumSignedRtn_tt_[lag] = vector<double>(5, 0.);
		lag_sumSignedRtn_rt_[lag] = vector<double>(5, 0.);
	}
	sumSignedRtn2d_rt_ = vector<vector<vector<double> > >(nBinsPerDay_, vector<vector<double> >(NL, vector<double>(5, 0.)));

	// Next day.
	int NLN = lagsNext_.size();
	for( vector<int>::iterator it = lagsNext_.begin(); it != lagsNext_.end(); ++it )
	{
		int lag = *it;
		lagNext_sumSignedRtn_tt_[lag] = vector<double>(5, 0.);
		lagNext_sumSignedRtn_rt_[lag] = vector<double>(5, 0.);
	}
	sumSignedRtn2dNext_rt_ = vector<vector<vector<double> > >(nBinsPerDay_, vector<vector<double> >(NLN, vector<double>(5, 0.)));

	return;
}

void HReturnAna::beginDay()
{
	GODBC::Instance()->close_all();

	return;
}

void HReturnAna::beginTicker(const string& ticker)
{
	// Trades.
	const vector<double>* msecT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, msecName_));
	const vector<double>* prcT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, prcName_));
	const vector<double>* volT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, volName_));
	const vector<double>* signT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, signName_));
	const vector<int>* schedT = static_cast<const vector<int>*>(HEvent::Instance()->get(ticker, schedName_));
	const vector<double>* midT = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, midName_));

	// Tickdata.
	const vector<double>* msecQ = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ"));
	const vector<double>* midQ = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ"));
	const vector<double>* msecQ1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ1s"));
	const vector<double>* midQ1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ1s"));

	bool tickdata_ok = (0 != msecQ && 0 != midQ && 0 != msecQ1s && 0 != midQ1s && !msecQ->empty() && !msecQ1s->empty() && !midQ->empty() && !midQ1s->empty());
	bool trade_ok = (0 != msecT && 0 != prcT && 0 != volT && 0 != signT && !msecT->empty() && !prcT->empty() && !volT->empty() && !signT->empty());

	if( tickdata_ok && trade_ok )
	{
		// Fill the returns, for return const calculation.
		fill_return(lag_sumSignedRtn_tt_, lags_, msecT, prcT, volT, signT, msecQ, midQ);
		fill_return(lag_sumSignedRtn_rt_, lags_, msecT, prcT, volT, signT, msecQ1s, midQ1s);
		fill_return_2d(sumSignedRtn2d_rt_, lags_, msecT, prcT, volT, signT, msecQ1s, midQ1s);
	}

	// Tickdata next day.
	const vector<double>* msecQ_p1 = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ_1"));
	const vector<double>* midQ_p1 = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ_1"));
	const vector<double>* msecQ1s_p1 = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ1s_1"));
	const vector<double>* midQ1s_p1 = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ1s_1"));

	bool tickdata_p1_ok = (0 != msecQ_p1 && 0 != midQ_p1 && 0 != msecQ1s_p1 && 0 != midQ1s_p1 && !msecQ_p1->empty() && !msecQ1s_p1->empty() && !midQ_p1->empty() && !midQ1s_p1->empty());

	if( tickdata_p1_ok && trade_ok )
	{
		// Fill the returns, for return const calculation.
		fill_return(lagNext_sumSignedRtn_tt_, lagsNext_, msecT, prcT, volT, signT, msecQ_p1, midQ_p1);
		fill_return(lagNext_sumSignedRtn_rt_, lagsNext_, msecT, prcT, volT, signT, msecQ1s_p1, midQ1s_p1);
		fill_return_2d(sumSignedRtn2dNext_rt_, lagsNext_, msecT, prcT, volT, signT, msecQ1s_p1, midQ1s_p1);
	}

	return;
}

void HReturnAna::endTicker(const string& ticker)
{
	return;
}

void HReturnAna::endDay()
{
	return;
}

void HReturnAna::endMarket()
{
	plot_return();

	return;
}

void HReturnAna::endJob()
{
	return;
}

void HReturnAna::fill_return(map<int, vector<double> >& all_lag_sumSignedRtn, vector<int>& lags,
	const vector<double>* msecsT, const vector<double>* prcT, const vector<double>* shrT, const vector<double>* signT,
	const vector<double>* msecsQ, const vector<double>* midQ)
{
	vector<double>::const_iterator msecsQ_begin = msecsQ->begin();
	vector<double>::const_iterator msecsQ_end = msecsQ->end();

	int NT = msecsT->size();
	int NQ = midQ->size();
	for( int nt=0; nt<NT; ++nt )
	{
		int sign = (*signT)[nt];
		if( abs(sign) == 1 )
		{
			double prc0 = (*prcT)[nt];
			double msT = (*msecsT)[nt];
			double shr = (*shrT)[nt];

			vector<double>::const_iterator itQ = lower_bound(msecsQ_begin, msecsQ_end, msT);
			int nq0 = distance(msecsQ_begin, itQ);
			if( nq0 > 0 )
				fill_return(all_lag_sumSignedRtn, lags, prc0, shr, sign, midQ, nq0);
		}
	}
	return;
}

void HReturnAna::fill_return(map<int, vector<double> >& lag_sumSignedRtn, vector<int>& lags,
							   double prc0, double shr, int sign, const vector<double>* midQ, int n0)
{
	string market = HEnv::Instance()->market();

	int N = midQ->size();
	int NL = lags.size();
	for( int i=0; i<NL; ++i )
	{
		int lag = lags[i];
		int n1 = n0 + lag;

		double prcNext = 0;
		if( n1 < N )
			prcNext = (*midQ)[n1];
		else
			break;

		double signed_diff = sign * (prcNext - prc0);
		double signed_return = signed_diff / prc0 * basis_pts_;
		double signed_pnl = signed_diff * shr;

		map<int, vector<double> >::iterator itall = lag_sumSignedRtn.find(lag);
		{
			boost::mutex::scoped_lock lock(lag_mutex_);
			itall->second[0] += signed_return;
			itall->second[1] += signed_return * signed_return;
			itall->second[2] += signed_pnl;
			itall->second[3] += signed_pnl * signed_pnl;
			++(itall->second[4]);
		}
	}
	return;
}

void HReturnAna::plot_return()
{
	plot_return("SameDay", lag_sumSignedRtn_tt_, "tt");
	plot_return("SameDay", lag_sumSignedRtn_rt_, "rt");
	plot_return_2d("SameDay", sumSignedRtn2d_rt_, lags_);
	plot_return("NextDay", lagNext_sumSignedRtn_tt_, "tt");
	plot_return("NextDay", lagNext_sumSignedRtn_rt_, "rt");
	plot_return_2d("NextDay", sumSignedRtn2dNext_rt_, lagsNext_);
	return;
}

void HReturnAna::plot_return(string dir, map<int, vector<double> >& m2, string flag)
{
	string market = HEnv::Instance()->market();

	vector<double> x;
	vector<double> yRtn;
	vector<double> yPnl;
	vector<double> yPnlSum;
	vector<double> ex;
	vector<double> eyRtn;
	vector<double> eyPnl;
	vector<double> eyPnlSum;
	for( map<int, vector<double> >::iterator itl = m2.begin(); itl != m2.end(); ++itl )
	{
		double lag = itl->first;
		double sumRtn = itl->second[0];
		double sumRtn2 = itl->second[1];
		double sumPnl = itl->second[2];
		double sumPnl2 = itl->second[3];
		double n = itl->second[4];

		if( n > 0 )
		{
			double t = 0;
			if( flag == "rt" )
			{
				t = lag / 3600.; // hour
				if( t > 9 )
					break;
			}
			else
			{
				t = lag; // number of ticks
				if( t > 30000 )
					break;
			}

			x.push_back(t);
			ex.push_back(0);

			double meanRtn = sumRtn / n;
			double stdevRtn = sqrt(sumRtn2 / n - meanRtn * meanRtn) / sqrt(n);
			yRtn.push_back(meanRtn);
			eyRtn.push_back(stdevRtn);

			double meanPnl = sumPnl / n;
			double stdevPnl = sqrt(sumPnl2 / n - meanPnl * meanPnl) / sqrt(n);
			yPnl.push_back(meanPnl);
			eyPnl.push_back(stdevPnl);

			yPnlSum.push_back(sumPnl);
			eyPnlSum.push_back(0.);
		}
	}

	int xs = x.size();
	if( xs > 0 )
	{
		make_graph(x, yRtn, ex, eyRtn, "Rtn", flag, market, dir);
		make_graph(x, yPnl, ex, eyPnl, "Pnl", flag, market, dir);
		make_graph(x, yPnlSum, ex, eyPnlSum, "PnlSum", flag, market, dir);
	}

	return;
}

void HReturnAna::make_graph(vector<double>& x, vector<double>& y, vector<double>& ex, vector<double>& ey,
							  string varName, string timeUnit, string market, string dir)
{
	if( !x.empty() )
	{
		string market = HEnv::Instance()->market();
		char name[100];
		sprintf(name, "%s_%s", varName.c_str(), timeUnit.c_str());
		TGraphErrors gr(x.size(), &x[0], &y[0], &ex[0], &ey[0]);
		gr.SetName( name );
		gr.SetTitle( name );

		TFile* f = HEnv::Instance()->outfile();
		f->cd(moduleName_.c_str());
		gDirectory->cd(market.c_str());
		gDirectory->cd(dir.c_str());
		gr.Write();
	}
	return;
}

void HReturnAna::fill_return_2d(vector<vector<vector<double> > >& sumSignedRtn2d, vector<int>& lags,
	const vector<double>* msecsT, const vector<double>* prcT, const vector<double>* shrT, const vector<double>* signT,
	const vector<double>* msecsQ, const vector<double>* midQ)
{
	vector<double>::const_iterator msecsQ_begin = msecsQ->begin();
	vector<double>::const_iterator msecsQ_end = msecsQ->end();

	int NT = msecsT->size();
	int NQ = midQ->size();
	for( int nt = 0; nt < NT; ++nt )
	{
		int sign = (*signT)[nt];
		if( abs(sign) == 1 )
		{
			double prc0 = (*prcT)[nt];
			double msT = (*msecsT)[nt];
			double shr = (*shrT)[nt];

			vector<double>::const_iterator itQ = lower_bound(msecsQ_begin, msecsQ_end, msT);
			int nq0 = distance(msecsQ_begin, itQ);
			if( nq0 > 0 )
				fill_return_2d(sumSignedRtn2d, lags, prc0, shr, sign, midQ, nq0);
		}
	}
	return;
}

void HReturnAna::fill_return_2d(vector<vector<vector<double> > >& sumSignedRtn2d, vector<int>& lags,
							   double prc0, double shr, int sign, const vector<double>* midQ, int n0)
{
	string market = HEnv::Instance()->market();
	int sec = n0;
	int iBin = sec / secsPerBin_;
	
	int N = midQ->size();
	int NL = lags.size();
	for( int i = 0; i < NL; ++i )
	{
		int lag = lags[i];
		int n1 = n0 + lag;
		int n1_prev = n0 + lags[max(i - 1, 0)];

		double prcNext = 0;
		if( n1 < N )
			prcNext = (*midQ)[n1];
		else if( n1_prev < N && n1 >= N )
			prcNext = (*midQ)[N - 1];
		else
			break;

		double signed_diff = sign * (prcNext - prc0);
		double signed_return = signed_diff / prc0 * basis_pts_;
		double signed_pnl = signed_diff * shr;

		vector<double>& v = sumSignedRtn2d[iBin][i];
		{
			boost::mutex::scoped_lock lock(lag_mutex_);
			v[0] += signed_return;
			v[1] += signed_return * signed_return;
			v[2] += signed_pnl;
			v[3] += signed_pnl * signed_pnl;
			++(v[4]);
		}
	}
	//for( int i = 0; i < nBinsPerDay_; ++i )
	//{
	//	int n1 = n0 + i * secsPerBin_;
	//	int n1_prev = n0 + (i - 1) * secsPerBin_;

	//	double prcNext = 0;
	//	if( n1 < N )
	//		prcNext = (*midQ)[n1];
	//	else if( n1_prev < N && n1 >= N )
	//		prcNext = (*midQ)[N - 1];
	//	else
	//		break;

	//	double signed_diff = sign * (prcNext - prc0);
	//	double signed_return = signed_diff / prc0 * basis_pts_;
	//	double signed_pnl = signed_diff * shr;

	//	vector<double>& v = sumSignedRtn2d[iBin][i];
	//	{
	//		boost::mutex::scoped_lock lock(lag_mutex_);
	//		v[0] += signed_return;
	//		v[1] += signed_return * signed_return;
	//		v[2] += signed_pnl;
	//		v[3] += signed_pnl * signed_pnl;
	//		++(v[4]);
	//	}
	//}
	return;
}

//void HReturnAna::plot_return_2d(string dir, vector<vector<vector<double> > >& vvv)
//{
//	string market = HEnv::Instance()->market();
//
//	double nHours = nBinsPerDay_ * secsPerBin_ / 3600.;
//	TH2F h2Rtn("h2Rtn", "Gpt vs time of trade and horizon", nBinsPerDay_, 0, nHours, NL, &lags[0]);
//	TH2F h2Pnl("h2Pnl", "Pnl vs time of trade and horizon", nBinsPerDay_, 0, nHours, NL, &lags[0]);
//	TH2F h2PnlSum("h2PnlSum", "PnlSum vs time of trade and horizon", nBinsPerDay_, 0, NL, &lags[0]);
//	for( int i1 = 0; i1 < nBinsPerDay_; ++i1 )
//	{
//		for( int i2 = 0; i2 < NL; ++i2 )
//		{
//			vector<double>& v = vvv[i1][i2];
//			double sumRtn = v[0];
//			double sumRtn2 = v[1];
//			double sumPnl = v[2];
//			double sumPnl2 = v[3];
//			double n = v[4];
//			if( n > 0 )
//			{
//				double meanRtn = sumRtn / n;
//				double stdevRtn = sqrt(sumRtn2 / n - meanRtn * meanRtn) / sqrt(n);
//				double meanPnl = sumPnl / n;
//				double stdevPnl = sqrt(sumPnl2 / n - meanPnl * meanPnl) / sqrt(n);
//
//				double meanRtn2 = ceil(meanRtn * 10. - 0.5) / 10.;
//				double meanPnl2 = ceil(meanPnl * 10. - 0.5) / 10.;
//				double sumPnl2 = ceil(sumPnl * 10. - 0.5) / 10.;
//
//				h2Rtn.SetBinContent(i1 + 1, i2 + 1, meanRtn2);
//				h2Rtn.SetBinError(i1 + 1, i2 + 1, stdevRtn);
//				h2Pnl.SetBinContent(i1 + 1, i2 + 1, meanPnl2);
//				h2Pnl.SetBinError(i1 + 1, i2 + 1, stdevPnl);
//				h2PnlSum.SetBinContent(i1 + 1, i2 + 1, sumPnl2);
//			}
//		}
//	}
//	TFile* f = HEnv::Instance()->outfile();
//	f->cd(moduleName_.c_str());
//	gDirectory->cd(market.c_str());
//	gDirectory->cd(dir.c_str());
//	h2Rtn.Write();
//	h2Pnl.Write();
//	h2PnlSum.Write();
//
//	return;
//}

void HReturnAna::plot_return_2d(string dir, vector<vector<vector<double> > >& vvv, vector<int>& lags)
{
	string market = HEnv::Instance()->market();
	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());
	gDirectory->cd(dir.c_str());

	double nHours = nBinsPerDay_ * secsPerBin_ / 3600.;
	for( int i1 = 0; i1 < nBinsPerDay_; ++i1 )
	{
		vector<double> x;
		vector<double> yRtn;
		vector<double> yPnl;
		vector<double> yPnlSum;
		vector<double> ex;
		vector<double> eyRtn;
		vector<double> eyPnl;
		vector<double> eyPnlSum;

		int NL = lags.size();
		for( int i2 = 0; i2 < NL; ++i2 )
		{
			double lag = lags[i2];

			vector<double>& v = vvv[i1][i2];
			double sumRtn = v[0];
			double sumRtn2 = v[1];
			double sumPnl = v[2];
			double sumPnl2 = v[3];
			double n = v[4];

			if( n > 0 )
			{
				double t = lag / 3600.;
				x.push_back(t);
				ex.push_back(0);

				double meanRtn = sumRtn / n;
				double stdevRtn = sqrt(sumRtn2 / n - meanRtn * meanRtn) / sqrt(n);
				yRtn.push_back(meanRtn);
				eyRtn.push_back(stdevRtn);

				double meanPnl = sumPnl / n;
				double stdevPnl = sqrt(sumPnl2 / n - meanPnl * meanPnl) / sqrt(n);
				yPnl.push_back(meanPnl);
				eyPnl.push_back(stdevPnl);

				yPnlSum.push_back(sumPnl);
				eyPnlSum.push_back(0.);
			}
		}

		int xs = x.size();
		if( xs > 0 )
		{
			char flag[40];
			sprintf(flag, "%d_of_%d", i1 + 1, nBinsPerDay_);
			make_graph(x, yRtn, ex, eyRtn, "Rtn", flag, market, dir);
			make_graph(x, yPnl, ex, eyPnl, "Pnl", flag, market, dir);
			make_graph(x, yPnlSum, ex, eyPnlSum, "PnlSum", flag, market, dir);
		}
	}

	return;
}
