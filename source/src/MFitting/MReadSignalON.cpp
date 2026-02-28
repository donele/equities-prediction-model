#include <MFitting/MReadSignalON.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MReadSignalON::MReadSignalON(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
minMsecs_(0),
maxMsecs_(86400000),
ndays_(0),
weight_(1.),
fitDesc_(MEnv::Instance()->fitDesc),
sigType_(MEnv::Instance()->sigType),
targetName_("tarON"),
cntOOSData_(0),
cntFitData_(0)
{
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	if( conf.count("weight") )
		weight_ = atof(conf.find("weight")->second.c_str());

	spectatorNames_.push_back("target");
	spectatorNames_.push_back("bmpred");
	spectatorNames_.push_back("pred");
	spectatorNames_.push_back("sprd");
	nSpectator_ = spectatorNames_.size();
}

MReadSignalON::~MReadSignalON()
{}

void MReadSignalON::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = MEnv::Instance()->model;
	vector<int> idates = MEnv::Instance()->idates;
	int udate = MEnv::Instance()->udate;
	vector<int>::iterator itU = lower_bound(idates.begin(), idates.end(), udate);

	// fitdates.
	{
		vector<int> fitdates(idates.begin(), itU);
		int cntDay = 0;
		for( vector<int>::iterator it = fitdates.begin(); it != fitdates.end(); ++it, ++cntDay )
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
					vNrowsFit_[idate] = sh.nrows;
					fitOffset_.insert(make_pair(idate, make_pair(cntFitData_ - sh.nrows, cntFitData_)));
				}
			}
		}
	}

	// oosdates.
	{
		vector<int> oosdates(itU, idates.end());
		int cntDay = 0;
		for( vector<int>::iterator it = oosdates.begin(); it != oosdates.end(); ++it, ++cntDay )
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
					vNrowsOOS_[idate] = sh.nrows;
					oosOffset_.insert(make_pair(idate, make_pair(cntOOSData_ - sh.nrows, cntOOSData_)));
				}
			}
		}
	}

	if( cntFitData_ > 0 )
		vSpectator_ = vector<float>(cntFitData_, 0.);
	if( cntOOSData_ > 0 )
		vSpectatorOOS_ = vector<float>(cntOOSData_, 0.);
}

void MReadSignalON::beginDay()
{
	int idate = MEnv::Instance()->idate;
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	bool is_oos = idate >= udate;
	int dayOffset = -1;
	int nrows = -1;
	if( oosOffset_.count(idate) )
	{
		dayOffset = oosOffset_[idate].first;
		nrows = vNrowsOOS_[idate];
	}
	else if( fitOffset_.count(idate) )
	{
		dayOffset = fitOffset_[idate].first;
		nrows = vNrowsFit_[idate];
	}

	string binTxt_path = get_sigTxt_path(MEnv::Instance()->baseDir, model, idate, "tm", "reg");
	ifstream ifsBinTxt(binTxt_path.c_str());

	string path = get_pred_path(MEnv::Instance()->baseDir, model, idate, targetName_, "reg", is_oos);
	ifstream ifs(path.c_str());
	if( ifsBinTxt.is_open() && ifs.is_open() )
	{
		hff::Prediction aPred(nSpectator_);
		string uid;
		string ticker;
		double time;
		string line;
		string lineBinTxt;
		getline(ifs, line); // first line.
		getline(ifsBinTxt, lineBinTxt);
		int nrow_day = 0;

		for( int r = 0; r < nrows && ifs.rdstate() == 0; ++r )
		{
			ifs >> aPred;
			ifsBinTxt >> uid >> ticker >> time;
			int msecs = ceil(time * 1000. - 0.5) + linMod.openMsecs;
			if( minMsecs_ <= msecs && msecs <= maxMsecs_ )
			{
				if( is_oos )
				{
					vSpectatorOOS_[dayOffset + r] = aPred.v[2] * weight_;
					++nrow_day;
				}
				else if (false)
				{
					vSpectator_[dayOffset + r] = aPred.v[2] * weight_;
					++nrow_day;
				}
			}
		}

		cout << idate << " " << targetName_ << " " << nrow_day << endl;
	}
}

void MReadSignalON::beginTicker(const string& ticker)
{
}

void MReadSignalON::endTicker(const string& ticker)
{
}

void MReadSignalON::endDay()
{
	++ndays_;
}

void MReadSignalON::endJob()
{
	if( !vSpectatorOOS_.empty() )
	{
		const vector<vector<vector<float> > >* pvvvSpectatorOOS = &MEvent::Instance()->get<vector<vector<vector<float> > > >("", "vvvSpectatorOOS");
		vvvSpectatorOOS_ = *pvvvSpectatorOOS;

		int N = vSpectatorOOS_.size();
		for( int i = 0; i < N; ++i )
			vvvSpectatorOOS_[0][2][i] += vSpectatorOOS_[i];

		MEvent::Instance()->add<vector<vector<vector<float> > > >("", "vvvSpectatorOOSON", &vvvSpectatorOOS_); // "&" intended.
	}
}
