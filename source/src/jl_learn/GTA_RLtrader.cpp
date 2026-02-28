#include "GTA_RLtrader.h"
#include "GTEvent.h"
#include "GTEnv.h"
#include "RmLin.h"
#//include "RmLin2.h"
#include "Bpnn0.h"
#include "mto.h"
#include "jlutil.h"
#include "TGraph.h"
#include "TH1.h"
#include <map>
#include <string>
#include <algorithm>
using namespace std;

GTA_RLtrader::GTA_RLtrader(const string& moduleName, const multimap<string, string>& conf)
:GTModule(moduleName),
iModel_(1),
thres_(0.3),
thresIncr_(1),
cost_(0),
adap_(0.001),
maxPos_(10000),
learn_(1),
rewardTime_(1000),
interval_(1000),
lenSeries_(500),
maxWgt_(10),
timeUnit_("tick"),
opt_("profit"),
verbose_(0),
trainOnline_(true),
writeRoot_(false)
{
	if( conf.count("iModel") )
		iModel_ = atoi( conf.find("iModel")->second.c_str() );
	if( conf.count("thres") )
		thres_ = atof( conf.find("thres")->second.c_str() );
	if( conf.count("thresIncr") )
		thresIncr_ = atof( conf.find("thresIncr")->second.c_str() );
	if( conf.count("cost") )
		cost_ = atof( conf.find("cost")->second.c_str() );
	if( conf.count("adap") )
		adap_ = atof( conf.find("adap")->second.c_str() );
	if( conf.count("maxPos") )
		maxPos_ = atoi( conf.find("maxPos")->second.c_str() );
	if( conf.count("learn") )
		learn_ = atof( conf.find("learn")->second.c_str() );
	if( conf.count("timeUnit") )
		timeUnit_ = conf.find("timeUnit")->second.c_str();
	if( conf.count("opt") )
		opt_ = conf.find("opt")->second.c_str();
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("rewardTime") )
		rewardTime_ = atoi( conf.find("rewardTime")->second.c_str() );
	if( conf.count("interval") )
		interval_ = atoi( conf.find("interval")->second.c_str() );
	if( conf.count("lenSeries") )
		lenSeries_ = atoi( conf.find("lenSeries")->second.c_str() );
	if( conf.count("maxWgt") )
		maxWgt_ = atof( conf.find("maxWgt")->second.c_str() );
	if( conf.count("trainOnline") )
	{
		string value = conf.find("trainOnline")->second;
		if( value == "true" )
			trainOnline_ = true;
		else if( value == "false" )
			trainOnline_ = false;
	}
	if( conf.count("writeRoot") )
		writeRoot_ = conf.find("writeRoot")->second == "true";
}

GTA_RLtrader::~GTA_RLtrader()
{}

void GTA_RLtrader::beginJob()
{
	return;
}

void GTA_RLtrader::beginMarket()
{
	static int nmkt = 0;
	if( ++nmkt > 1 ) // no more than one market.
		exit(1);

	string market = GTEvent::Instance()->market();
	sessions_ = mto::sessions(market, 30, 0, 10);

	if( 1 == iModel_ )
		model_ = new RmLin(rewardTime_, interval_, lenSeries_, maxPos_, learn_, thres_, thresIncr_,
		cost_, adap_, maxWgt_, verbose_, opt_, trainOnline_);
	else if( 3 == iModel_ )
		model_ = new Bpnn0(rewardTime_, interval_, lenSeries_, maxPos_, learn_, thres_, thresIncr_,
		cost_, adap_, maxWgt_, verbose_, trainOnline_);
	feeder_ = new RLfeeder();

	int nwgts = model_->get_nwgts();
	vvWgts_ = vector<vector<double> >(nwgts, vector<double>());

	if( writeRoot_ )
	{
		TFile* f = GTEnv::Instance()->outfile();
		f->cd();
		f->mkdir("pnl");
		f->mkdir("weight");
		f->mkdir("output");
	}

	return;
}

void GTA_RLtrader::beginDay()
{
	// Reset
	model_->beginDay();

	// Keep track of the weights.
	int nwgts = model_->get_nwgts();
	vector<double> vWgts = model_->get_wgts();
	for( int i=0; i<nwgts; ++i )
		vvWgts_[i].push_back(vWgts[i]);

	return;
}

