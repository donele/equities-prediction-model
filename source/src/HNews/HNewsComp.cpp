#include <HNews.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/mto.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HNewsComp::HNewsComp(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
window_(10)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HNewsComp::~HNewsComp()
{}

void HNewsComp::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	assert_loopingOrder_mdt();

	TFile* f = HEnv::Instance()->outfile();
	f->mkdir(moduleName_.c_str());

	return;
}

void HNewsComp::beginMarket()
{
	string market = HEnv::Instance()->market();

	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->mkdir(market.c_str());

	char name[100];
	char title[200];

	sprintf(name, "hCorr");
	sprintf(title, "Sentiment Reuters vs. RavenPack");
	hCorr_ = TH2F(name, title, 22, -1.1, 1.1, 22, -1.1, 1.1);

	sprintf(name, "hLag");
	sprintf(title, "Msecs RavenPack - Reuters");
	hLag_ = TH1F(name, title, 100, -10*60*1000, 10*60*1000);

	sprintf(name, "hLagZ");
	sprintf(title, "Msecs RavenPack - Reuters");
	hLagZ_ = TH1F(name, title, 100, -50*1000, 50*1000);

	sprintf(name, "hNovelRP");
	sprintf(title, "Novel RavenPack news");
	hNovelRP_ = TH1F(name, title, 540, 8*60*60*1000, 17*60*60*1000);

	sprintf(name, "hNovelRT");
	sprintf(title, "Novel Reuters news");
	hNovelRT_ = TH1F(name, title, 540, 8*60*60*1000, 17*60*60*1000);

	sprintf(name, "hLag_novelRP");
	sprintf(title, "Lags to the novel RavenPack news");
	hLag_novelRP_ = TH1F(name, title, 100, -10*60*1000, 10*60*1000);
	hLag_novelRP_.SetBit(TH1::kCanRebin);

	sprintf(name, "hLag_novelRPZ");
	sprintf(title, "Lags to the novel RavenPack news");
	hLag_novelRPZ_ = TH1F(name, title, 100, -50*1000, 50*1000);

	sprintf(name, "hLag_novelRT");
	sprintf(title, "Lags to the novel Reuters news");
	hLag_novelRT_ = TH1F(name, title, 100, -10*60*1000, 10*60*1000);
	hLag_novelRT_.SetBit(TH1::kCanRebin);

	sprintf(name, "hLag_novelRTZ");
	sprintf(title, "Lags to the novel Reuters news");
	hLag_novelRTZ_ = TH1F(name, title, 100, -50*1000, 50*1000);

	return;
}

void HNewsComp::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	sessions_ = Sessions(market, idate);

	return;
}

void HNewsComp::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	vector<int> vmsecsRP;
	vector<int> vmsecsRT;
	vector<int> vsentiRP;
	vector<int> vsentiRT;

	read_news(ticker, idate, vmsecsRP, vsentiRP, vmsecsRT, vsentiRT);
	compare(market, idate, vmsecsRP, vsentiRP, vmsecsRT, vsentiRT);

	return;
}

void HNewsComp::endTicker(const string& ticker)
{
	return;
}

void HNewsComp::endDay()
{
	return;
}

void HNewsComp::endMarket()
{
	string market = HEnv::Instance()->market();
	TFile* f = HEnv::Instance()->outfile();
	f->cd(moduleName_.c_str());
	gDirectory->cd(market.c_str());

	hCorr_.Write();
	hLag_.Write();
	hLagZ_.Write();

	hNovelRP_.Write();
	hNovelRT_.Write();

	hLag_novelRP_.Write();
	hLag_novelRT_.Write();
	hLag_novelRPZ_.Write();
	hLag_novelRTZ_.Write();

	return;
}

void HNewsComp::endJob()
{
	return;
}

