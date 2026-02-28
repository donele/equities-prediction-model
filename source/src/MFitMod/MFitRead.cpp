#include <MFitMod/MFitRead.h>
#include <gtlib_fitting/CorrInput.h>
#include <gtlib_fitting/PredFromBoostedTree.h>
#include <gtlib_model/mFtns.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <algorithm>
using namespace std;
using namespace gtlib;

MFitRead::MFitRead(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	scaleSprd_(false),
	fitDesc_(MEnv::Instance()->fitDesc),
	sigType_(MEnv::Instance()->sigType),
	pFitData_(nullptr),
	readSample_(1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("scaleSprd") )
		scaleSprd_ = conf.find("scaleSprd")->second == "true";
	if( conf.count("sigModel") )
		sigModel_ = conf.find("sigModel")->second;
	if( conf.count("predInputModel") )
		predInputModel_ = conf.find("predInputModel")->second;
	if( conf.count("readSample") )
		readSample_ = atoi(conf.find("readSample")->second.c_str());

	// Input Names, target names, target weights, and spectator names.
	varInfo_.inputNames = getConfigValuesString(conf, "input");
	varInfo_.targetNames = getConfigValuesString(conf, "targetName");
	varInfo_.bmpredNames = getConfigValuesString(conf, "bmpredName");
	varInfo_.spectatorNames = getConfigValuesString(conf, "spectator");
	varInfo_.targetWeights = getConfigValuesFloat(conf, "targetWeight");
	vector<string> ignoreNames = getConfigValuesString(conf, "ignore");
	ignoreInputNames_ = set<string>(begin(ignoreNames), end(ignoreNames));
	if(!ignoreNames.empty())
	{
		printf("Ignore inputs");
		for(auto& iname : ignoreNames)
			printf(" %s", iname.c_str());
		printf("\n");
	}

	initialize();
}

void MFitRead::initialize()
{
	if( varInfo_.targetNames.size() == 1 && varInfo_.targetWeights.empty() )
		varInfo_.targetWeights.push_back(1.);
	assert(varInfo_.targetNames.size() == varInfo_.targetWeights.size());
 
	sigModel_ = getSigModel();
}

string MFitRead::getSigModel()
{
	string s = sigModel_;
	if( s.empty() )
		s = MEnv::Instance()->model;
	return s;
}

MFitRead::~MFitRead()
{}

// beginJob() ------------------------------------------------------------------
void MFitRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	string model = MEnv::Instance()->model;
	string baseDir = MEnv::Instance()->baseDir;
	MEnv::Instance()->fullTargetName = varInfo_.fullTargetName();
	fitdates_ = get_fitdates();
	readData();
}

void MFitRead::readData()
{
	pFitData_ = new FitData(getDailyDataCount(), varInfo_.inputNames, varInfo_.spectatorNames);
	for( int idate : fitdates_ )
		readData(idate);
	pFitData_->printReadSummary();
	MEvent::Instance()->add<gtlib::FitData*>("", "pFitData", pFitData_);
}

vector<int> MFitRead::get_fitdates()
{
	int udate = MEnv::Instance()->udate;
	vector<int> idates = MEnv::Instance()->idates;
	auto itU = lower_bound(begin(idates), end(idates), udate);
	vector<int> fitdates;
	int N = idates.size();
	if( N > 0 )
	{
		if( idates[0] < udate && idates[N - 1] < udate )
			fitdates = vector<int>(idates.begin(), itU);
		else if( idates[0] >= udate )
			fitdates = vector<int>(itU, idates.end());
	}
	return fitdates;
}

DailyDataCount MFitRead::getDailyDataCount()
{
	DailyDataCount ddCount;
	for( int idate : fitdates_ )
	{
		int nNewData = getNNewData(get_sig_path(MEnv::Instance()->baseDir,
					sigModel_, idate, sigType_, MEnv::Instance()->fitDesc)) / readSample_;
		ddCount.add(idate, nNewData);
	}
	return ddCount;
}

int MFitRead::getNNewData(string path)
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

void MFitRead::readData(int idate)
{
	read_inputs(idate);
	read_bm_pred(idate); // Read benchmark predictions.
	calculate_fit_target(idate); // FitData.pTarget is totalTarget - bmpred.
}

void MFitRead::read_inputs(int idate)
{
	string path = get_sig_path(MEnv::Instance()->baseDir, sigModel_, idate, sigType_, fitDesc_);
	ifstream ifs(path.c_str(), ios::binary);
	if( ifs.is_open() )
	{
		cout << "Reading " << path << " ..." << endl;
		int nDataToday = read_input_file(idate, ifs);

		// Read pred inputs
		//vector<string> predInputNames = getPredInputNames();
		//for(string& predName : predInputNames)
		//{
		//	int nDataPred = read_pred_file(idate, predName);
		//	if(nDataToday != nDataPred)
		//	{
		//		cerr << "MFitRead::read_inputs() read " << nDataPred << ' ' << predName << " expecting " << nDataToday << '\n';
		//		exit(77);
		//	}
		//}
	}
	else
		cout << path << " is not open." << endl;
}

vector<string> MFitRead::getPredInputNames()
{
	vector<string> ret;
	for(auto& name : varInfo_.inputNames)
	{
		if(name.substr(0, 4) == "pred")
			ret.push_back(name);
	}
	return ret;
}

