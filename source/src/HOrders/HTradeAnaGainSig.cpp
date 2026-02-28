#include <HOrders/HTradeAnaGainSig.h>
#include <HOrders/HaltSimPnl.h>
#include <HOrders/HaltSimRet.h>
#include <HOrders/HaltSimRetWgt.h>
#include <HOrders/HaltSimCumPred.h>
#include <HOrders/HaltSimCumPredRat.h>
#include <gtlib_signal/QuoteSample.h>
#include <HLib/HEnv.h>
#include <HLib/HEvent.h>
#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
using namespace std;
using namespace gtlib;

HTradeAnaGainSig::HTradeAnaGainSig(const string& moduleName, const multimap<string, string>& conf)
	:HModule(moduleName, true),
	debug_(false),
	verbose_(0),
	minPrice_(0.),
	minMsecs_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("minPrice") )
		minPrice_ = atof(conf.find("minPrice")->second.c_str());
	if( conf.count("minMsecs") )
		minMsecs_ = atof(conf.find("minMsecs")->second.c_str());

	createHaltCondition(conf);
}

void HTradeAnaGainSig::createHaltCondition(const multimap<string, string>& conf)
{
	int iH = 0;
	if( conf.count("HaltSimPnl") )
	{
		pair<mmit, mmit> conds = conf.equal_range("HaltSimPnl");
		for( mmit mi = conds.first; mi != conds.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 3 )
			{
				int pastSec = atoi(vs[0].c_str());
				int haltSec = atoi(vs[1].c_str());
				double haltBpt = atof(vs[2].c_str());
				vhs_.push_back(new HaltSimPnl(++iH, minPrice_, minMsecs_, pastSec, haltSec, haltBpt));
			}
		}
	}
	if( conf.count("HaltSimRet") )
	{
		pair<mmit, mmit> conds = conf.equal_range("HaltSimRet");
		for( mmit mi = conds.first; mi != conds.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 3 )
			{
				int pastMsec = atoi(vs[0].c_str());
				int haltSec = atoi(vs[1].c_str());
				double haltBpt = atof(vs[2].c_str());
				vhs_.push_back(new HaltSimRet(++iH, minPrice_, minMsecs_, pastMsec, haltSec, haltBpt));
			}
		}
	}
	if( conf.count("HaltSimRetWgt") )
	{
		pair<mmit, mmit> conds = conf.equal_range("HaltSimRetWgt");
		for( mmit mi = conds.first; mi != conds.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 3 )
			{
				int pastMsec = atoi(vs[0].c_str());
				int haltSec = atoi(vs[1].c_str());
				double haltRat = atof(vs[2].c_str());
				vhs_.push_back(new HaltSimRetWgt(++iH, minPrice_, minMsecs_, pastMsec, haltSec, haltRat));
			}
		}
	}
	if( conf.count("HaltSimCumPred") )
	{
		pair<mmit, mmit> conds = conf.equal_range("HaltSimCumPred");
		for( mmit mi = conds.first; mi != conds.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 4 )
			{
				int pastSec = atoi(vs[0].c_str());
				int haltSec = atoi(vs[1].c_str());
				int ntrades = atoi(vs[2].c_str());
				double haltBpt = atof(vs[3].c_str());
				vhs_.push_back(new HaltSimCumPred(++iH, minPrice_, minMsecs_, pastSec, haltSec, ntrades, haltBpt));
			}
		}
	}
	if( conf.count("HaltSimCumPredRat") )
	{
		pair<mmit, mmit> conds = conf.equal_range("HaltSimCumPredRat");
		for( mmit mi = conds.first; mi != conds.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 4 )
			{
				int pastSec = atoi(vs[0].c_str());
				int haltSec = atoi(vs[1].c_str());
				int ntrades = atoi(vs[2].c_str());
				double haltRat = atof(vs[3].c_str());
				vhs_.push_back(new HaltSimCumPredRat(++iH, minPrice_, minMsecs_, pastSec, haltSec, ntrades, haltRat));
			}
		}
	}

	if( debug_ )
		for( auto& h : vhs_ )
			h->setDebug();
}

HTradeAnaGainSig::~HTradeAnaGainSig()
{}

void HTradeAnaGainSig::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	return;
}

void HTradeAnaGainSig::beginMarket()
{
	return;
}

void HTradeAnaGainSig::beginDay()
{
	int idate = HEnv::Instance()->idate();
	for( auto& h : vhs_ )
		h->beginDay(idate);
	return;
}

void HTradeAnaGainSig::beginTicker(const string& ticker)
{
	//const vector<MercatorTrade>* pvmt = static_cast<const vector<MercatorTrade>*>(HEvent::Instance()->get(ticker, "mtrades"));
	const vector<MercatorTrade>* pm = static_cast<const vector<MercatorTrade>*>(HEvent::Instance()->get(ticker, "morders"));
	if( pm->empty() )
		return;

	if( pm != nullptr )
	{
		const QuoteSample* pqs = static_cast<const QuoteSample*>(HEvent::Instance()->get(ticker, "qSample"));
		for( auto& h : vhs_ )
			h->beginTicker(ticker, *pm, *pqs);
	}

	return;
}

void HTradeAnaGainSig::endTicker(const string& ticker)
{
	return;
}

void HTradeAnaGainSig::endDay()
{
	for( auto& h : vhs_ )
		h->print();
	return;
}

void HTradeAnaGainSig::endMarket()
{
	return;
}

void HTradeAnaGainSig::endJob()
{
	for( auto& h : vhs_ )
	{
		h->print();
		delete h;
	}
	return;
}
