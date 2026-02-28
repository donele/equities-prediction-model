#include <MFramework/MModuleBase.h>
#include <MFramework/MEnv.h>
#include <iostream>
using namespace std;

MModuleBase::MModuleBase()
{}

MModuleBase::MModuleBase(const string& moduleName, bool threadSafe)
:moduleName_(moduleName),
threadSafe_(threadSafe)
{
}

MModuleBase::~MModuleBase(){}

void MModuleBase::beginJob()
{
	return;
}

void MModuleBase::beginMarket()
{
	return;
}

void MModuleBase::beginDay()
{
	return;
}

void MModuleBase::beginMarketDay()
{
	return;
}

void MModuleBase::beginTicker(const string& ticker)
{
	return;
}

void MModuleBase::endTicker(const string& ticker)
{
	return;
}

void MModuleBase::endMarketDay()
{
	return;
}

void MModuleBase::endDay()
{
	return;
}

void MModuleBase::endMarket()
{
	return;
}

void MModuleBase::endJob()
{
	return;
}

void MModuleBase::assert_loopingOrder_dmt()
{
	if( MEnv::Instance()->loopingOrder != "dmt" )
	{
		cerr << "module " << moduleName_ << " requires the loopingOrder 'dmt'." << endl;
		exit(7);
	}
	return;
}

void MModuleBase::assert_loopingOrder_mdt()
{
	if( MEnv::Instance()->loopingOrder != "mdt" )
	{
		cerr << "module " << moduleName_ << " requires the loopingOrder 'mdt'." << endl;
		exit(7);
	}
	return;
}
