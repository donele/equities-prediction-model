#include <HTickSeries/HTickSeriesHedge.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HTickSeriesHedge::HTickSeriesHedge(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HTickSeriesHedge::~HTickSeriesHedge()
{}

void HTickSeriesHedge::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	assert_loopingOrder_mdt();

	return;
}

void HTickSeriesHedge::beginMarket()
{
	string market = HEnv::Instance()->market();

	return;
}

void HTickSeriesHedge::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	msecClose_ = mto::msecClose(market, idate);
	msecOpen_ = mto::msecOpen(market, idate);

	const TickSeries<ReturnData>* ptsR = static_cast<const TickSeries<ReturnData>*>(HEvent::Instance()->get(mto::retName(market), "returns"));
	int nSecInterval = (msecClose_ - msecOpen_)/1000 + 1;
	secRetCum_ = vector<double>(nSecInterval, 0);
	vector<double> secRet = vector<double>(nSecInterval, 0);

	int ntr = ptsR->NTicks();
	const_cast<TickSeries<ReturnData>*>(ptsR)->StartRead();
	ReturnData r;
	for( int n=0; n<ntr; ++n )
	{
		const_cast<TickSeries<ReturnData>*>(ptsR)->Read(&r);
		int rIndx = (r.msecs - msecOpen_)/1000;
		secRet[rIndx] = r.ret;
	}

	secRetCum_[0] = 1.0;
	for( int n=1; n<nSecInterval; ++n )
		secRetCum_[n] = secRetCum_[n-1] * secRet[n] + secRetCum_[n-1];

	return;
}

void HTickSeriesHedge::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();

	//vector<double> vCumRet;
	//for( map<int, double>::iterator it2 = m_msecs_mid.begin(); it2 != m_msecs_mid.end(); ++it2 )
	//{
	//	int sec = it2->first / 1000;
	//	int indx = sec - msecOpen_ / 1000;
	//	vCumRet.push_back(secRetCum_[sec]);
	//}

	return;
}

void HTickSeriesHedge::endTicker(const string& ticker)
{
	return;
}

void HTickSeriesHedge::endDay()
{
	return;
}

void HTickSeriesHedge::endMarket()
{
	return;
}

void HTickSeriesHedge::endJob()
{
	return;
}
