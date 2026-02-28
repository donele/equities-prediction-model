#include <MFitting/MFitterTest.h>
#include <gtlib_predana/PredAnaByQuantile.h>
#include <gtlib_fitting/CorrInput.h>
#include <gtlib_fitting/PredFromBoostedTree.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fitana/TargetPred.h>
#include <gtlib_fitana/PredAnaBySprd.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib/util.h>
#include <jl_lib/mto.h>
#include <gtlib_model/mFtns.h>
#include <MFramework.h>
#include <map>
#include <thread>
#include <string>
#include <boost/filesystem.hpp>
#include <algorithm>
using namespace std;
using namespace gtlib;

MFitterTest::MFitterTest(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	writePred_(false),
	doAna_(true),
	doAnaSprdQuantile_(false),
	doAnaSprdQuantileOmTm_(false),
	fitDesc_(MEnv::Instance()->fitDesc),
	sigType_(MEnv::Instance()->sigType),
	flatFee_(-1.),
	pFitData_(0),
	nTrees_(0),
	shrinkage_(0.1),
	minPtsNode_(5000),
	maxLeafNodes_(500),
	maxTreeLevels_(1000)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("writePred") )
		writePred_ = conf.find("writePred")->second == "true";
	if( conf.count("doAna") )
		doAna_ = conf.find("doAna")->second == "true";
	if( conf.count("doAnaSprdQuantile") )
		doAnaSprdQuantile_ = conf.find("doAnaSprdQuantile")->second == "true";
	if( conf.count("smodel") )
		smodel_ = conf.find("smodel")->second;

	// Boosted Decisiontree parameters.
	if( conf.count("nTrees") )
		nTrees_ = atoi(conf.find("nTrees")->second.c_str());
	if( conf.count("minPtsNode") )
		minPtsNode_ = atoi(conf.find("minPtsNode")->second.c_str());

	// Input Names, target names, target weights, and spectator names.
	varInfo_.inputNames = getConfigValuesString(conf, "input");
	varInfo_.targetNames = getConfigValuesString(conf, "targetName");
	varInfo_.bmpredNames = getConfigValuesString(conf, "bmpredName");
	//varInfo_.spectatorNames = getConfigValuesString(conf, "spectator");
	varInfo_.targetWeights = getConfigValuesFloat(conf, "targetWeight");

	initialize();
}

void MFitterTest::initialize()
{
	smodel_ = getSigModel();
	nTrees_ = getNTrees();

	bdtParam_.bParam.nTrees = nTrees_;
	bdtParam_.bParam.shrinkage = shrinkage_;
	bdtParam_.dtParam.minPtsNode = minPtsNode_;
	bdtParam_.dtParam.maxLeafNodes = maxLeafNodes_;
	bdtParam_.dtParam.maxTreeLevels = maxTreeLevels_;
}

string MFitterTest::getSigModel()
{
	string s = smodel_;
	if( s.empty() )
		s = MEnv::Instance()->model;
	return s;
}

string MFitterTest::getSigType()
{
	string sigType;
	int max_horizon = varInfo_.maxHorizon();
	int tenMinInSec = 10 * 60;
	if( max_horizon <= tenMinInSec )
		sigType = "om";
	else
		sigType = "tm";
	return sigType;
}

int MFitterTest::getNTrees()
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

MFitterTest::~MFitterTest()
{}

// beginJob() ------------------------------------------------------------------
void MFitterTest::beginJob()
{
	timer_.restart();
	cout << moduleName_ << "::beginJob()" << endl;

	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;
	string baseDir = MEnv::Instance()->baseDir;
	fitDir_ = get_fit_dir(baseDir, model, varInfo_.fullTargetName(), fitDesc_);
	mkd(fitDir_);
	fitdates_ = get_fitdates(MEnv::Instance()->idates, udate);
	vector<thread> vThread;

	readData();
	vThread.push_back(thread(&MFitterTest::corr_input, this));

	if( *fitdates_.rbegin() < udate )
		fit();
	else if( *fitdates_.begin() >= udate )
		oosTest();

	for( auto& t : vThread )
		t.join();
}

