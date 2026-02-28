#include <HLib/HInitModel.h>
#include <HLib/HEnv.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
#include <map>
using namespace std;

HInitModel::HInitModel(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
verbose_(0),
multiThreadModule_(false),
multiThreadTicker_(false),
nMaxThreadTicker_(4),
baseDir_(xpf("\\\\smrc-ltc-mrct43\\f\\hffit"))
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );

	// Multithreading
	if( conf.count("multiThreadModule") )
		multiThreadModule_ = conf.find("multiThreadModule")->second == "true";
	if( conf.count("multiThreadTicker") )
		multiThreadTicker_ = conf.find("multiThreadTicker")->second == "true";
	if( conf.count("nMaxThreadTicker") )
		nMaxThreadTicker_ = atoi(conf.find("nMaxThreadTicker")->second.c_str());
	HEnv::Instance()->multiThreadModule(multiThreadModule_);
	HEnv::Instance()->multiThreadTicker(multiThreadTicker_);
	HEnv::Instance()->nMaxThreadTicker(nMaxThreadTicker_);

	// Model.
	if( conf.count("model") )
		model_ = conf.find("model")->second;
	HEnv::Instance()->model(model_);

	// Base dir.
	if( conf.count("hffitBaseDir") )
		baseDir_ = conf.find("hffitBaseDir")->second;

	// Looping order.
	HEnv::Instance()->loopingOrder( "dmt" );

	// Dates.
	if( conf.count("udate") )
		udate_ = atoi( conf.find("udate")->second.c_str() );
	if( conf.count("ndates") )
		ndates_ = atoi( conf.find("ndates")->second.c_str() );

	// Outfile
	if( conf.count("outfile") )
		HEnv::Instance()->outfile( conf.find("outfile")->second );

	// Linear Model.
	hff::LinearModel linMod(model_);
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
					linMod.addHorizon(horiz, lag);
				}
			}
		}
	}
	HEnv::Instance()->linearModel(linMod);

	// NonLinear Model.
	hff::NonLinearModel nonLinMod(model_, HEnv::Instance()->linearModel());
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
				nonLinMod.addHorizon(horiz, lag);
			}
		}
	}
	HEnv::Instance()->nonLinearModel(nonLinMod);

	// Filters.
	hff::IndexFilters filters(model_, linMod.clip_index, linMod.clip_fut_index);
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
					filters.addHorizon(horiz, lag);
				}
			}
		}
	}
	HEnv::Instance()->indexFilters(filters);
}

HInitModel::~HInitModel()
{}

void HInitModel::beginJob()
{
	HEnv::Instance()->baseDir(baseDir_);

	//// Filters.
	//HEnv::Instance()->indexFilters(hff::indexFilters(model_));

	//// Linear Models.
	//HEnv::Instance()->linearModel(hff::linearModel(model_));

	//// NonLinear Models.
	//HEnv::Instance()->nonLinearModel(hff::nonLinearModel(model_, HEnv::Instance()->linearModel()));

	// Markets.
	vector<string> markets;
	markets = hff::markets(model_);
	HEnv::Instance()->markets(markets);

	// idates.
	if( !markets.empty() )
	{
		string selMarket = "";
		for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
		{
			if( selMarket.empty() )
				selMarket += (string)"( ";
			else
				selMarket += (string)" or ";

			string market = *it;
			string code = mto::code(market);

			char buf[100];
			sprintf( buf, "market = '%s'", code.c_str() );
			selMarket += buf;
		}
		selMarket += " ) ";

		char cmd[1000];
		sprintf(cmd, " select idate from tickdataok \
					 where %s \
					 and idate < %d \
					 group by idate \
					 having sum(dataOK) >= %d \
					 and count(*) = %d \
					 order by idate desc ",
					 selMarket.c_str(),
					 udate_,
					 hff::minDataOK(model_),
					 markets.size() );
		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(markets[0]))->ReadTable(cmd, &vv);

		int cnt = 0;
		vector<int> idates;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it, ++cnt )
		{
			int idate = atoi( (*it)[0].c_str() );
			if( cnt < ndates_ )
				idates.push_back(idate);
			else
				break;
		}
		if( idates.size() < ndates_ )
		{
			cerr << "HInitModel::beginJob() Not enough dates.\n";
			exit(4);
		}
		else
		{
			sort(idates.begin(), idates.end());
			HEnv::Instance()->idates(idates);
			HEnv::Instance()->nDates(idates.size());
		}
	}

	// tickers.
	const vector<int>& idates = HEnv::Instance()->idates();
	int nDates = HEnv::Instance()->nDates();
	int idateFrom = idates[0];
	int idateTo = idates[nDates - 1];
	for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
	{
		string market = *it;
		vector<string> tickers;

		char cmd[1000];
		sprintf( cmd, "select distinct %s from stockcharacteristics "
			" where market = '%s' and idate >= %d and idate <= %d and uniqueID is not null ",
			mto::compTicker(market).c_str(),
			mto::code(market).c_str(),
			idateFrom,
			idateTo );
		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);

		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if( !ticker.empty() )
				tickers.push_back(ticker);
		}

		sort(tickers.begin(), tickers.end());
		marketTickers_[market] = tickers;
	}

	return;
}

void HInitModel::beginMarket()
{
	string market = HEnv::Instance()->market();
	vector<string> tickers;
	tickers = comp_ticker( marketTickers_[market], market );
	sort(tickers.begin(), tickers.end());
	HEnv::Instance()->tickers(tickers);

	if( verbose_ > 1 )
	{
		int nT = tickers.size();
		printf("HInitModel::beginMarket() %s, %d tickers:", market.c_str(), nT);
		int n = (nT > 3)? 3: nT;
		for( int i = 0; i < n; ++i )
			printf(" %s", tickers[i].c_str());
		if( nT > 3 )
			printf(", ...");
		cout << endl;
	}
}

void HInitModel::beginDay()
{
	int idate = HEnv::Instance()->idate();
	if( verbose_ > 0 )
	{
		cout << "\nHInitModel::beginDay() " << idate << " ";
		PrintMemoryInfoSimple();
	}
	cout << flush;
}

void HInitModel::beginTicker(const string& ticker)
{
}

void HInitModel::endTicker(const string& ticker)
{
}

void HInitModel::endDay()
{
}

void HInitModel::endMarket()
{
}

void HInitModel::endJob()
{
}
