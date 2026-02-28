#include <MFitting/MReadSignal.h>
#include <MFitting.h>
#include <jl_lib/GODBC.h>
#include <gtlib_model/pathFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
using namespace std;
using namespace gtlib;

MReadSignal::MReadSignal(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
etsTicker_(false),
ndays_(0),
fitDesc_(MEnv::Instance()->fitDesc),
sigType_(MEnv::Instance()->sigType),
cntOOSData_(0),
cntFitData_(0),
totFitRead_(0),
totOOSRead_(0)
{
	if( conf.count("etsTicker") )
		etsTicker_ = conf.find("etsTicker")->second == "true";

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

	// Pred Names.
	if( conf.count("predName") )
	{
		pair<mmit, mmit> predNames = conf.equal_range("predName");
		for( mmit mi = predNames.first; mi != predNames.second; ++mi )
		{
			string predName = mi->second;
			predNames_.push_back(predName);
		}
	}

	spectatorNames_.push_back("target");
	spectatorNames_.push_back("bmpred");
	spectatorNames_.push_back("pred");
	spectatorNames_.push_back("sprd");
	nSpectator_ = spectatorNames_.size();

	//sigType_ = targetNames_[0].substr(0, 3) == "res" ? "om" : "tm";
}

MReadSignal::~MReadSignal()
{}

void MReadSignal::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = MEnv::Instance()->model;
	vector<int> idates = MEnv::Instance()->idates;
	int udate = MEnv::Instance()->udate;
	vector<int>::iterator itU = lower_bound(idates.begin(), idates.end(), udate);

	// fitdates.
	{
		vector<int> fitdates(idates.begin(), itU);
		for( vector<int>::iterator it = fitdates.begin(); it != fitdates.end(); ++it )
		{
			int idate = *it;
			string path = get_sig_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitDesc_);
			ifstream ifs(path.c_str(), ios::binary);
			if( ifs.is_open() )
			{
				hff::SignalLabel sh;
				ifs >> sh;
				if( ifs.rdstate() == 0 )
				{
					cntFitData_ += sh.nrows;
				//	vNrowsFit_[idate] = sh.nrows;
					fitOffset_.insert(make_pair(idate, make_pair(cntFitData_ - sh.nrows, cntFitData_)));
				}
			}
		}
	}

	// oosdates.
	{
		vector<int> oosdates(itU, idates.end());
		for( vector<int>::iterator it = oosdates.begin(); it != oosdates.end(); ++it )
		{
			int idate = *it;
			string path = get_sig_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitDesc_);
			ifstream ifs(path.c_str(), ios::binary);
			if( ifs.is_open() )
			{
				hff::SignalLabel sh;
				ifs >> sh;
				if( ifs.rdstate() == 0 )
				{
					cntOOSData_ += sh.nrows;
					//vNrowsOOS_[idate] = sh.nrows;
					oosOffset_.insert(make_pair(idate, make_pair(cntOOSData_ - sh.nrows, cntOOSData_)));
				}
			}
		}
	}

	if( cntFitData_ > 0 )
	{
		vvvSpectator_ = vector<vector<vector<float> > >(nTargets_, vector<vector<float> >(nSpectator_, vector<float>()));
		for( int iT = 0; iT < nTargets_; ++iT )
			for( int iS = 0; iS < nSpectator_; ++iS )
				vvvSpectator_[iT][iS].reserve(cntFitData_);
	}
	if( cntOOSData_ > 0 )
	{
		vvvSpectatorOOS_ = vector<vector<vector<float> > >(nTargets_, vector<vector<float> >(nSpectator_, vector<float>()));
		for( int iT = 0; iT < nTargets_; ++iT )
			for( int iS = 0; iS < nSpectator_; ++iS )
				vvvSpectatorOOS_[iT][iS].reserve(cntOOSData_);
	}

	//read_target_pred();
}

void MReadSignal::beginDay()
{
	int idate = MEnv::Instance()->idate;
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;

	int dayOffset = -1;
	int nrows = -1;
	if( oosOffset_.count(idate) )
	{
		bool is_oos = true;
		dayOffset = oosOffset_[idate].first;
		nrows = oosOffset_[idate].second - oosOffset_[idate].first;
		int nrows_read = read_preds(model, idate, is_oos, dayOffset, nrows);
		totOOSRead_ += nrows_read;
		oosOffsetRead_.insert(make_pair(idate, make_pair(totOOSRead_ - nrows_read, totOOSRead_)));
	}
	else if( fitOffset_.count(idate) )
	{
		bool is_oos = false;
		dayOffset = fitOffset_[idate].first;
		nrows = fitOffset_[idate].second - fitOffset_[idate].first;
		int nrows_read = read_preds(model, idate, is_oos, dayOffset, nrows);
		totFitRead_ += nrows_read;
		fitOffsetRead_.insert(make_pair(idate, make_pair(totFitRead_ - nrows_read, totFitRead_)));
	}

}

