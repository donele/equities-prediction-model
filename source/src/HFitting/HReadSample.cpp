#include <HFitting/HReadSample.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HReadSample::HReadSample(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false),
index_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("index") )
		index_ = conf.find("index")->second == "true";
}

HReadSample::~HReadSample()
{}

void HReadSample::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void HReadSample::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
}

void HReadSample::endDay()
{
}

void HReadSample::endJob()
{
}
