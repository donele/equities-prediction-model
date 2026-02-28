#include <MFitMod/MFastSimDataDump.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fastsim/FastSimNet.h>
#include <gtlib_fastsim/FastSimSimple.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <gtlib/util.h>
#include <gtlib_model/pathFtns.h>
#include <MFramework.h>
#include <map>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MFastSimDataDump::MFastSimDataDump(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	pfs_(nullptr)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second;
	if( conf.count("ticker") )
		ticker_ = conf.find("ticker")->second;

	targetNames_ = getConfigValuesString(conf, "targetName");
	sigModels_ = getConfigValuesString(conf, "sigModel");
	predModels_ = getConfigValuesString(conf, "predModel");
	udates_ = getConfigValuesInt(conf, "udate");
}

MFastSimDataDump::~MFastSimDataDump()
{}

void MFastSimDataDump::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	string model = MEnv::Instance()->model;
	const string& baseDir = MEnv::Instance()->baseDir;

	fitDir_ = get_fit_dir(MEnv::Instance()->baseDir, model, concat(targetNames_), MEnv::Instance()->fitDesc);
	if( !coefModel_.empty() )
		coefFitDir_ = get_fit_dir(MEnv::Instance()->baseDir, coefModel_, concat(targetNames_), MEnv::Instance()->fitDesc);
	else
		coefFitDir_ = fitDir_;

	mkd(fitDir_);
	fitdates_ = get_fitdates(MEnv::Instance()->idates);

	if( predModels_.empty() )
	{
		for( int i = 0; i < targetNames_.size(); ++i )
			predModels_.push_back(model);
	}
	if( sigModels_.empty() )
		sigModels_ = predModels_;

	pfs_ = new FastSimDataDump(baseDir, fitDir_, predModels_, sigModels_, targetNames_);

	if( debug_ )
		pfs_->setDebug(true);
}

void MFastSimDataDump::beginDay()
{
}

void MFastSimDataDump::endDay()
{
	int idate = MEnv::Instance()->idate;
	const map<string, string>* mTicker2Uid = static_cast<const map<string, string>*>(MEvent::Instance()->get("", "mTicker2Uid"));
	const map<string, vector<double> >* mMid = static_cast<const map<string, vector<double> >*>(MEvent::Instance()->get("", "mMid"));
	int udate = 0;
	if( udates_.empty() )
		udate = MEnv::Instance()->udate;
	else
	{
		for( int modelDate : udates_ )
		{
			if( modelDate <= idate && modelDate > udate )
				udate = modelDate;
		}
	}
	pfs_->endDay(udate, idate, mTicker2Uid, mMid);
}

void MFastSimDataDump::endJob()
{
}
