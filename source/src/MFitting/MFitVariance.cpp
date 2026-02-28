#include <MFitting/MFitVariance.h>
#include <MFitting.h>
#include <MFramework.h>
#include <map>
#include <string>
//#include <MFitting/HFTrees.h>
//#include <MFitting/HFSTree.h>
#include <boost/filesystem.hpp>
using namespace std;
using namespace writeSig;

MFitVariance::MFitVariance(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	do_fit_(false),
	do_analyze_(false),
	read_multithread_(false),
	sigType_(MEnv::Instance()->sigType),
	cntFitData_(0),
	cntOOSData_(0),
	targetIndx_(-1),
	nTrees_(0),
	minPtsNode_(5000),
	maxLevels_(1000),
	monitor_(1),
	nthreads_(70),
	cattar_(0),
	maxNodes_(500),
	shrinkage_(0.1),
	treeMaxLevels_(1000),
	marketNumber_(107),
	zmin_(0),
	zmax_(0),
	treeStep_(5),
	baseDir_("\\\\smrc-ltc-mrct43\\f\\hffit\\")
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("doFit") )
		do_fit_ = conf.find("doFit")->second == "true";
	if( conf.count("doAnalyze") )
		do_analyze_ = conf.find("doAnalyze")->second == "true";
	//if( conf.count("readMultithread") )
	//	read_multithread_ = conf.find("readMultithread")->second == "true";

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
	if( targetName_.size() >= 6 && targetName_.substr(0, 6) == "restar" )
	{
		//sigType_ = "om";
		modelNumber_ = marketNumber_ * 10;
		zmin_ = 6;
		zmax_ = 6;
		writeDataAux_ = 6;
		addBMtoTarget_ = 1;
		isErrorCor_ = 1;
	}
	else if( targetName_.size() >= 3 && targetName_.substr(0, 3) == "tar" )
	{
		//sigType_ = "tm";
		modelNumber_ = marketNumber_ * 100;
		zmin_ = 10;
		zmax_ = 13;
		writeDataAux_ = 13;
		addBMtoTarget_ = 0;
		isErrorCor_ = 0;
	}

	string model = MEnv::Instance()->model;
	fitDir_ = baseDir_ + model + "\\fitVar\\" + targetName_ + "\\";
	predDir_ = fitDir_ + "preds\\";
	mkd(predDir_);
}

MFitVariance::~MFitVariance()
{}

void MFitVariance::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	if( nTrees_ == 0 )
	{
		if( sigType_ == "om" )
			nTrees_ = linMod.nTrees;
		else if( sigType_ == "tm" )
			nTrees_ = nonLinMod.nTrees;
	}
	string model = MEnv::Instance()->model;
	vector<int> idates = MEnv::Instance()->idates;
	int udate = MEnv::Instance()->udate;
	vector<int>::iterator itU = lower_bound(idates.begin(), idates.end(), udate);

	vector<int> fitdates(idates.begin(), itU);
	vector<int> oosdates(itU, idates.end());

	// fitdates.
	for( vector<int>::iterator it = fitdates.begin(); it != fitdates.end(); ++it )
	{
		int idate = *it;
		string path = get_sig_path(model, idate, sigType_);
		ifstream ifs(path.c_str(), ios::binary);
		if( ifs.is_open() )
		{
			hff::SignalLabel sh;
			ifs >> sh;
			if( inputIndx_.empty() )
				get_names(sh);

			if( ifs.rdstate() == 0 )
				cntFitData_ += sh.nrows;
			fitOffset_.insert(make_pair( idate, make_pair(cntFitData_ - sh.nrows, cntFitData_) ));
		}
	}
	cout << "Fit from " << fitdates[0] << " to " << fitdates[fitdates.size() - 1] << endl;

	// oosdates.
	for( vector<int>::iterator it = oosdates.begin(); it != oosdates.end(); ++it )
	{
		int idate = *it;
		string path = get_sig_path(model, idate, sigType_);
		ifstream ifs(path.c_str(), ios::binary);
		if( ifs.is_open() )
		{
			hff::SignalLabel sh;
			ifs >> sh;
			if( inputIndx_.empty() )
				get_names(sh);

			if( ifs.rdstate() == 0 )
				cntOOSData_ += sh.nrows;
			oosOffset_.insert(make_pair( idate, make_pair(cntOOSData_ - sh.nrows, cntOOSData_) ));
		}
	}
	cout << "OOS from " << oosdates[0] << " to " << oosdates[oosdates.size() - 1] << endl;

	if( do_fit_ && do_analyze_ )
	{
		vvInputTarget_ = vector<vector<float> >(nInput_ + 1, vector<float>(cntFitData_));
		vvInputTargetOOS_ = vector<vector<float> >(nInput_ + 1, vector<float>(cntOOSData_));
		vvSpectatorOOS_ = vector<vector<float> >(nSpectator_, vector<float>(cntOOSData_));
		vPred_ = vector<float>(cntFitData_);
		vPredOOS_ = vector<float>(cntOOSData_);
	}
	else if( do_fit_ && !do_analyze_ )
	{
		vvInputTarget_ = vector<vector<float> >(nInput_ + 1, vector<float>(cntFitData_));
		vvInputTargetOOS_ = vector<vector<float> >(nInput_ + 1, vector<float>(cntOOSData_));
		vPred_ = vector<float>(cntFitData_);
		vPredOOS_ = vector<float>(cntOOSData_);
	}
	else if( do_analyze_ && !do_fit_ )
	{
		vvInputTargetOOS_ = vector<vector<float> >(nInput_ + 1, vector<float>(cntOOSData_));
		vvSpectatorOOS_ = vector<vector<float> >(nSpectator_, vector<float>(cntOOSData_));
		vPredOOS_ = vector<float>(cntOOSData_);
	}

	if( read_multithread_ )
	{
		int nMaxThread = MEnv::Instance()->nMaxThreadTicker;
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int iThread = 0; iThread < nMaxThread; ++iThread )
		{
			listThread.push_back(
					boost::shared_ptr<boost::thread>(
						new boost::thread(boost::bind(&MFitVariance::loop_dates_thread, this, iThread))
						)
					);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
	}
}

