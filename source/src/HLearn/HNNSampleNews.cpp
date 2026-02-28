#include <HLearn/HNNSampleNews.h>
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

HNNSampleNews::HNNSampleNews(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
nInput_(0),
minMsecs_(0),
maxMsecs_(24*60*60*1000),
targetTime_(41),
sampleInterval_(60),
//adjust_market_(true),
hedge_inputs_(false),
hedge_target_(true),
log_share_(true),
target_residual_(false),
write_sample_(true),
require_news_(true),
lotSize_(1),
newsAdjust_(1.0),
input_bidSize_(false),
input_askSize_(false),
input_spread_(false),
input_quoteImb_(false),
input_volume_(false),
input_pred1m_(false),
input_pred40m_(false),
input_pred41m_(false)
{
	if( conf.count("targetTime") )
		targetTime_ = atoi( conf.find("targetTime")->second.c_str() );
	if( conf.count("sampleInterval") )
		sampleInterval_ = atoi( conf.find("sampleInterval")->second.c_str() );
	//if( conf.count("adjustMarket") )
	//	adjust_market_ = conf.find("adjustMarket")->second == "true";
	if( conf.count("hedgeInputs") )
		hedge_inputs_ = conf.find("hedgeInputs")->second == "true";
	if( conf.count("hedgeTarget") )
		hedge_target_ = conf.find("hedgeTarget")->second == "true";
	if( conf.count("logShare") )
		log_share_ = conf.find("logShare")->second == "true";
	if( conf.count("targetResidual") )
		target_residual_ = conf.find("targetResidual")->second == "true";
	if( conf.count("writeSample") )
		write_sample_ = conf.find("writeSample")->second == "true";
	if( conf.count("requireNews") )
		require_news_ = conf.find("requireNews")->second == "true";
	if( conf.count("sampleDir") )
		sample_dir_ = conf.find("sampleDir")->second;
	if( conf.count("minMsecs") )
		minMsecs_ = atoi( conf.find("minMsecs")->second.c_str() );
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi( conf.find("maxMsecs")->second.c_str() );
	if( conf.count("newsAdjust") )
		newsAdjust_ = atof( conf.find("newsAdjust")->second.c_str() );

	// Past Returns.
	if( conf.count("pastRtnTime") )
	{
		pair<mmit, mmit> values = conf.equal_range("pastRtnTime");
		for( mmit mi = values.first; mi != values.second; ++mi )
		{
			int seconds = atoi(mi->second.c_str());
			pastRtnTime_.insert(seconds);
		}
	}
	nPastRtn_ = pastRtnTime_.size();
	for( set<int>::iterator it = pastRtnTime_.begin(); it != pastRtnTime_.end(); ++it )
	{
		char iname[100];
		sprintf( iname, "rtn_%ds", *it );
		inputNames_.push_back(iname);
	}

	// Past News Index.
	if( conf.count("pastNewsTime") )
	{
		pair<mmit, mmit> values = conf.equal_range("pastNewsTime");
		for( mmit mi = values.first; mi != values.second; ++mi )
		{
			int seconds = atoi(mi->second.c_str());
			pastNewsTime_.insert(seconds);
		}
	}
	nPastNews_ = pastNewsTime_.size();
	for( set<int>::iterator it = pastNewsTime_.begin(); it != pastNewsTime_.end(); ++it )
	{
		char iname[100];
		sprintf( iname, "news_%ds", *it );
		inputNames_.push_back(iname);
	}

	if( conf.count("input") )
	{
		pair<mmit, mmit> values = conf.equal_range("input");
		for( mmit mi = values.first; mi != values.second; ++mi )
		{
			string name = mi->second;
			if( "bidSize" == name )
			{
				input_bidSize_ = true;
				inputNames_.push_back("bidSize");
			}
			else if( "askSize" == name )
			{
				input_askSize_ = true;
				inputNames_.push_back("askSize");
			}
			else if( "spread" == name )
			{
				input_spread_ = true;
				inputNames_.push_back("spread");
			}
			else if( "quoteImb" == name )
			{
				input_quoteImb_ = true;
				inputNames_.push_back("quoteImb");
			}
			else if( "volume" == name )
			{
				input_volume_ = true;
				inputNames_.push_back("volume");
			}
			else if( "pred1m" == name )
			{
				input_pred1m_ = true;
				inputNames_.push_back("pred1m");
			}
			else if( "pred40m" == name )
			{
				input_pred40m_ = true;
				inputNames_.push_back("pred40m");
			}
			else if( "pred41m" == name )
			{
				input_pred41m_ = true;
				inputNames_.push_back("pred41m");
			}
		}
	}
	nInput_ = inputNames_.size();

	if( HEnv::Instance()->multiThreadTicker() )
	{
		cerr << "HNNSampleNews:Error multiThreadTicker is not supported." << endl;
		exit(5);
	}

	sEUmarkets_.insert("EA");
	sEUmarkets_.insert("EB");
	sEUmarkets_.insert("EI");
	sEUmarkets_.insert("EP");
	sEUmarkets_.insert("EF");
	sEUmarkets_.insert("ED");
	sEUmarkets_.insert("EM");
	sEUmarkets_.insert("EZ");
	sEUmarkets_.insert("EO");
	sEUmarkets_.insert("EX");
	sEUmarkets_.insert("EC");
	sEUmarkets_.insert("EW");
	sEUmarkets_.insert("EY");
}

HNNSampleNews::~HNNSampleNews()
{}

void HNNSampleNews::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	// Create InputInfo with number of input only.
	hnn::InputInfo nii(nInput_);
	HEvent::Instance()->add<hnn::InputInfo>("", "input_info", nii);

	TFile* f = HEnv::Instance()->outfile();
	f->cd();
	f->mkdir(moduleName_.c_str());

	// h_input_eu_
	h_input_eu_ = vector<TH1F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "h1_%s", inputNames_[i].c_str());
		char title[100];
		sprintf(title, "Input %s", inputNames_[i].c_str());

		h_input_eu_[i] = new TH1F(name, title, 200, 0, 1e-5);
		h_input_eu_[i]->SetBit(TH1::kCanRebin);
	}

	// h_corr_eu_
	h_corr_eu_ = vector<TH2F*>(nInput_);
	for( int i=0; i<nInput_; ++i )
	{
		char name[40];
		sprintf(name, "h2_%s", inputNames_[i].c_str());
		char title[100];
		sprintf(title, "Target vs. Input %s", inputNames_[i].c_str());

		h_corr_eu_[i] = new TH2F(name, title, 40, 0, 1e-5, 40, 0, 1e-5);
		h_corr_eu_[i]->SetBit(TH1::kCanRebin);
	}

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
	mr_eu_ = MultiRegress(labels_);
	mr_all_ = MultiRegress(labels_);

	labels_won_ = labels_;
	for( vector<string>::iterator it = labels_won_.begin(); it != labels_won_.end(); )
	{
		string l = *it;
		if( l.find("news") != string::npos )
			labels_won_.erase(it);
		else
			++it;
	}
	mr_eu_won_ = MultiRegress(labels_won_);
	mr_all_won_ = MultiRegress(labels_won_);

	vector<string> markets = HEnv::Instance()->markets();
	int nMkts = markets.size();
	h_r2_with_news_ = TH1F("h_r2_with_news", "R^2 with news inputs", nMkts + 2, 0, nMkts + 2);
	h_r2_without_news_ = TH1F("h_r2_without_news", "R^2 without news inputs", nMkts + 2, 0, nMkts + 2);

	int bin = 1;
	for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it, ++bin )
	{
		string market = *it;
		h_r2_with_news_.GetXaxis()->SetBinLabel(bin, market.c_str());
		h_r2_without_news_.GetXaxis()->SetBinLabel(bin, market.c_str());
	}
	h_r2_with_news_.GetXaxis()->SetBinLabel(bin, "eu");
	h_r2_without_news_.GetXaxis()->SetBinLabel(bin, "eu");
	++bin;
	h_r2_with_news_.GetXaxis()->SetBinLabel(bin, "all");
	h_r2_without_news_.GetXaxis()->SetBinLabel(bin, "all");

	return;
}

