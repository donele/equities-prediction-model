#include <MFitMod/MPrintTrades.h>
#include <gtlib_fitting/CorrInput.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fitting/PredFromBoostedTree.h>
#include <gtlib_fitana/TargetPred.h>
#include <gtlib_predana/PredAnaByNTreeMultTarPrintTrades.h>
#include <gtlib/util.h>
#include <gtlib_model/pathFtns.h>
#include <jl_lib/mto.h>
#include <gtlib_model/mFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <algorithm>
using namespace std;
using namespace gtlib;

MPrintTrades::MPrintTrades(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	flatFee_(-1.),
	thres_(0.),
	omWgt_(1.),
	tmWgt_(1.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second;
	if( conf.count("omWgt") )
		omWgt_ = atof(conf.find("omWgt")->second.c_str());
	if( conf.count("tmWgt") )
		tmWgt_ = atof(conf.find("tmWgt")->second.c_str());
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());

	targetNames_ = getConfigValuesString(conf, "targetName");
	sigModels_ = getConfigValuesString(conf, "sigModel");
	predModels_ = getConfigValuesString(conf, "predModel");
}

MPrintTrades::~MPrintTrades()
{}

void MPrintTrades::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	string model = MEnv::Instance()->model;
	int udate = MEnv::Instance()->udate;

	fitDesc_ = MEnv::Instance()->fitDesc;
	fitDir_ = get_fit_dir(MEnv::Instance()->baseDir, model, concat(targetNames_), fitDesc_);
	if( !coefModel_.empty() )
		coefFitDir_ = get_fit_dir(MEnv::Instance()->baseDir, coefModel_, concat(targetNames_), fitDesc_);
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

	if( *fitdates_.begin() >= udate )
		print();
}

void MPrintTrades::beginDay()
{
}

void MPrintTrades::endDay()
{
}

void MPrintTrades::endJob()
{
}

void MPrintTrades::print()
{
	cout << "Printing..." << endl;
	const string& model = MEnv::Instance()->model;
	const string& baseDir = MEnv::Instance()->baseDir;
	int udate = MEnv::Instance()->udate;

	PredAnaByNTreeMultTarPrintTrades pbn(baseDir, fitDir_, predModels_, sigModels_, targetNames_, udate, fitdates_,
			omWgt_, tmWgt_, fitDesc_, flatFee_, thres_);
}
