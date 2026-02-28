#include "GTA_Btrainer.h"
#include "GTEvent.h"
#include "GTEnv.h"
#include "GenBPNN.h"
#include "BPTT.h"
#include "Butil.h"
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

GTA_Btrainer::GTA_Btrainer(const string& moduleName, const multimap<string, string>& conf)
:GTModule(moduleName),
iDay_(0),
iModel_(1),
nHidden_(0),
thres_(0.0001),
thresIncr_(1),
maxPos_(100),
cost_(1),
learn_(1),
gamma_(1),
maxWgt_(10),
updateFreq_("ticker"),
trainObj_("minPredErr"),
verbose_(0),
useBias_(0),
iEpoch_(0),
epoch_error_(0),
epoch_rms_prev_(0),
day_error_(0),
day_npoints_(0),
epoch_npoints_(0),
nticker_trained_day_(0),
nticker_trained_market_(0),
writeRoot_(false),
bold_driver_(false)
{
	if( conf.count("iModel") )
		iModel_ = atoi( conf.find("iModel")->second.c_str() );
	if( conf.count("thres") )
		thres_ = atof( conf.find("thres")->second.c_str() );
	if( conf.count("thresIncr") )
		thresIncr_ = atof( conf.find("thresIncr")->second.c_str() );
	if( conf.count("maxPos") )
		maxPos_ = atoi( conf.find("maxPos")->second.c_str() );
	if( conf.count("cost") )
		cost_ = atof( conf.find("cost")->second.c_str() );
	if( conf.count("learn") )
		learn_ = atof( conf.find("learn")->second.c_str() );
	if( conf.count("gamma") )
		gamma_ = atof( conf.find("gamma")->second.c_str() );
	if( conf.count("nHidden") )
		nHidden_ = atoi( conf.find("nHidden")->second.c_str() );
	if( conf.count("updateFreq") )
		updateFreq_ = conf.find("updateFreq")->second.c_str();
	if( conf.count("trainObj") )
		trainObj_ = conf.find("trainObj")->second.c_str();
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("useBias") )
		useBias_ = atoi( conf.find("useBias")->second.c_str() );
	if( conf.count("rewardTime") )
		rewardTime_ = atoi( conf.find("rewardTime")->second.c_str() );
	if( conf.count("maxWgt") )
		maxWgt_ = atof( conf.find("maxWgt")->second.c_str() );
	if( conf.count("writeRoot") )
		writeRoot_ = conf.find("writeRoot")->second == "true";
	if( conf.count("boldDriver") )
		bold_driver_ = true;
}

GTA_Btrainer::~GTA_Btrainer()
{}

void GTA_Btrainer::beginJob()
{
	nInput_ = Butil::get_nInput(iModel_);
	nOutput_ = Butil::get_nOutput(iModel_);

	vector<int> N;
	N.push_back(nInput_); // number of inputs.
	if( nHidden_ > 0 )
		N.push_back(nHidden_); // number of hidden units.
	N.push_back(nOutput_);
	bpnn_ = GenBPNN( N, learn_, maxWgt_, verbose_, useBias_ );
	bptt_ = BPTT( N, learn_, maxWgt_, verbose_, useBias_ );

	bpnn_.save();

	if( "maxProfitPos" == trainObj_ )
		vvWgts_ = vector<vector<double> >(bptt_.get_nwgts(), vector<double>());
	else
		vvWgts_ = vector<vector<double> >(bpnn_.get_nwgts(), vector<double>());

	if( writeRoot_ )
	{
		TFile* f = GTEnv::Instance()->outfile();
		f->cd();
		f->mkdir(moduleName_.c_str());

		f->cd(moduleName_.c_str());
		gDirectory->mkdir("weight");
		gDirectory->mkdir("output");
		gDirectory->mkdir("error");
	}

	return;
}

void GTA_Btrainer::beginMarket()
{
	epoch_error_ = 0;
	epoch_npoints_ = 0;

	return;
}

