#include "GTA_NNtrainer.h"
#include "GTEvent.h"
#include "GTEnv.h"
#include "NN.h"
#include "NN_bpnn.h"
#include "NN_bptt.h"
#include "NN_bptt_tanh.h"
#include "NNobj.h"
#include "mto.h"
#include "jlutil.h"
#include "optionlibs/TickData.h"
#include "TGraph.h"
#include "TH1.h"
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
using namespace std;

GTA_NNtrainer::GTA_NNtrainer(const string& moduleName, const multimap<string, string>& conf)
:GTModule(moduleName),
debug_(false),
iDay_(0),
nHidden_(0),
maxPos_(10000),
lotSize_(1),
costMult_(1),
learn_(1),
sigP_(1),
n_P_(0),
sum_P_(0),
sum_P2_(0),
maxWgt_(10),
poslim_(0),
trainObj_("minErr"),
verbose_(0),
biasOut_(0),
iEpoch_(0),
epoch_rms_prev_(0),
nticker_trained_day_(0),
nticker_trained_market_(0),
noTwoTradesAtSamePrice_(false),
bold_driver_(true),
bold_driver_scale_up_(1.1),
bold_driver_scale_down_(0.5),
nNoBold_(0),
trainingDetail_(false),
nn_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("maxPos") )
		maxPos_ = atoi( conf.find("maxPos")->second.c_str() );
	if( conf.count("costMult") )
		costMult_ = atof( conf.find("costMult")->second.c_str() );
	if( conf.count("learn") )
		learn_ = atof( conf.find("learn")->second.c_str() );
	if( conf.count("sigP") )
		sigP_ = atof( conf.find("sigP")->second.c_str() );
	if( conf.count("nHidden") )
		nHidden_ = atoi( conf.find("nHidden")->second.c_str() );
	if( conf.count("trainObj") )
		trainObj_ = conf.find("trainObj")->second.c_str();
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("biasOut") )
		biasOut_ = atof( conf.find("biasOut")->second.c_str() );
	if( conf.count("maxWgt") )
		maxWgt_ = atof( conf.find("maxWgt")->second.c_str() );
	if( conf.count("poslim") )
		poslim_ = atof( conf.find("poslim")->second.c_str() );
	if( conf.count("noTwoTradesAtSamePrice") )
		noTwoTradesAtSamePrice_ = conf.find("noTwoTradesAtSamePrice")->second == "true";
	if( conf.count("boldDriver") )
		bold_driver_ = conf.find("boldDriver")->second == "true";
	if( conf.count("boldDriverScaleUp") )
		bold_driver_scale_up_ = atof( conf.find("boldDriverScaleUp")->second.c_str() );
	if( conf.count("boldDriverScaleDown") )
		bold_driver_scale_down_ = atof( conf.find("boldDriverScaleDown")->second.c_str() );
}

GTA_NNtrainer::~GTA_NNtrainer()
{}

void GTA_NNtrainer::beginJob()
{
	NNinputInfo* nii = static_cast<NNinputInfo*>(GTEvent::Instance()->get("input_info"));
	if( nii != 0 )
	{
		vector<string> vmarket = GTEnv::Instance()->markets();
		if( !vmarket.empty() && vmarket[0] == "US" )
			lotSize_ = 100;

		// Create Neural Network.
		int nInput = nii->n;
		int nOutput = 1;

		vector<int> N;
		N.push_back(nInput); // number of inputs.
		if( nHidden_ > 0 )
			N.push_back(nHidden_); // number of hidden units.
		N.push_back(nOutput);

		bool maxRet = true;
		if( trainObj_.size() >= 9 && trainObj_.substr(0, 9) == "maxProfit" )
			maxRet = false;

		if( "maxReturn" == trainObj_ || "maxProfit" == trainObj_ )
			nn_ = new NN_bptt( N, learn_, maxWgt_, verbose_, biasOut_, costMult_, sigP_, poslim_, maxPos_, lotSize_, maxRet );
		else if( "maxReturnTanh" == trainObj_ || "maxProfitTanh" == trainObj_ )
			nn_ = new NN_bptt_tanh( N, learn_, maxWgt_, verbose_, biasOut_, costMult_, sigP_, poslim_, maxPos_, lotSize_, maxRet );
		else
			nn_ = new NN_bpnn( N, learn_, maxWgt_, verbose_, biasOut_ );

		nn_->save();
		vvWgts_ = vector<vector<double> >(nn_->get_nwgts(), vector<double>());
		if( nn_ != 0 )
		{
			// Keep track of the weights.
			int nwgts = nn_->get_nwgts();
			vector<double> vWgts = nn_->get_wgts();
			for( int i=0; i<nwgts; ++i )
				vvWgts_[i].push_back(vWgts[i]);
		}

		// Create ROOT directories.
		TFile* f = GTEnv::Instance()->outfile();
		f->cd();
		f->mkdir(moduleName_.c_str());

		f->cd(moduleName_.c_str());
		gDirectory->mkdir("performance");
		gDirectory->mkdir("weight");
		gDirectory->mkdir("output");
		if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
		{
			gDirectory->mkdir("output0");
			gDirectory->mkdir("tickers");
		}

		// Reset performance measures.
		npError_.clear();
		npReturn0_.clear();
		npProfit0_.clear();
		npRpt0_.clear();
		npGpt0_.clear();
		npReturn_.clear();
		npProfit_.clear();
		npRpt_.clear();
		npGpt_.clear();
	}

	return;
}