void MFitVariance::beginDay()
{
	if( !read_multithread_ )
	{
		int idate = MEnv::Instance()->idate;
		read_pred(idate);
		read_data(idate);
	}
}

void MFitVariance::beginTicker(const string& ticker)
{
}

void MFitVariance::endTicker(const string& ticker)
{
}

void MFitVariance::endDay()
{
}

void MFitVariance::endJob()
{
	if( do_fit_ )
		fit();
	if( do_analyze_ )
		analyze();
}

void MFitVariance::get_names(hff::SignalLabel& sh)
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

void MFitVariance::read_data(int idate)
{
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;

	string path = get_sig_path(model, idate, sigType_);
	ifstream ifs(path.c_str(), ios::binary);
	if( ifs.is_open() )
	{
		hff::SignalLabel sh;
		ifs >> sh;

		hff::SignalContent aSignal(sh.ncols);
		int nrow_day = 0;

		bool is_oos = idate >= udate;
		int dayOffset = -1;
		if( oosOffset_.count(idate) )
			dayOffset = oosOffset_[idate].first;
		else if( fitOffset_.count(idate) )
			dayOffset = fitOffset_[idate].first;

		if( dayOffset >= 0 )
		{
			if( do_fit_ && do_analyze_ )
			{
				if( is_oos )
				{
					for( int r = 0; r < sh.nrows && ifs.rdstate() == 0; ++r )
					{
						ifs >> aSignal;

						// inputs
						for( int i=0; i<nInput_; ++i )
							vvInputTargetOOS_[i][dayOffset + r] = aSignal.v[inputIndx_[i]];

						// target
						double target = aSignal.v[targetIndx_];
						double pred = vPredOOS_[dayOffset + r];
						vvInputTargetOOS_[nInput_][dayOffset + r] = (target - pred) * (target - pred);

						// spectator
						for( int i=0; i<nSpectator_; ++i )
						{
							if( i == 0 )
								vvSpectatorOOS_[i][dayOffset + r] = (target - pred) * (target - pred);
							else
								vvSpectatorOOS_[i][dayOffset + r] = aSignal.v[spectatorIndx_[i]];
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
						double target = aSignal.v[targetIndx_];
						double pred = vPred_[dayOffset + r];
						vvInputTarget_[nInput_][dayOffset + r] = (target - pred) * (target - pred);

						++nrow_day;
					}
				}
			}
			else if( do_fit_ && !do_analyze_ )
			{
				if( is_oos )
				{
					for( int r = 0; r < sh.nrows && ifs.rdstate() == 0; ++r )
					{
						ifs >> aSignal;

						// inputs
						for( int i=0; i<nInput_; ++i )
							vvInputTargetOOS_[i][dayOffset + r] = aSignal.v[inputIndx_[i]];

						// target
						double target = aSignal.v[targetIndx_];
						double pred = vPredOOS_[dayOffset + r];
						vvInputTargetOOS_[nInput_][dayOffset + r] = (target - pred) * (target - pred);

						//++cntOOSData_;
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
						double target = aSignal.v[targetIndx_];
						double pred = vPred_[dayOffset + r];
						vvInputTarget_[nInput_][dayOffset + r] = (target - pred) * (target - pred);

						++nrow_day;
					}
				}
			}
			else if( do_analyze_ && !do_fit_ )
			{
				if( is_oos )
				{
					for( int r = 0; r < sh.nrows && ifs.rdstate() == 0; ++r )
					{
						ifs >> aSignal;

						// inputs
						for( int i=0; i<nInput_; ++i )
							vvInputTargetOOS_[i][dayOffset + r] = aSignal.v[inputIndx_[i]];

						// target
						double target = aSignal.v[targetIndx_];
						double pred = vPredOOS_[dayOffset + r];
						vvInputTargetOOS_[nInput_][dayOffset + r] = (target - pred) * (target - pred);

						// spectator
						for( int i=0; i<nSpectator_; ++i )
						{
							if( i == 0 )
								vvSpectatorOOS_[i][dayOffset + r] = (target - pred) * (target - pred);
							else
								vvSpectatorOOS_[i][dayOffset + r] = aSignal.v[spectatorIndx_[i]];
						}

						++nrow_day;
					}
				}
			}
			cout << idate << " " << nrow_day << endl;
		}
	}
}

void MFitVariance::read_pred(int idate)
{
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;

	bool is_oos = idate >= udate;
	int dayOffset = -1;
	int nrows = -1;
	if( oosOffset_.count(idate) )
	{
		dayOffset = oosOffset_[idate].first;
		nrows = oosOffset_[idate].second - oosOffset_[idate].first;
	}
	else if( fitOffset_.count(idate) )
	{
		dayOffset = fitOffset_[idate].first;
		nrows = fitOffset_[idate].second - fitOffset_[idate].first;
	}

	string path = get_pred_path(model, idate, targetName_);
	ifstream ifs(path.c_str());
	if( ifs.is_open() )
	{
		string line;
		getline(ifs, line); // first line.
		int ncol = split(line, '\t').size();

		hff::Prediction aPred(ncol);

		for( int r = 0; r < nrows && ifs.rdstate() == 0; ++r )
		{
			ifs >> aPred;

			// Subtract bmpred if appropriate.
			double pred = aPred.v[2];
			if( sigType_ == "om" )
				pred -= aPred.v[1];

			if( is_oos )
				vPredOOS_[dayOffset + r] = pred;
			else
				vPred_[dayOffset + r] = pred;
		}
	}
}

void MFitVariance::loop_dates_thread(int iThread)
{
	vector<int> dates = MEnv::Instance()->idates;
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
				read_pred(idate);
				read_data(idate);
			}
		}
	}
}