void GTA_Btrainer::beginDay()
{
	day_error_ = 0;
	day_npoints_ = 0;

	// Keep track of the weights.
	if( "maxProfitPos" == trainObj_ )
	{
		int nwgts = bptt_.get_nwgts();
		vector<double> vWgts = bptt_.get_wgts();
		for( int i=0; i<nwgts; ++i )
			vvWgts_[i].push_back(vWgts[i]);
	}
	else
	{
		int nwgts = bpnn_.get_nwgts();
		vector<double> vWgts = bpnn_.get_wgts();
		for( int i=0; i<nwgts; ++i )
			vvWgts_[i].push_back(vWgts[i]);
	}

	int idate = GTEvent::Instance()->idate();
	char name[40];
	char title[40];

	sprintf(name, "output_%d_%d", iEpoch_, idate);
	sprintf(title, "Model Output %d %d", iEpoch_, idate);
	houtput_ = new TH1F(name, title, 100, -1e-4, 1e-4);
	houtput_->SetBit(TH1::kCanRebin);

	sprintf(name, "prtn_%d_%d", iEpoch_, idate);
	sprintf(title, "Return vs Prediction %d %d", iEpoch_, idate);
	prtn_ = new TProfile(name, title, 100, -1e-4, 1e-4);
	prtn_->SetBit(TH1::kCanRebin);

	return;
}

void GTA_Btrainer::doTicker()
{
	vector<Bsample>* sample_ = static_cast<vector<Bsample>*>(GTEvent::Instance()->get("sample"));

	if( sample_ != 0 )
	{
		++nticker_trained_day_;
		++nticker_trained_market_;

		if( "minPredErr" == trainObj_ )
			doTicker_minPredErr();
		else if( "minErrTD" == trainObj_ )
			doTicker_minErrTD();
		else if( "maxProfit" == trainObj_ )
			doTicker_maxProfit();
		else if( "maxProfitPos" == trainObj_ )
			doTicker_maxProfitPos();

		if( "maxProfitPos" != trainObj_ && "ticker" == updateFreq_ )
			bpnn_.update_wgt();
	}
	return;
}

void GTA_Btrainer::doTicker_minPredErr()
{
	vector<Bsample>* sample	= static_cast<vector<Bsample>*>(GTEvent::Instance()->get("sample"));

	// loop over the sample and train the model.
	vector<double> error(nOutput_);
	vector<Bsample>::iterator ite = sample->end();
	for( vector<Bsample>::iterator it = sample->begin(); it != ite; ++it )
	{
		vector<double>& input = it->input;
		vector<double>& target = it->target;

		vector<double> output = bpnn_.forward(input);
		if( !output.empty() )
		{
			error[0] = target[0] - output[0];
			bpnn_.backprop(error);

			if( "point" == updateFreq_ )
				bpnn_.update_wgt();

			double err = error[0]*error[0];
			day_error_ += err;
			epoch_error_ += err;
			++day_npoints_;
			++epoch_npoints_;

			houtput_->Fill(output[0]);
			prtn_->Fill(output[0], target[0]);
		}
	}

	return;
}

void GTA_Btrainer::doTicker_minErrTD()
{
	vector<Bsample>* sample	= static_cast<vector<Bsample>*>(GTEvent::Instance()->get("sample"));
	if( sample->size() < 5 )
		return;

	// loop over the sample and train the model.
	vector<double> error(nOutput_);
	vector<Bsample>::iterator ite = sample->end();
	--ite;
	for( vector<Bsample>::iterator it = sample->begin(); it != ite; ++it )
	{
		vector<double>& input = it->input;
		vector<double>& target = it->target;
		vector<double>& next_input = (it+1)->input;
		QuoteInfo& quote1 = it->quote;
		QuoteInfo& quote2 = (it+1)->quote;
		double mid1 = (quote1.bid + quote1.ask) / 2.0;
		double mid2 = (quote2.bid + quote2.ask) / 2.0;
		double rtn = 0;
		if( mid1 > 0 )
			rtn = (mid2 - mid1) / mid1;

		vector<double> output = bpnn_.forward(input);
		vector<double> next_output = bpnn_.forward(next_input);
		double dmin = ((it+1)->quote.msecs - it->quote.msecs)/1000.0/60.0;
		if( it + 1 == ite )
		{
			for( int i=0; i<next_output.size(); ++i )
				next_output[i] = 0;
		}
		if( !output.empty() && !target.empty() )
		{
			error[0] = rtn + pow(gamma_, dmin) * next_output[0] - output[0]; // do not use 'target'
			bpnn_.backpropTD(error);

			if( "point" == updateFreq_ )
				bpnn_.update_wgt();

			double err2 = (target[0] - output[0])*(target[0] - output[0]);
			//double err2 = (target[0] - output[0]) * (target[0] - output[0]);
			day_error_ += err2;
			epoch_error_ += err2;
			++day_npoints_;
			++epoch_npoints_;

			houtput_->Fill(output[0]);
			prtn_->Fill(output[0], target[0]);
		}
	}

	// end of the ticker-day.
	bpnn_.reset_grad();
	
	return;
}

