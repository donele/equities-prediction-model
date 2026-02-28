#include <HLearn/HNNSampleNewsFile.h>
#include <HLearnObj/Sample.h>
#include <HLearnObj/InputInfo.h>
#include "optionlibs/TickData.h"
#include <HLib.h>
#include <jl_lib.h>
#include "TFile.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

HNNSampleNewsFile::HNNSampleNewsFile(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
nInput_(0),
minMsecs_(0),
maxMsecs_(24*60*60*1000),
target_residual_(true),
write_sample_(true),
input_dir_("\\\\smrc-ltc-mrct45\\e\\hfs\\txtSigs\\tm\\tsxNews") // 20090202 - 20091026
{
	if( conf.count("targetResidual") )
		target_residual_ = conf.find("targetResidual")->second == "true";
	if( conf.count("writeSample") )
		write_sample_ = conf.find("writeSample")->second == "true";
	if( conf.count("sampleDir") )
		sample_dir_ = conf.find("sampleDir")->second;
	if( conf.count("minMsecs") )
		minMsecs_ = atoi( conf.find("minMsecs")->second.c_str() );
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi( conf.find("maxMsecs")->second.c_str() );

	// Input Names.
	if( conf.count("input") )
	{
		pair<mmit, mmit> inputs = conf.equal_range("input");
		for( mmit mi = inputs.first; mi != inputs.second; ++mi )
		{
			string name = mi->second;
			inputNames_.push_back(name);
		}
	}
	nInput_ = inputNames_.size();

	if( HEnv::Instance()->multiThreadTicker() )
	{
		cerr << "HNNSampleNewsFile:Error multiThreadTicker is not supported." << endl;
		exit(5);
	}
}

HNNSampleNewsFile::~HNNSampleNewsFile()
{}

void HNNSampleNewsFile::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	// Create InputInfo with number of input only.
	hnn::InputInfo nii(nInput_);
	HEvent::Instance()->add<hnn::InputInfo>("", "input_info", nii);

	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->mkdir(moduleName_.c_str());

	// h_input_all_
	h_input_all_ = vector<TH1F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "h1_%s", inputNames_[i].c_str());
		char title[100];
		sprintf(title, "Input %s", inputNames_[i].c_str());

		h_input_all_[i] = new TH1F(name, title, 200, 0, 1e-5);
		h_input_all_[i]->SetBit(TH1::kCanRebin);
	}

	// h_corr_all_
	h_corr_all_ = vector<TH2F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "h2_%s", inputNames_[i].c_str());
		char title[100];
		sprintf(title, "Target vs. Input %s", inputNames_[i].c_str());

		h_corr_all_[i] = new TH2F(name, title, 40, 0, 1e-5, 40, 0, 1e-5);
		h_corr_all_[i]->SetBit(TH1::kCanRebin);
	}

	// MultiRegress
	labels_ = inputNames_;
	labels_.push_back("target");
	mr_all_ = MultiRegress(labels_);

	vector<string> markets = HEnv::Instance()->markets();
	int nMkts = markets.size();
	h_r2_with_news_ = TH1F("h_r2_with_news", "R^2 with news inputs", nMkts + 1, 0, nMkts + 1);

	int bin = 1;
	for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it, ++bin )
	{
		string market = *it;
		h_r2_with_news_.GetXaxis()->SetBinLabel(bin, market.c_str());
	}
	h_r2_with_news_.GetXaxis()->SetBinLabel(bin, "all");

	return;
}

void HNNSampleNewsFile::beginMarket()
{
	string market = HEnv::Instance()->market();

	// h_input_
	h_input_ = vector<TH1F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "h1_%s_%s", market.c_str(), inputNames_[i].c_str());
		char title[100];
		sprintf(title, "Input %s %s", market.c_str(), inputNames_[i].c_str());

		h_input_[i] = new TH1F(name, title, 200, 0, 1e-5);
		h_input_[i]->SetBit(TH1::kCanRebin);
	}

	// h_corr_
	h_corr_ = vector<TH2F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "h2_%s_%s", market.c_str(), inputNames_[i].c_str());
		char title[100];
		sprintf(title, "Target vs. Input %s %s", market.c_str(), inputNames_[i].c_str());

		h_corr_[i] = new TH2F(name, title, 40, 0, 1e-5, 40, 0, 1e-5);
		h_corr_[i]->SetBit(TH1::kCanRebin);
	}

	// MultiRegress
	mr_.Reset(labels_);
	mr_ = MultiRegress(labels_);

	return;
}