void MFitterTest::readData()
{
	pFitData_ = new FitData(getDailyDataCount(), varInfo_.inputNames, varInfo_.spectatorNames);
	for( int idate : fitdates_ )
		readData(idate);
	pFitData_->printReadSummary();
	printElapsed();
}

DailyDataCount MFitterTest::getDailyDataCount()
{
	DailyDataCount ddCount;
	for( int idate : fitdates_ )
	{
		int nNewData = getNNewData(get_sig_path(MEnv::Instance()->baseDir, smodel_, idate, sigType_, fitDesc_));
		ddCount.add(idate, nNewData);
	}
	return ddCount;
}

int MFitterTest::getNNewData(string path)
{
	int nNewData = 0;
	ifstream ifs(path.c_str(), ios::binary);
	if( ifs.is_open() )
	{
		hff::SignalLabel sh;
		ifs >> sh;
		if( ifs.rdstate() == 0 )
			nNewData = sh.nrows;
	}
	else
		cout << path << " is not open.\n";
	return nNewData;
}

void MFitterTest::readData(int idate)
{
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;

	read_sig_file(idate);
	read_bm_pred(idate); // Read benchmark predictions.
	calculate_fit_target(idate); // FitData.pTarget is totalTarget - bmpred.
}

void MFitterTest::read_sig_file(int idate)
{
	string path = get_sig_path(MEnv::Instance()->baseDir, smodel_, idate, sigType_, fitDesc_);
	ifstream ifs(path.c_str(), ios::binary);
	if( ifs.is_open() )
	{
		cout << "Reading " << path << " ...";
		hff::SignalLabel sh;
		ifs >> sh;
		int nDataToday = sh.nrows;

		vector<int> inputIndx = get_index(sh, varInfo_.inputNames);
		vector<int> spectatorIndx = get_index(sh, varInfo_.spectatorNames);
		vector<int> targetIndx = get_index(sh, varInfo_.targetNames);
		vector<int> bmpredIndx = get_index(sh, varInfo_.bmpredNames);

		int nInputFields = pFitData_->nInputFields;
		int nSpectators = pFitData_->nSpectators;
		int nTargets = varInfo_.nTargets();
		int dayOffset = pFitData_->getDailyDataCount().getOffset(idate);
		vSumTargetToday_.resize(nDataToday);
		vSumBmpredToday_.resize(nDataToday);
		std::fill(begin(vSumTargetToday_), end(vSumTargetToday_), 0.);
		std::fill(begin(vSumBmpredToday_), end(vSumBmpredToday_), 0.);

		// Read inputs, targets, and spectators from sig file.
		hff::SignalContent aSignal(sh.ncols);
		for( int r = 0; r < nDataToday && ifs.rdstate() == 0; ++r )
		{
			ifs >> aSignal;

			for( int i = 0; i < nInputFields; ++i )
				pFitData_->input(i, dayOffset + r) = aSignal.v[inputIndx[i]];

			for( int i = 0; i < nSpectators; ++i )
				pFitData_->spectator(i, dayOffset + r) = aSignal.v[spectatorIndx[i]];

			for( int i = 0; i < nTargets; ++i ) // Sum of targets, weighted.
				vSumTargetToday_[r] += aSignal.v[targetIndx[i]] * varInfo_.targetWeights[i];

			for( int i = 0; i < bmpredIndx.size(); ++i ) // Sum of bmpreds.
				vSumBmpredToday_[r] += aSignal.v[bmpredIndx[i]];
		}
		cout << nDataToday << " rows read." << endl;
	}
	else
		cout << path << " is not open." << endl;
}

void MFitterTest::read_bm_pred(int idate)
{
	// Implement later.
}