void HNNSampleNews::beginMarket()
{
	string market = HEnv::Instance()->market();
	if( market == "US" )
		lotSize_ = 100;

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
	mr_won_.Reset(labels_won_);
	mr_won_ = MultiRegress(labels_won_);

	if( "CJ" == market )
		pred_dir_ = "\\\\smrc-ltc-mrct46\\f\\dump\\dumpPredsJedong\\TSX";
	else if( "EL" == market )
		pred_dir_ = "\\\\smrc-ltc-mrct46\\f\\dump\\dumpPredsJedong\\LSE";

	return;
}

void HNNSampleNews::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = mto::sessions(market, 0, 0, 0, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
	vector<string> tickers = HEnv::Instance()->tickers();

	// Read volume from stockCharacteristics table.
	if( input_volume_ )
	{
		mVol_.clear();
		string market = HEnv::Instance()->market();
		int idate = HEnv::Instance()->idate();
		int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();

		string cmd = (string)"select " + mto::ts(market) + ", medVolume "
			+ " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate_prev)
			+ " and medVolume is not null ";
		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if( !ticker.empty() )
			{
				double medVol = atof( (*it)[1].c_str() );
				mVol_.insert( make_pair(ticker, medVol) );
			}
		}
	}

	// Market Adjust
	//if( adjust_market_ )
	{
		const TickSeries<ReturnData>* ptsR = static_cast<const TickSeries<ReturnData>*>(HEvent::Instance()->get(mto::retName(market), "returns"));
		int nSecInterval = (msecClose_ - msecOpen_)/1000 + 1;
		secRetCum_ = vector<double>(nSecInterval, 0);
		vector<double> secRet = vector<double>(nSecInterval, 0);

		int ntr = ptsR->NTicks();
		const_cast<TickSeries<ReturnData>*>(ptsR)->StartRead();
		ReturnData r;
		for( int n=0; n<ntr; ++n )
		{
			const_cast<TickSeries<ReturnData>*>(ptsR)->Read(&r);
			int rIndx = (r.msecs - msecOpen_)/1000;
			if( rIndx < nSecInterval )
				secRet[rIndx] = r.ret;
		}

		secRetCum_[0] = 1.0;
		for( int n=1; n<nSecInterval; ++n )
			secRetCum_[n] = secRetCum_[n-1] * secRet[n] + secRetCum_[n-1];
	}

	if( !pred_dir_.empty() &&
		(
		target_residual_
		|| find(inputNames_.begin(), inputNames_.end(), "pred1m") != inputNames_.end()
		|| find(inputNames_.begin(), inputNames_.end(), "pred40m") != inputNames_.end()
		|| find(inputNames_.begin(), inputNames_.end(), "pred41m") != inputNames_.end()
		) )
		preds_ = MCPred(pred_dir_, market, idate);

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

	return;
}

