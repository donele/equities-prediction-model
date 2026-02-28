#include <MFitting/MModuleExample.h>
#include <MFramework.h>
#include <map>
#include <string>
using namespace std;

MModuleExample::MModuleExample(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

MModuleExample::~MModuleExample()
{}

void MModuleExample::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MModuleExample::beginDay()
{
	int idate = MEnv::Instance()->idate;
}

void MModuleExample::beginTicker(const string& ticker)
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
}

void MModuleExample::endTicker(const string& ticker)
{
}

void MModuleExample::endDay()
{
}

void MModuleExample::endJob()
{
}
