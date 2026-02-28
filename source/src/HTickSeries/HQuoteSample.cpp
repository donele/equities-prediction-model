#include <HTickSeries/HQuoteSample.h>
#include <optionlibs/TickData.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib.h>
#include <gtlib_signal/QuoteSample.h>
#include <boost/thread.hpp>
#include <iostream>
#include <algorithm>
#include <map>
#include <numeric>
#include <list>
#include <string>
using namespace std;
using namespace gtlib;

HQuoteSample::HQuoteSample(const string& moduleName, const multimap<string, string>& conf)
	:HModule(moduleName, true),
	debug_(false),
	requireValidQuote_(false),
	fillZeros_(false),
	allowCross_(false),
	msecOpen_(0),
	msecClose_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("requireValidQuote") )
		requireValidQuote_ = conf.find("requireValidQuote")->second == "true";
	if( conf.count("fillZeros") )
		fillZeros_ = conf.find("fillZeros")->second == "true";
}

HQuoteSample::~HQuoteSample()
{}

void HQuoteSample::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	return;
}

void HQuoteSample::beginMarketDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();
	sessions_ = Sessions(market, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);

	return;
}

void HQuoteSample::beginTicker(const string& ticker)
{
	const vector<QuoteInfo>* pQuotes = static_cast<const vector<QuoteInfo>*>(HEvent::Instance()->get(ticker, "quotes"));
	QuoteSample qSample(*pQuotes, sessions_, msecOpen_, msecClose_, 100, requireValidQuote_, fillZeros_, allowCross_);
	HEvent::Instance()->add<QuoteSample>(ticker, "qSample", qSample);
	return;
}

void HQuoteSample::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<QuoteSample>(ticker, "qSample");
	return;
}

void HQuoteSample::endMarketDay()
{
	return;
}

void HQuoteSample::endJob()
{
	return;
}

