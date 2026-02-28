#include <MFitMod/MFitAna.h>
#include <gtlib_fitting/CorrInput.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fitting/PredFromBoostedTree.h>
#include <gtlib_fitana/TargetPred.h>
#include <gtlib_predana/PredAnaByQuantile.h>
#include <gtlib_predana/PredAnaByQuantileTrade.h>
#include <gtlib_predana/PredAnaByNTree.h>
#include <gtlib_predana/PredAnaByNTreeMultTar.h>
#include <gtlib_predana/PredAnaByNTreeMultTarPrintTrades.h>
#include <gtlib_predana/PredAnaByQuantileUnhedgedTarget.h>
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
#include <thread>
using namespace std;
using namespace gtlib;

MFitAna::MFitAna(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	debug_ana_(false),
	doAna_(false),
	doAnaNTrees_(false),
	doAnaDay_(false),
	doAnaQuantile_(false),
	doAnaQuantileFeatures_(false),
	doAnaSprdQuantileUH_(false),
	volDepThres_(false),
	doAnaThres_(false),
	doAnaBreak_(false),
	doAnaMaxPos_(false),
	exch_('\0'),
	anaThres_(0.),
	anaBreak_(0.),
	anaMaxPos_(0.),
	tradedTickersOnly_(false),
	debugPredAna_(false),
	nTargets_(0),
	nQuantiles_(10),
	flatFee_(-1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugAna") )
		debug_ana_ = conf.find("debugAna")->second == "true";
	if( conf.count("doAna") )
		doAna_ = conf.find("doAna")->second == "true";
	if( conf.count("doAnaNTrees") )
		doAnaNTrees_ = conf.find("doAnaNTrees")->second == "true";
	if( conf.count("doAnaDay") )
		doAnaDay_ = conf.find("doAnaDay")->second == "true";
	if( conf.count("doAnaQuantile") )
		doAnaQuantile_ = conf.find("doAnaQuantile")->second == "true";
	if( conf.count("doAnaQuantileTrade") )
		doAnaQuantileTrade_ = conf.find("doAnaQuantileTrade")->second == "true";
	if( conf.count("debugPredAna") )
		debugPredAna_ = conf.find("debugPredAna")->second == "true";
	if( conf.count("volDepThres") )
		volDepThres_ = conf.find("volDepThres")->second == "true";
	if( conf.count("tradedTickersOnly") )
		tradedTickersOnly_ = conf.find("tradedTickersOnly")->second == "true";
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second;
	if( conf.count("doAnaSprdQuantileUH") )
		doAnaSprdQuantileUH_ = conf.find("doAnaSprdQuantileUH")->second == "true";
	if( conf.count("doAnaThres") )
		doAnaThres_ = conf.find("doAnaThres")->second == "true";
	if( conf.count("doAnaBreak") )
		doAnaBreak_ = conf.find("doAnaBreak")->second == "true";
	if( conf.count("doAnaMaxPos") )
		doAnaMaxPos_ = conf.find("doAnaMaxPos")->second == "true";
	if( conf.count("exch") )
		exch_ = conf.find("exch")->second.c_str()[0];
	if( conf.count("anaThres") )
		anaThres_ = atof(conf.find("anaThres")->second.c_str());
	if( conf.count("anaBreak") )
		anaBreak_ = atof(conf.find("anaBreak")->second.c_str());
	if( conf.count("anaMaxPos") )
		anaMaxPos_ = atof(conf.find("anaMaxPos")->second.c_str());
	if( conf.count("flatFee") )
		flatFee_ = atof(conf.find("flatFee")->second.c_str());
	if( conf.count("nQuantiles") )
		nQuantiles_ = atoi(conf.find("nQuantiles")->second.c_str());

	varAnaQuantile_ = getConfigValuesString(conf, "varAnaQuantile");
	vThres_ = getConfigValuesFloat(conf, "thres");
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

MFitAna::~MFitAna()
{}

void MFitAna::beginJob()
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
		oosTest();
}

void MFitAna::beginDay()
{
}

void MFitAna::endDay()
{
}

void MFitAna::endJob()
{
}

void MFitAna::oosTest()
{
	cout << "Analyze..." << endl;
	vector<thread> vThread;

	if( doAna_ && nTargets_ == 1)
		vThread.push_back(thread(&MFitAna::runAna, this));

	if( doAnaNTrees_ || doAnaDay_ )
		vThread.push_back(thread(&MFitAna::runAnaNTrees, this));

	if(doAnaQuantile_ && !varAnaQuantile_.empty())
		vThread.push_back(thread(&MFitAna::runAnaQuantile, this));

	if(doAnaQuantileTrade_ && !varAnaQuantile_.empty() && !vThres_.empty())
		vThread.push_back(thread(&MFitAna::runAnaQuantileTrade, this));

	if( doAnaSprdQuantileUH_ )
		vThread.push_back(thread(&MFitAna::runAnaQuantileUH, this));

	for(auto& th : vThread)
		th.join();
}

void MFitAna::runAna()
{
	int udate = MEnv::Instance()->udate;
	const string& model = MEnv::Instance()->model;
	cout << "  PredAna..." << endl;

	string anaDir = fitDir_;
	if(predModels_.size() == 1)
		anaDir = get_fit_dir(MEnv::Instance()->baseDir, predModels_[0], concat(targetNames_), fitDesc_);
	TargetPred targetPred(anaDir, coefFitDir_, fitdates_, udate);
	float fee = (flatFee_ >= 0) ? flatFee_ : mto::fee_bpt(model);
	PredAna* pAna = new PredAnaBySprd(&targetPred, getSprdSimpleRanges(model), anaDir, udate, fee);
	pAna->analyze();
	pAna->write();
}

void MFitAna::runAnaNTrees()
{
	int udate = MEnv::Instance()->udate;
	const string& baseDir = MEnv::Instance()->baseDir;
	if( nTargets_ == 1 )
	{
		cout << "  PredAnaByNTree..." << endl;
		// If add the output by quantile for each ntree, PredAnaByQuantile not necessary any more.
		if(doAnaNTrees_)
		{
			PredAnaByNTree pbn(baseDir, fitDir_, predModels_[0], sigModels_[0], targetNames_[0], udate, fitdates_, fitDesc_, flatFee_);
			pbn.anaNTree();
		}
	}
	else if( nTargets_ > 1 )
	{
		cout << "  PredAnaByNTreeMultTar..." << endl;
		// Om stat and Tm stat in a single output file.
		PredAnaByNTreeMultTar pbn(baseDir, fitDir_, predModels_, sigModels_, targetNames_, udate, fitdates_, vWgts_[0], vWgts_[1], fitDesc_, flatFee_, exch_, debugPredAna_, volDepThres_);

		vector<thread> vThread;

		if(doAnaNTrees_)
			vThread.push_back(thread(&PredAnaByNTreeMultTar::anaNTree, &pbn));
		if(doAnaDay_)
			vThread.push_back(thread(&PredAnaByNTreeMultTar::anaDay, &pbn));
		if(doAnaThres_)
			vThread.push_back(thread(&PredAnaByNTreeMultTar::anaThres, &pbn, vThres_));

		for(auto& th : vThread)
			th.join();
	}
}

void MFitAna::runAnaQuantile()
{
	int udate = MEnv::Instance()->udate;
	const string& baseDir = MEnv::Instance()->baseDir;
	// Single (or combined) target.
	// Processes the predictions with all the trees only.
	cout << "  PredAnaByQuantile..." << endl;
	PredAnaByQuantile pbq(baseDir, fitDir_, predModels_, sigModels_, targetNames_, vWgts_,
			udate, fitdates_, fitDesc_, flatFee_,
			volDepThres_, tradedTickersOnly_, debug_ana_);
	bool doBasic = true;

	vector<thread> vThread;

	if(doBasic)
	{
		cout << "    Basic..." << endl;
		for(string var : varAnaQuantile_)
			vThread.push_back(thread(&PredAnaByQuantile::anaQuantile, &pbq, var, nQuantiles_));
	}
	if(doAnaThres_)
	{
		cout << "    Thres ..." << endl;
		for(string var : varAnaQuantile_)
			vThread.push_back(thread(&PredAnaByQuantile::anaQuantileThres, &pbq, var, nQuantiles_));
	}
	if(doAnaBreak_)
	{
		cout << "    Break ..." << endl;
		for(string var : varAnaQuantile_)
			vThread.push_back(thread(&PredAnaByQuantile::anaQuantileBreak, &pbq, var, nQuantiles_));
	}
	if(doAnaMaxPos_)
	{
		cout << "    MaxPos ..." << endl;
		for(string var : varAnaQuantile_)
			vThread.push_back(thread(&PredAnaByQuantile::anaQuantileMaxPos, &pbq, var, nQuantiles_));
	}

	if( doAnaQuantileFeatures_ )
	{
		cout << "    Features ..." << endl;
		for(string var : varAnaQuantile_)
			vThread.push_back(thread(&PredAnaByQuantile::anaQuantileFeatures, &pbq, var, nQuantiles_,anaThres_, anaBreak_, anaMaxPos_));
	}

	for(auto& th : vThread)
		th.join();
}

void MFitAna::runAnaQuantileTrade()
{
	int udate = MEnv::Instance()->udate;
	const string& baseDir = MEnv::Instance()->baseDir;
	for(float thres : vThres_)
	{
		cout << "  PredAnaByQuantileTrade thres " << int(thres) << " ..." << endl;
		PredAnaByQuantileTrade pbq(baseDir, fitDir_, predModels_, sigModels_, targetNames_, vWgts_,
				udate, fitdates_, fitDesc_, flatFee_, thres,
				volDepThres_, tradedTickersOnly_, debug_ana_);

		vector<thread> vThread;

		for(string var : varAnaQuantile_)
			vThread.push_back(thread(&PredAnaByQuantileTrade::anaQuantile, &pbq, var, nQuantiles_));

		for(auto& th : vThread)
			th.join();
	}
}

void MFitAna::runAnaQuantileUH()
{
	int udate = MEnv::Instance()->udate;
	const string& baseDir = MEnv::Instance()->baseDir;
	cout << "  PredAnaByQuantileUH..." << endl;
	PredAnaByQuantileUnhedgedTarget tps(baseDir, fitDir_, predModels_, sigModels_, targetNames_, udate, fitdates_, fitDesc_, flatFee_);
	tps.anaQuantile(nQuantiles_);
}
