#include <MFitting/MFitISLE.h>
#include <MFitting.h>
#include <MFitObj.h>
#include <gtlib_model/pathFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
//#include <MFitting/HFTrees.h>
//#include <MFitting/HFSTree.h>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;
//using namespace writeSig;

MFitISLE::MFitISLE(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	fitDesc_(MEnv::Instance()->fitDesc),
	sigType_(MEnv::Instance()->sigType),
	addBMtoTarget_(false),
	rat_sample_(0.2),
	step_(0.005),
	do_fit_(false),
	do_analyze_ins_(false),
	do_analyze_(false),
	read_multithread_(false),
	targetIndx_(-1),
	ntrees_(0),
	minPtsNode_(5000),
	nthreads_(70),
	maxNodes_(500),
	shrinkage_(0.1),
	treeMaxLevels_(1000),
	baseDir_("\\\\smrc-ltc-mrct43\\f\\hffit\\")
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("doFit") )
		do_fit_ = conf.find("doFit")->second == "true";
	if( conf.count("doAnalyze") )
		do_analyze_ = conf.find("doAnalyze")->second == "true";
	if( conf.count("doAnalyzeIns") )
		do_analyze_ins_ = conf.find("doAnalyzeIns")->second == "true";
	if( conf.count("readMultithread") )
		read_multithread_ = conf.find("readMultithread")->second == "true";
	if( conf.count("nthreads") )
		nthreads_ = atoi(conf.find("nthreads")->second.c_str());
	if( conf.count("ntrees") )
		ntrees_ = atoi(conf.find("ntrees")->second.c_str());
	if( conf.count("maxNodes") )
		maxNodes_ = atoi(conf.find("maxNodes")->second.c_str());
	if( conf.count("ratSample") )
		rat_sample_ = atof(conf.find("ratSample")->second.c_str());
	if( conf.count("shrinkage") )
		shrinkage_ = atof(conf.find("shrinkage")->second.c_str());
	if( conf.count("step") )
		step_ = atof(conf.find("step")->second.c_str());

	// Input Names.
	if( conf.count("input") )
	{
		pair<mmit, mmit> inputs = conf.equal_range("input");
		for( mmit mi = inputs.first; mi != inputs.second; ++mi )
		{
			string name = mi->second;
			inputNames_.push_back(name);
		}
	}
	nInput_ = inputNames_.size();

	// Target Name.
	if( conf.count("targetName") )
		targetName_ = conf.find("targetName")->second;

	// Spectator Names.
	if( conf.count("spectator") )
	{
		pair<mmit, mmit> spectators = conf.equal_range("spectator");
		for( mmit mi = spectators.first; mi != spectators.second; ++mi )
		{
			string name = mi->second;
			spectatorNames_.push_back(name);
		}
	}
	nSpectator_ = spectatorNames_.size();

	//sigType_ = "om";
	//if( targetName_.size() >= 6 && targetName_.substr(0, 6) == "restar" )
	//{
	//sigType_ = "om";
	//addBMtoTarget_ = true;
	//}
	//else if( targetName_.size() >= 3 && targetName_.substr(0, 3) == "tar" )
	//{
	//sigType_ = "tm";
	//addBMtoTarget_ = false;
	//}

	string model = MEnv::Instance()->model;
	fitDir_ = xpf(baseDir_ + model + "\\fit\\" + targetName_ + "\\");
	predDir_ = xpf(fitDir_ + "preds\\");
	predDirIns_ = xpf(fitDir_ + "predsIns\\");
	mkd(fitDir_);
	mkd(predDir_);
}

MFitISLE::~MFitISLE()
{}

