#include <MFitting/MReadSample.h>
#include <MFramework.h>
#include <map>
#include <string>
using namespace std;

MReadSample::MReadSample(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
index_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("index") )
		index_ = conf.find("index")->second == "true";
}

MReadSample::~MReadSample()
{}

void MReadSample::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MReadSample::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
}

void MReadSample::endDay()
{
}

void MReadSample::endJob()
{
}