void MFitVariance::link_fit_data(MATRIXSHORT& data, vector<vector<float> >& vv, int offset1, int offset2)
{
	int nrow = vv.size();
	int ncol = vv[0].size();
	if( offset1 >= 0 && offset2 > offset1 )
		ncol = offset2 - offset1;

	data.elemF = new float*[nrow];
	for( int i = 0; i < nrow; ++i )
		data.elemF[i] = &vv[i][offset1];

	data.rows = nrow;
	data.cols = ncol;
	data.target = 0;
	data.elem = 0;
	data.rpartSorts = 0;
	data.rpartWhich = 0;

	if( nrow == nInput_ + 1 )
	{
		data.rowlabel = new char* [nInput_ + 1];
		for( int i = 0; i < nInput_ + 1; ++i )
			data.rowlabel[i] = new char [20];

		for( int i = 0; i < nInput_; ++i )
			strcpy(data.rowlabel[i], inputNames_[i].c_str());
		strcpy(data.rowlabel[nInput_], "target");
	}
	else if( nrow == nSpectator_ )
	{
		data.rowlabel = new char* [nSpectator_];
		for( int i = 0; i < nSpectator_; ++i )
			data.rowlabel[i] = new char [20];

		for( int i = 0; i < nSpectator_; ++i )
			strcpy(data.rowlabel[i], spectatorNames_[i].c_str());
	}
}

