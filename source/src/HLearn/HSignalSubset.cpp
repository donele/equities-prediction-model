#include <HLearn/HSignalSubset.h>
#include <HLearnObj/InputInfo.h>
#include <HLearnObj/Signal.h>
#include <HLearnObj/Sample.h>
#include <optionlibs/TickData.h>
#include <HLib.h>
#include <jl_lib.h>
#include <TFile.h>
#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
using namespace std;

HSignalSubset::HSignalSubset(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
verbose_(1),
scale_(0.3),
id_(""),
input_dir_("")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("inputDir") )
		input_dir_ = conf.find("inputDir")->second;
	if( conf.count("id") )
		id_ = conf.find("id")->second;

}

HSignalSubset::~HSignalSubset()
{}

void HSignalSubset::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void HSignalSubset::beginMarket()
{
	return;
}

void HSignalSubset::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
 
	// Open file.
	char filename[400];

	sprintf(filename, "%s\\%s\\%d.bin", input_dir_.c_str(), mto::region_long(market).c_str(), idate);
	ifs_.open(filename, ios::in | ios::binary);

	sprintf(filename, "%s_%s\\%s\\%d.bin", input_dir_.c_str(), id_.c_str(), mto::region_long(market).c_str(), idate);
	ofs_.open(filename, ios::out | ios::binary);

	// Read tickers.
	if( ifs_.is_open() && ofs_.is_open() )
	{
		hnn::SignalSummary st;
		ifs_ >> st;
		ofs_ << st;

		HEnv::Instance()->tickers(st.tickers);
	}
	else
		HEnv::Instance()->tickers(vector<string>());

	return;
}

void HSignalSubset::beginTicker(const string& ticker)
{
	read_sample(ticker);

	return;
}

void HSignalSubset::endTicker(const string& ticker)
{
	return;
}

void HSignalSubset::endDay()
{
	ifs_.close();
	ifs_.clear();
	ofs_.close();
	ofs_.clear();
	return;
}

void HSignalSubset::endMarket()
{
	return;
}

void HSignalSubset::endJob()
{
	char buf[200];
	sprintf(buf, "\n%20s %19s %13s\n", moduleName_.c_str(), "Real Time", "CPU Time");
	cout << buf;
}

void HSignalSubset::read_sample(string ticker)
{
	vector<hnn::Signal> vSignal;
	hnn::SignalLabel sh;
	ifs_ >> sh;

	if( sh.ticker == ticker )
	{
		for( int i=0; i<sh.nEntries; ++i )
		{
			hnn::SignalContent aSignal;
			ifs_ >> aSignal;

			if( Random::Instance()->select(scale_) )
				vSignal.push_back(aSignal);
		}
	}
	else
	{
		cerr << "HSignalSubset::read_sample read '" << sh.ticker << "' while expecting '" << ticker << "'" << endl;
		exit(5);
	}
	write_signal(vSignal, ticker);

	return;
}

void HSignalSubset::write_signal(vector<hnn::Signal>& vSignal, string ticker)
{
	if( !ticker.empty() )
	{
		hnn::SignalLabel sh(ticker, vSignal.size());
		ofs_ << sh;
		for( vector<hnn::Signal>::iterator it = vSignal.begin(); it != vSignal.end(); ++it )
			ofs_ << *it;
	}
	vSignal.clear();

	return;
}