void GTA_RLtrader::doTicker()
{
	TickSeries<QuoteInfo>* ptsQ = static_cast<TickSeries<QuoteInfo>*>(GTEvent::Instance()->get("quotes"));
	int ntq = ptsQ->NTicks();

	vector<QuoteInfo> vq;
	vq.reserve(ntq);
	ptsQ->StartRead();
	QuoteInfo quote;
	for( int n=0; n<ntq; ++n )
	{
		ptsQ->Read(&quote);
		int msecs = quote.msecs;
		bool inrange = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), msecs)) != sessions_.end();
		if(inrange)
			vq.push_back(quote);
	}

	string ticker = GTEvent::Instance()->ticker();
	feeder_->addTicker(ticker, vq);

	return;
}

void GTA_RLtrader::endDay()
{
	int idate = GTEvent::Instance()->idate();

	// trade
	map<string, RLticker> tickersToday;
	while( feeder_->advance() )
	{
		RLinput ri = feeder_->next();
		RLticker& rt = tickersToday[ri.ticker];
		rt.add(ri.mid);
		model_->evaluate(rt);
	}

	// outputs
	if( writeRoot_ )
	{
		char name[100];
		sprintf(name, "h%d", idate);
		TH1F* houtput = new TH1F(name, name, 100, -1.1, 1.1);
		for( map<string, RLticker>::iterator itt = tickersToday.begin(); itt != tickersToday.end(); ++itt )
			for( vector<RLtrade>::iterator it = itt->second.trades.begin(); it != itt->second.trades.end(); ++it )
				houtput->Fill(it->F);
		TFile* f = GTEnv::Instance()->outfile();
		f->cd();
		f->cd("output");
		houtput->Write();
		delete houtput;
	}

	// Calculate cl-cl, make plots, and summarize pnl. For each ticker.
	for(map<string, RLticker>::iterator it = tickersToday.begin(); it != tickersToday.end(); ++it )
	{
		string ticker = it->first;
		RLticker& rt = it->second;

		TickerDaySumm lastDay;
		if( !mTickerDay_[ticker].empty() )
			lastDay = mTickerDay_[ticker].rbegin()->second;

		double lastClosePrc = lastDay.closePrc;
		int lastClosePosition = lastDay.closePosition;
		double closePrc = rt.vmid[rt.indxT];
		double pnlClcl = lastClosePosition * (closePrc - lastClosePrc);
		double pnlIntra = rt.pnl(cost_);
		int closePosition = lastClosePosition + rt.position;
		double pnlIntra2 = rt.pnl2(cost_);

		double dollarvol = 0;
		int ntrades = 0;
		for(vector<RLtrade>::iterator it = rt.trades.begin(); it != rt.trades.end(); ++it )
		{
			if( it->shr != 0 )
				++ntrades;

			dollarvol += abs(it->shr) * rt.vmid[it->indx];
		}
		mTickerDay_[ticker][idate] = TickerDaySumm(closePrc, closePosition, pnlClcl, pnlIntra, dollarvol, ntrades, pnlIntra2);

		if( verbose_ >= 2 )
		{
			int posIncr = 0;
			int posDecr = 0;
			for(vector<RLtrade>::iterator it = rt.trades.begin(); it != rt.trades.end(); ++it )
			{
				if( it->shr > 0 )
					posIncr += it->shr;
				else if( it->shr < 0 )
					posDecr += it->shr;
			}
			double pnlIntraBps = 0;
			double pnlTotBps = 0;
			if( dollarvol > 0 )
			{
				pnlIntraBps = pnlIntra / dollarvol * 10000;
				pnlTotBps = (pnlClcl + pnlIntra) / dollarvol * 10000;
			}
			printf("%-5s POS: %+4d %+4d = %4d Close: %+6.2f = %6.2f\n",
				ticker.c_str(), posIncr, posDecr, closePosition, closePrc-lastClosePrc, closePrc);
			printf("  day Clcl = %6.2f Intra = %6.2f (%4.2f) Tot %6.2f (%4.2f)\n",
				pnlClcl, pnlIntra, pnlIntraBps, pnlClcl+pnlIntra, pnlTotBps);

			map<int, TickerDaySumm>& m = mTickerDay_[ticker];
			double tot_clcl = 0;
			double tot_intra = 0;
			double tot_dollarvol = 0;
			for(map<int, TickerDaySumm>::iterator it = m.begin(); it != m.end(); ++it )
			{
				tot_clcl += it->second.pnlClcl;
				tot_intra += it->second.pnlIntra;
				tot_dollarvol += it->second.dollarvol;
			}
			double totPnlIntraBps = 0;
			double totPnlTotBps = 0;
			if( tot_dollarvol > 0 )
			{
				totPnlIntraBps = tot_intra / tot_dollarvol * 10000;
				totPnlTotBps = (tot_clcl + tot_intra) / tot_dollarvol * 10000;
			}
			printf("  tot Clcl = %6.2f Intra = %6.2f (%4.2f) Tot %6.2f (%4.2f)\n",
				tot_clcl, tot_intra, totPnlIntraBps, tot_clcl+tot_intra, totPnlTotBps);
		}
	}

	// Pnl for all the tickers.
	double pnlClcl = 0;
	double pnlIntra = 0;
	double dollarvol = 0;
	int ntrades = 0;
	int ntradesCum = 0;
	int ntickers = 0;
	double pnlSum = 0;
	double pnl2Sum = 0;
	for( map<string, map<int, TickerDaySumm> >::iterator it = mTickerDay_.begin(); it != mTickerDay_.end(); ++it )
	{
		map<int, TickerDaySumm>::iterator itSumm = it->second.find(idate);
		if( itSumm != it->second.end() )
		{
			++ntickers;
			pnlClcl += itSumm->second.pnlClcl;
			pnlIntra += itSumm->second.pnlIntra;
			dollarvol += itSumm->second.dollarvol;
			ntrades += itSumm->second.ntrades;
		}
		for( map<int, TickerDaySumm>::iterator itSumm2 = it->second.begin(); itSumm2 != it->second.end(); ++itSumm2 )
		{
			double ntr = itSumm2->second.ntrades;
			if( ntr > 0 )
			{
				double pnl = itSumm2->second.pnlIntra;
				double pnl2 = itSumm2->second.pnlIntra2;
				pnlSum += pnl;
				pnl2Sum += pnl2;
				ntradesCum += ntr;
			}
		}
	}
	double shrpR = 0;
	if( ntradesCum > 0 )
	{
		double mean = pnlSum / ntradesCum;
		double mean2 = pnl2Sum / ntradesCum;
		double pnlStdev = sqrt( mean2 - mean*mean );
		if( pnlStdev > 0 )
			shrpR = mean / pnlStdev;
	}
	mDay_[idate] = DaySumm(pnlClcl, pnlIntra, dollarvol, ntrades, shrpR);

	if( verbose_ >= 1 )
	{
		double pnlIntraBps = 0;
		double pnlTotBps = 0;
		if( dollarvol > 0 )
		{
			pnlIntraBps = pnlIntra / dollarvol * 10000;
			pnlTotBps = (pnlClcl+pnlIntra) / dollarvol * 10000;
		}
		printf("TOTAL nTrades = %d\n", ntrades);
		printf("  day Clcl = %6.2f Intra = %6.2f (%4.2f) Tot %6.2f (%4.2f)\n",
			pnlClcl, pnlIntra, pnlIntraBps, pnlClcl+pnlIntra, pnlTotBps);

		{
			double tot_clcl = 0;
			double tot_intra = 0;
			double tot_dollarvol = 0;
			for(map<int, DaySumm>::iterator it = mDay_.begin(); it != mDay_.end(); ++it )
			{
				tot_clcl += it->second.pnlClcl;
				tot_intra += it->second.pnlIntra;
				tot_dollarvol += it->second.dollarvol;
			}
			double totPnlIntraBps = 0;
			double totPnlTotBps = 0;
			if( tot_dollarvol > 0 )
			{
				totPnlIntraBps = tot_intra / tot_dollarvol * 10000;
				totPnlTotBps = (tot_clcl + tot_intra) / tot_dollarvol * 10000;
			}
			printf("  tot Clcl = %6.2f Intra = %6.2f (%4.2f) Tot %6.2f (%4.2f)\n",
				tot_clcl, tot_intra, totPnlIntraBps, tot_clcl+tot_intra, totPnlTotBps);
		}
	}

	tickersToday.clear();
	feeder_->endDay();

	return;
}