void HNNSampleNewsFile::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market, 0, 0, 0, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
	vector<string> tickers;
	get_tickers(tickers, market, idate);
	HEnv::Instance()->tickers(tickers);

	if( write_sample_ )
	{
		// Open file to write the sample.
		char filename[400];
		sprintf(filename, "%s\\%s\\%d.bin", sample_dir_.c_str(), market.c_str(), idate);
		ofs_.open( filename, ios::out | ios::binary );

		// Write SampleSummary.
		hnn::SampleSummary ss(inputNames_, tickers);
		ofs_ << ss;
	}

	ic_ = InputCounter("#tickers:", 10);

	char predFile[200];
	char sigFile[200];
	sprintf(predFile, "%s\\pred%dJtm.txt", input_dir_.c_str(), idate);
	sprintf(sigFile, "%s\\sigTB%dJtm.txt", input_dir_.c_str(), idate);
	ifstream ifsPred(predFile);
	ifstream ifsSig(sigFile);
	if( ifsPred.is_open() && ifsSig.is_open() )
	{
		string linePred;
		string lineSig;
		string ticker = "";
		string lastTicker = "";
		vector<hnn::Sample> sample;
		while( getline(ifsPred, linePred) && getline(ifsSig, lineSig) )
		{
			// Input names and indexes.
			vector<string> v = split(lineSig, '\t');
			if( v[0] == "uid" )
				find_index(v);
			else
			{
				// Pred file.
				vector<string> vPred = split(linePred, '\t');
				double target = atof(vPred[4].c_str());
				double pred = atof(vPred[6].c_str());
				double resid = target - pred;

				// Sig file.
				ticker = v[1];
				if( ticker != lastTicker )
				{
					write_sample(sample, lastTicker);
					lastTicker = ticker;
				}
				add_sample(v, sample, ticker, resid);
			}
		}
		if( !sample.empty() )
			write_sample(sample, ticker);
	}

	return;
}

void HNNSampleNewsFile::beginTicker(const string& ticker)
{
	return;
}

void HNNSampleNewsFile::endTicker(const string& ticker)
{
	return;
}

void HNNSampleNewsFile::endDay()
{
	if( write_sample_ )
	{
		ofs_.close();
		ofs_.clear();
	}
	return;
}

void HNNSampleNewsFile::endMarket()
{
	string market = HEnv::Instance()->market();

	// Write.
	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->mkdir(market.c_str());
	gDirectory->cd(market.c_str());
	for( int i=0; i<nInput_; ++i )
	{
		h_input_[i]->Write();
		h_corr_[i]->Write();
	}

	// Construct and write hcorr.
	char name[100];
	sprintf(name, "hcorr_%s", market.c_str());
	TH1F hcorr(name, "Correlations", nInput_, 0, nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		hcorr.SetBinContent(i+1, h_corr_[i]->GetCorrelationFactor());
		hcorr.GetXaxis()->SetBinLabel(i+1, inputNames_[i].c_str());
	}
	hcorr.Write();

	// Print.
	for( int i=0; i<nInput_; ++i )
	{
		string name = h_corr_[i]->GetName();
		double corr = h_corr_[i]->GetCorrelationFactor();
		printf("%15s %3s %20s %10.4f\n", moduleName_.c_str(), market.c_str(), name.c_str(), corr);
	}
	print_mr(mr_, market);

	// Delete.
	for( int i=0; i<nInput_; ++i )
	{
		TH1F* h1 = h_input_[i];
		delete h1;
		TH2F* h2 = h_corr_[i];
		delete h2;
	}
	return;
}

