#include <MSignal/MMakePredIndexOLS.h>
#include <MSignal/flt.h>
#include <jl_lib/MovWinLinMod.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;

MMakePredIndexOLS::MMakePredIndexOLS(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
lag0_(1),
lagMult_(1),
verbose_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("fmodel") )
		fmodel_ = conf.find("fmodel")->second;
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("lag0") )
		lag0_ = atoi( conf.find("lag0")->second.c_str() );
	if( conf.count("lagMult") )
		lagMult_ = atoi( conf.find("lagMult")->second.c_str() );

	if( fmodel_.empty() )
		fmodel_ = MEnv::Instance()->model;
}

MMakePredIndexOLS::~MMakePredIndexOLS()
{}

void MMakePredIndexOLS::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	iim_ = IndexInputMaker(lag0_, lagMult_);
}

void MMakePredIndexOLS::beginDay()
{
	read_filter();
	make_prediction();
}

void MMakePredIndexOLS::read_filter()
{
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	vLinMod_.clear();
	int idate = MEnv::Instance()->idate;
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it )
	{
		const hff::IndexFilter& filter = *it;
		string filter_path = flt::get_filter_path_ols(filter, fmodel_, idate);
		if( exists(boost::filesystem::path(filter_path)) )
			vLinMod_.push_back(MovWinLinMod(filter_path));
	}
}

void MMakePredIndexOLS::make_prediction()
{
	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	if( vLinMod_.size() == filters.size() )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

		// There may be multiple filters to calculate.
		int iFilter = 0;
		for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf, ++iFilter )
		{
			const hff::IndexFilter& filter = *itf;
			vector<const vector<ReturnData>*> vp = flt::get_data(filter.names, idate);
			vector<float> vPred(linMod.n1sec);
			if( flt::valid_data(vp) )
			{
				int NF = filter.names.size();
				for( int t = 1; t < linMod.n1sec; ++t )
				{
					vector<float> input(vLinMod_[iFilter].getNInputs(), 0.);
					iim_.createInput(input, t, vp[1], filter.length);
					float pred = vLinMod_[iFilter].pred(input);
					clip(pred, linMod.clip_pred_index);
					vPred[t] = pred;

					if( debug_ && t % 5 == 0 )
					{
						float target = iim_.createTarget(t, vp[0], filter.horizon);
						cout << "OLS" << "\t" << t << "\t" << target << "\t" << pred << endl;
					}
				}
			}
			string predName = filter.title() + "_pred";
			MEvent::Instance()->add<vector<float> >("", predName, vPred);
		}
	}
}
