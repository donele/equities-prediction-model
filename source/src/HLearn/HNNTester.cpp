#include <HLearn/HNNTester.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <HLearnObj/NNBase.h>
#include <HLearnObj/BPNNbatch.h>
#include <HLearnObj/BPTT.h>
#include <HLearnObj/Pred2Sig.h>
#include <HLearnObj/Pred2Sig_2in.h>
#include <HLearnObj/Sample.h>
#include <HLearnObj/InputInfo.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
using namespace std;

HNNTester::HNNTester(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
verbose_(0),
targetIndx_(-1),
predIndx_(-1),
nn_(0),
nnFile_("")
{
	if( conf.count("trainObj") )
		trainObj_ = conf.find("trainObj")->second.c_str();
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("nnFile") )
		nnFile_ = conf.find("nnFile")->second;
}

HNNTester::~HNNTester()
{}

void HNNTester::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	const hnn::InputInfo* nii = static_cast<const hnn::InputInfo*>(HEvent::Instance()->get("", "input_info"));
	if( nii != 0 )
	{
		string nnPath = (string)"model\\" + nnFile_;
		ifstream ifs(nnPath.c_str());
		if( ifs.is_open() )
		{
			// Create Neural Network.
			if( "maxReturn" == trainObj_ || "maxProfit" == trainObj_ )
			{
				nn_ = new hnn::BPTT();
				nn_->read(ifs);
			}
			else if( "minErr" == trainObj_ )
			{
				nn_ = new hnn::BPNNbatch();
				nn_->read(ifs);
			}
		}
		else if( "pred2sig" == trainObj_ )
		{
			nn_ = new hnn::Pred2Sig();
		}
		else if( "pred2sig2in" == trainObj_ )
		{
			nn_ = new hnn::Pred2Sig_2in();
		}
		else
		{
			cerr << "file " << nnPath << " not found." << endl;
			exit(5);
		}

		// Create ROOT directories.
		TFile* f = HEnv::Instance()->outfile();
		f->cd();
		f->mkdir(moduleName_.c_str());
	}

	const vector<string>* spectatorNames = static_cast<const vector<string>*>(HEvent::Instance()->get("", "spectatorNames"));

	vector<string>::const_iterator itTarget = find(spectatorNames->begin(), spectatorNames->end(), "target");
	if( itTarget != spectatorNames->end() )
		targetIndx_ = distance(spectatorNames->begin(), itTarget);

	vector<string>::const_iterator itPred = find(spectatorNames->begin(), spectatorNames->end(), "tbpred");
	if( itPred != spectatorNames->end() )
		predIndx_ = distance(spectatorNames->begin(), itPred);

	return;
}

void HNNTester::beginMarket()
{
	string market = HEnv::Instance()->market();

	if( HEnv::Instance()->iMarketRep() == 0 )
	{
		houtput_ = TH1F("output1",  "Model Output", 100, -1e-4, 1e-4);
		prtn_ = TProfile("prtn1", "Return vs Prediction", 100, -1e-4, 1e-4);
		hpred_ref_ = TH1F("hpred_ref",  "Prediction Ref", 100, -1e-4, 1e-4);
		hpred_test_ = TH1F("hpred_test",  "Prediction Test", 100, -1e-4, 1e-4);

		houtput_.SetBit(TH1::kCanRebin);
		prtn_.SetBit(TH1::kCanRebin);
		hpred_ref_.SetBit(TH1::kCanRebin);
		hpred_test_.SetBit(TH1::kCanRebin);

		accError_.clear();
	}
	else
	{
		fillQuantileTable(mDecileNNout_, houtput_, 10);
		fillQuantileTable(mPercentilePredRef_, hpred_ref_, 100);
		fillQuantileTable(mPercentilePredTest_, hpred_test_, 100);
	}

	return;
}

void HNNTester::beginDay()
{
	nTickers_ = 0;
	if( HEnv::Instance()->iMarketRep() == 0 )
	{
		accErrorDay_.clear();
	}

	return;
}