void GTA_NNtrainer::beginMarket()
{
	n_P_ = 0;
	sum_P_ = 0;
	sum_P2_ = 0;
	iDay_ = 0;

	if( nn_ != 0 )
	{
		// Creat ROOT objects.
		char name[40];
		char title[40];

		// For each data point
		sprintf(name, "output_%d", iEpoch_);
		sprintf(title, "Model Output %d", iEpoch_);
		houtput_ = new TH1F(name, title, 100, -1e-4, 1e-4);
		houtput_->SetBit(TH1::kCanRebin);

		sprintf(name, "prtn_%d", iEpoch_);
		sprintf(title, "Return vs Prediction %d", iEpoch_);
		prtn_ = new TProfile(name, title, 100, -1e-4, 1e-4);
		prtn_->SetBit(TH1::kCanRebin);

		if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
		{
			sprintf(name, "output0_%d", iEpoch_);
			sprintf(title, "Model Output %d", iEpoch_);
			houtput0_ = new TH1F(name, title, 60, -3, 3);

			sprintf(name, "prtn0_%d", iEpoch_);
			sprintf(title, "Return vs Prediction %d", iEpoch_);
			prtn0_ = new TProfile(name, title, 60, -3, 3);
		}

		// For each stock-day
		if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
		{
			sprintf(name, "return0_%d", iEpoch_);
			sprintf(title, "Return0 of each Ticker %d", iEpoch_);
			hreturn0_ = new TH1F(name, title, 100, -1e-4, 1e-4);
			hreturn0_->SetBit(TH1::kCanRebin);

			sprintf(name, "profit0_%d", iEpoch_);
			sprintf(title, "Profit0 of each Ticker %d", iEpoch_);
			hprofit0_ = new TH1F(name, title, 100, -1e-4, 1e-4);
			hprofit0_->SetBit(TH1::kCanRebin);

			sprintf(name, "return_%d", iEpoch_);
			sprintf(title, "Return of each Ticker %d", iEpoch_);
			hreturn_ = new TH1F(name, title, 100, -1e-4, 1e-4);
			hreturn_->SetBit(TH1::kCanRebin);

			sprintf(name, "profit_%d", iEpoch_);
			sprintf(title, "Profit of each Ticker %d", iEpoch_);
			hprofit_ = new TH1F(name, title, 100, -1e-4, 1e-4);
			hprofit_->SetBit(TH1::kCanRebin);
		}

		// Performance measures.
		npError_.clear();
		npReturn0_.clear();
		npProfit0_.clear();
		npRpt0_.clear();
		npGpt0_.clear();
		npReturn_.clear();
		npProfit_.clear();
		npRpt_.clear();
		npGpt_.clear();

		// Training detail
		if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
			//&& GTEnv::Instance()->iMarketRep() == (GTEnv::Instance()->marketRep() - 1)
			)
			trainingDetail_ = true;
		else
			trainingDetail_ = false;

		// BPTT.
		mPos_.clear();
	}

	mvMsecs_.clear();
	mvPrice_.clear();
	mvPosition_.clear();
	mvNormPos_.clear();
	mvNormPos0_.clear();
	mvReturn0_.clear();
	mvProfit0_.clear();
	mvReturn_.clear();
	mvProfit_.clear();

	return;
}

void GTA_NNtrainer::beginDay()
{
	n_Pday_ = 0;
	sum_Pday_ = 0;
	sum_Pday2_ = 0;
	nTradeDay_ = 0;
	nShareDay_ = 0;
	return;
}

