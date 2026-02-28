#include <HOrders/HTradeAnaGainSigMult.h>
#include <HOrders/HaltSimMult.h>
#include <HOrders/HaltCondPnl.h>
#include <HOrders/HaltCondRet.h>
#include <HOrders/HaltCondRetWgt.h>
#include <gtlib_signal/QuoteSample.h>
#include <HLib/HEnv.h>
#include <HLib/HEvent.h>
#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
using namespace std;
using namespace gtlib;

HTradeAnaGainSigMult::HTradeAnaGainSigMult(const string& moduleName, const multimap<string, string>& conf)
	:HModule(moduleName, true),
	debug_(false),
	id_(0),
	verbose_(0),
	minPrice_(0.),
	minMsecs_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("id") )
		id_ = atof(conf.find("id")->second.c_str());
	if( conf.count("haltLenSec") )
		haltLenSec_ = atoi(conf.find("haltLenSec")->second.c_str());
	if( conf.count("minPrice") )
		minPrice_ = atof(conf.find("minPrice")->second.c_str());
	if( conf.count("minMsecs") )
		minMsecs_ = atof(conf.find("minMsecs")->second.c_str());

	createHaltCondition(conf);
}

void HTradeAnaGainSigMult::createHaltCondition(const multimap<string, string>& conf)
{
	hs_ = new HaltSimMult(id_, haltLenSec_, minPrice_, minMsecs_);
	if( conf.count("HaltSimPnl") )
	{
		pair<mmit, mmit> conds = conf.equal_range("HaltSimPnl");
		for( mmit mi = conds.first; mi != conds.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				int pastSec = atoi(vs[0].c_str());
				double haltBpt = atof(vs[1].c_str());
				hs_->addCond(new HaltCondPnl(haltLenSec_, pastSec, haltBpt));
			}
		}
	}
	if( conf.count("HaltSimRet") )
	{
		pair<mmit, mmit> conds = conf.equal_range("HaltSimRet");
		for( mmit mi = conds.first; mi != conds.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				int pastMsec = atoi(vs[0].c_str());
				double haltBpt = atof(vs[1].c_str());
				hs_->addCond(new HaltCondRet(haltLenSec_, pastMsec, haltBpt));
			}
		}
	}
	if( conf.count("HaltSimRetWgt") )
	{
		pair<mmit, mmit> conds = conf.equal_range("HaltSimRetWgt");
		for( mmit mi = conds.first; mi != conds.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 2 )
			{
				int pastMsec = atoi(vs[0].c_str());
				double haltRat = atof(vs[1].c_str());
				hs_->addCond(new HaltCondRetWgt(haltLenSec_, pastMsec, haltRat));
			}
		}
	}
}

HTradeAnaGainSigMult::~HTradeAnaGainSigMult()
{}

void HTradeAnaGainSigMult::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	return;
}

void HTradeAnaGainSigMult::beginMarket()
{
	return;
}

void HTradeAnaGainSigMult::beginDay()
{
	int idate = HEnv::Instance()->idate();
	hs_->beginDay(idate);
	return;
}

void HTradeAnaGainSigMult::beginTicker(const string& ticker)
{
	const vector<MercatorTrade>* pm = static_cast<const vector<MercatorTrade>*>(HEvent::Instance()->get(ticker, "morders"));
	if( pm->empty() )
		return;

	if( pm != nullptr )
	{
		const QuoteSample* pqs = static_cast<const QuoteSample*>(HEvent::Instance()->get(ticker, "qSample"));
		hs_->beginTicker(ticker, *pm, *pqs);
	}

	return;
}

void HTradeAnaGainSigMult::endTicker(const string& ticker)
{
	return;
}

void HTradeAnaGainSigMult::endDay()
{
	hs_->print();
	return;
}

void HTradeAnaGainSigMult::endMarket()
{
	return;
}

void HTradeAnaGainSigMult::endJob()
{
	hs_->print();
	delete hs_;
	return;
}