void HNNTester::beginTicker(const string& ticker)
{
	hnn::NNBase* nnClone = 0;
	if( nn_ != 0 )
		nnClone = nn_->clone();
	const vector<hnn::Sample>* sample = static_cast<const vector<hnn::Sample>*>(HEvent::Instance()->get(ticker, "sample"));
	int idate = HEnv::Instance()->idate();

	if( verbose_ >= 1 )
		cout << "HNNTester " << ticker << "\t" << idate << endl;

	if( sample != 0 && sample->size() > 0 )
	{
		++nTickers_;
		// loop over the sample and trade.
		vector<hnn::Sample>::const_iterator ite = sample->end();
		for( vector<hnn::Sample>::const_iterator it = sample->begin(); it != ite; ++it )
		{
			double trainTarget = it->target;
			QuoteInfo quote = it->quote;
			double specTarget = it->spectator[targetIndx_];
			double specPred = it->spectator[predIndx_];
			double nnOutput = 0;

			vector<double> output = nnClone->trading_signal(it->input, quote);
			nnOutput = output[0];
			double error = (trainTarget - nnOutput) * (trainTarget - nnOutput);

			double pred = nnOutput + specPred;

			process_error(error, nnOutput, trainTarget);
			process_corr(specTarget, specPred, pred);
			process_eval(specTarget, specPred, pred);
		}
	}

	if( nnClone != 0 )
		delete nnClone;
	return;
}

void HNNTester::endTicker(const string& ticker)
{
	return;
}

void HNNTester::endDay()
{
	if( nTickers_ > 0 )
	{
		// Performance measures.
		if( HEnv::Instance()->iMarketRep() == 0 && "minErr" == trainObj_ )
		{
			double v = sqrt(accErrorDay_.mean());
			//cout << " day error " << v << endl;
			vDayErr_.push_back(v);
		}

		if( HEnv::Instance()->iMarketRep() == 0 )
		{
			double corr_ref = corrRef_.corr();
			double corr_test = corrTest_.corr();
			printf("Corr: Ref = %7.4f Test = %7.4f\n", corr_ref, corr_test);

			vCorrRef_.push_back(corr_ref);
			vCorrTest_.push_back(corr_test);
			corrRef_.clear();
			corrTest_.clear();
		}

		if( HEnv::Instance()->iMarketRep() == 1 )
		{
			double econVal_ref = econValRef_.econVal();
			double econVal_test = econValTest_.econVal();
			printf("EconVal: Ref = %7.4f Test = %7.4f\n", econVal_ref, econVal_test);

			vEconValRef_.push_back(econVal_ref);
			vEconValTest_.push_back(econVal_test);
			vBiasRef_.push_back(econValRef_.bias());
			vBiasTest_.push_back(econValTest_.bias());
			econValRef_.clear();
			econValTest_.clear();
		}
	}

	return;
}

