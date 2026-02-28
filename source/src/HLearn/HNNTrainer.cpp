#include "HLib/HEvent.h"
#include "HLib/HEnv.h"
#include <HLearn/HNNTrainer.h>
#include <HLearnObj/Sample.h>
#include <HLearnObj/InputInfo.h>
#include <HLearnObj/BPNNbatch.h>
#include <HLearnObj/BPTT.h>
#include <HLearnObj/BPTT2.h>
#include <HLearnObj/BPTTpred.h>
#include <HLearnObj/BPTTpred2in.h>
#include <HLearnObj/BPTTpred2inD.h>
#include <jl_lib.h>
#include "optionlibs/TickData.h"
#include <map>
#include <string>
using namespace std;

HNNTrainer::HNNTrainer(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
updateFreq_(""),
monFreq_(""),
iDay_(0),
nHidden_(0),
maxPos_(20000),
maxPosChg_(1000),
lotSize_(1),
costMult_(1),
learn_(1),
wgtDecay_(0),
sigP_(1),
maxWgt_(10),
poslim_(0),
trainObj_("minErr"),
nnFile_(""),
verbose_(0),
biasOut_(0),
iEpoch_(0),
epoch_rms_prev_(0),
earlyEpoch_(2),
nticker_trained_day_(0),
nticker_trained_market_(0),
noTwoTradesAtSamePrice_(false),
bold_driver_(true),
bold_driver_scale_up_(1.1),
bold_driver_scale_down_(0.5),
trainingDetail_(false),
nn_(0)
{
	if( conf.count("nnFile") )
		nnFile_ = conf.find("nnFile")->second;
	if( conf.count("initPerformance") )
		epoch_rms_prev_ = atof( conf.find("initPerformance")->second.c_str() );
	if( conf.count("earlyEpoch") )
		earlyEpoch_ = atoi( conf.find("earlyEpoch")->second.c_str() );

	if( conf.count("updateFreq") )
		updateFreq_ = conf.find("updateFreq")->second;
	if( conf.count("monFreq") )
		monFreq_ = conf.find("monFreq")->second;
	else
		monFreq_ = updateFreq_;
	if( conf.count("maxPos") )
		maxPos_ = atoi( conf.find("maxPos")->second.c_str() );
	if( conf.count("maxPosChg") )
		maxPosChg_ = atoi( conf.find("maxPosChg")->second.c_str() );
	if( conf.count("costMult") )
		costMult_ = atof( conf.find("costMult")->second.c_str() );
	if( conf.count("learn") )
		learn_ = atof( conf.find("learn")->second.c_str() );
	if( conf.count("wgtDecay") )
		wgtDecay_ = atof( conf.find("wgtDecay")->second.c_str() );
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
	//if( trainObj_ == "bpttPred2in" || trainObj_ == "bpttPred2inD" )
	//	costMult_ = 0;
}

HNNTrainer::~HNNTrainer()
{}

/*
* Functions inherited from HModule.
*/

void HNNTrainer::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	//doClone_ = HEnv::Instance()->multiThreadTicker();
	if( updateFreq_ == "" )
	{
		cout << "ERROR: updateFreq_ is empty\n";
		exit(5);
	}
	doClone_ = "input" != updateFreq_;

	const hnn::InputInfo* nii = static_cast<const hnn::InputInfo*>(HEvent::Instance()->get("", "input_info"));
	if( nii != 0 )
	{
		// Lot size.
		vector<string> vmarket = HEnv::Instance()->markets();
		if( !vmarket.empty() && (mto::region(vmarket[0]) == "U" || mto::region(vmarket[0]) == "C" ) )
			lotSize_ = 100;

		// Create Neural Network.
		int nInput = nii->n;
		int nOutput = 1;

		vector<int> N;
		N.push_back(nInput); // number of inputs.
		if( nHidden_ > 0 )
			N.push_back(nHidden_); // number of hidden units.
		N.push_back(nOutput);

		bool maxRet = false;
		if( trainObj_.size() >= 9 && trainObj_.substr(0, 9) == "maxReturn" )
			maxRet = true;

		string nnPath = (string)"model\\" + nnFile_;
		ifstream ifs(nnPath.c_str());
		if( ifs.is_open() ) // Read from file.
		{
			if( "maxReturn" == trainObj_ || "maxProfit" == trainObj_ )
			{
				nn_ = new hnn::BPTT();
				nn_->read(ifs);
			}
			else if( "maxProfit2" == trainObj_ )
			{
				nn_ = new hnn::BPTT2();
				nn_->read(ifs);
			}
			else if( "minErr" == trainObj_ )
			{
				nn_ = new hnn::BPNNbatch();
				nn_->read(ifs);
			}

			// leaning rate, wgtDecay.
			nn_->set_learn(learn_);
			nn_->set_wgtDecay(wgtDecay_);
			//nn_->set_sigP(sigP_);
		}
		else // Create one.
		{
			if( "maxReturn" == trainObj_ || "maxProfit" == trainObj_ )
				nn_ = new hnn::BPTT( N, learn_, wgtDecay_, maxWgt_, verbose_, biasOut_, costMult_, sigP_, poslim_, maxPos_, lotSize_,
									maxRet, noTwoTradesAtSamePrice_ );
			else if( "maxProfit2" == trainObj_ )
				nn_ = new hnn::BPTT2( N, learn_, wgtDecay_, maxWgt_, verbose_, biasOut_, costMult_, sigP_, poslim_, maxPos_, maxPosChg_, lotSize_,
									maxRet, noTwoTradesAtSamePrice_ );
			else if( "bpttPred" == trainObj_ )
				nn_ = new hnn::BPTTpred( N, learn_, wgtDecay_, maxWgt_, verbose_, biasOut_, costMult_, sigP_, poslim_, maxPos_, lotSize_,
									maxRet, noTwoTradesAtSamePrice_ );
			else if( "bpttPred2in" == trainObj_ )
				nn_ = new hnn::BPTTpred2in( N, learn_, wgtDecay_, maxWgt_, verbose_, biasOut_, costMult_, sigP_, poslim_, maxPos_, maxPosChg_, lotSize_,
									maxRet, noTwoTradesAtSamePrice_ );
			else if( "bpttPred2inD" == trainObj_ )
				nn_ = new hnn::BPTTpred2inD( N, learn_, wgtDecay_, maxWgt_, verbose_, biasOut_, costMult_, sigP_, poslim_, maxPos_, lotSize_,
									maxRet, noTwoTradesAtSamePrice_ );
			else
				nn_ = new hnn::BPNNbatch( N, learn_, wgtDecay_, maxWgt_, verbose_, biasOut_ );
		}

		nn_->save();

		// Keep track of the weights.
		vvWgts_ = vector<vector<double> >(nn_->get_nwgts(), vector<double>());
		if( nn_ != 0 )
		{
			int nwgts = nn_->get_nwgts();
			vector<double> vWgts = nn_->get_wgts();
			for( int i=0; i<nwgts; ++i )
			{
				double val = vWgts[i];
				if( fabs(val) > 1e4 )
					val = 0;
				vvWgts_[i].push_back(val);
			}
		}

		// Create ROOT file directories.
		TFile* f = HEnv::Instance()->outfile();
		f->cd();
		f->mkdir(moduleName_.c_str());

		f->cd(moduleName_.c_str());
		gDirectory->mkdir("performance");
		gDirectory->mkdir("weight");
		gDirectory->mkdir("output");
		if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
			|| "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
		{
			gDirectory->mkdir("output0");
			gDirectory->mkdir("tickers");
		}
	}
	return;
}