void HNNSampleNewsFile::endJob()
{
	// Write.
	TFile* f = HEnv::Instance()->outfile();

	{
		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->mkdir("all");
		gDirectory->cd("all");
		for( int i=0; i<nInput_; ++i )
		{
			h_input_all_[i]->Write();
			h_corr_all_[i]->Write();
		}

		// Construct and write hcorr.
		TH1F hcorr("hcorr_all", "Correlations", nInput_, 0, nInput_);
		for( int i=0; i<nInput_; ++i )
		{
			hcorr.SetBinContent(i+1, h_corr_all_[i]->GetCorrelationFactor());
			hcorr.GetXaxis()->SetBinLabel(i+1, inputNames_[i].c_str());
		}
		hcorr.Write();

		// Print.
		for( int i=0; i<nInput_; ++i )
		{
			string name = h_corr_all_[i]->GetName();
			double corr = h_corr_all_[i]->GetCorrelationFactor();
			printf("%15s %20s %10.4f\n", moduleName_.c_str(), name.c_str(), corr);
		}

		print_mr(mr_all_, "all");
	}

	{
		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->mkdir("r2");
		gDirectory->cd("r2");
		h_r2_with_news_.Write();
	}

	// Delete.
	for( int i=0; i<nInput_; ++i )
	{
		TH1F* h1 = h_input_all_[i];
		delete h1;
		TH2F* h2 = h_corr_all_[i];
		delete h2;
	}
	return;
}

//void HNNSampleNewsFile::read_news_binary(string market)
//{
//	//mTickerTimeSent_.clear();
//	string newsdir = (string)"\\\\smrc-ltc-mrct43\\f\\jelee\\work\\hf\\ravenpack\\1.4\\test\\" + market;
//
//
//
//	return;
//}

void HNNSampleNewsFile::get_quotes(const TickSeries<QuoteInfo>* ptsQ, vector<QuoteInfo>& vq)
{
	int ntq = ptsQ->NTicks();
	vq.reserve(ntq);

	const_cast<TickSeries<QuoteInfo>*>(ptsQ)->StartRead();
	QuoteInfo quote;
	double last_mid = -1;
	for( int n=0; n<ntq; ++n )
	{
		const_cast<TickSeries<QuoteInfo>*>(ptsQ)->Read(&quote);
		int msecs = quote.msecs;
		bool inrange = find_if(sessions_.begin(), sessions_.end(), bind2nd(inRange(), msecs)) != sessions_.end();
		if(inrange)
		{
			double mid = (quote.ask + quote.bid) / 2.0;
			bool valid_mid = mid > 0 && (last_mid < 0 || mid/last_mid < 1.05);

			if( valid_mid )
			{
				double sprd = (quote.ask - quote.bid) / mid;
				if( sprd > 0 && sprd < 0.05 )
				{
					vq.push_back(quote);
					last_mid = mid;
				}
			}
		}
	}

	return;
}

void HNNSampleNewsFile::print_mr(MultiRegress& mr, string market, string flag)
{
	cout << "R2 summary " << market << " " << flag << "\n";

	int ndim = mr.NDim();
	vector<double> coeffs = mr.ForecastCoeffs(ndim - 1);
	vector<double> sensi = mr.Sensitivities(ndim - 1);

	cout << "ndim: " << ndim << endl;
	cout << "coeffs: ";
	copy(coeffs.begin(), coeffs.end(), ostream_iterator<double>(cout, "\t"));
	cout << endl;

	cout << "sensis: ";
	copy(sensi.begin(), sensi.end(), ostream_iterator<double>(cout, "\t"));
	cout << "\n\n";

	cout << "variables:\n";
	cout << "\t";
	copy(labels_.begin(), labels_.end(), ostream_iterator<string>(cout, "\n\t"));
	cout << endl;

	double r2 = mr.R2(ndim - 1);
	cout << "nPoints: " << mr.NPoints() << endl;
	cout << "R^2: " << r2 << endl;

	for( int i=0; i<ndim; ++i )
	{
		cout << "\ndim: " << i << " " << labels_[i] << endl;
		cout << "\tmean: " << mr.Mean(i) << endl;
		cout << "\tvariance: " << mr.Delta2(i) << endl;

		if( i < ndim - 1 )
		{
			vector<unsigned> inOut;
			for( int j=0; j<ndim; ++j )
				if( j != i )
					inOut.push_back(j);
			cout << "\tPartial_R^2: " << r2 - mr.R2(inOut) << endl;
		}
	}

	cout << "\n";
	for( int i=0; i<ndim; ++i )
	{
		cout << "corr_dim_" << i << ":";
		{
			for( int j=0; j<ndim; ++j )
			{
				char buf[100];
				sprintf( buf, "%9.3f", mr.Corr(i,j) );
				cout << buf;
			}
			cout << endl;
		}
	}
	cout << endl;

	update_hist(r2, market, flag);

	return;
}