void HNNSampleNews::beginTicker(const string& ticker)
{
	string market = HEnv::Instance()->market();

	// Access tick data.
	const vector<double>* msecQ1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecQ1s"));
	const vector<double>* midQ1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "midQ1s"));
	const vector<double>* msecN1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "msecN1s"));
	const vector<double>* valN1s = static_cast<const vector<double>*>(HEvent::Instance()->get(ticker, "valN1s"));

	// Make sample.
	vector<hnn::Sample> sample;
	get_sample(ticker, msecQ1s, midQ1s, msecN1s, valN1s, sample);

	if( write_sample_ )
	{
		// Save sample.
		hnn::SampleHeader sh(ticker, sample.size());
		ofs_ << sh;
		for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
			ofs_ << *it;
	}

	// correlation.
	for( vector<hnn::Sample>::iterator it = sample.begin(); it != sample.end(); ++it )
	{
		vector<float>& input = it->input;
		double target = it->target;

		vector<double> dataPt = vector<double>(input.begin(), input.end());
		dataPt.push_back(target);
		mr_.AddPoint(&dataPt[0]);
		mr_all_.AddPoint(&dataPt[0]);
		if( sEUmarkets_.count(market) )
			mr_eu_.AddPoint(&dataPt[0]);

		vector<double> dataPtwon;
		for( int i=0; i<nInput_; ++i )
		{
			if( inputNames_[i].find("news") == string::npos )
				dataPtwon.push_back(input[i]);
		}
		dataPtwon.push_back(target);
		mr_won_.AddPoint(&dataPtwon[0]);
		mr_all_won_.AddPoint(&dataPtwon[0]);
		if( sEUmarkets_.count(market) )
			mr_eu_won_.AddPoint(&dataPtwon[0]);

		{
			boost::mutex::scoped_lock lock(HEnv::Instance()->root_mutex());
			for( int i=0; i<nInput_; ++i )
			{
				h_input_[i]->Fill(input[i]);
				h_input_all_[i]->Fill(input[i]);
				h_corr_[i]->Fill(input[i], target);
				h_corr_all_[i]->Fill(input[i], target);

				if( sEUmarkets_.count(market) )
				{
					h_input_eu_[i]->Fill(input[i]);
					h_corr_eu_[i]->Fill(input[i], target);
				}
			}
		}
	}

	++ic_;

	return;
}