void HNNTrainer::beginMarket()
{
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

		if( "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
		{
			sprintf(name, "hTarget_%d", iEpoch_);
			sprintf(title, "Target %d", iEpoch_);
			hTarget_ = TH1F(name, title, 100, -1e-4, 1e-4);
			hTarget_.SetBit(TH1::kCanRebin);

			sprintf(name, "hnnOut1_%d", iEpoch_);
			sprintf(title, "Output | target = 1 %d", iEpoch_);
			hnnOut1_ = TH1F(name, title, 100, -1e-4, 1e-4);
			hnnOut1_.SetBit(TH1::kCanRebin);

			sprintf(name, "hnnOut2_%d", iEpoch_);
			sprintf(title, "Output | target = -1 %d", iEpoch_);
			hnnOut2_ = TH1F(name, title, 100, -1e-4, 1e-4);
			hnnOut2_.SetBit(TH1::kCanRebin);
		}

		if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
			|| "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
		{
			sprintf(name, "output0_%d", iEpoch_);
			sprintf(title, "Model Output %d", iEpoch_);
			houtput0_ = new TH1F(name, title, 60, -3, 3);

			sprintf(name, "prtn0_%d", iEpoch_);
			sprintf(title, "Return vs Prediction %d", iEpoch_);
			prtn0_ = new TProfile(name, title, 60, -3, 3);
		}

		// For each stock-day
		if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
			|| "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
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
		accDV_.clear();
		accError_.clear();
		accReturn_.clear();
		accProfit_.clear();
		accReturn0_.clear();
		accProfit0_.clear();

		accP_.clear();

		// Training detail
		if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
			|| "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
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

void HNNTrainer::beginDay()
{
	nTradeDay_ = 0;
	nShareDay_ = 0;

	accDVDay_.clear();
	accErrorDay_.clear();
	accReturnDay_.clear();
	accProfitDay_.clear();
	accReturn0Day_.clear();
	accProfit0Day_.clear();

	accPDay_.clear();

	nn_->beginDay();

	return;
}

void HNNTrainer::beginTicker(const string& ticker)
{
	hnn::NNBase* nn = 0;
	if( doClone_ )
		nn = nn_->clone();
	else
		nn = nn_;

	const vector<hnn::Sample>* sample = 0;
	sample = static_cast<const vector<hnn::Sample>*>(HEvent::Instance()->get(ticker, "sample"));

	if( sample != 0 )
	{
		{
			boost::mutex::scoped_lock lock(private_mutex_);
			++nticker_trained_day_;
			++nticker_trained_market_;
		}

		if( "minErr" == trainObj_ )
			doTicker_minErr(ticker, nn);
		else if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
			|| "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
			doTicker_maxProfit(ticker, nn);
	}

	if( doClone_ )
	{
		boost::mutex::scoped_lock lock(nn_mutex_);
		nn_->update_dw(*nn);
		if( "ticker" == updateFreq_ )
			nn_->update_wgts();
		if( "ticker" == monFreq_ )
			record_weights();
	}

	if( doClone_ && nn != 0 )
		delete nn;

	return;
}

void HNNTrainer::endTicker(const string& ticker)
{
	return;
}

void HNNTrainer::endDay()
{
	if( nticker_trained_day_ > 0 )
	{
		double v = sqrt(accErrorDay_.mean());
		double r = accReturnDay_.sum;
		double p = accProfitDay_.sum;

		if( "day" == updateFreq_ )
			nn_->update_wgts();

		// Performance measures.
		if( "minErr" == trainObj_ )
		{
			char buf[200];
			sprintf(buf, " %s day error %.4f\n", moduleName_.c_str(), v);
			cout << buf;
			vDayErr_.push_back(v);
		}
		else if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3)) )
		{
			if( trainObj_.find("Return") != string::npos )
				vDayErr_.push_back(-r);
			else if( trainObj_.find("Profit") != string::npos )
				vDayErr_.push_back(-p);
		}
		else if( "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
		{
			vDayErr_.push_back(-p);
		}

		// Trading summary
		if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
			|| "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
		{
			double rpt = (accDVDay_.sum > 0) ? (accReturnDay_.sum / accDVDay_.sum * 10000) : 0;
			double gpt = (accDVDay_.sum > 0) ? (accProfitDay_.sum / accDVDay_.sum * 10000) : 0;
			double r0 = accReturn0Day_.sum;
			double p0 = accProfit0Day_.sum;
			double rpt0 = (accDVDay_.sum > 0) ? (accReturn0Day_.sum / accDVDay_.sum * 10000) : 0;
			double gpt0 = (accDVDay_.sum > 0) ? (accProfit0Day_.sum / accDVDay_.sum * 10000) : 0;

			char buf1[200];
			char buf2[200];
			sprintf(buf1, " %s day r0 %5.1f p0 %5.1f rpt0 %.4f gpt0 %5.2f\n", moduleName_.c_str(), r0, p0, rpt0, gpt0);
			sprintf(buf2, " %s day r  %5.1f p  %5.1f rpt  %.4f gpt  %5.2f\n", moduleName_.c_str(), r, p, rpt, gpt);
			cout << buf1 << buf2;

			if( accPDay_.n > 0 )
			{
				char buf[200];
				sprintf(buf, " nP= %5d mean= %.1f stdev= %4.2e nTrade %d nShare %d\n",
					(int)accPDay_.n, accPDay_.mean(), accPDay_.stdev(), (int)nTradeDay_, (int)nShareDay_);
				cout << buf;
			}
		}

		// Keep track of the weights.
		if( "day" == monFreq_ )
			record_weights();
/*
		{
			int nwgts = nn_->get_nwgts();
			vector<double> vWgts = nn_->get_wgts();
			for( int i=0; i<nwgts; ++i )
			{
				double val = vWgts[i];
				if( fabs(val) > 1e4 )
					val = 0;
				vvWgts_[i].push_back(val);
			}
		}*/

		// Reset.
		nticker_trained_day_ = 0;
	}

	++iDay_;
	return;
}

