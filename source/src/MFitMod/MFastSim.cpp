#include <MFitMod/MFastSim.h>
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

MFastSim::MFastSim(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	maxDollarPosNet_(0.),
	maxDollarPosTicker_(0.),
	restore_(0.),
	thres_(0.),
	maxShrTrade_(numeric_limits<int>::max()),
	tradeBias_(0.),
	pfs_(nullptr)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second;
	if( conf.count("maxDollarPosNet") )
		maxDollarPosNet_ = atof(conf.find("maxDollarPosNet")->second.c_str());
	if( conf.count("maxDollarPosTicker") )
		maxDollarPosTicker_ = atof(conf.find("maxDollarPosTicker")->second.c_str());
	if( conf.count("restore") )
		restore_ = atof(conf.find("restore")->second.c_str());
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());
	if( conf.count("maxShrTrade") )
		maxShrTrade_ = atoi(conf.find("maxShrTrade")->second.c_str());
	if( conf.count("tradeBias") )
		tradeBias_ = atof(conf.find("tradeBias")->second.c_str());
	if( conf.count("ticker") )
		ticker_ = conf.find("ticker")->second;

	targetNames_ = getConfigValuesString(conf, "targetName");
	sigModels_ = getConfigValuesString(conf, "sigModel");
	predModels_ = getConfigValuesString(conf, "predModel");
	weights_ = getConfigValuesDouble(conf, "weight");
	udates_ = getConfigValuesInt(conf, "udate");
	viPred_ = getConfigValuesInt(conf, "iPred");
	if( viPred_.empty() )
	{
		int nTarget = targetNames_.size();
		viPred_ = vector<int>(nTarget, 0);
	}
}

MFastSim::~MFastSim()
{}

void MFastSim::beginJob()
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

	pfs_ = new FastSimNet(moduleName_, baseDir, fitDir_, predModels_, sigModels_, targetNames_,
			weights_, restore_, thres_, maxShrTrade_, maxDollarPosNet_, maxDollarPosTicker_);

	if( debug_ )
		pfs_->setDebug(true);
	pfs_->setIPred(viPred_);
}

void MFastSim::beginDay()
{
}

void MFastSim::endDay()
{
	int idate = MEnv::Instance()->idate;
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));
	const map<string, string>* mTicker2Uid = static_cast<const map<string, string>*>(MEvent::Instance()->get("", "mTicker2Uid"));
	const map<string, vector<double> >* mMid = static_cast<const map<string, vector<double> >*>(MEvent::Instance()->get("", "mMid"));

	DailySimStat dss;
	int udate = MEnv::Instance()->udate;
	//pfs_->endDay(udate, idate, mClose, mTicker2Uid, mMid);

	const string& baseDir = MEnv::Instance()->baseDir;
	vector<PredWholeDay*> vPredDay;
	int nPred = predModels_.size();
	for( int i = 0; i < nPred; ++i )
		vPredDay.push_back(new PredWholeDay(getBinSigBuf(baseDir, predModels_[i], sigModels_[i], targetNames_[i], udate, idate)));

	pfs_->endDay(vPredDay, udate, idate, mClose, mTicker2Uid, mMid);

	for( auto p : vPredDay )
		delete p;
}

void MFastSim::endJob()
{
	pfs_->print();
}

SignalBuffer* MFastSim::getBinSigBuf(const string& baseDir, const string& predModel,
		const string& sigModel, const string& targetName, int udate, int idate)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), "reg");
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, "reg", udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPredMulti(predPath), binPath);
	return pSigBuf;
}