void GTA_NNtrainer::doTicker()
{
	vector<NNsample>* sample = static_cast<vector<NNsample>*>(GTEvent::Instance()->get("sample"));

	if( sample != 0 )
	{
		++nticker_trained_day_;
		++nticker_trained_market_;

		if( "minErr" == trainObj_ )
			doTicker_minPredErr();
		else if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
			doTicker_maxProfitPos();
	}

	return;
}

void GTA_NNtrainer::doTicker_minPredErr()
{
	vector<NNsample>* sample = static_cast<vector<NNsample>*>(GTEvent::Instance()->get("sample"));

	// loop over the sample and train the model.
	vector<NNsample>::iterator ite = sample->end();
	for( vector<NNsample>::iterator it = sample->begin(); it != ite; ++it )
	{
		vector<double>& input = it->input;
		double target = it->target;
		QuoteInfo& quote = it->quote;

		vector<double> output = nn_->forward(input, quote);
		double nnOutput = output[0];
		double error = target - nnOutput;
		nn_->backprop(error);

		double err = error*error;
		npError_.add(err);

		houtput_->Fill(nnOutput);
		prtn_->Fill(nnOutput, target);
	}

	return;
}

void GTA_NNtrainer::doTicker_maxProfitPos()
{
	vector<NNsample>* sample = static_cast<vector<NNsample>*>(GTEvent::Instance()->get("sample"));

	// Detail.
	string ticker = GTEvent::Instance()->ticker();
	double sumR0 = 0;
	double sumP0 = 0;
	double sumR = 0;
	double sumP = 0;
	bool tickerDetail = false;
	if( trainingDetail_ && (mvMsecs_.count(ticker) || mvMsecs_.size() < 10) )
		tickerDetail = true;
	vector<double>* vMsecs = 0;
	vector<double>* vPrice = 0;
	vector<double>* vPosition = 0;
	vector<double>* vNormPos = 0;
	vector<double>* vNormPos0 = 0;
	vector<double>* vReturn0 = 0;
	vector<double>* vProfit0 = 0;
	vector<double>* vReturn = 0;
	vector<double>* vProfit = 0;
	int msecsOffset = 0;
	if( tickerDetail )
	{
		if( verbose_ > 2 )
			cout << " " << moduleName_ << " start ticker " << ticker << endl;

		string market = GTEvent::Instance()->market();
		int msecOpen = mto::msecOpen(market);
		int msecClose = mto::msecClose(market);
		msecsOffset = - msecOpen + iDay_ * (msecClose - msecOpen);

		vMsecs = &mvMsecs_[ticker];
		vPrice = &mvPrice_[ticker];
		vPosition = &mvPosition_[ticker];
		vNormPos = &mvNormPos_[ticker];
		vNormPos0 = &mvNormPos0_[ticker];
		vReturn0 = &mvReturn0_[ticker];
		vProfit0 = &mvProfit0_[ticker];
		vReturn = &mvReturn_[ticker];
		vProfit = &mvProfit_[ticker];
	}

	// Loop
	int& position = mPos_[ticker];
	nn_->start_ticker(position);
	double lastMid = 0;
	double lastPos = position;
	double lastTradePrice = 0;
	vector<NNsample>::iterator ite = sample->end();
	for( vector<NNsample>::iterator it = sample->begin(); it != ite; ++it )
	{
		vector<double>& input = it->input;
		double target = it->target;
		QuoteInfo& quote = it->quote;
		double mid = (quote.bid + quote.ask) / 2.0;

		// forward propagation.
		vector<double> output = nn_->forward(input, quote);

		double F = output[0];
		position = output[1];
		double R0 = output[2];
		double R = output[3];
		double P0 = R0 * lastMid;
		double P = R * lastMid;

		double normPos0 = 0;
		double normPos = 0;
		if( position >= lastPos )
		{
			if( trainObj_.find("Tanh") != string::npos )
				normPos0 = F;
			else
				normPos0 = (F * sigP_ - lastPos) / (quote.askSize * lotSize_);
			normPos = (position - lastPos) / (quote.askSize * lotSize_);
		}
		else
		{
			if( trainObj_.find("Tanh") != string::npos )
				normPos0 = F;
			else
				normPos0 = (F * sigP_ - lastPos) / (quote.bidSize * lotSize_);
			normPos = (position - lastPos) / (quote.bidSize * lotSize_);
		}

		if( noTwoTradesAtSamePrice_ )
		{
			double tradePrice = (fabs(normPos) > 1e-10) ? (normPos > 0 ? (quote.ask) : (-quote.bid)) : 0;
			if( fabs(lastTradePrice) > 1e-10 && fabs(tradePrice - lastTradePrice) < 1e-10 ) // attempt to trade at the same price.
				continue;
			if( fabs(tradePrice) > 1e-10 )
				lastTradePrice = tradePrice;
		}

		// back propagation.
		nn_->backprop();

		++n_P_;
		sum_P_ += position;
		sum_P2_ += position * position;
		++n_Pday_;
		sum_Pday_ += position;
		sum_Pday2_ += position * position;
		if( fabs(position - lastPos) > 0.5 )
		{
			++nTradeDay_;
			nShareDay_ += fabs(position - lastPos);
		}

		double dv = fabs(position - lastPos) * mid;
		npReturn0_.add(R0);
		npProfit0_.add(P0);
		npRpt0_.add(R0, dv);
		npGpt0_.add(P0, dv);
		npReturn_.add(R);
		npProfit_.add(P);
		npRpt_.add(R, dv);
		npGpt_.add(P, dv);

		sumR0 += R0;
		sumP0 += P0;
		sumR += R;
		sumP += P;

		houtput_->Fill(normPos);
		prtn_->Fill(normPos, target);
		houtput0_->Fill(normPos0);
		prtn0_->Fill(normPos0, target);

		// Gather information if tickerDetail.
		if( tickerDetail )
		{
			vMsecs->push_back( msecsOffset + quote.msecs );
			vPrice->push_back( mid );
			vPosition->push_back( position );
			vNormPos->push_back( normPos );
			vNormPos0->push_back( normPos0 );
			vReturn0->push_back( R0 );
			vProfit0->push_back( P0 );
			vReturn->push_back( R );
			vProfit->push_back( P );
		}

		if( verbose_ > 2 && tickerDetail )
		{
			if( "maxProfitPos" == trainObj_ )
				printf("bs %3d as %3d F*sigP %6.1f Position %4d %+3d = %4d Rt %+4.1e\n",
					quote.bidSize * lotSize_, quote.askSize * lotSize_, F * sigP_, (int)lastPos, (int)(position - lastPos), position, R);
			else if( "maxProfit" == trainObj_ )
				printf("bs %3d as %3d tanh(net) %4.1f Position %4d %+3d = %4d Rt %+4.1e\n",
					quote.bidSize * lotSize_, quote.askSize * lotSize_, F, (int)lastPos, (int)(position - lastPos), position, R);
			cout << flush;
		}

		lastMid = mid;
		lastPos = position;
	}

	hreturn0_->Fill(sumR0);
	hprofit0_->Fill(sumP0);
	hreturn_->Fill(sumR);
	hprofit_->Fill(sumP);

	return;
}