void HNNTrainer::endMarket()
{
	cout << endl << "endMarket.\n";
	if( nticker_trained_market_ > 0 )
	{
		++iEpoch_;

		// Performance measures.
		double performance = get_performance();

		print_epoch();

		if( !(fabs(performance) > 0 && fabs(performance) < 1e10) )
			vEpochErr_.push_back(0);
		else
			vEpochErr_.push_back(performance);

		if( nn_ != 0 )
		{
			//
			// Update weights.
			//
			if( "epoch" == updateFreq_ )
				nn_->update_wgts();

			// Bold Driver algorithm.
			if( bold_driver_ )
			{
				bool well_behaving = fabs(performance) > 0 && fabs(performance) < 1e10;
				//bool early_training = iEpoch_ <= 2 || fabs(epoch_rms_prev_) <= 1e-100;
				bool early_training = iEpoch_ <= earlyEpoch_ || fabs(epoch_rms_prev_) <= 1e-100;
				bool weights_valid = nn_->weights_valid();

				if( well_behaving && weights_valid )
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

			// Write the model every epoch.
			char filename[200];
			sprintf(filename, "model\\model_%s_%d.txt", moduleName_.c_str(), iEpoch_);
			ofstream os(filename);
			os << *nn_;

			// Keep track of the weights.
			if( "epoch" == monFreq_ )
				record_weights();
/*
			{
				int nwgts = nn_->get_nwgts();
				vector<double> vWgts = nn_->get_wgts();
				for( int i=0; i<nwgts; ++i )
				{
					double val = vWgts[i];
					if( fabs(val) > 1e4 )
						val = 0;
					vvWgts_[i].push_back(val);
				}
			}*/
		}

		// Reset
		nticker_trained_market_ = 0;

		// Write root file.
		TFile* f = HEnv::Instance()->outfile();
		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->cd("output");

		if( "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
		{
			hTarget_.Write();
			hnnOut1_.Write();
			hnnOut2_.Write(); 
		}

		houtput_->Write();
		delete houtput_;

		prtn_->Write();
		delete prtn_;

		if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
			|| "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
		{
			if( accP_.n > 0 )
			{
				char buf[200];
				sprintf(buf, " nP= %5d mean= %.1f stdev= %4.2e sigP_= %4.1f\n", (int)accP_.n, accP_.mean(), accP_.stdev(), sigP_);
				cout << buf;
			}

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

			char subdir[100];
			sprintf(subdir, "epoch_%d", iEpoch_);
			TFile* f = HEnv::Instance()->outfile();
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

				string tickerDir = ticker;
				int pos = ticker.find(":");
				if( pos != string::npos )
					tickerDir = ticker.replace(pos, 1, "_");
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
	return;
}

void HNNTrainer::endJob()
{
	if( nn_ != 0 )
		delete nn_;

	// Write the graphes.
	TFile* f = HEnv::Instance()->outfile();

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

	// Write.
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

	// Create the error graphs.
	TGraph* grDayErr = make_graph(vDayErr_);
	grDayErr->SetName("grDayErr");
	grDayErr->SetTitle("Daily Error");
	TGraph* grEpochErr = make_graph(vEpochErr_);
	grEpochErr->SetName("grEpochErr");
	grEpochErr->SetTitle("Epoch Error");

	// Write.
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

/*
*	Other functions.
*/

void HNNTrainer::doTicker_minErr(string ticker, hnn::NNBase* nn)
{
	const vector<hnn::Sample>* sample = 0;
	sample = static_cast<const vector<hnn::Sample>*>(HEvent::Instance()->get(ticker, "sample"));

	// loop over the sample and train the model.
	vector<hnn::Sample>::const_iterator ite = sample->end();
	for( vector<hnn::Sample>::const_iterator it = sample->begin(); it != ite; ++it )
	{
		/*
		*	Train.
		*/
		const vector<float>& input = it->input;
		double target = it->target;
		const QuoteInfo& quote = it->quote;

		vector<double> output = nn->train(input, quote, target);
		double nnOutput = output[0];

		/*
		*	Performance Monitoring.
		*/
		double error = (target - nnOutput) * (target - nnOutput);
		{
			boost::mutex::scoped_lock lock(acc_mutex_);
			accErrorDay_.add(error);
			accError_.add(error);
		}

		/*
		*	Plots.
		*/
		{
			boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
			houtput_->Fill(nnOutput);
			prtn_->Fill(nnOutput, target);
		}

		if( "input" == updateFreq_ )
			nn->update_wgts();
	}

	return;
}

void HNNTrainer::doTicker_maxProfit(string ticker, hnn::NNBase* nn)
{
	const vector<hnn::Sample>* sample = 0;
	sample = static_cast<const vector<hnn::Sample>*>(HEvent::Instance()->get(ticker, "sample"));

	// Detail.
	double sumR0 = 0;
	double sumP0 = 0;
	double sumR = 0;
	double sumP = 0;
	bool tickerDetail = false;
	{
		boost::mutex::scoped_lock lock(mv_mutex_);
		if( trainingDetail_ && (mvMsecs_.count(ticker) || mvMsecs_.size() < 10) )
			tickerDetail = true;
	}
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
		string market = HEnv::Instance()->market();
		int idate = HEnv::Instance()->idate();
		int msecOpen = mto::msecOpen(market, idate);
		int msecClose = mto::msecClose(market, idate);
		msecsOffset = - msecOpen + iDay_ * (msecClose - msecOpen);

		{
			boost::mutex::scoped_lock lock(mv_mutex_);
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
	}

	// Loop

	int position = 0;
	{
		boost::mutex::scoped_lock lock(mpos_mutex_);
		position = mPos_[ticker];
	}
	nn->start_ticker(position);
	double lastMid = 0;
	double lastPos = position;
	double lastTradePrice = 0;
	vector<hnn::Sample>::const_iterator ite = sample->end();
	for( vector<hnn::Sample>::const_iterator it = sample->begin(); it != ite; ++it )
	{
		const vector<float>& input = it->input;
		double target = it->target;
		const QuoteInfo& quote = it->quote;
		double mid = (quote.bid + quote.ask) / 2.0;

		vector<double> output = nn->train(input, quote);

		double F = output[0];
		position = output[1];
		double R0 = output[2];
		double R = output[3];
		double P0 = R0 * lastMid;
		double P = R * lastMid;

		if( verbose_ >= 3 )
		{
			printf("%7.1f %7.1f %8.5f %8.5f\n", lastMid, mid, R, P);
		}

		if( "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
		{
			double error = (output[4] - output[5]) * (output[4] - output[5]);
			{
				boost::mutex::scoped_lock lock(acc_mutex_);
				accErrorDay_.add(error);
				accError_.add(error);
			}
		}

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

		double dv = fabs(position - lastPos) * mid;

		{
			boost::mutex::scoped_lock lock(acc_mutex_);

			accP_.add(position);
			accPDay_.add(position);

			accReturn0_.add(R0);
			accProfit0_.add(P0);
			accReturn_.add(R);
			accProfit_.add(P);
			accReturn0Day_.add(R0);
			accProfit0Day_.add(P0);
			accReturnDay_.add(R);
			accProfitDay_.add(P);

			accDV_.add(dv);
			accDVDay_.add(dv);
		}
		if( fabs(position - lastPos) > 0.5 )
		{
			boost::mutex::scoped_lock lock(private_mutex_);
			++nTradeDay_;
			nShareDay_ += fabs(position - lastPos);
		}

		sumR0 += R0;
		sumP0 += P0;
		sumR += R;
		sumP += P;

		//if( verbose_ >= 2 && fabs(normPos) > 1 )
		//	cout << "houtput " << normPos << "\n";

		{
			boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
			if( "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
			{
				double targetPos = output[4];
				double nnPos = output[5];
				houtput_->Fill(nnPos);
				prtn_->Fill(targetPos - lastPos, nnPos - lastPos);
				houtput0_->Fill(nnPos);
				prtn0_->Fill(targetPos - lastPos, nnPos - lastPos);

				hTarget_.Fill(targetPos - lastPos);
				if( fabs(targetPos - lastPos - 1) < 0.1 )
					hnnOut1_.Fill(nnPos - lastPos);
				else if( fabs(targetPos - lastPos + 1) < 0.1 )
					hnnOut2_.Fill(nnPos - lastPos);
			}
			else
			{
				houtput_->Fill(normPos);
				prtn_->Fill(normPos, target);
				houtput0_->Fill(normPos0);
				prtn0_->Fill(normPos0, target);
			}
		}

		// Gather information if tickerDetail.
		if( tickerDetail )
		{
			boost::mutex::scoped_lock lock(mv_mutex_);
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

		lastMid = mid;
		lastPos = position;

		if( "input" == updateFreq_ )
			nn->update_wgts();
	}

	{
		boost::mutex::scoped_lock lock(mpos_mutex_);
		mPos_[ticker] = position;
	}

	{
		boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
		hreturn0_->Fill(sumR0);
		hprofit0_->Fill(sumP0);
		hreturn_->Fill(sumR);
		hprofit_->Fill(sumP);
	}

	return;
}

TGraph* HNNTrainer::make_graph(vector<double>& v)
{
	int N = v.size();
	vector<double> x;
	for( int i=0; i<N; ++i )
		x.push_back(i+1);
	TGraph* gr = new TGraph(N, &x[0], &v[0]);
	return gr;
}

void HNNTrainer::plot_tickerDetail(string ticker, string var, vector<double> vMsecs, vector<double> v, bool cumulative)
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

	TH1F h = TH1F(name, title, N, &vMsecs[0]);

	if( cumulative )
	{
		double sum = 0;
		for( int b=1; b<=N; ++b )
		{
			sum += v[b-1];
			h.SetBinContent(b, sum);
		}
	}
	else
	{
		for( int b=1; b<=N; ++b )
		{
			double val = v[b-1];
			h.SetBinContent(b, val);
		}
	}

	char subdir[100];
	sprintf(subdir, "epoch_%d", iEpoch_);
	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("tickers");
	gDirectory->cd(subdir);
	gDirectory->cd(ticker.c_str());
	h.Write();

	return;
}

double HNNTrainer::get_performance()
{
	double performance = 0;
	if( "minErr" == trainObj_ )
	//if( "minErr" == trainObj_ || "bpttPred" == trainObj_
	//	|| "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
	{
		performance = sqrt(accError_.mean());
		char buf[200];
		sprintf(buf, " %s epoch error %.4f\n", moduleName_.c_str(), performance);
		cout << buf;
	}
	else if( "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
	{
		double r = accReturn_.sum;
		double p = accProfit_.sum;
		performance = -p;
	}
	else if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
	{
		double r = accReturn_.sum;
		double p = accProfit_.sum;
		if( trainObj_.find("Return") != string::npos )
			performance = -r;
		else if( trainObj_.find("Profit") != string::npos )
			performance = -p;
	}
	return performance;
}

void HNNTrainer::print_epoch()
{
	if( (trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3))
		|| "bpttPred" == trainObj_ || "bpttPred2in" == trainObj_ || "bpttPred2inD" == trainObj_ )
	{
		double r0 = accReturn0_.sum;
		double p0 = accProfit0_.sum;
		double rpt0 = (accDV_.sum > 0) ? (accReturn0_.sum / accDV_.sum * 10000) : 0;
		double gpt0 = (accDV_.sum > 0) ? (accProfit0_.sum / accDV_.sum * 10000) : 0;
		double r = accReturn_.sum;
		double p = accProfit_.sum;
		double rpt = (accDV_.sum > 0) ? (accReturn_.sum / accDV_.sum * 10000) : 0;
		double gpt = (accDV_.sum > 0) ? (accProfit_.sum / accDV_.sum * 10000) : 0;

		char buf1[200];
		char buf2[200];
		sprintf(buf1, " %s epoch r0 %5.1f p0 %5.1f rpt0 %.4f gpt0 %5.2f\n", moduleName_.c_str(), r0, p0, rpt0, gpt0);
		sprintf(buf2, " %s epoch r  %5.1f p  %5.1f rpt  %.4f gpt  %5.2f\n", moduleName_.c_str(), r, p, rpt, gpt);
		cout << buf1 << buf2;
	}
	return;
}

void HNNTrainer::record_weights()
{
	int nwgts = nn_->get_nwgts();
	vector<double> vWgts = nn_->get_wgts();
	for( int i=0; i<nwgts; ++i )
	{
		double val = vWgts[i];
		if( fabs(val) > 1e4 )
			val = 0;
		vvWgts_[i].push_back(val);
	}
	return;
}