void MFitVariance::fit()
{
	int udate = MEnv::Instance()->udate;

	TreeBoost treeBoost(fitDir_, predDir_, nInput_, nTrees_, shrinkage_, maxNodes_, minPtsNode_, treeMaxLevels_, modelNumber_, nthreads_, cattar_);
	treeBoost.dI.uDate = udate;

	link_fit_data(treeBoost.dataFit, vvInputTarget_);
	link_fit_data(treeBoost.dataTest, vvInputTargetOOS_);

	treeBoost.dI.sortDataRows(treeBoost.dataFit, treeBoost.nThreads);

	treeBoost.makeTrees();

	// for performance monitoring 
	MATRIX *z;
	z = MakeMatrix(1,9);    	

	char buf[1000]; 	
	sprintf(buf, "%s\\coef%d%s.txt", treeBoost.statsDir.c_str(), udate, treeBoost.dI.market.c_str());

	vector<float> predsFit;
	vector<float> predsTst; 
	if(treeBoost.dataTest.cols > 0)
	{
		predsFit.resize(treeBoost.dataFit.cols);
		predsTst.resize(treeBoost.dataTest.cols);
	}

	// loop over ntrees
	char fsigs[1000];
	for(int i=0;i<treeBoost.nTrees;i++)
	{
		if(i>0 && treeBoost.catTar==0) 
			treeBoost.calcResiduals(i);

		// look at oos perf 
		int count5 = (i) % 5;
		if(treeBoost.dataTest.cols > 0 && (count5 == 0 || i == treeBoost.nTrees -1))
		{
			sprintf(fsigs, "%s\\perf%d%d.txt", treeBoost.statsDir.c_str(), udate, marketNumber_);
			TreeBoostPerformance(z, &(treeBoost.dataFit), &(treeBoost.dataTest), &(treeBoost.trees), 0, i-1, i, &(predsFit[0]), &(predsTst[0]) );
			if(i==0)
				MccDumpMatrixNew2(z,fsigs,"",10,"w");	//w=write
			else
				MccDumpMatrixNew2(z,fsigs,"",10,"a");	//a=append
		}

		// fit tree for tree boosting
		treeBoost.fitTree(monitor_,i);
		TnodeDumpFname2(treeBoost.trees.elem[i],buf,treeBoost.trees.wts[i],i);  // write tree by tree 

		cout << "fit tree number  " << i << endl;	
	}
	// calc stats and write coefs
	cout << "Calc Stats" << endl; 
	treeBoost.calcStats(); 
	cout << "Write Coefs" << endl; 
	treeBoost.writeFile();  // write all at once  (overwrites the other one but that's fine)
}