void MFitISLE::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	if( ntrees_ == 0 )
	{
		if( sigType_ == "om" )
			ntrees_ = linMod.nTreesOmFit;
		else if( sigType_ == "tm" )
			ntrees_ = nonLinMod.nTreesTmFit;
	}
	vector<int> idates = MEnv::Instance()->idates;
	int udate = MEnv::Instance()->udate;
	vector<int>::iterator itU = lower_bound(idates.begin(), idates.end(), udate);

	fitDates_ = vector<int>(idates.begin(), itU);
	oosDates_ = vector<int>(itU, idates.end());
	if( fitDates_.size() > 0 )
		cout << "Fit from " << fitDates_[0] << " to " << fitDates_[fitDates_.size() - 1] << endl;
	if( oosDates_.size() > 0 )
		cout << "OOS from " << oosDates_[0] << " to " << oosDates_[oosDates_.size() - 1] << endl;

	pre_read_data(oosDates_, oosOffset_, vvInputTargetOOS_, vvSpectatorOOS_);
}

void MFitISLE::beginDay()
{
	int idate = MEnv::Instance()->idate;
	read_data(idate, vvInputTargetOOS_, vvSpectatorOOS_, oosOffset_);
}

void MFitISLE::beginTicker(const string& ticker)
{
}

void MFitISLE::endTicker(const string& ticker)
{
}

void MFitISLE::endDay()
{
}

void MFitISLE::endJob()
{
	if( do_fit_ )
		fit();
	if( do_analyze_ins_ )
		analyze("ins");
	if( do_analyze_ )
		analyze("oos");
}

void MFitISLE::pre_read_data( vector<int>& dates, map<int, pair<int, int> >& mOffset, vector<vector<float> >& vvInput, vector<vector<float> >& vvSpect )
{
	string model = MEnv::Instance()->model;

	int cnt = 0;
	mOffset.clear();
	for( vector<int>::iterator it = dates.begin(); it != dates.end(); ++it )
	{
		int idate = *it;
		string path = get_sig_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitDesc_);
		ifstream ifs(path.c_str(), ios::binary);
		if( ifs.is_open() )
		{
			hff::SignalLabel sh;
			ifs >> sh;
			if( inputIndx_.empty() )
				get_names(sh);

			if( ifs.rdstate() == 0 )
				cnt += sh.nrows;
			mOffset.insert(make_pair( idate, make_pair(cnt - sh.nrows, cnt) ));
		}
	}

	vvInput.resize(nInput_ + 1);
	for( int i = 0; i < nInput_ + 1; ++i )
		vvInput[i].resize(cnt);

	vvSpect.resize(nSpectator_);
	for( int i = 0; i < nSpectator_; ++i )
		vvSpect[i].resize(cnt);
}

void MFitISLE::get_names(hff::SignalLabel& sh)
{
	vector<string> sigNames = sh.labels;

	for( vector<string>::iterator it = inputNames_.begin(); it != inputNames_.end(); ++it )
	{
		string inputName = *it;
		int indx = -1;
		vector<string>::iterator it2 = find(sigNames.begin(), sigNames.end(), inputName);
		if( it2 != sigNames.end() )
			indx = distance(sigNames.begin(), it2);

		if( indx >= 0 )
			inputIndx_.push_back(indx);
		else
			exit(5);
	}

	vector<string>::iterator itT = find(sigNames.begin(), sigNames.end(), targetName_);
	if( itT != sigNames.end() )
	{
		int indx = distance(sigNames.begin(), itT);

		if( indx >= 0 )
			targetIndx_ = indx;
		else
			exit(5);
	}

	for( vector<string>::iterator it = spectatorNames_.begin(); it != spectatorNames_.end(); ++it )
	{
		string spectatorName = *it;
		int indx = -1;
		vector<string>::iterator it2 = find(sigNames.begin(), sigNames.end(), spectatorName);
		if( it2 != sigNames.end() )
			indx = distance(sigNames.begin(), it2);

		if( indx >= 0 )
			spectatorIndx_.push_back(indx);
		else
			exit(5);
	}
}