void HNNTester::endMarket()
{
	// Write root file.
	TFile* f = HEnv::Instance()->outfile();

	f->cd();
	f->cd(moduleName_.c_str());
	
	if( HEnv::Instance()->iMarketRep() == 0 )
	{
		double performance = sqrt(accError_.mean());
		char buf[200];
		sprintf(buf, " %s epoch error %.4f\n", moduleName_.c_str(), performance);
		cout << buf;

		// Create and write the error graphs.
		TGraph grDayErr = make_graph(vDayErr_);
		grDayErr.SetName("grDayErr");
		grDayErr.SetTitle("Daily Error");

		// Corr
		TGraph grCorrRef = make_graph(vCorrRef_);
		grCorrRef.SetName("grCorrRef");
		grCorrRef.SetTitle("Correlation");

		TGraph grCorrTest = make_graph(vCorrTest_);
		grCorrTest.SetName("grCorrTest");
		grCorrTest.SetTitle("Correlation");

		TGraph grCorrRefMA = make_graph_ma(vCorrRef_, 20);
		grCorrRefMA.SetName("grCorrRefMA");
		grCorrRefMA.SetTitle("Correlation MA");

		TGraph grCorrTestMA = make_graph_ma(vCorrTest_, 20);
		grCorrTestMA.SetName("grCorrTestMA");
		grCorrTestMA.SetTitle("Correlation MA");

		TGraph grCorrMAdiff = make_graph_diff(grCorrRefMA, grCorrTestMA);
		grCorrMAdiff.SetName("grCorrMAdiff");
		grCorrMAdiff.SetTitle("#Delta(Correlation MA)");

		grDayErr.Write();
		houtput_.Write();
		prtn_.Write();
		grCorrRef.Write();
		grCorrTest.Write();
		grCorrRefMA.Write();
		grCorrTestMA.Write();
		grCorrMAdiff.Write();
	}
	else if( HEnv::Instance()->iMarketRep() == 1 )
	{
		// EconVal
		TGraph grEconValRef = make_graph(vEconValRef_);
		grEconValRef.SetName("grEconValRef");
		grEconValRef.SetTitle("Economic Value");

		TGraph grEconValTest = make_graph(vEconValTest_);
		grEconValTest.SetName("grEconValTest");
		grEconValTest.SetTitle("Economic Value");

		TGraph grEconValRefMA = make_graph_ma(vEconValRef_, 20);
		grEconValRefMA.SetName("grEconValRefMA");
		grEconValRefMA.SetTitle("Economic Value MA");

		TGraph grEconValTestMA = make_graph_ma(vEconValTest_, 20);
		grEconValTestMA.SetName("grEconValTestMA");
		grEconValTestMA.SetTitle("Economic Value MA");

		TGraph grEconValMAdiff = make_graph_diff(grEconValRefMA, grEconValTestMA);
		grEconValMAdiff.SetName("grEconValMAdiff");
		grEconValMAdiff.SetTitle("#Delta(Economic Value MA)");

		// Bias
		TGraph grBiasRef = make_graph(vBiasRef_);
		grBiasRef.SetName("grBiasRef");
		grBiasRef.SetTitle("Bias");

		TGraph grBiasTest = make_graph(vBiasTest_);
		grBiasTest.SetName("grBiasTest");
		grBiasTest.SetTitle("Bias");

		TGraph grBiasRefMA = make_graph_ma(vBiasRef_, 20);
		grBiasRefMA.SetName("grBiasRefMA");
		grBiasRefMA.SetTitle("Bias MA");

		TGraph grBiasTestMA = make_graph_ma(vBiasTest_, 20);
		grBiasTestMA.SetName("grBiasTestMA");
		grBiasTestMA.SetTitle("Bias MA");

		TGraph grBiasMAabsrat = make_graph_absrat(grBiasRefMA, grBiasTestMA);
		grBiasMAabsrat.SetName("grBiasMAabsrat");
		grBiasMAabsrat.SetTitle("| Bias^{Test} / Bias^{Ref} |");

		grEconValRef.Write();
		grEconValTest.Write();
		grEconValRefMA.Write();
		grEconValTestMA.Write();
		grEconValMAdiff.Write();
		grBiasRef.Write();
		grBiasTest.Write();
		grBiasRefMA.Write();
		grBiasTestMA.Write();
		grBiasMAabsrat.Write();

		int NQ = 10;
		double* x = new double[NQ];
		double* y = new double[NQ];
		double* ex = new double[NQ];
		double* ey = new double[NQ];
		for( int i=0; i<NQ; ++i )
		{
			x[i] = accX_[i].mean();
			y[i] = accY_[i].mean();
			ex[i] = (accX_[i].n > 0)? accX_[i].stdev() / sqrt(accX_[i].n): 0;
			ey[i] = (accY_[i].n > 0)? accY_[i].stdev() / sqrt(accY_[i].n): 0;
		}

		delete [] x;
		delete [] y;
		delete [] ex;
		delete [] ey;

		TGraphErrors gr(NQ, x, y, ex, ey);
		gr.SetName("grQuantile");
		gr.Write();
	}

	return;
}

void HNNTester::endJob()
{
	if( nn_ != 0 )
		delete nn_;
	return;
}

void HNNTester::process_error(double error, double nnOutput, double trainTarget)
{
	if( HEnv::Instance()->iMarketRep() == 0 )
	{
		{
			boost::mutex::scoped_lock lock(acc_mutex_);
			accErrorDay_.add(error);
			accError_.add(error);
		}

		{
			boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
			houtput_.Fill(nnOutput);
			prtn_.Fill(nnOutput, trainTarget);
		}
	}
	else if( HEnv::Instance()->iMarketRep() == 1 )
	{
		map<double, int>::iterator it = mDecileNNout_.lower_bound(nnOutput);
		if( it != mDecileNNout_.end() )
		{
			int indx = it->second;
			if( indx >= 0 && indx < 10 )
			{
				boost::mutex::scoped_lock lock(acc_mutex_);
				accX_[indx].add(nnOutput);
				accY_[indx].add(trainTarget);
			}
		}
	}
	return;
}

void HNNTester::process_corr(double specTarget, double specPred, double pred)
{
	if( HEnv::Instance()->iMarketRep() == 0 )
	{
		boost::mutex::scoped_lock lock(corr_mutex_);
		corrRef_.add(specPred, specTarget);
		corrTest_.add(pred, specTarget);
	}
	return;
}