void GTA_NNtrainer::endDay()
{
	if( debug_ )
	{
		// Write the model.
		int idate = GTEvent::Instance()->idate();
		char filename[40];
		sprintf(filename, "debug_model_%s_%d_%d_%d.txt", trainObj_.c_str(), nHidden_, iEpoch_, idate);
		ofstream os(filename);
		os << *nn_;
	}

	if( nticker_trained_day_ > 0 )
	{
		if( nn_ != 0 )
		{
			// Keep track of the weights.
			int nwgts = nn_->get_nwgts();
			vector<double> vWgts = nn_->get_wgts();
			for( int i=0; i<nwgts; ++i )
				vvWgts_[i].push_back(vWgts[i]);
		}

		// Performance measures.
		if( "minErr" == trainObj_ )
		{
			double v = npError_.endofdayerr();
			cout << " day error " << v << endl;
			vDayErr_.push_back(v);
		}
		else if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
		{
			double r0 = npReturn0_.endofdaysum();
			double p0 = npProfit0_.endofdaysum();
			double rpt0 = npRpt0_.endofdayrat();
			double gpt0 = npGpt0_.endofdayrat();
			double r = npReturn_.endofdaysum();
			double p = npProfit_.endofdaysum();
			double rpt = npRpt_.endofdayrat();
			double gpt = npGpt_.endofdayrat();
			printf(" %s day r0 %5.1f p0 %5.1f rpt0 %.4f gpt0 %5.2f\n", moduleName_.c_str(), r0, p0, rpt0, gpt0);
			printf(" %s day r  %5.1f p  %5.1f rpt  %.4f gpt  %5.2f\n", moduleName_.c_str(), r, p, rpt, gpt);
			if( trainObj_.find("Return") != string::npos )
				vDayErr_.push_back(-r);
			else if( trainObj_.find("Profit") != string::npos )
				vDayErr_.push_back(-p);

			// sigP_
			if( n_Pday_ > 0 )
			{
				double mean_P = sum_Pday_ / n_Pday_;
				double mean_P2 = sum_Pday2_ / n_Pday_;
				double stdev_P = sqrt( mean_P2 - mean_P * mean_P );
				printf(" nP= %5d mean= %.1f stdev= %4.2e nTrade %d nShare %d\n", (int)n_Pday_, mean_P, stdev_P, (int)nTradeDay_, (int)nShareDay_);
				cout << flush;
			}
		}

		// Reset.
		nticker_trained_day_ = 0;
	}

	++iDay_;

	return;
}

