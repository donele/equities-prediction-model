#include <HFitting/HMakeSampleTarget.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HMakeSampleTarget::HMakeSampleTarget(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HMakeSampleTarget::~HMakeSampleTarget()
{}

void HMakeSampleTarget::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void HMakeSampleTarget::beginDay()
{
	int idate = HEnv::Instance()->idate();
}

void HMakeSampleTarget::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
}

void HMakeSampleTarget::endTicker(const string& ticker)
{
}

void HMakeSampleTarget::endDay()
{
}

void HMakeSampleTarget::endJob()
{
}