void GTA_Btrainer::doTicker_maxProfit()
{
	vector<Bsample>* sample	= static_cast<vector<Bsample>*>(GTEvent::Instance()->get("sample"));

	// loop over the sample and train the model.
	vector<double> error(nOutput_);
	vector<Bsample>::iterator ite = sample->end();
	for( vector<Bsample>::iterator it = sample->begin(); it != ite; ++it )
	{
		vector<double>& input = it->input;
		vector<double>& target = it->target;
		QuoteInfo& quote = it->quote;

		vector<double> output = bpnn_.forward(input);
		if( !output.empty() )
		{
			error[0] = 0;
			if( fabs(output[0]) > thres_ )
			{
				double sprd = 2.0 * (quote.ask - quote.bid) / (quote.ask + quote.bid);
				error[0] = target[0] - cost_ * sprd/2.0 * sign(output[0]);
				bpnn_.backprop(error);

				if( "point" == updateFreq_ )
					bpnn_.update_wgt();

				double err = error[0]*sign(output[0]);
				day_error_ += err;
				epoch_error_ += err;
				++day_npoints_;
				++epoch_npoints_;

				prtn_->Fill(output[0], target[0]);
			}
			houtput_->Fill(output[0]);
		}
	}

	return;
}

void GTA_Btrainer::doTicker_maxProfitPos()
{
	vector<Bsample>* sample	= static_cast<vector<Bsample>*>(GTEvent::Instance()->get("sample"));

	string ticker = GTEvent::Instance()->ticker();
	int& position = mPos_[ticker];

	bptt_.start_ticker(position);

	double lastPos = position;
	vector<Bsample>::iterator ite = sample->end();
	for( vector<Bsample>::iterator it = sample->begin(); it != ite; ++it )
	{
		vector<double>& input = it->input;
		vector<double>& target = it->target;
		QuoteInfo& quote = it->quote;

		vector<double> output = bptt_.forward(input, quote);
		//position = bptt_.forward(input, quote);
		bptt_.backprop();

		double F = output[0];
		position = output[1];
		double R = output[2];

		double normPos0 = 0;
		double normPos = 0;
		if( position >= lastPos )
		{
			normPos0 = (F - lastPos) / quote.askSize;
			normPos = (position - lastPos) / quote.askSize;
		}
		else
		{
			normPos0 = (F - lastPos) / quote.bidSize;
			normPos = (position - lastPos) / quote.bidSize;
		}

		double err = -R;
		day_error_ += err;
		epoch_error_ += err;

		houtput_->Fill(normPos0);
		prtn_->Fill(normPos, target[0]);
		lastPos = position;
		if( verbose_ > 2 )
			cout << " position: " << position << endl;
	}

	return;
}