void GTA_NNtrainer::endMarket()
{
	cout << endl << "endMarket." << endl;
	if( nticker_trained_market_ > 0 )
	{
		++iEpoch_;

		// Performance measures.
		double performance = 0;
		if( "minErr" == trainObj_ )
		{
			double v = npError_.resulterr();
			//cout << " epoch error " << v << endl;
			performance = v;
		}
		else if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
		{
			double r0 = npReturn0_.resultsum();
			double p0 = npProfit0_.resultsum();
			double rpt0 = npRpt0_.resultrat();
			double gpt0 = npGpt0_.resultrat();
			double r = npReturn_.resultsum();
			double p = npProfit_.resultsum();
			double rpt = npRpt_.resultrat();
			double gpt = npGpt_.resultrat();
			printf(" %s epoch r0 %5.1f p0 %5.1f rpt0 %.4f gpt0 %5.2f\n", moduleName_.c_str(), r0, p0, rpt0, gpt0);
			printf(" %s epoch r  %5.1f p  %5.1f rpt  %.4f gpt  %5.2f\n", moduleName_.c_str(), r, p, rpt, gpt);
			if( trainObj_.find("Return") != string::npos )
				performance = -r;
			else if( trainObj_.find("Profit") != string::npos )
				performance = -p;
		}

		if( !(fabs(performance) > 0 && fabs(performance) < 1e10) )
			vEpochErr_.push_back(0);
		else
			vEpochErr_.push_back(performance);

		// Reset
		nticker_trained_market_ = 0;

		// Write root file.
		TFile* f = GTEnv::Instance()->outfile();

		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->cd("output");

		houtput_->Write();
		delete houtput_;

		prtn_->Write();
		delete prtn_;

		if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
		{
			f->cd();
			f->cd(moduleName_.c_str());
			gDirectory->cd("output0");

			houtput0_->Write();
			delete houtput0_;

			prtn0_->Write();
			delete prtn0_;

			f->cd();
			f->cd(moduleName_.c_str());
			gDirectory->cd("performance");

			hreturn0_->Write();
			hprofit0_->Write();
			hreturn_->Write();
			hprofit_->Write();
			delete hreturn0_;
			delete hprofit0_;
			delete hreturn_;
			delete hprofit_;
		}

		// Write the model every epoch.
		char filename[40];
		//sprintf(filename, "model_%s_%d_%d.txt", trainObj_.c_str(), nHidden_, iEpoch_);
		sprintf(filename, "model_%s_%d.txt", moduleName_.c_str(), iEpoch_);
		ofstream os(filename);
		os << *nn_;

		// Bold Driver algorithm.
		if( bold_driver_ )
		{
			bool well_behaving = fabs(performance) > 0 && fabs(performance) < 1e10;
			bool early_training = iEpoch_ <= 2 || fabs(epoch_rms_prev_) <= 1e-100;

			if( well_behaving )
			{
				if( early_training ) // Do nothing and move on.
				{
					nn_->save();
					epoch_rms_prev_ = performance;
				}
				else // Bold Driver.
				{
					if( performance >= epoch_rms_prev_ ) // not improved.
					{
						nn_->restore();
						nn_->scale_learn(bold_driver_scale_down_);
					}
					else // improved.
					{
						nn_->save();
						nn_->scale_learn(bold_driver_scale_up_);
						epoch_rms_prev_ = performance;
					}
				}
			}
			else // Undo the training and reduce the learning rate.
			{
				nn_->scale_learn(0.1);
				nn_->restore();
			}
		}

		if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
		{
			// sigP_
			if( n_P_ > 0 )
			{
				double mean_P = sum_P_ / n_P_;
				double mean_P2 = sum_P2_ / n_P_;
				double stdev_P = sqrt( mean_P2 - mean_P * mean_P );
				printf(" nP= %5d mean= %.1f stdev= %4.2e sigP_= %4.1f\n", (int)n_P_, mean_P, stdev_P, sigP_);
				cout << flush;
			}

			//if( trainingDetail_ )
			{
				char subdir[100];
				sprintf(subdir, "epoch_%d", iEpoch_);
				TFile* f = GTEnv::Instance()->outfile();
				f->cd();
				f->cd(moduleName_.c_str());
				gDirectory->cd("tickers");
				gDirectory->mkdir(subdir);

				for( map<string, vector<double> >::iterator it = mvMsecs_.begin(); it != mvMsecs_.end(); ++it )
				{
					string ticker = it->first;

					f->cd();
					f->cd(moduleName_.c_str());
					gDirectory->cd("tickers");
					gDirectory->cd(subdir);
					gDirectory->mkdir(ticker.c_str());

					plot_tickerDetail(ticker, "Price", mvMsecs_[ticker], mvPrice_[ticker]);
					plot_tickerDetail(ticker, "Position", mvMsecs_[ticker], mvPosition_[ticker]);
					plot_tickerDetail(ticker, "NormPos", mvMsecs_[ticker], mvNormPos_[ticker]);
					plot_tickerDetail(ticker, "NormPos0", mvMsecs_[ticker], mvNormPos0_[ticker]);
					plot_tickerDetail(ticker, "CumReturn0", mvMsecs_[ticker], mvReturn0_[ticker], true);
					plot_tickerDetail(ticker, "CumProfit0", mvMsecs_[ticker], mvProfit0_[ticker], true);
					plot_tickerDetail(ticker, "CumReturn", mvMsecs_[ticker], mvReturn_[ticker], true);
					plot_tickerDetail(ticker, "CumProfit", mvMsecs_[ticker], mvProfit_[ticker], true);
				}
			}
		}
	}

	return;
}

