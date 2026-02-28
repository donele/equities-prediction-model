#include <MFitMod/MFitNtrees.h>
#include <gtlib_fitting/CorrInput.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fitting/PredFromBoostedTree.h>
#include <gtlib_fitana/TargetPred.h>
#include <gtlib_fitana/PredAnaBySprd.h>
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

MFitNtrees::MFitNtrees(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(true),
	write_(true),
	nTargets_(0),
	minSprd_(0.),
	maxSprd_(100.),
	default_(1000000)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";

	targetNames_ = getConfigValuesString(conf, "targetName");
	sigModels_ = getConfigValuesString(conf, "sigModel");
	predModels_ = getConfigValuesString(conf, "predModel");
	nTargets_ = targetNames_.size();
}

MFitNtrees::~MFitNtrees()
{}

void MFitNtrees::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	string model = MEnv::Instance()->model;
	int udate = MEnv::Instance()->udate;

	fitDir_ = get_fit_dir(MEnv::Instance()->baseDir, model, concat(targetNames_), MEnv::Instance()->fitDesc);
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
		oosTest();
}

void MFitNtrees::beginDay()
{
}

void MFitNtrees::endDay()
{
}

void MFitNtrees::endJob()
{
}

void MFitNtrees::oosTest()
{
	cout << "Analyze..." << endl;
	const string& model = MEnv::Instance()->model;
	const string& baseDir = MEnv::Instance()->baseDir;
	int udate = MEnv::Instance()->udate;
	vector<pair<float, float>> range = {make_pair(minSprd_, maxSprd_)};

	if( nTargets_ == 1)
	{
		TargetPred targetPred(fitDir_, fitdates_, udate);
		float fee = mto::fee_bpt(model);

		PredAnaBySprd* pAna = new PredAnaBySprd(&targetPred, range, fitDir_, udate, fee);
		pAna->analyze();
		if( debug_ )
			pAna->write();

		vector<pair<int, float>> vp = pAna->getNtreeMbias();
		for( auto p : vp )
			cout << p.first << '\t' << p.second << endl;

		int nTrees = getBestNtree(vp);
		cout << MEnv::Instance()->sigType << '\t' << MEnv::Instance()->fitDesc << '\t' << nTrees << endl;

		writeDatabase(nTrees);
	}
}

int MFitNtrees::getBestNtree(const vector<pair<int, float>>& vp)
{
	int minNtree = 10;
	bool firstRow = true;
	float prevBias = -999.;
	int prevNtree = -1;
	int bestNtree = -1;
	for( auto& p : vp )
	{
		int ntree = p.first;
		float bias = p.second;

		if( !firstRow )
		{
			if( bias > 0. && prevBias < 0. )
			{
				bestNtree = interpolateBestNtree(prevNtree, ntree, prevBias, bias);
				cout << "best " << bestNtree << endl;
				break;
			}
			else if( prevBias > 0. && bias >= prevBias )
			{
				bestNtree = prevNtree;
				break;
			}
		}

		firstRow = false;
		prevNtree = ntree;
		prevBias = bias;
	}

	if( bestNtree < 0 )
		bestNtree = prevNtree;

	if( bestNtree < minNtree )
		bestNtree = 10;

	return bestNtree;
}

int MFitNtrees::interpolateBestNtree(int prevNtree, int ntree, float prevBias, float bias)
{
	cout << prevNtree << '\t' << ntree << '\t' << prevBias << '\t' << bias << endl;
	return (bias * prevNtree - prevBias * ntree ) / (bias - prevBias);
}

void MFitNtrees::writeDatabase(int nTrees)
{
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const string& mCode = linMod.mCode;
	const string& market = MEnv::Instance()->market;
	string dbname = mto::hf(market);
	if( debug_ )
		dbname = "hfstock";

	int idate = *MEnv::Instance()->idates.rbegin();
	//int dbDate = nextOpen(market, itoday(market));
	int dbDate = nextOpen(market, idate, 2);
	string rowUpdate;
	int reg1m = default_;
	int reg40m = default_;
	int evt1m = default_;

	if(MEnv::Instance()->sigType == "om" && MEnv::Instance()->fitDesc == "reg")
	{
		reg1m = nTrees;
		rowUpdate = "reg1m";
	}
	else if(MEnv::Instance()->sigType == "tm" && MEnv::Instance()->fitDesc == "reg")
	{
		reg40m = nTrees;
		rowUpdate = "reg40m";
	}
	else if(MEnv::Instance()->sigType == "om" && MEnv::Instance()->fitDesc == "tevt")
	{
		evt1m = nTrees;
		rowUpdate = "evt1m";
	}

	char chk[1000];
	char ins[1000];
	char upt[1000];
	sprintf(chk, "select count(*) from numTrees where idate = %d and exchange = '%s'",
			dbDate, mCode.c_str());
	sprintf(ins, "insert into numTrees (idate, exchange, reg1m, reg40m, evt1m) "
			" values (%d, '%s', %d, %d, %d) ",
			dbDate, mCode.c_str(), reg1m, reg40m, evt1m);
	sprintf(upt, "update numTrees set %s = %d "
			" where idate = %d and exchange = '%s' ",
			rowUpdate.c_str(), nTrees, dbDate, mCode.c_str());

	if( write_ )
		check_and_insert_or_update(dbname, chk, ins, upt);
	if( debug_ )
	{
		cout << chk << endl;
		cout << ins << endl;
		cout << upt << endl;
	}
}

