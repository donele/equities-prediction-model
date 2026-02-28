#include <MFramework/MInitFut.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
#include <map>
using namespace std;

MInitFut::MInitFut(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
verbose_(0),
elapsed_prev_(0),
mem_prev_(0),
market_("US"),
udate_(0),
nfitdates_(0),
noosdates_(0),
baseDir_(xpf("\\\\smrc-nas10\\l\\hffit"))
{
	timer_.restart();

	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );

	// Multithreading
	bool multiThreadModule;
	bool multiThreadTicker;
	int nMaxThreadTicker;
	if( conf.count("multiThreadModule") )
		multiThreadModule = conf.find("multiThreadModule")->second == "true";
	if( conf.count("multiThreadTicker") )
		multiThreadTicker = conf.find("multiThreadTicker")->second == "true";
	if( conf.count("nMaxThreadTicker") )
		nMaxThreadTicker = atoi(conf.find("nMaxThreadTicker")->second.c_str());
	MEnv::Instance()->multiThreadModule = multiThreadModule;
	MEnv::Instance()->multiThreadTicker = multiThreadTicker;
	MEnv::Instance()->nMaxThreadTicker = nMaxThreadTicker;

	// Base dir.
	if( conf.count("baseDir") )
		baseDir_ = conf.find("baseDir")->second;

	// Looping order.
	MEnv::Instance()->loopingOrder = "dmt";

	// Dates.
	if( conf.count("udate") )
		udate_ = atoi( conf.find("udate")->second.c_str() );
	MEnv::Instance()->udate = udate_;

	if( conf.count("dateFrom") && conf.count("dateTo") )
	{
		d1_ = atoi( conf.find("dateFrom")->second.c_str() );
		d2_ = atoi( conf.find("dateTo")->second.c_str() );
	}
	else
	{
		if( conf.count("nfitdates") )
			nfitdates_ = atoi( conf.find("nfitdates")->second.c_str() );

		if( conf.count("noosdates") )
			noosdates_ = atoi( conf.find("noosdates")->second.c_str() );
	}

	// Markets.
	vector<string> markets;
	markets.push_back("US");
	MEnv::Instance()->markets = markets;

	// projName.
	if( conf.count("projName") )
		projName_ = conf.find("projName")->second;

	// Products.
	if( conf.count("product") )
	{
		pair<mmit, mmit> products = conf.equal_range("product");
		for( mmit mi = products.first; mi != products.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 1 )
				products_.push_back(vs[0]);
		}
	}

	// Futures signal parameters.
	FutSigPar fsp(projName_);
	if( conf.count("horizShort") )
	{
		pair<mmit, mmit> horizs = conf.equal_range("horizShort");
		for( mmit mi = horizs.first; mi != horizs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			int vs_size = vs.size();
			int horiz = 0;
			int lag = 0;
			if( vs_size >= 1 )
			{
				horiz = atoi(vs[0].c_str());
				if( horiz != 60 )
				{
					if( vs_size >= 2 )
						lag = atoi(vs[1].c_str());
					fsp.addOmHorizon(horiz, lag);
				}
			}
		}
	}
	if( conf.count("horizLong") )
	{
		pair<mmit, mmit> horizs = conf.equal_range("horizLong");
		for( mmit mi = horizs.first; mi != horizs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			int vs_size = vs.size();
			int horiz = 0;
			int lag = 0;
			if( vs_size >= 1 )
			{
				horiz = atoi(vs[0].c_str());
				if( vs_size >= 2 )
					lag = atoi(vs[1].c_str());
				fsp.addTmHorizon(horiz, lag);
			}
		}
	}
	MEnv::Instance()->futSigPar = fsp;
}

MInitFut::~MInitFut()
{
	int elapsed = timer_.elapsed();

	int hh = int(elapsed / 3600);
	elapsed -= hh * 3600;
	int mm = int(elapsed / 60);
	elapsed -= mm * 60;

	char buf[200];
	sprintf(buf, "%s %3d:%02d:%02d\n", "Elapsed:", hh, mm, elapsed);
	cout << buf;
	cout << flush;
}

void MInitFut::beginJob()
{
	MEnv::Instance()->baseDir = baseDir_;

	set_idate_list();

	return;
}

void MInitFut::beginDay()
{
	int idate = MEnv::Instance()->idate;
	MEnv::Instance()->linearModel.set_idate(idate);
	char buf[200];
	sprintf(buf, "\nMInitFut::beginDay() %d; ", idate);
	cout << buf;
	PrintMemoryInfoSimple();
	cout << flush;

	set_ticker_list(idate);
}

void MInitFut::endDay()
{
	int idate = MEnv::Instance()->idate;
	if( verbose_ > 0 )
	{
		// Daily elapsed time.
		int elapsed = timer_.elapsed();
		int elapsed_day = elapsed - elapsed_prev_;
		elapsed_prev_ = elapsed;

		int hh = int(elapsed_day / 3600);
		elapsed_day -= hh * 3600;
		int mm = int(elapsed_day / 60);
		elapsed_day -= mm * 60;

		// Daily emory usage.
		//int mem_now = GetMemCurrent( get_pid() );
		int mem_now = 0.;
		int mem_change = mem_now - mem_prev_;
		mem_prev_ = mem_now;

		char buf[200];
		sprintf(buf, "MInitFut::endDay() %d; Elapsed %2d:%02d:%02d; Mem Chg %d K\n", idate, hh, mm, elapsed_day, mem_change);
		cout << buf;
		cout << flush;
	}
}

void MInitFut::endJob()
{
}

void MInitFut::set_idate_list()
{
	vector<int> idates;

	if( nfitdates_ > 0 || noosdates_ > 0 )
	{
		if( udate_ > 20000000 && udate_ < 21000000 )
		{
			{
				QuoteTime today(udate_, 040000, mto::tz(market_));
				int idate = (int)GEX::Instance()->get(market_)->PrevOpen(today).Date();
				for( int cnt = 0; cnt < nfitdates_; ++cnt, idate = (int)GEX::Instance()->get(market_)->PrevOpen( QuoteTime(idate, 040000, mto::tz(market_)) ).Date() )
					idates.push_back(idate);
			}
			{
				QuoteTime today(udate_, 040000, mto::tz(market_));
				int idate = (int)GEX::Instance()->get(market_)->NextOpen(today).Date();
				for( int cnt = 0; cnt < nfitdates_; ++cnt, idate = (int)GEX::Instance()->get(market_)->NextOpen( QuoteTime(idate, 040000, mto::tz(market_)) ).Date() )
					idates.push_back(idate);
			}
		}
	}
	else if( d2_ >= d1_ )
	{
		QuoteTime today(d1_, 040000, mto::tz(market_));
		int idate = (int)GEX::Instance()->get(market_)->NextOpen(today).Date();
		for( ; idate < d2_; idate = (int)GEX::Instance()->get(market_)->NextOpen( QuoteTime(idate, 200000, mto::tz(market_)) ).Date() )
			idates.push_back(idate);
	}

	sort(idates.begin(), idates.end());
	MEnv::Instance()->idates = idates;
}

void MInitFut::set_ticker_list(int idate)
{
	// tickers.
	vector<string> tickers;
	for( vector<string>::iterator it = products_.begin(); it != products_.end(); ++it )
	{
		string ticker = "";


		tickers.push_back(ticker);
	}
	MEnv::Instance()->tickers = tickers;
}