void MFitVariance::analyze()
{
	int isSingleTree = 0;

	vector<int> idates = MEnv::Instance()->idates;
	int udate = MEnv::Instance()->udate;
	char fout[BUFFSIZE];

	sprintf(fout,"%soos%d%d.txt",predDir_.c_str(),udate,marketNumber_);
	ofstream fsWrite(fout);
	fsWrite.precision(6);
	fsWrite << "date" << "\t" << "sprd" << "\t" << "cbm" << "\t"<< "ctb" << "\t"<< "ebm" << "\t"<< 
		"etb" << "\t"<< "ofbm" << "\t"<< "oftb" << "\t"<<  "malpredbm" << "\t"<< "malpredtb" << "\t"<< "npts" << "\t"<< "nTrees" << 
		"\t" << "sumMEVbm" << "\t" << "sumMEV" << "\t" << "sumBiasbm" << "\t" <<  "sumBias" << "\t" "nTradablebm" << "\t" "nTradable" << endl;

	TreeBoost treeBoost(fitDir_, predDir_, nInput_, nTrees_, shrinkage_,
			maxNodes_, minPtsNode_, treeMaxLevels_, modelNumber_, nthreads_, cattar_);

	cout << "Get Test Dates" << endl;
	vector<int> testDates = vector<int>( lower_bound(idates.begin(), idates.end(), udate), idates.end() );
	int nDates=testDates.size();

	treeBoost.dI.uDate = udate;	
	treeBoost.dI.testDates = testDates;
	treeBoost.dI.nTestDays = nDates;

	cout << "Get Model Dates" << endl; 
	vector<int> modelDts;
	{
		treeBoost.getModelDates(modelDts);
		if(modelDts.size()==0)
			return;
	}

	cout << "Start Loop over Test Dates " << endl; 
	for(int u=0;u<nDates;u++)
	{
		cout << "Test Date = " << "\t" << testDates[u] << endl; 
		treeBoost.dI.uDate = treeBoost.getModelDate(modelDts,testDates[u]);

		if(treeBoost.dI.uDate==0)
			return;

		// get oos data; 
		int offset1 = oosOffset_[testDates[u]].first;
		int offset2 = oosOffset_[testDates[u]].second;
		link_fit_data(treeBoost.dataTest, vvInputTargetOOS_, offset1, offset2);
		link_fit_data(treeBoost.dataAux, vvSpectatorOOS_, offset1, offset2);
		treeBoost.dI.totTestPts = offset2 - offset1;

		if(treeBoost.dI.totTestPts==0)
			continue;
		treeBoost.readFile();

		//// modify target if appropriate
		//if(addBMtoTarget_) // add bm to target and treepred NOT 8
		//	for(int i=0;i<treeBoost.dataAux.cols;i++)	
		//		treeBoost.dataAux.elemF[0][i]+=treeBoost.dataAux.elemF[1][i];

		for(int z=zmin_;z<=zmax_;z++)
		{
			if(isSingleTree)
				treeBoost.getPreds(1);
			else
				treeBoost.getPreds(z*treeStep_);

			//if(isErrorCor_) // add bm to treepred
			//	for(int i=0;i<treeBoost.dataAux.cols;i++)
			//		treeBoost.dataAux.elemF[2][i]+=treeBoost.dataAux.elemF[1][i];

			// make tight and wide dataObjs
			statsObject stats;
			int nDrows = treeBoost.dataAux.rows; 
			if(1) // do by sprd 
			{
				dataObject  dataa(nDrows), datat(nDrows),dataw(nDrows);
				int sprdRow=3;
				{
					datat.dH.selectBySprd(treeBoost.dataAux,true,sprdRow,0,20,datat.mat);
					dataw.dH.selectBySprd(treeBoost.dataAux,true,sprdRow,20,100,dataw.mat);
					stats.summary(datat,0,cattar_);
					stats.summary(dataw,0,cattar_);
					fsWrite << testDates[u] << "\t" << "t" << "\t" << datat.cor[1][0] << "\t"<< datat.cor[2][0]<< "\t" << 
						datat.ev[1] << "\t"<< datat.ev[2] << "\t"<< datat.of[1] << "\t"<< datat.of[2] << "\t" <<
						datat.malpred[1] << "\t" << datat.malpred[2] << "\t" << datat.mat.rows <<"\t" << z*treeStep_ << 
						"\t" << datat.mev[1] << "\t" << datat.mev[2] << "\t" << datat.mbias[1] <<  "\t" << datat.mbias[2] <<  "\t" << datat.ntrds[1] << "\t" << datat.ntrds[2] <<  
						endl;
					fsWrite << testDates[u] << "\t" << "w" << "\t" << dataw.cor[1][0] << "\t"<< dataw.cor[2][0]<< "\t" << 
						dataw.ev[1] << "\t"<< dataw.ev[2] << "\t"<< dataw.of[1] << "\t"<< dataw.of[2] << "\t"<<  
						dataw.malpred[1] << "\t" << dataw.malpred[2] << "\t" << dataw.mat.rows <<"\t" << z*treeStep_ <<
						"\t" << dataw.mev[1] << "\t" << dataw.mev[2] << "\t" << dataw.mbias[1] <<  "\t" << dataw.mbias[2] << "\t" << dataw.ntrds[1] << "\t" << dataw.ntrds[2] <<  
						endl;
				}
			}

			if( (isSingleTree && writeDataAux_) || (!isSingleTree && z>=writeDataAux_ ))
			{
				sprintf(fout,"%s%dPred%d%s.txt",predDir_.c_str(),z,testDates[u],treeBoost.dI.market.c_str());
				ofstream fpreds(fout);
				fpreds.precision(6);
				fpreds << "target" << "\t" << "bmpred" << "\t" << "tbpred" << "\t"<< "sprd" <<  endl;

				for(int k=0;k<treeBoost.dataAux.cols;k++)
					fpreds << treeBoost.dataAux.elemF[0][k] << "\t" << treeBoost.dataAux.elemF[1][k] << "\t" << treeBoost.dataAux.elemF[2][k] << "\t"<< treeBoost.dataAux.elemF[3][k]  <<  endl;
			}
		}
	}
}