void HNNSampleNews::endTicker(const string& ticker)
{
	return;
}

void HNNSampleNews::endDay()
{
	if( write_sample_ )
	{
		ofs_.close();
		ofs_.clear();
	}
	return;
}

void HNNSampleNews::endMarket()
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
	print_mr(mr_won_, market, "won");

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

void HNNSampleNews::endJob()
{
	// Write.
	TFile* f = HEnv::Instance()->outfile();
	
	{
		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->mkdir("eu");
		gDirectory->cd("eu");
		for( int i=0; i<nInput_; ++i )
		{
			h_input_eu_[i]->Write();
			h_corr_eu_[i]->Write();
		}

		// Construct and write hcorr.
		TH1F hcorr("hcorr_eu", "Correlations", nInput_, 0, nInput_);
		for( int i=0; i<nInput_; ++i )
		{
			hcorr.SetBinContent(i+1, h_corr_eu_[i]->GetCorrelationFactor());
			hcorr.GetXaxis()->SetBinLabel(i+1, inputNames_[i].c_str());
		}
		hcorr.Write();

		// Print.
		for( int i=0; i<nInput_; ++i )
		{
			string name = h_corr_eu_[i]->GetName();
			double corr = h_corr_eu_[i]->GetCorrelationFactor();
			printf("%15s %20s %10.4f\n", moduleName_.c_str(), name.c_str(), corr);
		}

		print_mr(mr_eu_, "eu");
		print_mr(mr_eu_won_, "eu", "won");
	}
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
		print_mr(mr_all_won_, "all", "won");
	}

	{
		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->mkdir("r2");
		gDirectory->cd("r2");
		h_r2_with_news_.Write();
		h_r2_without_news_.Write();
	}

	// Delete.
	for( int i=0; i<nInput_; ++i )
	{
		TH1F* h1 = h_input_all_[i];
		delete h1;
		TH2F* h2 = h_corr_all_[i];
		delete h2;
		TH1F* h1eu = h_input_eu_[i];
		delete h1eu;
		TH2F* h2eu = h_corr_eu_[i];
		delete h2eu;
	}
	return;
}

void HNNSampleNews::get_quotes(const TickSeries<QuoteInfo>* ptsQ, vector<QuoteInfo>& vq)
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