void HNNTester::process_eval(double specTarget, double specPred, double pred)
{
	if( HEnv::Instance()->iMarketRep() == 0 )
	{
		boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
		hpred_ref_.Fill(specPred);
		hpred_test_.Fill(pred);
	}
	else if( HEnv::Instance()->iMarketRep() == 1 )
	{
		{
			map<double, int>::iterator it = mPercentilePredRef_.lower_bound(specPred);
			if( it != mPercentilePredRef_.end() )
			{
				int indx = it->second;
				if( indx == 0 )
				{
					boost::mutex::scoped_lock lock(acc_mutex_);
					econValRef_.addBottom(specTarget, specPred);
				}
				else if( indx == 99 )
				{
					boost::mutex::scoped_lock lock(acc_mutex_);
					econValRef_.addTop(specTarget, pred);
				}
			}
		}
		{
			map<double, int>::iterator it = mPercentilePredTest_.lower_bound(specPred);
			if( it != mPercentilePredTest_.end() )
			{
				int indx = it->second;
				if( indx == 0 )
				{
					boost::mutex::scoped_lock lock(acc_mutex_);
					econValTest_.addBottom(specTarget, specPred);
				}
				else if( indx == 99 )
				{
					boost::mutex::scoped_lock lock(acc_mutex_);
					econValTest_.addTop(specTarget, pred);
				}
			}
		}
	}

	return;
}

TGraph HNNTester::make_graph(vector<double>& v)
{
	int N = v.size();
	vector<double> x;
	for( int i=0; i<N; ++i )
		x.push_back(i+1);
	return TGraph(N, &x[0], &v[0]);
}

TGraph HNNTester::make_graph_ma(vector<double>& v, int n)
{
	int N = v.size();
	vector<double> x;
	for( int i=0; i<N; ++i )
		x.push_back(i+1);
	vector<double> ma;
	get_ma(ma, v, n);
	return TGraph(N, &x[0], &ma[0]);
}

TGraph HNNTester::make_graph_diff(TGraph& grRef, TGraph& grTest)
{
	TGraph gr;
	int N = grRef.GetN();
	int Ntest = grTest.GetN();
	if( N == Ntest )
	{
		double *x = new double[N];
		double *y = new double[N];
		double *xRef = grRef.GetX();
		double *yRef = grRef.GetY();
		double *xTest = grTest.GetX();
		double *yTest = grTest.GetY();
		for( int i=0; i<N; ++i )
		{
			x[i] = xRef[i];
			y[i] = yTest[i] - yRef[i];
		}
		gr = TGraph(N, x, y);
		delete [] x;
		delete [] y;
		delete [] xRef;
		delete [] yRef;
		delete [] xTest;
		delete [] yTest;
	}
	else
	{
		double zero = 0;
		double *x = &zero;
		double *y = &zero;
		gr = TGraph(1, x, y);
		delete x;
		delete y;
	}
	return gr;
}

TGraph HNNTester::make_graph_absrat(TGraph& grRef, TGraph& grTest)
{
	TGraph gr;
	int N = grRef.GetN();
	int Ntest = grTest.GetN();
	if( N == Ntest )
	{
		double *x = new double[N];
		double *y = new double[N];
		double *xRef = grRef.GetX();
		double *yRef = grRef.GetY();
		double *xTest = grTest.GetX();
		double *yTest = grTest.GetY();
		for( int i=0; i<N; ++i )
		{
			x[i] = xRef[i];
			y[i] = 0;
			if( fabs(yRef[i]) > 0 )
				y[i] = fabs(yTest[i] / yRef[i]);
		}
		gr = TGraph(N, x, y);
		delete [] x;
		delete [] y;
		delete [] xRef;
		delete [] yRef;
		delete [] xTest;
		delete [] yTest;
	}
	else
	{
		double zero = 0;
		double *x = &zero;
		double *y = &zero;
		gr = TGraph(1, x, y);
		delete x;
		delete y;
	}
	return gr;
}

void HNNTester::fillQuantileTable(map<double, int>& m, TH1F& hist, int nbins)
{
	//int nbins = 10;
	int nq = nbins + 1;
	double* xq = new double[nq];
	double* yq = new double[nq];
	for( int i=0; i<nq; ++i )
		xq[i] = double(i) / nbins;

	hist.GetQuantiles(nq, yq, xq);

	for( int i=1; i<nq; ++i )
		m[yq[i]] = i-1;

	delete [] xq;
	delete [] yq;
	return;
}
