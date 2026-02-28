#include <MFitMod/MVolSurpAna.h>
#include <gtlib_fitting/CorrInput.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_predana/PredAnaByQuantileTradeLastBin.h>
#include <gtlib/util.h>
#include <gtlib_model/pathFtns.h>
#include <jl_lib/mto.h>
#include <gtlib_model/mFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <thread>
using namespace std;
using namespace gtlib;

MVolSurpAna::MVolSurpAna(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	debug_ana_(false),
	volDepThres_(false),
	tradedTickersOnly_(false),
	varAnaQuantile_("relVolat"),
	thres_(1.),
	nTargets_(0),
	nQuantiles_(10),
	flatFee_(-1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugAna") )
		debug_ana_ = conf.find("debugAna")->second == "true";
	if( conf.count("volDepThres") )
		volDepThres_ = conf.find("volDepThres")->second == "true";
	if( conf.count("tradedTickersOnly") )
		tradedTickersOnly_ = conf.find("tradedTickersOnly")->second == "true";
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second;
	if( conf.count("flatFee") )
		flatFee_ = atof(conf.find("flatFee")->second.c_str());
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());
	if( conf.count("nQuantiles") )
		nQuantiles_ = atoi(conf.find("nQuantiles")->second.c_str());
	if( conf.count("varAnaQuantile") )
		varAnaQuantile_ = conf.find("varAnaQuantile")->second;

	targetNames_ = getConfigValuesString(conf, "targetName");
	sigModels_ = getConfigValuesString(conf, "sigModel");
	predModels_ = getConfigValuesString(conf, "predModel");
	vWgts_ = getConfigValuesFloat(conf, "wgt");
	nTargets_ = targetNames_.size();
	if(vWgts_.empty())
		vWgts_ = vector<float>(nTargets_, 1.);
	else if(vWgts_.size() != nTargets_)
	{
		cerr << vWgts_.size() << " weights for " << nTargets_ << " targets.\n";
		exit(73);
	}
}

MVolSurpAna::~MVolSurpAna()
{}

void MVolSurpAna::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	string model = MEnv::Instance()->model;
	int udate = MEnv::Instance()->udate;
	string baseDir = MEnv::Instance()->baseDir;

	fitDesc_ = MEnv::Instance()->fitDesc;
	fitDir_ = get_fit_dir(baseDir, model, concat(targetNames_), fitDesc_);
	if( !coefModel_.empty() )
		coefFitDir_ = get_fit_dir(baseDir, coefModel_, concat(targetNames_), fitDesc_);
	else
		coefFitDir_ = fitDir_;
	mkd(fitDir_);
	fitdates_ = get_fitdates(MEnv::Instance()->idates, udate);

	if( predModels_.empty() )
	{
		for( int i = 0; i < targetNames_.size(); ++i )
			predModels_.push_back(model);
	}
	if( sigModels_.empty() )
		sigModels_ = predModels_;

	pbq_ = new PredAnaByQuantileTradeLastBin
		(baseDir, fitDir_, predModels_, sigModels_, targetNames_, vWgts_,
		udate, fitdates_, fitDesc_, flatFee_, thres_, volDepThres_, tradedTickersOnly_, debug_ana_);
	pbq_->anaQuantile(varAnaQuantile_, nQuantiles_);
}

void MVolSurpAna::beginDay()
{
	int idate = MEnv::Instance()->idate;
	set<string> t = pbq_->getTickers(idate);
	MEnv::Instance()->tickers = vector<string>(t.begin(), t.end());
}

void MVolSurpAna::beginTicker(const string& ticker)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
	vector<double> vMid1s;
	get_mid_series(vMid1s, quotes, linMod.openMsecs, linMod.closeMsecs, 0, false);
	vector<PredAnaByQuantileTradeLastBin::LastBinTrade> trades = pbq_->getTrades(MEnv::Instance()->idate, ticker);
	for(auto& trade : trades)
	{
		float price = trade.price;
		for(int secTo = (trade.msecs - linMod.openMsecs + 60*1000)/1000; secTo <= (linMod.closeMsecs-linMod.openMsecs)/1000; secTo += 60)
		{
			float priceTo = vMid1s[secTo];
			if(price > 0. && priceTo > 0.)
			{
				float ret = trade.sign * (priceTo / price - 1.);
				printf("stat: %s %d %d %.10f\n", ticker.c_str(), trade.msecs, secTo, ret);

				price = priceTo;
			}
		}
	}
}

void MVolSurpAna::endTicker(const string& ticker)
{
}

void MVolSurpAna::endDay()
{
}

void MVolSurpAna::endJob()
{
	delete pbq_;
}
