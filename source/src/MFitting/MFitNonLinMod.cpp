#include <MFitting/MFitNonLinMod.h>
#include <MFitting.h>
#include <MFitObj.h>
#include <gtlib_model/pathFtns.h>
#include <MFramework.h>
#include <MFitObj/CorrInput.h>
#include <map>
#include <string>
#include <algorithm>
#include <thread>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
using namespace std;
using namespace gtlib;

MFitNonLinMod::MFitNonLinMod(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	debug_corr_ticker_(false),
	nSpectator_(4),
	fitDesc_(MEnv::Instance()->fitDesc),
	sigType_(MEnv::Instance()->sigType),
	do_corr_(true),
	do_fit_(false),
	do_analyze_ins_(false),
	do_analyze_(false),
	read_multithread_(false),
	rollingModelDate_(false),
	cntFitData_(0),
	cntOOSData_(0),
	nTrees_(0),
	evQuantile_(0.01),
	minPtsNode_(5000),
	nthreads_(70),
	maxNodes_(500),
	shrinkage_(0.1),
	treeMaxLevels_(1000),
	marketNumber_(107),
	zmin_(0),
	zmax_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugCorrTicker") )
		debug_corr_ticker_ = conf.find("debugCorrTicker")->second == "true";
	if( conf.count("doCorr") )
		do_corr_ = conf.find("doCorr")->second == "true";
	if( conf.count("doFit") )
		do_fit_ = conf.find("doFit")->second == "true";
	if( conf.count("doAnalyze") )
		do_analyze_ = conf.find("doAnalyze")->second == "true";
	if( conf.count("doAnalyzeIns") )
		do_analyze_ins_ = conf.find("doAnalyzeIns")->second == "true";
	if( conf.count("readMultithread") )
		read_multithread_ = conf.find("readMultithread")->second == "true";
	if( conf.count("rollingModelDate") )
		rollingModelDate_ = conf.find("rollingModelDate")->second == "true";
	if( conf.count("nthreads") )
		nthreads_ = atoi(conf.find("nthreads")->second.c_str());
	if( conf.count("nTrees") )
		nTrees_ = atoi(conf.find("nTrees")->second.c_str());
	if( conf.count("evQuantile") )
		evQuantile_ = atoi(conf.find("evQuantile")->second.c_str());
	if( conf.count("minPtsNode") )
		minPtsNode_ = atoi(conf.find("minPtsNode")->second.c_str());
	if( conf.count("smodel") )
		smodel_ = conf.find("smodel")->second;
	if( conf.count("coefModel") )
		coefModel_ = conf.find("coefModel")->second;

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

	if( conf.count("zeroInput") )
	{
		pair<mmit, mmit> inputs = conf.equal_range("zeroInput");
		for( mmit mi = inputs.first; mi != inputs.second; ++mi )
		{
			string name = mi->second;
			zeroInputs_.push_back(name);
		}
	}

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

	// Spectator Names.
	//if( conf.count("spectator") )
	//{
	//	pair<mmit, mmit> spectators = conf.equal_range("spectator");
	//	for( mmit mi = spectators.first; mi != spectators.second; ++mi )
	//	{
	//		string name = mi->second;
	//		spectatorNames_.push_back(name);
	//	}
	//}
	//nSpectator_ = spectatorNames_.size();

	//sigType_ = "om";
	int max_horizon = get_max_horizon(targetNames_);
	if( max_horizon <= 10 * 60 ) // ten minutes in seconds.
	{
		//sigType_ = "om";
		modelNumber_ = marketNumber_ * 10;
		zmin_ = 6;
		zmax_ = 6;
		addBMtoTarget_ = 1;
	}
	else
	{
		//sigType_ = "tm";
		modelNumber_ = marketNumber_ * 100;
		zmin_ = 10;
		zmax_ = 13;
		addBMtoTarget_ = 1; // BM will be zero if no linear model.
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

MFitNonLinMod::~MFitNonLinMod()
{}

void MFitNonLinMod::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	timer_.restart();

	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;
	string baseDir = MEnv::Instance()->baseDir;
	if( smodel_.empty() )
		smodel_ = model;
	if( coefModel_.empty() )
		coefModel_ = model;

	// dirs.
	fitDir_ = get_fit_dir(baseDir, model, targetName_, fitDesc_);
	statDir_ = fitDir_ + xpf("\\stat_") + itos(udate);
	//coefDir_ = fitDir_ + xpf("\\coef");
	predDir_ = statDir_ + xpf("\\preds\\");
	predDirIns_ = statDir_ + xpf("\\predsIns\\");
	mkd(statDir_);
	mkd(predDir_);

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	if( nTrees_ == 0 )
	{
		if( sigType_ == "om" )
			nTrees_ = linMod.nTreesOmFit;
		else if( sigType_ == "tm" )
			nTrees_ = nonLinMod.nTreesTmFit;
	}
	vector<int> idates = MEnv::Instance()->idates;
	auto itU = lower_bound(idates.begin(), idates.end(), udate);

	vector<int> fitdates;
	if( do_corr_ || do_fit_ || do_analyze_ins_ )
		fitdates = vector<int>(idates.begin(), itU);
	vector<int> oosdates(itU, idates.end());

	// fitdates.
	for( auto it = begin(fitdates); it != end(fitdates); ++it )
	{
		int idate = *it;
		string path = get_sig_path(MEnv::Instance()->baseDir, smodel_, idate, sigType_, fitDesc_);
		ifstream ifs(path.c_str(), ios::binary);
		if( ifs.is_open() )
		{
			hff::SignalLabel sh;
			ifs >> sh;

			if( ifs.rdstate() == 0 )
				cntFitData_ += sh.nrows;
			fitOffset_.insert(make_pair( idate, make_pair(cntFitData_ - sh.nrows, cntFitData_) ));
		}
		else
		{
			cout << path << " is not open.\n";
			fitOffset_.insert(make_pair( idate, make_pair(cntFitData_, cntFitData_) ));
		}
	}
	if( fitdates.size() > 0 )
		cout << "Fit from " << fitdates[0] << " to " << fitdates[fitdates.size() - 1] << endl;

	// oosdates.
	for( auto it = oosdates.begin(); it != oosdates.end(); ++it )
	{
		int idate = *it;
		string path = get_sig_path(MEnv::Instance()->baseDir, smodel_, idate, sigType_, fitDesc_);
		ifstream ifs(path.c_str(), ios::binary);
		if( ifs.is_open() )
		{
			hff::SignalLabel sh;
			ifs >> sh;

			if( ifs.rdstate() == 0 )
				cntOOSData_ += sh.nrows;
			oosOffset_.insert(make_pair( idate, make_pair(cntOOSData_ - sh.nrows, cntOOSData_) ));
		}
		else
		{
			cout << path << " is not open.\n";
			oosOffset_.insert(make_pair( idate, make_pair(cntOOSData_, cntOOSData_) ));
		}
	}
	if( oosdates.size() > 0 )
		cout << "OOS from " << oosdates[0] << " to " << oosdates[oosdates.size() - 1] << endl;

	cout << nInput_ << "\t" << nSpectator_ << "\t" << cntFitData_ << "\t" << cntOOSData_ << endl;
	vvInputTarget_ = vector<vector<float> >(nInput_ + 1, vector<float>(cntFitData_));
	vvSpectator_ = vector<vector<float> >(nSpectator_, vector<float>(cntFitData_));
	vvInputTargetOOS_ = vector<vector<float> >(nInput_ + 1, vector<float>(cntOOSData_));
	vvSpectatorOOS_ = vector<vector<float> >(nSpectator_, vector<float>(cntOOSData_));

	vvInputTargetIndx_ = vector<vector<int> >(nInput_, vector<int>(cntFitData_));

	if( read_multithread_ )
	{
		cout << "Reading data multithread..." << flush;
		int nMaxThread = MEnv::Instance()->nMaxThreadTicker;
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int iThread = 0; iThread < nMaxThread; ++iThread )
		{
			listThread.push_back(
					boost::shared_ptr<boost::thread>(
						new boost::thread(boost::bind(&MFitNonLinMod::loop_dates_thread, this, iThread))
						)
					);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
		cout << " done" << endl;;
	}
}

void MFitNonLinMod::beginDay()
{
	if( !read_multithread_ )
	{
		int idate = MEnv::Instance()->idate;
		read_data(idate);
	}
}

void MFitNonLinMod::endDay()
{
}

void MFitNonLinMod::endJob()
{
	int elapsed = timer_.elapsed();

	int hh = int(elapsed / 3600);
	elapsed -= hh * 3600;
	int mm = int(elapsed / 60);
	elapsed -= mm * 60;

	char buf[200];
	sprintf(buf, "%s %3d:%02d:%02d\n", "Elapsed:", hh, mm, elapsed);
	cout << buf;
	cout << flush;

    vector<thread> vThread;
	if( do_corr_ )
	{
		cout << "Correlation ..." << endl;
		vThread.push_back(thread(&MFitNonLinMod::corr_input, this));
	}
	if( do_fit_ )
	{
		cout << "Fitting ..." << endl;
		fit();
	}
	if( do_analyze_ins_ )
	{
		cout << "Analyzing ..." << endl;
		analyze("ins");
	}
	if( do_analyze_ )
	{
		cout << "Analyzing ..." << endl;
		analyze("oos");
	}

	for( auto& t : vThread )
		t.join();
}

int MFitNonLinMod::get_max_horizon(const vector<string>& targetNames)
{
	int max_horizon = 0;
	for( auto it = begin(targetNames); it != end(targetNames); ++it )
	{
		int N = it->size();
		int pos_semicolon = it->find(";");
		if( N > 3 && it->substr(0, 3) == "tar" )
		{
			int horizon = atoi(it->substr(3, pos_semicolon - 3).c_str()) + atoi(it->substr(pos_semicolon + 1, N - pos_semicolon).c_str());
			if( horizon > max_horizon )
				max_horizon = horizon;
		}
		else if( N > 6 && it->substr(0, 6) == "restar" )
		{
			int horizon = atoi(it->substr(3, pos_semicolon - 6).c_str()) + atoi(it->substr(pos_semicolon + 1, N - pos_semicolon).c_str());
			if( horizon > max_horizon )
				max_horizon = horizon;
		}
	}
	return max_horizon;
}

void MFitNonLinMod::get_names(hff::SignalLabel& sh)
{
	vector<string> sigNames = sh.labels;
	inputIndx_.clear();
	inputMask_.clear();
	targetIndx_.clear();
	spectatorIndx_.clear();

	for( vector<string>::iterator it = inputNames_.begin(); it != inputNames_.end(); ++it )
	{
		string inputName = *it;
		int indx = -1;
		vector<string>::iterator it2 = find(sigNames.begin(), sigNames.end(), inputName);
		if( it2 != sigNames.end() )
			indx = distance(sigNames.begin(), it2);

		if( indx >= 0 )
		{
			inputIndx_.push_back(indx);
			if( find(begin(zeroInputs_), end(zeroInputs_), inputName) != begin(zeroInputs_) )
				inputMask_.push_back(0);
			else
				inputMask_.push_back(1);
		}
		else
		{
			cout << "input " << inputName << " not found.\n";
			exit(5);
		}
	}

	for( vector<string>::iterator it = targetNames_.begin(); it != targetNames_.end(); ++it )
	{
		string targetName = *it;
		int indx = -1;
		vector<string>::iterator it2 = find(sigNames.begin(), sigNames.end(), targetName);
		if( it2 != sigNames.end() )
			indx = distance(sigNames.begin(), it2);

		if( indx >= 0 )
			targetIndx_.push_back(indx);
		else
		{
			cout << "target " << targetName << " not found.\n";
			exit(5);
		}
	}

	// Get spectatorIndx_ from spectatorNames_.
	//for( vector<string>::iterator it = spectatorNames_.begin(); it != spectatorNames_.end(); ++it )
	//{
	//	string spectatorName = *it;
	//	int indx = -1;
	//	vector<string>::iterator it2 = find(sigNames.begin(), sigNames.end(), spectatorName);
	//	if( it2 != sigNames.end() )
	//		indx = distance(sigNames.begin(), it2);

	//	if( indx >= 0 )
	//		spectatorIndx_.push_back(indx);
	//	else
	//	{
	//		cout << "spectator " << spectatorName << " not found.\n";
	//		exit(5);
	//	}
	//}

	// spectatorIndx_ depends on the target. If there are multiple targets, spectatorIndx is multi-dimensional.
	vector<vector<string> > spectatorNames(nTargets_, vector<string>(nSpectator_));
	for( int iT = 0; iT < nTargets_; ++iT )
	{
		string targetName = targetNames_[iT];

		// 0: target.
		spectatorNames[iT][0] = targetName;

		// 1: bmpred.
		string bmpred_name = get_bmpred_name(targetName);
		spectatorNames[iT][1] = bmpred_name;

		// 2: total pred.
		spectatorNames[iT][2] = ""; // bmpred will be assigned, pred will be added in DTBoost::analyze().

		// 3: sprd.
		spectatorNames[iT][3] = "sprd";
	}

	spectatorIndx_ = vector<vector<int> >(nTargets_, vector<int>(nSpectator_, -1));
	for( int iT = 0; iT < nTargets_; ++iT )
	{
		for( int iS = 0; iS < nSpectator_; ++iS )
		{
			string spectatorName = spectatorNames[iT][iS];
			if( !spectatorName.empty() )
			{
				vector<string>::iterator it2 = find(sigNames.begin(), sigNames.end(), spectatorName);
				if( it2 != sigNames.end() )
				{
					int indx = distance(sigNames.begin(), it2);
					spectatorIndx_[iT][iS] = indx;
				}
				else
				{
					cout << "spectator " << spectatorName << " not found.\n";
					exit(5);
				}
			}
		}
	}
}

string MFitNonLinMod::get_bmpred_name(string targetName)
{
	string bmpred_name;
	int N = targetName.size();
	if( N > 6 && targetName.substr(0, 6) == "restar" )
		bmpred_name = targetName.replace(0, 6, "bmpred");
	return bmpred_name;
}

void MFitNonLinMod::read_data(int idate)
{
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;
	bool is_oos = idate >= udate;

	string path = get_sig_path(MEnv::Instance()->baseDir, smodel_, idate, sigType_, fitDesc_);
	ifstream ifs(path.c_str(), ios::binary);
	if( ifs.is_open() )
	{
		cout << "Reading " << path << endl;
		hff::SignalLabel sh;
		ifs >> sh;
		get_names(sh);

		hff::SignalContent aSignal(sh.ncols);
		int nrow_day = 0;

		int dayOffset = -1;
		if( oosOffset_.count(idate) )
			dayOffset = oosOffset_[idate].first;
		else if( fitOffset_.count(idate) )
			dayOffset = fitOffset_[idate].first;

		if( dayOffset >= 0 )
		{
			if( is_oos )
			{
				for( int r = 0; r < sh.nrows && ifs.rdstate() == 0; ++r )
				{
					ifs >> aSignal;

					// inputs
					for( int i=0; i<nInput_; ++i )
						vvInputTargetOOS_[i][dayOffset + r] = aSignal.v[inputIndx_[i]] * inputMask_[i];

					// target
					vvInputTargetOOS_[nInput_][dayOffset + r] = 0.;
					for( int i = 0; i < nTargets_; ++i )
						vvInputTargetOOS_[nInput_][dayOffset + r] += aSignal.v[targetIndx_[i]] * targetWeights_[i];

					// spectator. 0: target, 1: bmpred, 2: total pred, 3: sprd.
					//vvSpectatorOOS_[0][dayOffset + r] = vvInputTargetOOS_[nInput_][dayOffset + r]; // target.
					for( int iT = 0; iT < nTargets_; ++iT )
					{
						for( int i = 0; i < nSpectator_; ++i )
						{
							int sIndx = spectatorIndx_[iT][i];
							if( sIndx >= 0 )
							{
								if( i == 0 )
									vvSpectatorOOS_[i][dayOffset + r] += aSignal.v[sIndx] * targetWeights_[iT]; // target.
								else if( i == 1 || i == 2 )
									vvSpectatorOOS_[i][dayOffset + r] += aSignal.v[sIndx] * targetWeights_[iT];
								else
									vvSpectatorOOS_[i][dayOffset + r] = aSignal.v[sIndx];
							}
						}
					}

					++nrow_day;
				}
			}
			else
			{
				for( int r = 0; r < sh.nrows && ifs.rdstate() == 0; ++r )
				{
					ifs >> aSignal;

					// inputs
					for( int i=0; i<nInput_; ++i )
						vvInputTarget_[i][dayOffset + r] = aSignal.v[inputIndx_[i]];

					// target
					vvInputTarget_[nInput_][dayOffset + r] = 0.;
					for( int i = 0; i < nTargets_; ++i )
						vvInputTarget_[nInput_][dayOffset + r] += aSignal.v[targetIndx_[i]] * targetWeights_[i];

					// spectator. 0: target, 1: bmpred, 2: total pred, 3: sprd.
					//vvSpectator_[0][dayOffset + r] = vvInputTarget_[nInput_][dayOffset + r]; // target.
					//for( int i = 1; i < nSpectator_; ++i )
					//{
					//	int sIndx = spectatorIndx_[i];
					//	if( sIndx >= 0 )
					//		vvSpectator_[i][dayOffset + r] = aSignal.v[sIndx];
					//}
					for( int iT = 0; iT < nTargets_; ++iT )
					{
						for( int i = 0; i < nSpectator_; ++i )
						{
							int sIndx = spectatorIndx_[iT][i];
							if( sIndx >= 0 )
							{
								if( i == 0 )
									vvSpectator_[i][dayOffset + r] += aSignal.v[sIndx] * targetWeights_[iT]; // target.
								else if( i == 1 || i == 2 )
									vvSpectator_[i][dayOffset + r] += aSignal.v[sIndx] * targetWeights_[iT];
								else
									vvSpectator_[i][dayOffset + r] = aSignal.v[sIndx];
							}
						}
					}

					++nrow_day;
				}
			}
			cout << idate << " " << nrow_day << endl;
		}
	}
	else
		cout << path << " is not open." << endl;
}

void MFitNonLinMod::loop_dates_thread(int iThread)
{
	vector<int>& dates = MEnv::Instance()->idates;
	int ndates = dates.size();
	int nMaxThread = MEnv::Instance()->nMaxThreadTicker;
	int nDateEachThread = ceil((float)ndates / nMaxThread);
	vector<int>::iterator itBeg = dates.begin();

	int offset1 = iThread * nDateEachThread;
	if( offset1 < ndates )
	{
		int offset2 = (iThread + 1) * nDateEachThread;
		if( offset2 > ndates || iThread == nMaxThread - 1 )
			offset2 = ndates;

		if( offset2 > offset1 )
		{
			vector<int>::iterator itFrom = itBeg + offset1;
			vector<int>::iterator itTo = itBeg + offset2;

			for( vector<int>::iterator it = itFrom; it != itTo; ++it )
			{
				int idate = *it;
				read_data(idate);
			}
		}
	}
}

void MFitNonLinMod::corr_input()
{
	string model = MEnv::Instance()->model;
	vector<int> idates = MEnv::Instance()->idates;
	int udate = MEnv::Instance()->udate;
	vector<int> insDates = vector<int>( idates.begin(), lower_bound(idates.begin(), idates.end(), udate) );
	if( !insDates.empty() )
	{
		CorrInput ci(&vvInputTarget_, &vvSpectator_, debug_corr_ticker_);
		ci.stat(fitDir_, udate, inputNames_, insDates, fitOffset_, model, sigType_, fitDesc_);
	}
	vector<int> oosDates = vector<int>( lower_bound(idates.begin(), idates.end(), udate), idates.end() );
	if( !oosDates.empty() )
	{
		CorrInput ci(&vvInputTargetOOS_, &vvSpectatorOOS_, debug_corr_ticker_);
		ci.stat(fitDir_, udate, inputNames_, oosDates, oosOffset_, model, sigType_, fitDesc_);
	}
}

void MFitNonLinMod::fit()
{
	int udate = MEnv::Instance()->udate;
	sort_input();

	DTBoost dtBoost(nTrees_, shrinkage_, maxNodes_, minPtsNode_, treeMaxLevels_, nthreads_);
	dtBoost.setData(&vvInputTarget_, &vvInputTargetOOS_, &vvInputTargetIndx_);
	dtBoost.setDir(fitDir_, udate);
	dtBoost.fit();
	dtBoost.stat(inputNames_);
}

void MFitNonLinMod::sort_input()
{
	if( nthreads_ <= 1 )
	{
		for( int i = 0; i < nInput_; ++i )
		{
			if( 1 )
				gsl_heapsort_index<int, float>(vvInputTargetIndx_[i], vvInputTarget_[i]);
			else
				stl_sort_index<int, float>(vvInputTargetIndx_[i], vvInputTarget_[i]);
		}
	}
	else
	{
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int iThread = 0; iThread < nthreads_; ++iThread )
		{
			listThread.push_back(
					boost::shared_ptr<boost::thread>(
						new boost::thread(boost::bind(&MFitNonLinMod::sort_input, this, iThread))
						)
					);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
	}
}

void MFitNonLinMod::sort_input(int iThread)
{
	int nJobEachThread = ceil((double)nInput_ / nthreads_);
	int offset1 = iThread * nJobEachThread;
	if( offset1 < nInput_ )
	{
		int offset2 = (iThread + 1) * nJobEachThread;
		if( offset2 > nInput_ || iThread == nthreads_ - 1 )
			offset2 = nInput_;

		if( offset2 > offset1 )
		{
			for( int i = offset1; i < offset2; ++i )
			{
				if( 1 )
					gsl_heapsort_index<int, float>(vvInputTargetIndx_[i], vvInputTarget_[i]);
				else
					stl_sort_index<int, float>(vvInputTargetIndx_[i], vvInputTarget_[i]);
			}
		}
	}
}

void MFitNonLinMod::analyze(const string& sample)
{
	vector<int> idates = MEnv::Instance()->idates;
	const string& baseDir = MEnv::Instance()->baseDir;
	const string& model = MEnv::Instance()->model;
	int udate = MEnv::Instance()->udate;
	string coefDir = get_fit_dir(baseDir, coefModel_, targetName_, fitDesc_) + "/coef";

	// Test dates.
	vector<int> insDates = vector<int>( idates.begin(), lower_bound(idates.begin(), idates.end(), udate) );
	vector<int> oosDates = vector<int>( lower_bound(idates.begin(), idates.end(), udate), idates.end() );

	DTBoost dtBoost(nTrees_, addBMtoTarget_, evQuantile_);
	dtBoost.setDir(fitDir_, coefDir, udate);

	if( "ins" == sample )
	{
		dtBoost.analyze(insDates, vvInputTarget_, vvSpectator_, fitOffset_, model, "ins"); // ins
		write_prediction(insDates, vvSpectator_, fitOffset_, predDirIns_);
	}
	else if( "oos" == sample )
	{
		dtBoost.analyze(oosDates, vvInputTargetOOS_, vvSpectatorOOS_, oosOffset_, model, "oos", rollingModelDate_); // oos
		write_prediction(oosDates, vvSpectatorOOS_, oosOffset_, predDir_);
	}
}

void MFitNonLinMod::write_prediction(const vector<int>& testDates, const vector<vector<float> >& vvSpectator,
		map<int, pair<int, int> >& mOffset, const string& predDir)
{
	for( auto it = testDates.begin(); it != testDates.end(); ++it )
	{
		int testDate = *it;

		// find offset.
		int offset1 = mOffset[testDate].first;
		int offset2 = mOffset[testDate].second;

		char fout[1000];
		sprintf(fout, "%s\\pred%d.txt", predDir.c_str(), testDate);
		ofstream fpreds(xpf(fout).c_str());
		if( fpreds.is_open() )
		{
			fpreds << "target" << "\t" << "bmpred" << "\t" << "tbpred" << "\t"<< "sprd" <<  endl;
			char buf[400];
			for( int ic = offset1; ic < offset2; ++ic )
			{
				sprintf(buf, "%.4f\t%.4f\t%.4f\t%.4f\n", vvSpectator[0][ic], vvSpectator[1][ic], vvSpectator[2][ic], vvSpectator[3][ic]);
				fpreds << buf;
			}
		}
	}
	//}
}