void HNewsComp::read_news(string ticker, int idate, vector<int>& vmsecsRP, vector<int>& vsentiRP, vector<int>& vmsecsRT, vector<int>& vsentiRT)
{
	{
		const TickSeries<QuoteInfo>* ptsN = static_cast<const TickSeries<QuoteInfo>*>(HEvent::Instance()->get(ticker, "newsRP"));
		int ntq = ptsN->NTicks();
		const_cast<TickSeries<QuoteInfo>*>(ptsN)->StartRead();
		QuoteInfo quote;
		for( int n=0; n<ntq; ++n )
		{
			const_cast<TickSeries<QuoteInfo>*>(ptsN)->Read(&quote);

			if( sessions_.isAfterOpenBeforeClose(quote.msecs) )
			{
				int rel = quote.bidEx;
				int css = quote.bidSize;
				if( rel > 75 )
				{
					int senti = (css > 50) ? 1 : ( (css < 50 ) ? -1 : 0 );
					vmsecsRP.push_back(quote.msecs);
					vsentiRP.push_back(senti);
				}
			}
		}
	}

	{
		const TickSeries<QuoteInfo>* ptsN = static_cast<const TickSeries<QuoteInfo>*>(HEvent::Instance()->get(ticker, "newsRT"));
		int ntq = ptsN->NTicks();
		const_cast<TickSeries<QuoteInfo>*>(ptsN)->StartRead();
		QuoteInfo quote;
		for( int n=0; n<ntq; ++n )
		{
			const_cast<TickSeries<QuoteInfo>*>(ptsN)->Read(&quote);
			if( sessions_.isAfterOpenBeforeClose(quote.msecs) )
			{
				double rel = quote.bid;
				int senti = quote.bidSize;
				if( rel > 0.35 )
				{
					vmsecsRT.push_back(quote.msecs);
					vsentiRT.push_back(senti);
				}
			}
		}
	}

	return;
}

void HNewsComp::compare(string market, int idate, vector<int>& vmsecsRP, vector<int>& vsentiRP, vector<int>& vmsecsRT, vector<int>& vsentiRT)
{
	set<int> smsecs;
	for( vector<int>::iterator it = vmsecsRP.begin(); it != vmsecsRP.end(); ++it )
		smsecs.insert(*it);
	for( vector<int>::iterator it = vmsecsRT.begin(); it != vmsecsRT.end(); ++it )
		smsecs.insert(*it);

	vector<int> newsTimes;
	int last_msecs = mto::msecOpen(market, idate);
	for( set<int>::iterator it = smsecs.begin(); it != smsecs.end(); ++it )
	{
		int msecs = *it;
		if( msecs - last_msecs > 60*60*1000 )
			newsTimes.push_back( msecs );

		last_msecs = msecs;
	}

	for( vector<int>::iterator it = newsTimes.begin(); it != newsTimes.end(); ++it )
	{
		int t0 = *it;
		int msecsRP = 0;
		int msecsRT = 0;
		int sentiRP = -999;
		int sentiRT = -999;
		bool foundRP = findNews(t0, msecsRP, sentiRP, vmsecsRP, vsentiRP);
		bool foundRT = findNews(t0, msecsRT, sentiRT, vmsecsRT, vsentiRT);
		if( foundRP )
		{
			if( msecsRT == 0 || msecsRP <= msecsRT )
				hNovelRP_.Fill(msecsRP);
			if( msecsRP <= msecsRT )
			{
				hLag_novelRP_.Fill(msecsRT - msecsRP);
				hLag_novelRPZ_.Fill(msecsRT - msecsRP);
			}
		}
		if( foundRT )
		{
			if( msecsRP == 0 || msecsRT <= msecsRP )
				hNovelRT_.Fill(msecsRT);
			if( msecsRT <= msecsRP )
			{
				hLag_novelRT_.Fill(msecsRP - msecsRT);
				hLag_novelRTZ_.Fill(msecsRP - msecsRT);
			}
		}
		if( foundRP && foundRT )
		{
			boost::mutex::scoped_lock lock(hist_mutex_);
			hLag_.Fill(msecsRP - msecsRT);
			hLagZ_.Fill(msecsRP - msecsRT);
			hCorr_.Fill(sentiRP, sentiRT);
		}
	}

	return;
}

bool HNewsComp::findNews(int t0, int& msecs, int& senti, vector<int>& vmsecs, vector<int>& vsenti)
{
	int indx = 0;
	for( vector<int>::iterator it = vmsecs.begin(); it != vmsecs.end(); ++it, ++indx )
	{
		int this_msecs = *it;
		if( this_msecs < t0 )
			continue;
		else if( this_msecs > t0 + window_*60*1000 )
			break;
		else
		{
			msecs = this_msecs;
			senti = vsenti[indx];
			break;
		}
	}
	if( msecs > 0 && senti > -999 )
		return true;
	return false;
}
