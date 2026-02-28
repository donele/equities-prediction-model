#include <MSignal/MTickReadIndex.h>
#include <MFramework.h>
#include <jl_lib.h>
#include "optionlibs/TickData.h"
#include <vector>
#include <string>
#include <map>
#include <algorithm>
using namespace std;

MTickReadIndex::MTickReadIndex(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName, true),
verbose_(0),
do_clip_(true),
bps_(true),
fitting_index_(true),
format_("vector"),
return_dir_("")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("bps") )
		bps_ = conf.find("bps")->second == "true";
	if( conf.count("doClip") )
		do_clip_ = conf.find("doClip")->second == "true";
	if( conf.count("fittingIndex") )
		fitting_index_ = atoi(conf.find("fittingIndex")->second.c_str());
	if( conf.count("format") )
		format_ = conf.find("format")->second;
}

MTickReadIndex::~MTickReadIndex()
{}

void MTickReadIndex::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = MEnv::Instance()->model;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	for( auto it1 = begin(filters); it1 != end(filters); ++it1 )
		for( auto it2 = begin(it1->names); it2 != end(it1->names); ++it2 )
			retNames_.insert(*it2);

	return;
}

void MTickReadIndex::beginDay()
{
	cout << moduleName_ << "::beginDay()" << endl;
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	string model02 = model.substr(0, 2);
	vector<string> markets = MEnv::Instance()->markets;
	tickersRead_.clear();

	if( "CA" == model02 || "US" == model02 || "UF" == model02 )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		if( "U" == model02.substr(0, 1) && linMod.use_etf_filter )
			read_etf(markets[0], idate);

		string dir = mto::bindirReturn(markets[0], idate);
		read_dir(dir, markets[0], idate);
	}
	else
	{
		for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
		{
			string market = *it;
			string dir = mto::bindirReturn(market, idate);

			// Each market.
			read_dir(dir, market, idate);
		}
	}

	if( verbose_ >= 1 )
	{
		cout << "\MTickReadIndex::beginDay() " << idate << ".\n";
		if( verbose_ > 1 )
			PrintMemoryInfoSimple();
	}
	return;
}

void MTickReadIndex::read_etf(const string& market, int idate)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<string> dirs = mto::nbbodirs(market, idate);

	TickAccessMulti<QuoteInfo> taQ;
	for( vector<string>::iterator it = dirs.begin(); it != dirs.end(); ++it )
		taQ.AddRoot(*it);

	vector<string> names;
	taQ.GetNames(idate, &names);
	sort(names.begin(), names.end());

	// Calculate one second returns.
	OneSecRet ret(market, idate);
	for( set<string>::iterator it = retNames_.begin(); it != retNames_.end(); ++it )
	{
		string name = *it;
		string etf_name = split(name, '_')[0];
		if( binary_search(names.begin(), names.end(), etf_name) )
		{
			TickSeries<QuoteInfo> tsq;
			taQ.GetTickSeries(etf_name, idate, &tsq);
			int nTicks = tsq.NTicks();
			if( nTicks > 0 )
			{
				//tickersRead_.insert(name);
				QuoteInfo quote;
				tsq.StartRead();
				for( int i = 0; i < nTicks; ++i )
				{
					tsq.Read(&quote);
					ret.add_quote(etf_name, quote);
				}
			}
		}
	}
	ret.calculate_each_return();

	// Load the returns to the memory for future use.
	for( set<string>::iterator it = retNames_.begin(); it != retNames_.end(); ++it )
	{
		string name = *it;
		string etf_name = split(name, '_')[0];
		TickSeries<ReturnData> ts;
		ret.insert_return(etf_name, ts);
		int nTicks = ts.NTicks();
		if( nTicks > 0 )
		{
			add_return(linMod.n1sec, name, ts, linMod.openMsecs, idate);
		}
	}
}

void MTickReadIndex::read_dir(const string& dir, const string& market, int idate)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	if( !dir.empty() )
	{
		TickAccess<ReturnData> taR(dir, mto::longTicker(market));
		vector<string> names;
		taR.GetNames(idate, &names);

		if( names.empty() )
		{
			cerr << "MTickReadIndex::read_dir() ERROR reading returns." << endl;
			exit(7);
		}
		else
		{
			for( vector<string>::iterator itn = names.begin(); itn != names.end(); ++itn )
			{
				string name = *itn;
				if( retNames_.count(name) && !tickersRead_.count(name) )
				{
					TickSeries<ReturnData> ts;
					taR.GetTickSeries( name, idate, &ts );
					int nTicks = ts.NTicks();

					if( nTicks > 0 )
					{
						add_return(linMod.n1sec, name, ts, linMod.openMsecs, idate);
					}
					else
					{
						cerr << "MTickReadIndex::read_dir() ERROR reading " << name << endl;
						exit(7);
					}
				}
			}
		}
	}
}

void MTickReadIndex::endDay()
{
	int idate = MEnv::Instance()->idate;

	if( !fitting_index_ ) // Remove the series at the end of day.
	{
		remove_data(idate);
	}
	else // Remove the past series out of the horizon.
	{
		int horizon = MEnv::Instance()->indexFilters.filters[0].horizon;
		const vector<int>& idates = MEnv::Instance()->idates;
		vector<int>::const_iterator it = lower_bound(idates.begin(), idates.end(), idate);
		if( distance(idates.begin(), it) >= horizon )
		{
			int idate_remove = *(it - horizon);
			remove_data(idate_remove);
		}
	}

	return;
}

void MTickReadIndex::endJob()
{
	return;
}

void MTickReadIndex::add_return(int n1sec, const string& name, TickSeries<ReturnData>& ts, int openMsecs, int idate)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	int nTicks = ts.NTicks();
	ReturnData *rets = 0;
	ts.ReadSeries(&rets, &nTicks);

	int lenRet = n1sec - 1;
	vector<ReturnData> vRet(lenRet);
	for( int i = 0; i < nTicks; ++i )
	{
		int rIndx = (rets[i].msecs - openMsecs) / 1000 - 1;
		if( rIndx >= 0 && rIndx < nTicks && rIndx < lenRet )
			vRet[rIndx] = rets[i];
	}

	string nameAdd = name;
	if( bps_ )
	{
		nameAdd += "_bps";
		for( vector<ReturnData>::iterator it = vRet.begin(); it != vRet.end(); ++it )
			it->ret *= 10000.;
		if( do_clip_ )
		{
			for( vector<ReturnData>::iterator it = vRet.begin(); it != vRet.end(); ++it )
				if( fabs(it->ret) > linMod.clip_index)
					clip(it->ret, linMod.clip_index);
		}
	}
	nameAdd += "_" + itos(idate);

	MEvent::Instance()->add<vector<ReturnData> >("", nameAdd, vRet);
	tickersRead_.insert(name);
}

void MTickReadIndex::remove_data(int idate)
{
	for( set<string>::iterator it = tickersRead_.begin(); it != tickersRead_.end(); ++it )
	{
		string name = *it;
		if( bps_ )
			name += "_bps";
		name += "_" + itos(idate);
		MEvent::Instance()->remove<vector<ReturnData> >("", name);
	}
}
