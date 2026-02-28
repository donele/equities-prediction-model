#include <MFitMod/MFitter.h>
#include <gtlib_fitting/CorrInput.h>
#include <gtlib_fitting/PredFromBoostedTree.h>
#include <gtlib_fitting/BDT_wgtTradable.h>
#include <gtlib_fitting/BDT_costWgtTar.h>
#include <gtlib_fitana/TargetPred.h>
#include <gtlib_fitana/PredAnaBySprd.h>
#include <gtlib/util.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_model/mFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <algorithm>
#include <thread>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MFitter::MFitter(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	debugWgtTradable_(false),
	wgtTradableConstWgt_(false),
	wgtTradableNaturalUnit_(false),
	wgtTradableNaturalToCost_(false),
	wgtTradableNaturalToNegCost_(false),
	wgtTradableMaxPredAdjApplyCut_(1.),
	wgtTradableMaxSprdApplyCut_(100.),
	wgtTradableMaxWgt_(1.5),
	doCorr_(false),
	costWgtTar_(false),
	prune_(false),
	wgt_(false),
	wgtTradable_(false),
	wgtFacLimit_(.5),
	minTreeWgt_(0),
	fitDesc_(MEnv::Instance()->fitDesc),
	sigType_(MEnv::Instance()->sigType),
	modelSource_("disk"),
	pFitData_(nullptr),
	nTrees_(0),
	shrinkage_(0.1),
	minPtsNode_(5000),
	maxLeafNodes_(500),
	maxTreeLevels_(1000),
	maxMu_(200.),
	nForceCutLevels_(2),
	forceCutTreeLower_(0),
	forceCutTreeUpper_(0),
	reduction_(0.),
	decayFactor_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugWgtTradable") )
		debugWgtTradable_ = conf.find("debugWgtTradable")->second == "true";
	if( conf.count("wgtTradableConstWgt") )
		wgtTradableConstWgt_ = conf.find("wgtTradableConstWgt")->second == "true";
	if( conf.count("wgtTradableNaturalUnit") )
		wgtTradableNaturalUnit_ = conf.find("wgtTradableNaturalUnit")->second == "true";
	if( conf.count("wgtTradableNaturalToCost") )
		wgtTradableNaturalToCost_ = conf.find("wgtTradableNaturalToCost")->second == "true";
	if( conf.count("wgtTradableNaturalToNegCost") )
		wgtTradableNaturalToNegCost_ = conf.find("wgtTradableNaturalToNegCost")->second == "true";
	forceCutInput_ = getConfigValuesString(conf, "forceCutInput");
	if( conf.count("doCorr") )
		doCorr_ = conf.find("doCorr")->second == "true";
	if( conf.count("prune") )
		prune_ = conf.find("prune")->second == "true";
	if( conf.count("wgt") )
		wgt_ = conf.find("wgt")->second == "true";
	if( conf.count("wgtTradable") )
		wgtTradable_ = conf.find("wgtTradable")->second == "true";
	if( conf.count("minTreeWgt") )
		minTreeWgt_ = atoi(conf.find("minTreeWgt")->second.c_str());
	if( conf.count("wgtFacLimit") )
		wgtFacLimit_ = atof(conf.find("wgtFacLimit")->second.c_str());
	if( conf.count("wgtTradableMaxPredAdjApplyCut") )
		wgtTradableMaxPredAdjApplyCut_ = atof(conf.find("wgtTradableMaxPredAdjApplyCut")->second.c_str());
	if( conf.count("wgtTradableMaxSprdApplyCut") )
		wgtTradableMaxSprdApplyCut_ = atof(conf.find("wgtTradableMaxSprdApplyCut")->second.c_str());
	if( conf.count("wgtTradableMaxWgt") )
		wgtTradableMaxWgt_ = atof(conf.find("wgtTradableMaxWgt")->second.c_str());
	if( conf.count("costWgtTar") )
		costWgtTar_ = conf.find("costWgtTar")->second == "true";
	if( conf.count("reduction") )
		reduction_ = atof(conf.find("reduction")->second.c_str());
	if( conf.count("decayFactor") )
		decayFactor_ = atof(conf.find("decayFactor")->second.c_str());
	if( conf.count("forceCutTreeLower") )
		forceCutTreeLower_ = atof(conf.find("forceCutTreeLower")->second.c_str());
	if( conf.count("forceCutTreeUpper") )
		forceCutTreeUpper_ = atof(conf.find("forceCutTreeUpper")->second.c_str());
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second;
	if( conf.count("modelSource") )
		modelSource_ = conf.find("modelSource")->second;

	// Boosted Decisiontree parameters.
	if( conf.count("nTrees") )
		nTrees_ = atoi(conf.find("nTrees")->second.c_str());
	if( conf.count("shrinkage") )
		shrinkage_ = atof(conf.find("shrinkage")->second.c_str());
	if( conf.count("minPtsNode") )
		minPtsNode_ = atoi(conf.find("minPtsNode")->second.c_str());
	if( conf.count("maxLeafNodes") )
		maxLeafNodes_ = atoi(conf.find("maxLeafNodes")->second.c_str());
	if( conf.count("maxTreeLevels") )
		maxTreeLevels_ = atoi(conf.find("maxTreeLevels")->second.c_str());
	if( conf.count("maxMu") )
		maxMu_ = atof(conf.find("maxMu")->second.c_str());
	if( conf.count("nForceCutLevels") )
		nForceCutLevels_ = atoi(conf.find("nForceCutLevels")->second.c_str());

	initialize();
}

