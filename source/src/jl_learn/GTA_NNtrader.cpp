#include "GTA_NNtrader.h"
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

GTA_NNtrader::GTA_NNtrader(const string& moduleName, const multimap<string, string>& conf)
:GTModule(moduleName),
iDay_(0),
nHidden_(0),
maxPos_(10000),
maxPosChg_(1000),
predMin_(0),
predMax_(1.0),
costMult_(1),
lotSize_(1),
tradeInterval_(60),
costMultForced_(false),
noTwoTradesAtSamePrice_(true),
thres_(0),
restore_(1),
randomTradeFreq_(0.2),
trainObj_("minErr"),
verbose_(0),
useBias_(0),
day_error_(0),
day_npoints_(0),
maxNTradesTickerDay_(1000000),
nn_(0),
nnFile_("")
{
	if( conf.count("maxPos") )
		maxPos_ = atoi( conf.find("maxPos")->second.c_str() );
	if( conf.count("maxPosChg") )
		maxPosChg_ = atoi( conf.find("maxPosChg")->second.c_str() );
	if( conf.count("predMin") )
		predMin_ = atof( conf.find("predMin")->second.c_str() );
	if( conf.count("predMax") )
		predMax_ = atof( conf.find("predMax")->second.c_str() );
	if( conf.count("costMult") )
	{
		costMult_ = atof( conf.find("costMult")->second.c_str() );
		costMultForced_ = true;
	}
	if( conf.count("noTwoTradesAtSamePrice") )
		noTwoTradesAtSamePrice_ = conf.find("noTwoTradesAtSamePrice")->second == "true";
	if( conf.count("thres") )
		thres_ = atof( conf.find("thres")->second.c_str() );
	if( conf.count("restore") )
		restore_ = atof( conf.find("restore")->second.c_str() );
	if( conf.count("tradeInterval") )
		tradeInterval_ = atoi( conf.find("tradeInterval")->second.c_str() );
	if( conf.count("randomTradeFreq") )
		randomTradeFreq_ = atof( conf.find("randomTradeFreq")->second.c_str() );
	if( conf.count("nHidden") )
		nHidden_ = atoi( conf.find("nHidden")->second.c_str() );
	if( conf.count("trainObj") )
		trainObj_ = conf.find("trainObj")->second.c_str();
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("useBias") )
		useBias_ = atoi( conf.find("useBias")->second.c_str() );
	if( conf.count("maxNTradesTickerDay") )
		maxNTradesTickerDay_ = atoi( conf.find("maxNTradesTickerDay")->second.c_str() );
	if( conf.count("nnFile") )
		nnFile_ = conf.find("nnFile")->second;
}

GTA_NNtrader::~GTA_NNtrader()
{}