void MFitISLE::fit()
{
	int udate = MEnv::Instance()->udate;

	FitISLE fitIsle(ntrees_, shrinkage_, maxNodes_, minPtsNode_, treeMaxLevels_, nthreads_, step_);
	fitIsle.setDir(fitDir_, udate);

	// Base learners.
	for( int iTree = 0; iTree < ntrees_; ++iTree )
	{
		vector<int> fitDates = sample_fitDates();

		pre_read_data(fitDates, fitOffset_, vvInputTarget_, vvSpectator_);
		for( vector<int>::iterator it = fitDates.begin(); it != fitDates.end(); ++it )
			read_data(*it, vvInputTarget_, vvSpectator_, fitOffset_);

		fitIsle.fit(iTree, &vvInputTarget_, &vvInputTargetOOS_);
	}

	// Post processing.
	pre_read_data(fitDates_, fitOffset_, vvInputTarget_, vvSpectator_);
	for( vector<int>::iterator it = fitDates_.begin(); it != fitDates_.end(); ++it )
		read_data(*it, vvInputTarget_, vvSpectator_, fitOffset_);
	fitIsle.postProcess(vvInputTarget_, vvInputTargetOOS_);
}

vector<int> MFitISLE::sample_fitDates()
{
	int nFitDates = fitDates_.size();
	int nSample = nFitDates * rat_sample_;
	set<int> pool(fitDates_.begin(), fitDates_.end());
	vector<int> selected;
	for( int i = 0; i < nSample; ++i )
	{
		int nPool = pool.size();
		int iSel = Random::Instance()->next_int(nPool);
		set<int>::iterator itSel = pool.begin();
		advance(itSel, iSel);
		selected.push_back( *itSel );
		pool.erase(itSel);
	}
	sort(selected.begin(), selected.end());
	return selected;
}

void MFitISLE::read_data(int idate, vector<vector<float> >& vvInput, vector<vector<float> >& vvSpect, map<int, pair<int, int> >& mOffset)
{
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;

	string path = get_sig_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitDesc_);
	ifstream ifs(path.c_str(), ios::binary);
	if( ifs.is_open() )
	{
		hff::SignalLabel sh;
		ifs >> sh;

		hff::SignalContent aSignal(sh.ncols);
		int nrow_day = 0;

		if( mOffset.count(idate) )
		{
			int dayOffset = mOffset[idate].first;
			if( dayOffset >= 0 )
			{
				for( int r = 0; r < sh.nrows && ifs.rdstate() == 0; ++r )
				{
					ifs >> aSignal;

					// inputs
					for( int i=0; i<nInput_; ++i )
						vvInput[i][dayOffset + r] = aSignal.v[inputIndx_[i]];

					// target
					vvInput[nInput_][dayOffset + r] = aSignal.v[targetIndx_];

					// spectator
					for( int i=0; i<nSpectator_; ++i )
						vvSpect[i][dayOffset + r] = aSignal.v[spectatorIndx_[i]];

					++nrow_day;
				}
				cout << idate << " " << nrow_day << endl;
			}
		}
	}
}

void MFitISLE::analyze(string sample)
{
	vector<int> idates = MEnv::Instance()->idates;
	int udate = MEnv::Instance()->udate;
	const string& model = MEnv::Instance()->model;

	// Test dates.
	vector<int> insDates = vector<int>( idates.begin(), lower_bound(idates.begin(), idates.end(), udate) );
	vector<int> oosDates = vector<int>( lower_bound(idates.begin(), idates.end(), udate), idates.end() );

	DTBoost dtBoost(ntrees_, addBMtoTarget_);
	dtBoost.setDir(fitDir_, udate);

	if( "ins" == sample )
		dtBoost.analyze(insDates, vvInputTarget_, vvSpectator_, fitOffset_, model, "ins"); // ins
	else if( "oos" == sample )
		dtBoost.analyze(oosDates, vvInputTargetOOS_, vvSpectatorOOS_, oosOffset_, model, "oos"); // oos
}
