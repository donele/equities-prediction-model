#include <HFitting/HCompSig.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HCompSig::HCompSig(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HCompSig::~HCompSig()
{}

void HCompSig::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	int tmtxtinput[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80};
	int tmbininput[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 24, 25, 26, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 47, 50, 51, 52, 53, 54, 55, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87};
	int omtxtinput[] = {2, 3, 4, 5, 6, 7, 8, 9, 40, 20, 21, 22, 42, 16, 17, 23, 24, 25, 26, 41, 28, 29, 30, 43, 44, 47, 48, 50, 51, 32, 33, 34, 35, 36, 37};
	int ombininput[] = {0, 1, 2, 3, 4, 5, 6, 7, 37, 18, 19, 20, 39, 13, 14, 15, 17, 21, 22, 38, 30, 31, 32, 40, 41, 44, 45, 47, 48};

	if( type_ == "tm" )
	{
		if( input_ == "txt" )
		{
			vVars1_ = vector<int>(tmtxtinput, tmtxtinput + 70);
			vVars1_.push_back(24);
			vVars1_.push_back(25);
			vVars2_ = vector<int>(tmtxtinput, tmtxtinput + 70);
			vVars2_.push_back(24);
			vVars2_.push_back(25);
		}
		else
		{
			vVars1_ = vector<int>(tmbininput, tmbininput + 70);
			vVars1_.push_back(22);
			vVars2_ = vector<int>(tmbininput, tmbininput + 70);
			vVars2_.push_back(22);
		}
	}
	else if( type_ == "om" )
	{
		if( input_ == "txt" )
		{
			vVars1_ = vector<int>(omtxtinput, omtxtinput + 35);
			vVars1_.push_back(18);
			vVars1_.push_back(19);
			vVars1_.push_back(59);
			vVars2_ = vector<int>(omtxtinput, omtxtinput + 35);
			vVars2_.push_back(18);
			vVars2_.push_back(61);
			vVars2_.push_back(59);
		}
		else
		{
			vVars1_ = vector<int>(ombininput, ombininput + 29);
			vVars1_.push_back(69);
			vVars1_.push_back(70);
			vVars2_ = vector<int>(ombininput, ombininput + 29);
			vVars2_.push_back(9);
			vVars2_.push_back(10);
		}
	}
}

void HCompSig::beginMarket()
{
	string market = HEnv::Instance()->market();
}

void HCompSig::beginDay()
{
	int idate = HEnv::Instance()->idate();
}

void HCompSig::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
}

void HCompSig::endTicker(const string& ticker)
{
}

void HCompSig::endDay()
{
}

void HCompSig::endMarket()
{
}

void HCompSig::endJob()
{
}
