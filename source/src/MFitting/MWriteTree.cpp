#include <MFitting/MWriteTree.h>
#include <MFitting.h>
#include <gtlib_model/pathFtns.h>
#include <jl_lib/jlutil.h>
#include <MFitObj.h>
#include <MFramework.h>
#include <time.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MWriteTree::MWriteTree(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	write_debug_(true),
	one_query_(true),
	nTrees_(0),
	fitDesc_(MEnv::Instance()->fitDesc),
	sigType_(MEnv::Instance()->sigType)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("writeDebug") )
		write_debug_ = conf.find("writeDebug")->second == "true";
	if( conf.count("oneQuery") )
		one_query_ = conf.find("oneQuery")->second == "true";
	if( conf.count("nTrees") )
		nTrees_ = atoi(conf.find("nTrees")->second.c_str());
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second.c_str();

	// Target Names.
	if( conf.count("targetName") )
	{
		pair<mmit, mmit> targetNames = conf.equal_range("targetName");
		for( mmit mi = targetNames.first; mi != targetNames.second; ++mi )
		{
			string targetName = mi->second;
			targetNames_.push_back(targetName);
		}
	}
	nTargets_ = targetNames_.size();

	// Target weights.
	if( conf.count("targetWeight") )
	{
		pair<mmit, mmit> weights = conf.equal_range("targetWeight");
		for( mmit mi = weights.first; mi != weights.second; ++mi )
		{
			double weight = atof(mi->second.c_str());
			targetWeights_.push_back(weight);
		}
	}
	if( nTargets_ == 1 && targetWeights_.empty() )
		targetWeights_.push_back(1.);

	if( sigType_ == "om" )
	{
		if( fitDesc_ == "reg" )
			tablename_ = "hfTree";
		else if( fitDesc_ == "tevt" )
			tablename_ = "hfTreeTradeEvent";
	}
	else if( sigType_ == "tm" )
	{
		tablename_ = "hfPureTreeBoost";
	}

	if( nTargets_ == 1 )
		targetName_ = targetNames_[0];
	else if( nTargets_ > 1 )
	{
		targetName_ = "";
		for( int i = 0; i < nTargets_; ++i )
		{
			char buf[200];
			sprintf(buf, "%s_%.1f", targetNames_[i].c_str(), targetWeights_[i]);
			if( targetName_.empty() )
				targetName_ += buf;
			else
				targetName_ += (string)"_" + buf;
		}
	}
}

MWriteTree::~MWriteTree()
{}

void MWriteTree::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	//const string& model = MEnv::Instance()->model;
	if(coefModel_.empty())
		coefModel_ = MEnv::Instance()->model;
	const string& baseDir = MEnv::Instance()->baseDir;
	fitDir_ = get_fit_dir(baseDir, coefModel_, targetName_, fitDesc_);

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	if( nTrees_ <= 0 )
	{
		if( sigType_ == "om" )
			nTrees_ = linMod.nTreesOmProd;
		else if( sigType_ == "tm" )
			nTrees_ = nonLinMod.nTreesTmProd;
	}
	int udate = MEnv::Instance()->udate;
	cout << "MWriteTree date = " << udate << " ntrees = " << nTrees_ << endl;
}

void MWriteTree::beginDay()
{
}

void MWriteTree::endDay()
{
}

void MWriteTree::endJob()
{
	write_trees();
}

void MWriteTree::write_trees()
{
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	vector<string> markets = MEnv::Instance()->markets;
	int udate = MEnv::Instance()->udate;

	DTBoost dtBoost(nTrees_);
	dtBoost.setDir(fitDir_, udate);

	string dbname = write_debug_ ? mto::hfdbg(markets[0]) : mto::hf(markets[0]);
	int dbDate = getDBDate(markets[0]);
	dtBoost.write_trees(dbname, tablename_, linMod.mCode, udate, dbDate, one_query_);
}

int MWriteTree::getDBDate(const string& market)
{
	char syyyymmdd[128];
	char shhmmss[128];
	struct tm* now;
	time_t ltime;
	time( &ltime );
	now = gmtime( &ltime );
	strftime( syyyymmdd, 128, "%Y%m%d", now );
	strftime( shhmmss, 128, "%H%M%S", now );
	int utc_yyyymmdd = atoi(syyyymmdd);
	int utc_hhmmss = atoi(shhmmss);

	QuoteTime qt(utc_yyyymmdd, utc_hhmmss, "UTC"); // idate can be today or yesterday.
	qt.SetTimeZone(mto::tz(market));
	int idate = (int)qt.Date();
	//int dbDate = nextOpen(market, idate);
	return idate;
}
