#include <MSignal/MMakePredIndex.h>
#include <MSignal.h>
#include <gtlib_linmod/ARMulti.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MMakePredIndex::MMakePredIndex(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
verbose_(0),
fitAlg_("AR")
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("fitAlg") )
		fitAlg_ = conf.find("fitAlg")->second;
	if( conf.count("fmodel") )
		fmodel_ = conf.find("fmodel")->second;
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("transform") ) // "", "asinh", "cubicRoot"
	{
		string id = conf.find("transform")->second;
		if( id == "" )
			transform_ = None;
		else if( id == "asinh" )
			transform_ = Asinh;
		else if( id == "cubicRoot" )
			transform_ = CubicRoot;
	}

	if( fmodel_.empty() )
		fmodel_ = MEnv::Instance()->model;
}

MMakePredIndex::~MMakePredIndex()
{}

void MMakePredIndex::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MMakePredIndex::beginDay()
{
	cout << moduleName_ << "::beginDay()" << endl;
	get_filter();
	make_prediction();
}

void MMakePredIndex::get_filter()
{
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	read_filter();
}

void MMakePredIndex::read_filter()
{
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	ar_.clear();
	int idate = MEnv::Instance()->idate;
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it )
	{
		const hff::IndexFilter& filter = *it;
		string filter_path = flt::get_filter_path(filter, fmodel_, idate);
		if( exists(boost::filesystem::path(filter_path)) )
			ar_.push_back(ARMulti(filter_path));
	}
}

void MMakePredIndex::make_prediction()
{
	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	if( ar_.size() == filters.size() )
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
				int iSerTarget = 0;
				int totlen = vp[0]->size();

				//if( debug_ )
				//{
				//	for( int i = 0; i < totlen; ++i )
				//		cout << i << "\t" << (*vp[0])[i].ret << endl;
				//}

				if( 0 ) // calculate for every gpts.
				{
					for( int i = 1; i < linMod.gpts; ++i )
					{
						int t = i * linMod.gridInterval;

						// Predictors.
						vector<vector<float> > vv(NF - 1, vector<float>(filter.length));
						for( int iSer = 1; iSer < NF; ++iSer )
						{
							int jBegin = t - filter.length;
							int jEnd = t; // exclusive.
							for( int j = jBegin; j < jEnd; ++j )
							{
								if( j >= 0 )
								{
									double val2 = clip_off( (*vp[iSer])[j].ret, filter.clip[iSer] );
									vv[iSer - 1][j - jBegin] = val2;
								}
							}
						}
						float pred = ar_[iFilter].pred(vv);
						clip(pred, linMod.clip_pred_index);
						vPred[i] = getTransform(pred);
					}
				}
				else // calculate for every second.
				{
					for( int t = 1; t < linMod.n1sec; ++t )
					{
						// Predictors.
						vector<vector<float> > vv(NF - 1, vector<float>(filter.length));
						for( int iSer = 1; iSer < NF; ++iSer )
						{
							int jBegin = t - filter.length;
							int jEnd = t; // exclusive.
							for( int j = jBegin; j < jEnd; ++j )
							{
								if( j >= 0 )
								{
									double val2 = clip_off( (*vp[iSer])[j].ret, filter.clip[iSer] );
									vv[iSer - 1][j - jBegin] = val2;
								}
							}
						}
						float pred = ar_[iFilter].pred(vv);
						clip(pred, linMod.clip_pred_index);
						vPred[t] = getTransform(pred);

						if( debug_ && t % 5 == 0)
						{
							typedef std::vector<ReturnData>::const_iterator retIt;
							retIt tBeg = vp[0]->begin();
							retIt t1 = tBeg + t;
							retIt t2 = tBeg + t + filter.horizon;
							float target = sum_returns(t1, t2);
							cout << "Filter" << "\t" << t << "\t" << target << "\t" << pred << endl;
						}
					}
				}
			}
			string predName = filter.title() + "_pred";
			MEvent::Instance()->add<vector<float> >("", predName, vPred);
		}
	}
}

float MMakePredIndex::sum_returns(vector<ReturnData>::const_iterator& it1, vector<ReturnData>::const_iterator& it2)
{
	float ret = 0.;
	for( ; it1 != it2; ++it1 )
		ret += it1->ret;
	return ret;
}

void MMakePredIndex::endDay()
{
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it )
	{
		string predName = it->title() + "_pred";
		MEvent::Instance()->remove<vector<float> >("", predName);
	}
}

float MMakePredIndex::getTransform(float pred)
{
	if( transform_ == None )
		return pred;
	else if( transform_ == Asinh )
		return asinh(pred);
	else if( transform_ == CubicRoot )
		return cbrt(pred);
	return pred;
}

void MMakePredIndex::endJob()
{
}