int MFitRead::read_input_file(int idate, ifstream& ifs)
{
	hff::SignalLabel sh;
	ifs >> sh;
	int nDataToday = sh.nrows / readSample_;

	vector<int> inputLocation = get_location(sh, varInfo_.inputNames);
	vector<int> spectatorLocation = get_location(sh, varInfo_.spectatorNames);
	vector<int> targetLocation = get_location(sh, varInfo_.targetNames);
	vector<int> bmpredLocation = get_location(sh, varInfo_.bmpredNames);
	set<int> ignoreInputIndex = getIgnoreInputIndex(varInfo_.inputNames);

	int sprdLocation = get_location(sh, {"sprd"})[0];
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
	int readCnt = 0;
	for( int r = 0; r < sh.nrows && ifs.rdstate() == 0; ++r )
	{
		ifs >> aSignal;
		if(r % readSample_ == 0)
		{
			for( int i = 0; i < nInputFields; ++i )
			{
				if(!ignoreInputIndex.count(i))
				{
					int inputLoc = inputLocation[i];
					if(inputLoc >= 0)
						pFitData_->input(i, dayOffset + readCnt) = aSignal.v[inputLoc];
				}
			}

			for( int i = 0; i < nSpectators; ++i )
				pFitData_->spectator(i, dayOffset + readCnt) = aSignal.v[spectatorLocation[i]];

			for( int i = 0; i < nTargets; ++i ) // Sum of targets, weighted.
				vSumTargetToday_[readCnt] += aSignal.v[targetLocation[i]] * varInfo_.targetWeights[i];

			for( int i = 0; i < bmpredLocation.size(); ++i ) // Sum of bmpreds.
				vSumBmpredToday_[readCnt] += aSignal.v[bmpredLocation[i]];

			if(scaleSprd_)
			{
				float halfSprd = .5 * aSignal.v[sprdLocation];
				if(halfSprd > 0.)
				{
					for( int i = 0; i < nInputFields; ++i )
					{
						//if(nInputFields==70 && (i != 21 && i != 0 && i != 1 && i != 2 && i != 3 && i != 27 && i != 35))
						if(nInputFields==70 && ((i>=4&&i<=11) || (i==29 || i==31 || i==32)))
							pFitData_->input(i, dayOffset + readCnt) /= halfSprd;
					}
					vSumTargetToday_[readCnt] /= halfSprd;
				}
			}
			++readCnt;
		}
	}
	if(sh.nrows != readCnt)
		cout << readCnt << " rows read out of " << sh.nrows << endl;
	else
		cout << readCnt << " rows read " << endl;
	return readCnt;
}

int MFitRead::read_pred_file(int idate, const string& predName)
{
	if(predInputModel_.empty())
		predInputModel_ = sigModel_;
	string targetName = predName;
	targetName = targetName.replace(0, 4, "tar");
	string path = get_pred_path(MEnv::Instance()->baseDir, predInputModel_, idate, targetName, fitDesc_);
	ifstream ifsPred(path.c_str(), ios::binary);
	//int inputIndx = get_location(varInfo_.inputNames, {predName})[0];
	int inputIndx = distance(varInfo_.inputNames.begin(), find(varInfo_.inputNames.begin(), varInfo_.inputNames.end(), predName));
	int nRow = 0;
	int readCnt = 0;
	if( ifsPred.is_open() )
	{
		cout << "Reading " << path << " ..." << endl;

		int dayOffset = pFitData_->getDailyDataCount().getOffset(idate);
		string line;
		getline(ifsPred, line);
		while(getline(ifsPred, line))
		{
			if(nRow % readSample_ == 0)
			{
				vector<string> sl = split(line);
				float pred = atof(sl[2].c_str());
				pFitData_->input(inputIndx, dayOffset + readCnt) = pred;
			}
			++nRow;
			++readCnt;
		}
	}
	return readCnt;
}

void MFitRead::read_bm_pred(int idate)
{
	// Implement later.
}

void MFitRead::calculate_fit_target(int idate)
{
	int dayOffset = pFitData_->getDailyDataCount().getOffset(idate);
	int nDataToday = pFitData_->getDailyDataCount().getNdata(idate);
	for( int i = 0; i < nDataToday; ++i )
	{
		pFitData_->bmpred(dayOffset + i) = vSumBmpredToday_[i];
		pFitData_->target(dayOffset + i) = vSumTargetToday_[i] - vSumBmpredToday_[i];
	}
}

vector<int> MFitRead::get_location(hff::SignalLabel& sh, const vector<string>& names)
{
	vector<string> namesInFile = sh.labels;
	auto nameBegin = begin(namesInFile);
	auto nameEnd = end(namesInFile);
	vector<int> vLocation;
	for( string name : names )
	{
		auto it = find(nameBegin, nameEnd, name);
		if( it != nameEnd )
		{
			int indx = it - nameBegin;
			vLocation.push_back(indx);
		}
		else if(name.substr(0, 4) == "pred")
			vLocation.push_back(-1);
		else
		{
			cerr << "ERROR MFitRead input " << name << " not found\n";
			exit(67);
		}
	}
	return vLocation;
}

set<int> MFitRead::getIgnoreInputIndex(const vector<string>& inputNames)
{
	set <int> ignoreInputIndex;
	if(!ignoreInputNames_.empty())
	{
		int N = inputNames.size();
		for(int i = 0; i < N; ++i)
		{
			if(ignoreInputNames_.count(inputNames[i]))
					ignoreInputIndex.insert(i);
		}
	}
	return ignoreInputIndex;
}

void MFitRead::beginDay()
{
}

void MFitRead::endDay()
{
}

void MFitRead::endJob()
{
}