void MFitter::initialize()
{
	nTrees_ = getNTrees();

	bdtParam_.bParam.nTrees = nTrees_;
	bdtParam_.bParam.shrinkage = shrinkage_;
	bdtParam_.dtParam.minPtsNode = minPtsNode_;
	bdtParam_.dtParam.maxLeafNodes = maxLeafNodes_;
	bdtParam_.dtParam.maxTreeLevels = maxTreeLevels_;
	bdtParam_.dtParam.maxMu = maxMu_;
}

int MFitter::getNTrees()
{
	int n = nTrees_;
	if( n == 0 )
	{
		if( sigType_ == "om" )
		{
			const auto& linMod = MEnv::Instance()->linearModel;
			n = linMod.nTreesOmFit;
		}
		else if( sigType_ == "tm" )
		{
			const auto& nonLinMod = MEnv::Instance()->nonLinearModel;
			n = nonLinMod.nTreesTmFit;
		}
	}
	return n;
}

MFitter::~MFitter()
{}

// beginJob() ------------------------------------------------------------------
void MFitter::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;
	string baseDir = MEnv::Instance()->baseDir;

	const string& fullTargetName = MEnv::Instance()->fullTargetName;
	fitDir_ = get_fit_dir(baseDir, model, fullTargetName, fitDesc_);
	mkd(fitDir_);

	pFitData_ = MEvent::Instance()->get<gtlib::FitData*>("", "pFitData");

	vector<thread> vThread;
	bool multiThreadCorr = true;
	if( multiThreadCorr )
		vThread.push_back(thread(&MFitter::corrInput, this));
	else
		corrInput();

	vector<int> fitdates = pFitData_->getIdates();
	if( *fitdates.rbegin() < udate )
		fit();
	else if( *fitdates.begin() >= udate )
		writePred();

	for( auto& t : vThread )
		t.join();
}

void MFitter::corrInput()
{
	if( !doCorr_ )
		return;
	cout << "Correlation ..." << endl;
	int udate = MEnv::Instance()->udate;
	CorrInput ci(pFitData_, fitDir_, udate);
	ci.stat();
}

// beginDay() ------------------------------------------------------------------
void MFitter::beginDay()
{
}

// endDay() --------------------------------------------------------------------
void MFitter::endDay()
{
}

// endJob() --------------------------------------------------------------------
void MFitter::endJob()
{
}

void MFitter::fit()
{
	cout << "Fitting ..." << endl;
	DecisionTree* fitter = nullptr;
	if(costWgtTar_)
		fitter = new BDT_costWgtTar(bdtParam_, pFitData_, fitDir_, MEnv::Instance()->udate, false);
	else if(wgtTradable_)
		fitter = new BDT_wgtTradable(bdtParam_, pFitData_, fitDir_, MEnv::Instance()->udate, minTreeWgt_, wgtFacLimit_, wgtTradableMaxPredAdjApplyCut_, wgtTradableMaxSprdApplyCut_, wgtTradableMaxWgt_, wgtTradableConstWgt_, wgtTradableNaturalUnit_, wgtTradableNaturalToCost_, wgtTradableNaturalToNegCost_, debugWgtTradable_);
	else
	{
		BoostedDecisionTree* bfitter = new BoostedDecisionTree(bdtParam_, pFitData_, fitDir_, MEnv::Instance()->udate);
		if( !forceCutInput_.empty() )
		{
			vector<int> forceCutIndex;
			for(auto input : forceCutInput_)
				forceCutIndex.push_back(pFitData_->getInputIndex(input));
			set<int> forceCutTrees;
			for(int i = 0; i < forceCutTreeLower_; ++i)
				forceCutTrees.insert(i);
			if(forceCutTreeUpper_ > 0)
			{
				for(int i = forceCutTreeUpper_; i < nTrees_; ++i)
					forceCutTrees.insert(i);
			}
			bfitter->setForceCut(forceCutIndex, nForceCutLevels_, forceCutTrees);
		}
		else if( prune_ )
		{
			bfitter->setPrune(reduction_);
		}
		else if( wgt_ )
		{
			bfitter->setDecay(decayFactor_);
		}
		fitter = bfitter;
	}
	fitter->fit();
	fitter->writeModel(getCoefPath());
	delete fitter;
}

string MFitter::getCoefPath()
{
	string coefDir = fitDir_ + "/coef";
	mkd(coefDir);
	int udate = MEnv::Instance()->udate;
	string coefPath = coefDir + "/coef" + itos(udate) + ".txt";
	return coefPath;
}

void MFitter::writePred()
{
	string model = MEnv::Instance()->model;
	string baseDir = MEnv::Instance()->baseDir;
	const string& fullTargetName = MEnv::Instance()->fullTargetName;
	if( coefModel_.empty() )
		coefModel_ = model;
	string coefFitDir = get_fit_dir(baseDir, coefModel_, fullTargetName, fitDesc_);

	if(modelSource_ == "disk")
	{
		PredFromBoostedTree pred(pFitData_, fitDir_, coefFitDir, MEnv::Instance()->udate, costWgtTar_);
		pred.calculatePred();
		pred.writePred();
	}
	else if(modelSource_ == "db")
	{
		string dbname = "hfstockeu";
		string dbtable = (sigType_ == "om") ? "hftree" : "hfpuretreeboost";
		string mkt = "E";
		PredFromBoostedTree pred(pFitData_, fitDir_, dbname, dbtable, mkt, MEnv::Instance()->udate, costWgtTar_);
		pred.calculatePred();
		pred.writePred();
	}
}
