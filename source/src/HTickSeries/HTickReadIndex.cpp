#include <HTickSeries/HTickReadIndex.h>
#include <HLib.h>
#include <jl_lib.h>
#include "optionlibs/TickData.h"
#include <vector>
#include <string>
#include <map>
using namespace std;

HTickReadIndex::HTickReadIndex(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName, true),
verbose_(0),
bps_(true),
fitting_index_(true),
format_("vector"),
return_dir_("")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("bps") )
		bps_ = conf.find("bps")->second == "true";
	if( conf.count("fittingIndex") )
		fitting_index_ = atoi(conf.find("fittingIndex")->second.c_str());
	if( conf.count("format") )
		format_ = conf.find("format")->second;
}

HTickReadIndex::~HTickReadIndex()
{}

void HTickReadIndex::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = HEnv::Instance()->model();
	const vector<hff::IndexFilter>& filters = HEnv::Instance()->indexFilters().filters;
	for( vector<hff::IndexFilter>::const_iterator it1 = filters.begin(); it1 != filters.end(); ++it1 )
		for( vector<string>::const_iterator it2 = it1->names.begin(); it2 != it1->names.end(); ++it2 )
			retNames_.insert(*it2);

	return;
}

void HTickReadIndex::beginDay()
{
	int idate = HEnv::Instance()->idate();
	vector<string> markets = HEnv::Instance()->markets();
	tickersRead_.clear();

	for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
	{
		string market = *it;
		string dir = mto::bindirReturn(market, idate);
		read_dir(dir, market, idate);
		if( mto::region(market) == "E" )
			read_dir(xpf("\\\\smrc-ltc-mrct16\\data00\\tickC\\eu\\return"), market, idate);
	}

	if( verbose_ >= 1 )
	{
		cout << "\HTickReadIndex::beginDay() " << idate << ".\n";
		if( verbose_ > 1 )
			PrintMemoryInfoSimple();
	}
	return;
}

void HTickReadIndex::read_dir(string dir, string market, int idate)
{
	if( !dir.empty() )
	{
		TickAccess<ReturnData> taR(dir, mto::longTicker(market));
		vector<string> names;
		taR.GetNames(idate, &names);

		for( vector<string>::iterator itn = names.begin(); itn != names.end(); ++itn )
		{
			string name = *itn;
			if( retNames_.count(name) && !tickersRead_.count(name) )
			{
				TickSeries<ReturnData> ts;
				taR.GetTickSeries( name, idate, &ts );
				int nTicks = ts.NTicks();

				ReturnData *rets = 0;
				ts.ReadSeries(&rets, &nTicks);
				int internationalOffset = 1;
				vector<ReturnData> vRet(rets + internationalOffset, rets + nTicks);

				string nameAdd = name;
				if( bps_ )
				{
					nameAdd += "_bps";
					for( vector<ReturnData>::iterator it = vRet.begin(); it != vRet.end(); ++it )
						it->ret *= 10000.;
				}
				nameAdd += "_" + itos(idate);

				HEvent::Instance()->add<vector<ReturnData> >("", nameAdd, vRet);
				tickersRead_.insert(name);
			}
		}
	}
}

void HTickReadIndex::endDay()
{
	int idate = HEnv::Instance()->idate();

	if( !fitting_index_ ) // Remove the series at the end of day.
	{
		remove_data(idate);
	}
	else // Remove the past series out of the horizon.
	{
		int horizon = HEnv::Instance()->indexFilters().filters[0].horizon;
		const vector<int>& idates = HEnv::Instance()->idates();
		vector<int>::const_iterator it = lower_bound(idates.begin(), idates.end(), idate);
		if( distance(idates.begin(), it) >= horizon )
		{
			int idate_remove = *(it - horizon);
			remove_data(idate_remove);
		}
	}

	return;
}

void HTickReadIndex::endJob()
{
	return;
}

void HTickReadIndex::remove_data(int idate)
{
	for( set<string>::iterator it = tickersRead_.begin(); it != tickersRead_.end(); ++it )
	{
		string name = *it;
		if( bps_ )
			name += "_bps";
		name += "_" + itos(idate);
		HEvent::Instance()->remove<vector<ReturnData> >("", name);
	}
}
