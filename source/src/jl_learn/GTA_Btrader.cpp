#include "GTA_Btrader.h"
#include "GTEvent.h"
#include "GTEnv.h"
#include "GenBPNN.h"
#include "Butil.h"
#include "mto.h"
#include "jlutil.h"
#include "optionlibs/TickData.h"
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
using namespace std;

GTA_Btrader::GTA_Btrader(const string& moduleName, const multimap<string, string>& conf)
:GTModule(moduleName),
iModel_(1),
nHidden_(0),
thres_(0.1),
thresIncr_(1),
cost_(0),
maxPos_(1),
modelFile_(""),
verbose_(0),
iEpoch_(0)
{
	if( conf.count("iModel") )
		iModel_ = atoi( conf.find("iModel")->second.c_str() );
	if( conf.count("thres") )
		thres_ = atof( conf.find("thres")->second.c_str() );
	if( conf.count("thresIncr") )
		thresIncr_ = atof( conf.find("thresIncr")->second.c_str() );
	if( conf.count("cost") )
		cost_ = atof( conf.find("cost")->second.c_str() );
	if( conf.count("maxPos") )
		maxPos_ = atoi( conf.find("maxPos")->second.c_str() );
	if( conf.count("nHidden") )
		nHidden_ = atoi( conf.find("nHidden")->second.c_str() );
	if( conf.count("modelFile") )
		modelFile_ = conf.find("modelFile")->second.c_str();
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
}

GTA_Btrader::~GTA_Btrader()
{}

void GTA_Btrader::beginJob()
{
	nInput_ = Butil::get_nInput(iModel_);
	nOutput_ = Butil::get_nOutput(iModel_);

	bpnn_ = GenBPNN(modelFile_);

	return;
}

void GTA_Btrader::beginMarket()
{
	return;
}

void GTA_Btrader::beginDay()
{
	return;
}

void GTA_Btrader::doTicker()
{
	if( iEpoch_ > 0 )
		return;

	vector<Bsample>* sample
		= static_cast<vector<Bsample>*>(GTEvent::Instance()->get("sample"));

	vector<double> msecB_series;
	vector<double> signB_series;
	vector<double> prcB_series;
	vector<double> volB_series;

	// loop over the sample and trade.
	vector<double> error(nOutput_);
	for( vector<Bsample>::iterator it = sample->begin(); it != sample->end(); ++it )
	{
		vector<double>& input = it->input;
		QuoteInfo& quote = it->quote;

		vector<double> output = bpnn_.forward(input);
		if( !output.empty() )
		{
			double sprd = 2.0 * (quote.ask - quote.bid) / (quote.ask + quote.bid);
			double cost = cost_ * max(0, sprd);
			double F = output[0];
			if( F > 0 )
			{
				if( F > cost ) // buy
				{
					msecB_series.push_back(quote.msecs);
					prcB_series.push_back(quote.ask);
					signB_series.push_back(1);
					volB_series.push_back(1);
				}
			}
			else if( F < 0 )
			{
				if( F < -cost ) // sell
				{
					msecB_series.push_back(quote.msecs);
					prcB_series.push_back(quote.bid);
					signB_series.push_back(-1);
					volB_series.push_back(1);
				}
			}
		}
	}

	// Add to the event
	GTEvent::Instance()->add<vector<double> >("msecB", msecB_series);
	GTEvent::Instance()->add<vector<double> >("signB", signB_series);
	GTEvent::Instance()->add<vector<double> >("prcB", prcB_series);
	GTEvent::Instance()->add<vector<double> >("volB", volB_series);

	return;
}

void GTA_Btrader::endDay()
{
	return;
}

void GTA_Btrader::endMarket()
{
	++iEpoch_;
	return;
}

void GTA_Btrader::endJob()
{
	return;
}