void GTA_NNtrainer::endJob()
{
	if( nn_ != 0 )
		delete nn_;

	// Create the weight graphs.
	int nwgts = vvWgts_.size();
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

	// Create the error graphs.
	TGraph* grDayErr = make_graph(vDayErr_);
	grDayErr->SetName("grDayErr");
	grDayErr->SetTitle("Daily Error");
	TGraph* grEpochErr = make_graph(vEpochErr_);
	grEpochErr->SetName("grEpochErr");
	grEpochErr->SetTitle("Epoch Error");

	// Write the graphes.
	TFile* f = GTEnv::Instance()->outfile();

	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("weight");
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

	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("performance");
	grDayErr->Write();
	grEpochErr->Write();
	delete grDayErr;
	delete grEpochErr;

	vvWgts_.clear();

	return;
}

TGraph* GTA_NNtrainer::make_graph(vector<double>& v)
{
	int N = v.size();
	vector<double> x;
	for( int i=0; i<N; ++i )
		x.push_back(i+1);
	TGraph* gr = new TGraph(N, &x[0], &v[0]);
	return gr;
}

void GTA_NNtrainer::plot_tickerDetail(string ticker, string var, vector<double> vMsecs, vector<double> v, bool cumulative)
{ 
	int N = vMsecs.size();
	if( N < 10 )
		return;

	vMsecs.push_back( vMsecs[N-1] + vMsecs[N-1] - vMsecs[N-2] );
	v.push_back( v[N-1] );

	char name[40];
	char title[100];
	sprintf(name, "%s_%s", ticker.c_str(), var.c_str());
	sprintf(title, "%s_%s", ticker.c_str(), var.c_str());

	TH1F* h = new TH1F(name, title, N, &vMsecs[0]);

	if( cumulative )
	{
		double sum = 0;
		for( int b=1; b<=N; ++b )
		{
			sum += v[b-1];
			h->SetBinContent(b, sum);
		}
	}
	else
	{
		for( int b=1; b<=N; ++b )
		{
			double val = v[b-1];
			h->SetBinContent(b, val);
		}
	}

	char subdir[100];
	sprintf(subdir, "epoch_%d", iEpoch_);
	TFile* f = GTEnv::Instance()->outfile();
	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("tickers");
	gDirectory->cd(subdir);
	gDirectory->cd(ticker.c_str());
	h->Write();
	delete h;

	return;
}