void GTA_Btrainer::endDay()
{
	if( nticker_trained_day_ > 0 )
	{
		if( "day" == updateFreq_ )
			bpnn_.update_wgt();

		double day_rms = 0;
		if( ("minPredErr" == trainObj_ || "minErrTD" == trainObj_) && day_npoints_ > 0 )
			day_rms = sqrt(day_error_/day_npoints_);
		else if( "maxProfit" == trainObj_ || "maxProfitPos" == trainObj_ )
			day_rms = day_error_;

		cout << " day error " << day_rms << endl;

		if( fabs(day_rms) <= 0 || fabs(day_rms) > 1e100 )
			day_rms = 0;
		vDayErr_.push_back(day_rms);

		// Write root file.
		TFile* f = GTEnv::Instance()->outfile();

		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->cd("output");
		houtput_->Write();
		delete houtput_;
		houtput_ = 0;

		//f->cd();
		//f->cd(moduleName_.c_str());
		//gDirectory->cd("error");
		prtn_->Write();
		delete prtn_;
		prtn_ = 0;

		// Write the model every five days.
		int idate = GTEvent::Instance()->idate();
		if( iDay_++ % 5 == 0 )
		{
			char filename[40];
			sprintf(filename, "model_%s_%d_%d.txt", trainObj_.c_str(), nHidden_, iDay_);
			ofstream os(filename);
			os << bpnn_;
		}
		nticker_trained_day_ = 0;
	}

	return;
}

void GTA_Btrainer::endMarket()
{
	if( nticker_trained_market_ > 0 )
	{
		if( "set" == updateFreq_ )
			bpnn_.update_wgt();

		++iEpoch_;
		double epoch_rms = 0;
		if( ("minPredErr" == trainObj_ || "minErrTD" == trainObj_) && epoch_npoints_ > 0 )
			epoch_rms = sqrt(epoch_error_/epoch_npoints_);
		else if( "maxProfit" == trainObj_ || "maxProfitPos" == trainObj_ )
			epoch_rms = epoch_error_;

		cout << " epoch error " << epoch_rms << endl;

		if( !(fabs(epoch_rms) > 0 && fabs(epoch_rms) < 1e10) )
			vEpochErr_.push_back(0);
		else
			vEpochErr_.push_back(epoch_rms);
		nticker_trained_market_ = 0;

		if( bold_driver_ )
		{
			bool well_behaving = fabs(epoch_rms) > 0 && fabs(epoch_rms) < 1e10;
			bool early_training = epoch_rms_prev_ <= 1e-100 || iEpoch_ <= 2;

			if( well_behaving )
			{
				if( early_training ) // Do nothing and move on.
				{
					if( "maxProfitPos" == trainObj_ )
					{
						bptt_.save();
					}
					else
					{
						bpnn_.save();
					}
					epoch_rms_prev_ = epoch_rms;
				}
				else // Bold Driver.
				{
					if( epoch_rms >= epoch_rms_prev_ )
					{
						if( "maxProfitPos" == trainObj_ )
						{
							///bptt_.restore();
							//bptt_.scale_learn(0.5);
						}
						else
						{
							bpnn_.restore();
							bpnn_.scale_learn(0.5);
						}
					}
					else
					{
						if( "maxProfitPos" == trainObj_ )
						{
							//bptt_.save();
							//bptt_.scale_learn(1.1);
						}
						else
						{
							bpnn_.save();
							bpnn_.scale_learn(1.1);
						}
						epoch_rms_prev_ = epoch_rms;
					}
				}
			}
			else // Undo the training and reduce the learning rate.
			{
				if( "maxProfitPos" == trainObj_ )
				{
					bptt_.scale_learn(0.1);
					bptt_.restore();
				}
				else
				{
					bpnn_.scale_learn(0.1);
					bpnn_.restore();
				}
			}
		}
	}

	return;
}

void GTA_Btrainer::endJob()
{
	// summarize the train.
	if( writeRoot_ )
	{
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
		gDirectory->cd("error");
		grDayErr->Write();
		grEpochErr->Write();
		delete grDayErr;
		delete grEpochErr;
	}
	vvWgts_.clear();

	// Write the model.
	ofstream os("model.txt");
	os << bpnn_;

	return;
}

TGraph* GTA_Btrainer::make_graph(vector<double>& v)
{
	int N = v.size();
	vector<double> x;
	for( int i=0; i<N; ++i )
		x.push_back(i+1);
	TGraph* gr = new TGraph(N, &x[0], &v[0]);
	return gr;
}

int GTA_Btrainer::FtoInt(double F)
{
	int ret = ceil(maxPos_ * F - 0.5);

	return ret;
}