void GTA_RLtrader::endMarket()
{
	string market = GTEvent::Instance()->market();
	int idate0 = mDay_.begin()->first;
	int idate1 = mDay_.rbegin()->first;
	cout << "------------------------------------------------------------\n";
	cout << "market " << market << " dates from " << idate0 << " to " << idate1 << endl;

	// Total pnl.
	double pnlClcl = 0;
	double pnlIntra = 0;
	double pnlTotal = 0;
	double pnlClcl2 = 0;
	double pnlIntra2 = 0;
	double pnlTotal2 = 0;
	double dollarvol = 0;
	double ntrades = 0;
	double ndays = 0;
	for( map<int, DaySumm>::iterator it = mDay_.begin(); it != mDay_.end(); ++it )
	{
		pnlClcl += it->second.pnlClcl;
		pnlIntra += it->second.pnlIntra;
		pnlTotal += it->second.pnlClcl + it->second.pnlIntra;
		pnlClcl2 += pnlClcl * pnlClcl;
		pnlIntra2 += pnlIntra * pnlIntra;
		pnlTotal2 += pnlTotal * pnlTotal;
		dollarvol += it->second.dollarvol;
		ntrades += it->second.ntrades;
		++ndays;
	}
	double shrpRClcl = 0;
	double shrpRIntra = 0;
	double shrpRTotal = 0;
	if( ndays > 0 )
	{
		double meanClcl = pnlClcl / ndays;
		double mean2Clcl = pnlClcl2 / ndays;
		shrpRClcl = meanClcl / sqrt(mean2Clcl - meanClcl*meanClcl);
		double meanIntra = pnlIntra / ndays;
		double mean2Intra = pnlIntra2 / ndays;
		shrpRIntra = meanIntra / sqrt(mean2Intra - meanIntra*meanIntra);
		double meanTotal = pnlTotal / ndays;
		double mean2Total = pnlTotal2 / ndays;
		shrpRTotal = meanTotal / sqrt(mean2Total - meanTotal*meanTotal);
	}
	double pnlIntraBps = 0;
	double pnlTotBps = 0;
	double gpt = 0;
	if( dollarvol > 0 )
	{
		pnlIntraBps = pnlIntra / dollarvol * 10000;
		pnlTotBps = (pnlClcl + pnlIntra) / dollarvol * 10000;
		gpt = pnlIntra / dollarvol * 10000;
	}
	printf("  tot Clcl = %6.2f Intra = %6.2f (%4.2f) Tot %6.2f (%4.2f) GPT %6.2f\n",
		pnlClcl, pnlIntra, pnlIntraBps, pnlTotal, pnlTotBps, gpt);
	printf("  shR Clcl = %6.2f Intra = %6.2f Tot %6.2f\n",
		shrpRClcl, shrpRIntra, shrpRTotal);

	if( writeRoot_ )
	{
		// Create the weight graphs.
		int nwgts = model_->get_nwgts();
		TGraph** grW = new TGraph*[nwgts];
		double grMax = 0;
		double grMin = 0;
		for( int ngr=0; ngr<nwgts; ++ngr )
		{
			vector<double>& vWgts = vvWgts_[ngr];
			int N = vWgts.size();

			vector<double> x;
			for( int i=1; i<=N; ++i )
				x.push_back(i);

			{
				double iMax = *max_element(vWgts.begin(), vWgts.end());
				double iMin = *min_element(vWgts.begin(), vWgts.end());
				if( iMax > grMax )
					grMax = iMax;
				if( iMin < grMin )
					grMin = iMin;
			}

			grW[ngr] = new TGraph(N, &x[0], &vWgts[0]);
		}
		for( int ngr=0; ngr<nwgts; ++ngr )
		{
			grW[ngr]->SetMaximum(grMax);
			grW[ngr]->SetMinimum(grMin);
		}

		// Pnl graphs for each ticker.
		/*
		for( map<string, map<int, TickerDaySumm> >::iterator it = mTickerDay_.begin(); it != mTickerDay_.end(); ++it )
		{
			map<int, TickerDaySumm>::iterator itSumm = it->second.find(idate);
			if( itSumm != it->second.end() )
			{
				++ntickers;
				pnlClcl += itSumm->second.pnlClcl;
				pnlIntra += itSumm->second.pnlIntra;
				dollarvol += itSumm->second.dollarvol;
				ntrades += itSumm->second.ntrades;
			}
			for( map<int, TickerDaySumm>::iterator itSumm2 = it->second.begin(); itSumm2 != it->second.end(); ++itSumm2 )
			{
				double ntr = itSumm2->second.ntrades;
				if( ntr > 0 )
				{
					double pnl = itSumm2->second.pnlIntra;
					double pnl2 = itSumm2->second.pnlIntra2;
					pnlSum += pnl;
					pnl2Sum += pnl2;
					ntradesCum += ntr;
				}
			}
		}*/

		// Pnl graphs.
		vector<double> x; // dates.
		map<string, vector<double> > mVal; // daily values.
		vector<double> tempVec;

		mVal.insert(make_pair("pnlClclCum", tempVec));
		mVal.insert(make_pair("pnlIntraCum", tempVec));
		mVal.insert(make_pair("pnlTotalCum", tempVec));

		mVal.insert(make_pair("dollarvol", tempVec));
		mVal.insert(make_pair("dollarvolCum", tempVec));

		mVal.insert(make_pair("ntrades", tempVec));
		mVal.insert(make_pair("ntradesCum", tempVec));

		mVal.insert(make_pair("clclCumBps", tempVec));
		mVal.insert(make_pair("intraCumBps", tempVec));
		mVal.insert(make_pair("totalCumBps", tempVec));

		double pnlClclCum = 0;
		double pnlIntraCum = 0;
		double dollarvolCum = 0;
		double ntradesCum = 0;
		int nd = 0;
		for( map<int, DaySumm>::iterator it = mDay_.begin(); it != mDay_.end(); ++it )
		{
			int idate = it->first;
			x.push_back(++nd);

			DaySumm& dsumm = it->second;

			mVal["pnlClclCum"].push_back(pnlClclCum += dsumm.pnlClcl);
			mVal["pnlIntraCum"].push_back(pnlIntraCum += dsumm.pnlIntra);
			mVal["pnlTotalCum"].push_back(pnlClclCum + pnlIntraCum);

			mVal["dollarvol"].push_back(dsumm.dollarvol);
			mVal["dollarvolCum"].push_back(dollarvolCum += dsumm.dollarvol);

			mVal["ntrades"].push_back(dsumm.ntrades);
			mVal["ntradesCum"].push_back(ntradesCum += dsumm.ntrades);

			mVal["clclCumBps"].push_back( (dollarvolCum > 0)?(pnlClclCum / dollarvolCum * 10000):0 );
			mVal["intraCumBps"].push_back( (dollarvolCum > 0)?(pnlIntraCum / dollarvolCum * 10000):0 );
			mVal["totalCumBps"].push_back( (dollarvolCum > 0)?((pnlClclCum+pnlIntraCum) / dollarvolCum * 10000):0 );
		}

		// Write the graphes.
		TFile* f = GTEnv::Instance()->outfile();

		f->cd();
		f->cd("pnl");
		int ng = mVal.size();
		int nx = x.size(); // number of dates.
		TGraph** gr = new TGraph*[ng];
		int i = 0;
		for( map<string, vector<double> >::iterator it = mVal.begin(); it != mVal.end(); ++it, ++i )
		{
			string name = it->first;
			vector<double>& vVal = it->second;
			gr[i] = new TGraph(nx, &x[0], &vVal[0]);
			gr[i]->SetName(name.c_str());
			gr[i]->Write();
		}
		for( int i=0; i<ng; ++i )
			delete gr[i];
		delete [] gr;

		f->cd();
		f->cd("weight");
		for( int ngr=0; ngr<nwgts; ++ngr )
		{
			char name[100];
			sprintf( name, "par_%03d", ngr+1);
			grW[ngr]->SetName(name);
			grW[ngr]->Write();
		}
		for( int i=0; i<nwgts; ++i )
			delete grW[i];
		delete [] grW;

		f->Close();

		// Delete the graphs.
	}

	delete model_;
	delete feeder_;
	vvWgts_.clear();
	return;
}

void GTA_RLtrader::endJob()
{
	return;
}