void MFitterTest::calculate_fit_target(int idate)
{
	int dayOffset = pFitData_->getDailyDataCount().getOffset(idate);
	int nDataToday = pFitData_->getDailyDataCount().getNdata(idate);
	for( int i = 0; i < nDataToday; ++i )
	{
		pFitData_->bmpred(dayOffset + i) = vSumBmpredToday_[i];
		pFitData_->target(dayOffset + i) = vSumTargetToday_[i] - vSumBmpredToday_[i];
	}
}

vector<int> MFitterTest::get_index(hff::SignalLabel& sh, vector<string>& names)
{
	vector<string> namesInFile = sh.labels;
	auto nameBegin = begin(namesInFile);
	auto nameEnd = end(namesInFile);
	vector<int> vIndx;
	for( string name : names )
	{
		auto it = find(nameBegin, nameEnd, name);
		if( it != nameEnd )
		{
			int indx = it - nameBegin;
			vIndx.push_back(indx);
		}
	}
	return vIndx;
}

// beginDay() ------------------------------------------------------------------
void MFitterTest::beginDay()
{
}

// endDay() --------------------------------------------------------------------
void MFitterTest::endDay()
{
}

// endJob() --------------------------------------------------------------------
void MFitterTest::endJob()
{
}

void MFitterTest::printElapsed()
{
	int elapsed = timer_.elapsed();
	int hh = elapsed / 3600;
	int mm = elapsed % 3600 / 60;
	int ss = elapsed % 3600 % 60;
	char buf[100];
	sprintf(buf, "Elapsed: %3d:%02d:%02d\n", hh, mm, ss);
	cout << buf;
	cout.flush();
}

void MFitterTest::corr_input()
{
	cout << "Correlation ..." << endl;
	int udate = MEnv::Instance()->udate;
	CorrInput ci(pFitData_, fitDir_, udate);
	ci.stat();
	printElapsed();
}

void MFitterTest::fit()
{
	cout << "Fitting ..." << endl;
	float reduction = 0.;
	BoostedDecisionTree* fitter = new BoostedDecisionTree(bdtParam_, pFitData_, fitDir_, MEnv::Instance()->udate);
	fitter->fit();
	fitter->writeModel(getCoefPath());
	delete fitter;
	printElapsed();
}

string MFitterTest::getCoefPath()
{
	string coefDir = fitDir_ + "/coef";
	mkd(coefDir);
	int udate = MEnv::Instance()->udate;
	string coefPath = coefDir + "/coef" + itos(udate) + ".txt";
	return coefPath;
}

void MFitterTest::oosTest()
{
	string model = MEnv::Instance()->model;
	string market = MEnv::Instance()->market;
	int udate = MEnv::Instance()->udate;
	const string& baseDir = MEnv::Instance()->baseDir;

	if( writePred_ )
	{
		cout << "Prediction..." << endl;
		PredFromBoostedTree pred(pFitData_, fitDir_, fitDir_, MEnv::Instance()->udate);
		pred.calculatePred();
		pred.writePred();
	}

	cout << "Analyze..." << endl;
	if( doAna_ )
	{
		TargetPred targetPred(fitDir_, pFitData_->getDailyDataCount().getIdates(), udate);
		float fee = mto::fee_bpt(market);
		PredAna* pAna = new PredAnaBySprd(&targetPred, getSprdFineRanges(model), fitDir_, udate, fee);
		//PredAna* pAna = new PredAnaByThres(TargetPred);
		//PredAna* pAna = new PredAnaByMarketCap(TargetPred);
		pAna->analyze();
		pAna->write();
	}

	if( doAnaSprdQuantile_ )
	{
		int nBins = 20;
		PredAnaByQuantile tps(baseDir, fitDir_, model, varInfo_.fullTargetName(), 1., udate, fitdates_, MEnv::Instance()->fitDesc, flatFee_);
		tps.anaQuantile("sprdAdj", nBins);
	}
}