int  MReadSignal::read_preds(const string& model, int idate, bool is_oos, int dayOffset, int nrows)
{
	int nrow_day = 0;
	int udate = MEnv::Instance()->udate;
	if( etsTicker_ )
		get_ets_tickers(idate);

	// Read pred files.
	{
		int iTarget = 0;
		for( vector<string>::iterator it = targetNames_.begin(); it != targetNames_.end(); ++it, ++iTarget )
		{
			string targetName = *it;
			string path = get_pred_path(MEnv::Instance()->baseDir, model, idate, targetName, "reg", udate, is_oos);
			string txtPath = get_sigTxt_path(MEnv::Instance()->baseDir, model, idate, sigType_, "reg");
			ifstream ifs(path.c_str());
			ifstream ifsTxt(txtPath.c_str());
			if( ifs.is_open() && ifsTxt.is_open() )
			{
				hff::Prediction aPred(nSpectator_);
				string line;
				string lineTxt;
				getline(ifs, line); // first line.
				getline(ifsTxt, lineTxt); // first line.

				for( int r = 0; r < nrows && ifs.rdstate() == 0; ++r )
				{
					ifs >> aPred;
					getline(ifsTxt, lineTxt);
					vector<string> splitTxt = split(lineTxt, '\t');
					string uid = splitTxt[0];
					string ticker = splitTxt[1];

					if( vTickers_.empty() || binary_search(vTickers_.begin(), vTickers_.end(), ticker) )
					{
						if( is_oos )
						{
							for( int s = 0; s < nSpectator_; ++s )
								vvvSpectatorOOS_[iTarget][s].push_back(aPred.v[s]);
							++nrow_day;
						}
						else
						{
							for( int s = 0; s < nSpectator_; ++s )
								vvvSpectator_[iTarget][s].push_back(aPred.v[s]);
							++nrow_day;
						}
					}
				}

				//// If necessary, read prediction from another pred files.
				//int nrow_pred = 0;
				//if( iTarget < predNames_.size() )
				//{
				//	string predName = predNames_[iTarget];
				//	string path = get_pred_path(model, idate, predName);
				//	ifstream ifsPred(path.c_str());
				//	if( ifsPred.is_open() )
				//	{
				//		hff::Prediction aPred(nSpectator_);
				//		string line;
				//		getline(ifsPred, line);

				//		for( int r = 0; r < nrows && ifsPred.rdstate() == 0; ++r )
				//		{
				//			ifsPred >> aPred;

				//			vvvSpectatorOOS_[iTarget][2].push_back(aPred.v[2]);

				//			++nrow_pred;
				//		}
				//	}
				//}

				cout << idate << " " << targetName << " " << nrow_day << endl;
				//if( nrow_pred > 0 )
				//	cout << "    pred " << predNames_[iTarget] << " " << nrow_pred << endl;
			}
		}
	}
	return nrow_day;
}

void MReadSignal::beginTicker(const string& ticker)
{
}

void MReadSignal::endTicker(const string& ticker)
{
}

void MReadSignal::endDay()
{
	++ndays_;
}

void MReadSignal::endJob()
{
	if( !vvvSpectator_.empty() )
		MEvent::Instance()->add<vector<vector<vector<float> > > >("", "vvvSpectator", &vvvSpectator_);
	if( !vvvSpectatorOOS_.empty() )
		MEvent::Instance()->add<vector<vector<vector<float> > > >("", "vvvSpectatorOOS", &vvvSpectatorOOS_);
	if( !oosOffset_.empty() )
		MEvent::Instance()->add<map<int, pair<int, int> > >("", "oosOffset", &oosOffsetRead_);
	if( !fitOffset_.empty() )
		MEvent::Instance()->add<map<int, pair<int, int> > >("", "fitOffset", &fitOffsetRead_);
}

void MReadSignal::get_ets_tickers(int idate)
{
	vTickers_.clear();
	int min_idate = 0;
	{
		char cmd[200];
		sprintf(cmd, "select min(idate) from etsuniverse");
		vector<vector<string> > vv;
		GODBC::Instance()->read("hfstock", cmd, vv);
		min_idate = atoi(vv[0][0].c_str());
	}
	char cmd[200];
	sprintf(cmd, "select symbol from etsuniverse where idate = %d", max(idate, min_idate));
	vector<vector<string> > vv;
	GODBC::Instance()->read("hfstock", cmd, vv);
	for(vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it)
	{
		string symbol = (*it)[0];
		boost::trim(symbol);
		vTickers_.push_back(symbol);
	}
}

void MReadSignal::read_target_pred()
{
	string model = MEnv::Instance()->model;
	vector<int> idates = MEnv::Instance()->idates;
	int udate = MEnv::Instance()->udate;

	for( vector<int>::iterator it = idates.begin(); it != idates.end(); ++it )
	{
		int idate = *it;
		if( etsTicker_ )
			get_ets_tickers(idate);

		int nrow_day = 0;
		string path = get_sig_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitDesc_);
		ifstream ifs(path.c_str(), ios::binary);
		if( ifs.is_open() )
		{
			hff::SignalLabel sh;
			ifs >> sh;
			if( ifs.rdstate() == 0 )
			{
				cntFitData_ += sh.nrows;
				//fitOffset_.insert(make_pair(idate, make_pair(cntFitData_ - sh.nrows, cntFitData_)));
			}
		}
	}
}