void GTA_NNtrader::beginJob()
{
	NNinputInfo* nii = static_cast<NNinputInfo*>(GTEvent::Instance()->get("input_info"));
	if( nii != 0 )
	{
		// Create Neural Network.
		if( "maxReturn" == trainObj_ || "maxProfit" == trainObj_ )
			nn_ = new NN_bptt( nnFile_ );
		else if( "maxReturnTanh" == trainObj_ || "maxProfitTanh" == trainObj_ )
			nn_ = new NN_bptt_tanh( nnFile_ );
		else if( "random" != trainObj_ )
			nn_ = new NN_bpnn( nnFile_ );

		// costMult.
		if( !costMultForced_ && nn_ != 0 )
			costMult_ = nn_->costMult();
		GTEvent::Instance()->add<double>( (moduleName_ + "_costMult").c_str(), costMult_);

		// Random number generator.
		if( "random" == trainObj_ )
			tr_ = TRandom3(0);

		// Create ROOT directories.
		TFile* f = GTEnv::Instance()->outfile();
		f->cd();
		f->mkdir(moduleName_.c_str());
		f->cd(moduleName_.c_str());
		gDirectory->mkdir("performance");
		gDirectory->mkdir("output");
		gDirectory->mkdir("tickers");
		gDirectory->mkdir("position");
		gDirectory->mkdir("output0");

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

void GTA_NNtrader::beginMarket()
{
	string market = GTEvent::Instance()->market();
	if( market == "US" )
		lotSize_ = 100;

	// For each data point
	houtput_ = TH1F("output1",  "Model Output", 100, -1e-4, 1e-4);
	prtn_ = TProfile("prtn1", "Return vs Prediction", 100, -1e-4, 1e-4);
	houtput0_ = TH1F("output0", "Model Output", 100, -1e-4, 1e-4);
	prtn0_ = TProfile("prtn0", "Return vs Prediction", 100, -1e-4, 1e-4);

	houtput_.SetBit(TH1::kCanRebin);
	prtn_.SetBit(TH1::kCanRebin);
	houtput0_.SetBit(TH1::kCanRebin);
	prtn0_.SetBit(TH1::kCanRebin);

	// For each stock-day
	hreturn0_ = TH1F("return0", "Return0 by Tickerday", 100, -1e-4, 1e-4);
	hprofit0_ = TH1F("profit0", "Profit0 by Tickerday", 100, -1e-4, 1e-4);
	hreturn_ = TH1F("return", "Return by Tickerday", 100, -1e-4, 1e-4);
	hprofit_ = TH1F("profit", "Profit by Tickerday", 100, -1e-4, 1e-4);

	hreturn0_.SetBit(TH1::kCanRebin);
	hprofit0_.SetBit(TH1::kCanRebin);
	hreturn_.SetBit(TH1::kCanRebin);
	hprofit_.SetBit(TH1::kCanRebin);

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

	mPos_.clear();

	return;
}

void GTA_NNtrader::beginDay()
{
	// Add the last close positions to the event.
	GTEvent::Instance()->add<map<string, int> >( (moduleName_ + "_mLastClosePos").c_str(), mPos_);

	// Daily position distribution.
	int idate = GTEvent::Instance()->idate();
	char name [100];
	sprintf(name, "hTickerPosDay_%d", idate);
	char title [200];
	sprintf(title, "Tickers Positions %d", idate);
	hTickerPosDay_ = TH1F(name, title, 100, -1, 1);
	hTickerPosDay_.SetBit(TH1::kCanRebin);

	return;
}

void GTA_NNtrader::doTicker()
{
	vector<NNsample>* sample = static_cast<vector<NNsample>*>(GTEvent::Instance()->get("sample"));
	GTA_NNtrader::TickerDaySumm tds;
	string ticker = GTEvent::Instance()->ticker();
	int idate = GTEvent::Instance()->idate();

	if( verbose_ >= 1 )
		cout << "GTA_NNtrader " << ticker << "\t" << idate << endl;

	if( sample != 0 && sample->size() > 0 )
	{
		// Add sample open prices to the event.
		double sampleOpen = (sample->begin()->quote.ask + sample->begin()->quote.bid) / 2.0;
		GTEvent::Instance()->add<double>( (moduleName_ + "_sampleOpen").c_str(), sampleOpen);

		int& position = mPos_[ticker];
		double lastMid = 0;
		double lastPos = position;
		int lastTradeMsecs = 0;
		int nTradesTickerDay = 0;
		double lastTradePrice = 0;

		// loop over the sample and trade.
		vector<NNsample>::iterator ite = sample->end();
		for( vector<NNsample>::iterator it = sample->begin(); it != ite; ++it )
		{
			if( nTradesTickerDay >= maxNTradesTickerDay_ )
				break;
			vector<double>& input = it->input;
			double target = it->target;
			QuoteInfo& quote = it->quote;
			double mid = (quote.bid + quote.ask) / 2.0;
			double sprd = (quote.ask - quote.bid) / (quote.ask + quote.bid) * costMult_; // half the spread.

			double deltaPos = 0;
			double R0 = 0;
			double R = 0;
			double P0 = 0;
			double P = 0;
			double nnOutput0 = 0;
			double nnOutput = 0;

			if( "minErr" == trainObj_ )
			{
				vector<double> output = nn_->forward_trade(input, quote);
				nnOutput0 = output[0];
				double cost = (quote.ask - quote.bid) / (quote.ask + quote.bid) * costMult_;

				nnOutput = nnOutput0;
				if( position > 0 && nnOutput0 > 0 )
					nnOutput = nnOutput0 - restore_ * fabs(nnOutput0) * (position * position) / (maxPos_ * maxPos_);
				else if( position < 0 && nnOutput0 < 0 )
					nnOutput = nnOutput0 + restore_ * fabs(nnOutput0) * (position * position) / (maxPos_ * maxPos_);

				if( fabs(nnOutput) > fabs(cost + thres_)  && fabs(nnOutput0) < predMax_ && fabs(nnOutput) < predMax_ && fabs(nnOutput0) > predMin_ )
				{
					double maxPosChgAdj = maxPosChg_;
					if( nnOutput0 > 0 && nnOutput > 0 ) // buy
						deltaPos = min(maxPosChgAdj, min(quote.askSize * lotSize_, maxPos_ - lastPos));
					else if( nnOutput0 < 0 && nnOutput < 0 ) // sell
						deltaPos = max(-maxPosChgAdj, max(-quote.bidSize * lotSize_, -maxPos_ - lastPos));
				}
				else
					deltaPos = 0;

				// tradeInterval and lastTradePrice.
				if( lastTradeMsecs + tradeInterval_ >= quote.msecs )
					deltaPos = 0;
				if( noTwoTradesAtSamePrice_ )
				{
					if( deltaPos > 0 && fabs(lastTradePrice - quote.ask) < 1e-6 )
						deltaPos = 0;
					else if( deltaPos < 0 && fabs(lastTradePrice - quote.ask) < 1e-6 )
						deltaPos = 0;
				}
			}
			else if( "random" == trainObj_ )
			{
				double x = tr_.Rndm(3);
				if( x < randomTradeFreq_/2.0 )
					deltaPos = min(maxPosChg_, min(quote.askSize * lotSize_, maxPos_ - lastPos));
				else if( x > 1.0 - randomTradeFreq_/2.0 )
					deltaPos = max(-maxPosChg_, max(-quote.bidSize * lotSize_, -maxPos_ - lastPos));

				// tradeInterval and lastTradePrice.
				if( lastTradeMsecs + tradeInterval_ >= quote.msecs )
					deltaPos = 0;
				if( noTwoTradesAtSamePrice_ )
				{
					if( deltaPos > 0 && fabs(lastTradePrice - quote.ask) < 1e-6 )
						deltaPos = 0;
					else if( deltaPos < 0 && fabs(lastTradePrice - quote.ask) < 1e-6 )
						deltaPos = 0;
				}
			}
			else if( trainObj_.size() >= 3 && "max" == trainObj_.substr(0,3) )
			{
				nn_->set_position(lastPos);
				vector<double> output = nn_->forward_trade(input, quote);
				nnOutput0 = output[0];
				double temp_newPos = output[1];
				double temp_deltaPos = temp_newPos - position;

				// Adjust the position.
				if( temp_deltaPos > 0 )
					temp_newPos = min( position + temp_deltaPos, min( position + maxPosChg_, maxPos_ ) );
				else
					temp_newPos = max( position + temp_deltaPos, max( position - maxPosChg_, -maxPos_ ) );
				deltaPos = temp_newPos - position;

				// tradeInterval and lastTradePrice.
				if( lastTradeMsecs + tradeInterval_ >= quote.msecs )
					deltaPos = 0;
				if( noTwoTradesAtSamePrice_ )
				{
					if( deltaPos > 0 && fabs(lastTradePrice - quote.ask) < 1e-6 )
						deltaPos = 0;
					else if( deltaPos < 0 && fabs(lastTradePrice - quote.ask) < 1e-6 )
						deltaPos = 0;
				}

				double newPos = position + deltaPos;
				double normPos = (newPos > position)?
					(newPos - position) / (quote.askSize * lotSize_):
					(newPos - position) / (quote.bidSize * lotSize_);

				nnOutput = normPos;
			}

			// Update the pnl.
			double cost = fabs(deltaPos) * sprd * costMult_;
			if( lastMid > 1e-10 )
				R0 = lastPos * (mid - lastMid) / lastMid;
			R = R0 - cost;
			P0 = R0 * lastMid;
			P = P0 - cost * mid;

			if( fabs(deltaPos) > 0.5 ) // trade.
			{
				if( deltaPos > 0 )
					lastTradePrice = quote.ask;
				else
					lastTradePrice = quote.bid;

				position += deltaPos;
				doTrade(quote, deltaPos, tds);
				lastTradeMsecs = quote.msecs;
				++nTradesTickerDay;
			}

			// performance monitor.
			double dv = fabs(deltaPos) * mid;
			npReturn0_.add(R0);
			npProfit0_.add(P0);
			npRpt0_.add(R0, dv);
			npGpt0_.add(P0, dv);
			npReturn_.add(R);
			npProfit_.add(P);
			npRpt_.add(R, dv);
			npGpt_.add(P, dv);

			tds.sumR0 += R0;
			tds.sumP0 += P0;
			tds.sumR += R;
			tds.sumP += P;

			if( fabs(deltaPos) > 0 && verbose_ == 2 )
				cout << ticker << "\t" << tds.sumP << endl;

			double error = target - nnOutput;
			double err = error*error;
			npError_.add(err);

			houtput0_.Fill(nnOutput0);
			prtn0_.Fill(nnOutput0, target);
			houtput_.Fill(nnOutput);
			prtn_.Fill(nnOutput, target);

			lastMid = mid;
			lastPos = position;

			if( verbose_ == 3 )
			{
				char buf[100];
				sprintf(buf, "%10.5f %10d\n", nnOutput, position);
				cout << buf;
			}
		}
	}

	if( verbose_ == 1 )
		cout << ticker << "\t" << tds.sumP << endl;

	hreturn0_.Fill(tds.sumR0);
	hprofit0_.Fill(tds.sumP0);
	hreturn_.Fill(tds.sumR);
	hprofit_.Fill(tds.sumP);

	// Add to the event
	GTEvent::Instance()->add<vector<double> >((moduleName_ + "_msec").c_str(), tds.msec_series);
	GTEvent::Instance()->add<vector<double> >((moduleName_ + "_sign").c_str(), tds.sign_series);
	GTEvent::Instance()->add<vector<double> >((moduleName_ + "_prc").c_str(), tds.prc_series);
	GTEvent::Instance()->add<vector<double> >((moduleName_ + "_vol").c_str(), tds.vol_series);

	// mid series.
	vector<double> midM;
	vector<double>* msecQ = static_cast<vector<double>*>(GTEvent::Instance()->get("msecQ"));
	vector<double>* midQ = static_cast<vector<double>*>(GTEvent::Instance()->get("midQ"));
	get_mid_series(midM, tds.msec_series, msecQ, midQ);
	GTEvent::Instance()->add<vector<double> >((moduleName_ + "_mid").c_str(), midM);

	return;
}

void GTA_NNtrader::endDay()
{
	// Record the end of day positions.
	for( map<string, int>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
		hTickerPosDay_.Fill(it->second);
	TFile* f = GTEnv::Instance()->outfile();
	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("position");
	hTickerPosDay_.Write();

	// End of day report.
	double r0 = npReturn0_.endofdaysum();
	double p0 = npProfit0_.endofdaysum();
	double rpt0 = npRpt0_.endofdayrat();
	double gpt0 = npGpt0_.endofdayrat();
	double r = npReturn_.endofdaysum();
	double p = npProfit_.endofdaysum();
	double rpt = npRpt_.endofdayrat();
	double gpt = npGpt_.endofdayrat();
	printf("\n %s day r0 %5.1f p0 %5.1f rpt0 %.4f gpt0 %5.2f\n", moduleName_.c_str(), r0, p0, rpt0, gpt0);
	printf(" %s day r  %5.1f p  %5.1f rpt  %.4f gpt  %5.2f\n", moduleName_.c_str(), r, p, rpt, gpt);

	// Performance measures.
	if( "minErr" == trainObj_ )
	{
		double v = npError_.endofdayerr();
		cout << " day error " << v << endl;
		vDayErr_.push_back(v);
	}
	else
	{
		if( trainObj_.find("Return") != string::npos )
			vDayErr_.push_back(-r);
		else if( trainObj_.find("Profit") != string::npos )
			vDayErr_.push_back(-p);
	}

	return;
}

void GTA_NNtrader::endMarket()
{
	// Write root file.
	TFile* f = GTEnv::Instance()->outfile();

	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("output");

	houtput_.Write();
	prtn_.Write();

	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("output0");

	houtput0_.Write();
	prtn0_.Write();

	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("performance");

	hreturn0_.Write();
	hprofit0_.Write();
	hreturn_.Write();
	hprofit_.Write();

	for( map<string, vector<double> >::iterator it = mvMsecs_.begin(); it != mvMsecs_.end(); ++it )
	{
		string ticker = it->first;

		f->cd();
		f->cd(moduleName_.c_str());
		gDirectory->cd("tickers");
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
	
	// Create and write the error graphs.
	TGraph grDayErr = make_graph(vDayErr_);
	grDayErr.SetName("grDayErr");
	grDayErr.SetTitle("Daily Error");
	TGraph grEpochErr = make_graph(vEpochErr_);
	grEpochErr.SetName("grEpochErr");
	grEpochErr.SetTitle("Epoch Error");

	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("performance");
	grDayErr.Write();
	grEpochErr.Write();

	return;
}

void GTA_NNtrader::endJob()
{
	if( nn_ != 0 )
		delete nn_;
	return;
}

void GTA_NNtrader::doTrade(QuoteInfo& quote, double deltaPos, TickerDaySumm& tds)
{
	int sign = (deltaPos > 0)?1:-1;
	double size = 0;
	double price = 0;
	if( deltaPos > 0 )
	{
		size = ceil( deltaPos - 0.5 );
		double cost = max(0, (quote.ask - quote.bid) / 2.0 );
		price = quote.ask - cost * (1.0 - costMult_);
	}
	else if( deltaPos < 0 )
	{
		size = ceil( -deltaPos - 0.5 );
		double cost = max(0, (quote.ask - quote.bid) / 2.0 );
		price = quote.bid + cost * (1.0 - costMult_);
	}

	tds.msec_series.push_back(quote.msecs);
	tds.prc_series.push_back(price);
	tds.sign_series.push_back(sign);
	tds.vol_series.push_back(size);

	return;
}

TGraph GTA_NNtrader::make_graph(vector<double>& v)
{
	int N = v.size();
	vector<double> x;
	for( int i=0; i<N; ++i )
		x.push_back(i+1);
	return TGraph(N, &x[0], &v[0]);
}

void GTA_NNtrader::plot_tickerDetail(string ticker, string var, vector<double> vMsecs, vector<double> v, bool cumulative)
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

	TFile* f = GTEnv::Instance()->outfile();

	f->cd();
	f->cd(moduleName_.c_str());
	gDirectory->cd("tickers");
	gDirectory->cd(ticker.c_str());
	h.Write();

	return;
}

void GTA_NNtrader::get_mid_series(vector<double>& midM, vector<double>& msecM, vector<double>* msecQ, vector<double>* midQ)
{
	// Fill signT_series and midT_series.
	int NT = msecM.size();
	midM = vector<double>(NT, 0);
	if( msecQ != 0 )
	{
		int NQ = msecQ->size();
		if( msecQ != 0 && midQ != 0 && NT > 0 && NQ > 0 )
		{
			{
				int iQ = NQ - 1;
				for( int iT=NT-1; iT >= 0; --iT )
				{
					while( iQ > 0 && (*msecQ)[iQ] >= msecM[iT] )
						--iQ;

					if( (*msecQ)[iQ] < msecM[iT] )
					{
						midM[iT] = (*midQ)[iQ];

						// Rewind one step.
						if( iQ + 1 < NQ )
							++iQ;
					}
				}
			}
		}
		replace_zeros(midM);
	}

	return;
}

void GTA_NNtrader::replace_zeros(vector<double>& series)
{
	double last = 0;
	int N = series.size();
	for( int i=0; i<N; ++i )
	{
		if( series[i] < 1e-6 )
			series[i] = last;

		if( series[i] > 1e-6 )
			last = series[i];
	}

	int izero = 0;
	for( int i=0; i<N; ++i )
	{
		double prc = series[i];
		if( prc > 1e-6 )
		{
			for( int j=0; j<izero; ++j )
				series[j] = prc;
			break;
		}
		else
			++izero;
	}

	return;
}