void HNNSampleNewsFile::update_hist(double r2, string market, string flag)
{
	if( r2 != 0 && r2 < 1.0 && r2 > -1.0 && !(r2 > 1.0) && !(r2 < -1.0) )
	{
		TH1F* hist = 0;
		if( flag == "" )
			hist = &h_r2_with_news_;

		int N = hist->GetNbinsX();
		for( int b = 1; b <= N; ++b )
		{
			string label = hist->GetXaxis()->GetBinLabel(b);
			if( label == market )
			{
				hist->SetBinContent(b, r2);
			}
		}
	}

	return;
}

void HNNSampleNewsFile::find_index(vector<string>& v)
{
	indx_.clear();
	int N = v.size();
	for( vector<string>::iterator it = inputNames_.begin(); it != inputNames_.end(); ++it )
	{
		string name = *it;
		int indx = -1;
		for( int i = 0; i < N; ++i )
		{
			string field = v[i];
			if( name == field )
			{
				indx = i;
				break;
			}
		}

		if( indx < 0 )
		{
			cout << "input " << name << " is not found.\n";
			exit(5);
		}
		else
			indx_.push_back(indx);
	}
	return;
}

void HNNSampleNewsFile::write_sample(vector<hnn::Sample>& sample, string ticker)
{
	if( !ticker.empty() )
	{
		hnn::SampleHeader sh(ticker, sample.size());
		ofs_ << sh;
		for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
			ofs_ << *it;

		for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
		{
			vector<float>& input = it->input;
			double target = it->target;

			vector<double> dataPt = vector<double>(input.begin(), input.end());
			dataPt.push_back(target);
			mr_.AddPoint(&dataPt[0]);
			mr_all_.AddPoint(&dataPt[0]);

			{
				boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
				for( int i=0; i<nInput_; ++i )
				{
					h_input_[i]->Fill(input[i]);
					h_input_all_[i]->Fill(input[i]);
					h_corr_[i]->Fill(input[i], target);
					h_corr_all_[i]->Fill(input[i], target);

				}
			}
		}
		++ic_;
	}
	sample.clear();

	return;
}

void HNNSampleNewsFile::add_sample(vector<string>& v, vector<hnn::Sample>& sample, string ticker, float resid)
{
	vector<float> input;
	for( vector<int>::iterator it = indx_.begin(); it != indx_.end(); ++it )
	{
		double val = atof(v[*it].c_str());
		input.push_back(val);
	}
	sample.push_back(hnn::Sample(input, resid));
	return;
}

void HNNSampleNewsFile::get_tickers(vector<string>& tickers, string market, int idate)
{
	char predFile[200];
	sprintf(predFile, "%s\\pred%dJtm.txt", input_dir_.c_str(), idate);
	ifstream ifsPred(predFile);
	if( ifsPred.is_open() )
	{
		string lastTicker = "";
		string linePred;
		while( getline(ifsPred, linePred) )
		{
			vector<string> v = split(linePred, '\t');
			if( v[0] != "uid" )
			{
				string ticker = v[1];
				if( ticker != lastTicker )
				{
					tickers.push_back(ticker);
					lastTicker = ticker;
				}
			}
		}
	}
	return;
}