void HNNSampleNews::get_sample(string ticker, const vector<double>* msecQ1s, const vector<double>* midQ1s,
		const vector<double>* msecN1s, const vector<double>* valN1s,
		vector<hnn::Sample>& sample)
{
	int N = msecQ1s->size();
	if( N > 0 )
	{
		for( int i=0; i<N; i += sampleInterval_ )
		{
			int msecs = (*msecQ1s)[i];
			if( msecs > minMsecs_ && msecs < maxMsecs_ )
			{
				if( ((targetTime_ != 1 && targetTime_ != 41 && targetTime_ != 40) && i + targetTime_ + 1 <= N - 1)
					|| ((targetTime_ == 41 || targetTime_ == 40) && i + 1 + 60 + 600 + 3600 <= N - 1)
					|| (targetTime_ == 1 && i + 1 + 60 <= N - 1)
					)
				{
					// Find past returns.
					vector<float> vPastRet;
					int cumPastRtnTime = 0;
					for( set<int>::iterator it = pastRtnTime_.begin(); it != pastRtnTime_.end(); ++it )
					{
						int length = *it;
						cumPastRtnTime += length;
						double rtn = 0;

						int iFromTemp = i - cumPastRtnTime;
						int iTo = iFromTemp + length;
						if( iTo >= 0 )
						{
							int iFrom = max(0, iFromTemp);
							if( (*midQ1s)[iFrom] < 1e-100 )
							{
								cerr << "ERROR: price is zero.\n";
								exit(4);
							}
							if( hedge_inputs_ )
							{
								double p0 = (*midQ1s)[iFrom] / secRetCum_[iFrom];
								double p1 = (*midQ1s)[iTo] / secRetCum_[iTo];
								rtn = (p1 - p0) / p0;
							}
							else
								rtn = ((*midQ1s)[iTo] - (*midQ1s)[iFrom]) / (*midQ1s)[iFrom];
						}
						vPastRet.push_back(rtn);
					}

					// Find past news.
					bool news_present = false;
					vector<float> vPastNews;
					int cumPastNewsTime = 0;
					int newsSize = valN1s->size();
					for( set<int>::iterator it = pastNewsTime_.begin(); it != pastNewsTime_.end(); ++it )
					{
						int length = *it;
						cumPastNewsTime += length;
						double rtn = 0;

						int iFromTemp = i - cumPastNewsTime;
						int iTo = iFromTemp + length;
						if( iTo >= 0 )
						{
							int iFrom = max(0, iFromTemp);
							for( int i = iFrom + 1; i <= iTo; ++i )
							{
								if( i < newsSize )
								{
									double val = (*valN1s)[i] * newsAdjust_;
									rtn += val;
									if( val > 1.0 )
										news_present = true;
								}
							}
						}
						vPastNews.push_back(rtn);
					}

					vector<float> input;

					// Past returns input.
					for( vector<float>::iterator itp = vPastRet.begin(); itp != vPastRet.end(); ++itp )
						input.push_back(*itp);

					// Past News input.
					for( vector<float>::iterator itp = vPastNews.begin(); itp != vPastNews.end(); ++itp )
						input.push_back(*itp);

					// Other inputs.
					bool predFail = false;
					if( input_pred1m_ )
					{
						double pred1m = preds_.get_pred1m(ticker, msecs);
						if( fabs(pred1m) < 1e3 )
							input.push_back(pred1m);
						else
						{
							input.push_back(0);
							predFail = true;
						}
					}
					if( input_pred40m_ )
					{
						double pred40m = preds_.get_pred40m(ticker, msecs);
						if( fabs(pred40m) < 1e3 )
							input.push_back(pred40m);
						else
						{
							input.push_back(0);
							predFail = true;
						}
					}
					if( input_pred41m_ )
					{
						double pred41m = preds_.get_pred1m(ticker, msecs) + preds_.get_pred40m(ticker, msecs);
						if( fabs(pred41m) < 1e3 )
							input.push_back(pred41m);
						else
						{
							input.push_back(0);
							predFail = true;
						}
					}

					double target_adjust = 0;
					if( target_residual_ )
					{
						double target_adjust_bps = 0;
						if( targetTime_ == 41 )
							target_adjust_bps = preds_.get_pred1m(ticker, msecs) + preds_.get_pred40m(ticker, msecs);
						else if( targetTime_ == 1 )
							target_adjust_bps = preds_.get_pred1m(ticker, msecs);
						else if( targetTime_ == 40 )
							target_adjust_bps = preds_.get_pred40m(ticker, msecs);

						if( fabs(target_adjust_bps) < 1e3 )
							target_adjust = target_adjust_bps / 10000.0;
						else
							predFail = true;
					}

					if( !require_news_ || (require_news_ && news_present) )
					{
						if( !predFail || (!target_residual_ && !input_pred1m_ && !input_pred40m_ && !input_pred41m_) )
						{
							int indx_1s = i + 1;
							int indx_1m = i + 1 + 60;
							int indx_11m = i + 1 + 60 + 600;
							int indx_71m = i + 1 + 60 + 600 + 3600;
							if( targetTime_ == 41 )
							{
								double prc_1s = (*midQ1s)[indx_1s];
								double prc_1m = (*midQ1s)[indx_1m];
								double prc_11m = (*midQ1s)[indx_11m];
								double prc_71m = (*midQ1s)[indx_71m];
								if( hedge_target_ )
								{
									prc_1s /= secRetCum_[indx_1s];
									prc_1m /= secRetCum_[indx_1m];
									prc_11m /= secRetCum_[indx_11m];
									prc_71m /= secRetCum_[indx_71m];
								}
								if( prc_1s > 0 && prc_1m > 0 && prc_11m > 0 && prc_71m > 0 )
								{
									double ret_1m = (prc_1m - prc_1s) / prc_1s;
									double ret_10m = (prc_11m - prc_1m) / prc_1m;
									double ret_60m = (prc_71m - prc_11m) / prc_11m;
									double ret_40m = ret_10m + 0.5 * ret_60m;
									double ret_41m = ret_1m + ret_40m;
									float target = ret_41m - target_adjust;
									sample.push_back(hnn::Sample(input, target));
								}
							}
							else if( targetTime_ == 1 )
							{
								double prc_1s = (*midQ1s)[indx_1s];
								double prc_1m = (*midQ1s)[indx_1m];
								if( hedge_target_ )
								{
									prc_1s /= secRetCum_[indx_1s];
									prc_1m /= secRetCum_[indx_1m];
								}
								if( prc_1s > 0 && prc_1m > 0 )
								{
									double ret_1m = (prc_1m - prc_1s) / prc_1s;
									float target = ret_1m - target_adjust;
									sample.push_back(hnn::Sample(input, target));
								}
							}
							else if( targetTime_ == 40 )
							{
								double prc_1m = (*midQ1s)[indx_1m];
								double prc_11m = (*midQ1s)[indx_11m];
								double prc_71m = (*midQ1s)[indx_71m];
								if( hedge_target_ )
								{
									prc_1m /= secRetCum_[indx_1m];
									prc_11m /= secRetCum_[indx_11m];
									prc_71m /= secRetCum_[indx_71m];
								}
								if( prc_1m > 0 && prc_11m > 0 && prc_71m > 0 )
								{
									double ret_10m = (prc_11m - prc_1m) / prc_1m;
									double ret_60m = (prc_71m - prc_11m) / prc_11m;
									double ret_40m = ret_10m + 0.5 * ret_60m;
									float target = ret_40m - target_adjust;
									sample.push_back(hnn::Sample(input, target));
								}
							}
							else if( targetTime_ > 0 )
							{
								double targetPrc = (*midQ1s)[i + targetTime_ + 1];
								double oneSecPrc = (*midQ1s)[i + 1];
								if( hedge_target_ )
								{
									targetPrc /= secRetCum_[i + targetTime_ + 1];
									oneSecPrc /= secRetCum_[i + 1];
								}
								float target = (targetPrc - oneSecPrc) / oneSecPrc - target_adjust;
								sample.push_back(hnn::Sample(input, target));
							}
						}
					}
				}
			}
		}
	}

	return;
}

void HNNSampleNews::print_mr(MultiRegress& mr, string market, string flag)
{
	cout << "R2 summary " << market << " " << flag << "\n";

	vector<string> labels = (flag == "won")? labels_won_: labels_;

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
	copy(labels.begin(), labels.end(), ostream_iterator<string>(cout, "\n\t"));
	cout << endl;

	//cout << "destination:\n";
	//copy(dest_sample_.begin(), dest_sample_.end(), ostream_iterator<int>(cout, "\t"));
	//cout << "\n\n";

	double r2 = mr.R2(ndim - 1);
	cout << "nPoints: " << mr.NPoints() << endl;
	cout << "R^2: " << r2 << endl;

	for( int i=0; i<ndim; ++i )
	{
		cout << "\ndim: " << i << " " << labels[i] << endl;
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

void HNNSampleNews::update_hist(double r2, string market, string flag)
{
	if( r2 != 0 && r2 < 1.0 && r2 > -1.0 && !(r2 > 1.0) && !(r2 < -1.0) )
	{
		TH1F* hist = 0;
		if( flag == "" )
			hist = &h_r2_with_news_;
		else if( flag == "won" )
			hist = &h_r2_without_news_;

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