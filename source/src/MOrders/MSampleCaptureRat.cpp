#include <MOrders/MSampleCaptureRat.h>
#include "optionlibs/TickData.h"
#include <MFramework/MEvent.h>
#include <MFramework/MEnv.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GEX.h>
#include <jl_lib/jlutil.h>
#include <gtlib/util.h>
#include <jl_lib/mto.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <numeric>
#include <string>
using namespace std;
using namespace gtlib;

MSampleCaptureRat::MSampleCaptureRat(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName, true),
	debug_(false),
	verbose_(0),
	ntrade_(0),
	execType_({'L'}),
	schedType_({1, 2, 4}),
	vSampleFreq_({1000, 5000, 15000, 60000, 300000}),
	tol_(1000),
	minCondVar_(-max_double_),
	maxCondVar_(max_double_)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("execType") )
		execType_ = getConfigValuesChar(conf, "execType");
	if( conf.count("schedType") )
		schedType_ = getConfigValuesInt(conf, "schedType");
	if( conf.count("condVar") && conf.count("minCondVar") && conf.count("maxCondVar") )
	{
		condVar_ = conf.find("condVar")->second;
		minCondVar_ = atof(conf.find("minCondVar")->second.c_str());
		maxCondVar_ = atof(conf.find("maxCondVar")->second.c_str());
	}
	vNSample_ = vector<int>(vSampleFreq_.size(), 0);
}

MSampleCaptureRat::~MSampleCaptureRat()
{}

void MSampleCaptureRat::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	return;
}

void MSampleCaptureRat::beginMarket()
{
	string market = MEnv::Instance()->market;
	vector<int> idates = MEnv::Instance()->idates;

	if( condVar_.empty() && !idates.empty() )
	{
		int idate1 = idates[0];
		int idate2 = idates[idates.size() - 1];
		string selMarket;

		if( mto::isInternational(market) )
			selMarket = " and exchange = '" + market.substr(1, 1) + "' ";

		char cmd[1000];
		sprintf(cmd, "select symbol, count(*) as cnt from hforderevents "
				" where idate >= %d and idate <= %d and qty > 0 %s "
				" group by symbol order by cnt ",
				idate1, idate2, selMarket.c_str());
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hfo(market, idate1), cmd, vv);
	}

	return;
}

void MSampleCaptureRat::beginDay()
{
	if( !condVar_.empty() )
		vCondTickers_ = getCondTickers();
	return;
}

vector<string> MSampleCaptureRat::getCondTickers()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;

	char cmd[1000];
	sprintf(cmd, "select %s from stockcharacteristics "
			" where %s "
			" and %s >= %f and %s < %f ",
			mto::compTicker(market).c_str(), mto::selChara(market, idate).c_str(), condVar_.c_str(), minCondVar_, condVar_.c_str(), maxCondVar_);

	vector<string> vCondTickers;
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		if( !ticker.empty() )
			vCondTickers.push_back(ticker);
	}

	sort(vCondTickers.begin(), vCondTickers.end());
	return vCondTickers;
}

void MSampleCaptureRat::beginTicker(const string& ticker)
{
	if( condVar_.empty() || binary_search(vCondTickers_.begin(), vCondTickers_.end(), ticker) )
	{
		const vector<MercatorTrade>* pvmt = static_cast<const vector<MercatorTrade>*>(MEvent::Instance()->get(ticker, "mtrades"));
		intra_day_stat2(pvmt, ticker);
	}

	return;
}

vector<int> MSampleCaptureRat::getTradeIndex(const vector<MercatorTrade>* pvmt)
{
	vector<int> tradeIndex;
	int NT = pvmt->size();
	for(int i = 0; i < NT; ++i)
	{
		bool okExec = false;
		bool okSched = false;
		char execType = (*pvmt)[i].execType;
		int schedType = (*pvmt)[i].schedType;
		if(find(begin(execType_), end(execType_), execType) != end(execType_))
			okExec = true;
		if(find(begin(schedType_), end(schedType_), schedType) != end(schedType_))
			okSched = true;
		if(okExec && okSched)
			tradeIndex.push_back(i);
	}
	return tradeIndex;
}

void MSampleCaptureRat::intra_day_stat2(const vector<MercatorTrade>* pvmt, const string& ticker)
{
	int idate = MEnv::Instance()->idate;
	vector<int> tradeIndex = getTradeIndex(pvmt);
	ntrade_ += tradeIndex.size();
	for(int iTrd : tradeIndex)
	{
		const MercatorTrade& mt = (*pvmt)[iTrd];
		//int sec = mt.msecs / 1000;
		for(int iFrq = 0; iFrq < vSampleFreq_.size(); ++iFrq)
		{
			int freq = vSampleFreq_[iFrq];
			//int res = mt.msecs % freq;
			//if(res <= 1 || res == freq - 1)
			if(mt.msecs % freq > freq - tol_)
				++vNSample_[iFrq];
		}
	}
}

void MSampleCaptureRat::endTicker(const string& ticker)
{
}

void MSampleCaptureRat::endDay()
{
}

void MSampleCaptureRat::endMarket()
{
	string market = MEnv::Instance()->market;
	printf("%s %10d\n", moduleName_.c_str(), ntrade_);
	for(int iFrq = 0; iFrq < vSampleFreq_.size(); ++iFrq)
	{
		printf("%7d %10d\n", vSampleFreq_[iFrq], vNSample_[iFrq]);
	}
}

void MSampleCaptureRat::endJob()
{
}

