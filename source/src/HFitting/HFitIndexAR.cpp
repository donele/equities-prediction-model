#include <HFitting/HFitIndexAR.h>
#include <HLib.h>
#include <HFitting/flt.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HFitIndexAR::HFitIndexAR(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false),
do_fit_(false),
fitAlg_("AR")
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HFitIndexAR::~HFitIndexAR()
{}

void HFitIndexAR::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void HFitIndexAR::beginDay()
{
	int idate = HEnv::Instance()->idate();
	const vector<hff::IndexFilter>& filters = HEnv::Instance()->indexFilters().filters;

	vector<int> data_idates = flt::get_idates(filters[0].fitDays, idate);
	do_fit_ = data_idates.size() == filters[0].fitDays;

	if( do_fit_ )
	{
		ar_.clear();
		// Create the AR objects.
		for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it )
			ar_.push_back(ARMulti(it->names.size(), it->length, it->horizon));

		// Loop the dates.
		for( vector<int>::iterator itd = data_idates.begin(); itd != data_idates.end(); ++itd )
		{
			int data_idate = *itd;

			// There may be multiple filters to calculate.
			int iFilter = 0;
			for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf, ++iFilter )
			{
				const hff::IndexFilter& filter = *itf;
				vector<const vector<ReturnData>*> vp = flt::get_data(filter.names, data_idate);
				if( flt::valid_data(vp) )
				{
					ar_[iFilter].add(filter, vp);
				}
			}
		}
	}
}

void HFitIndexAR::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
}

void HFitIndexAR::endTicker(const string& ticker)
{
}

void HFitIndexAR::endDay()
{
	if( do_fit_ )
	{
		int idate = HEnv::Instance()->idate();
		const vector<hff::IndexFilter>& filters = HEnv::Instance()->indexFilters().filters;
		int iFilter = 0;
		for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf, ++iFilter )
		{
			ar_[iFilter].getCoeffs();

			// Write to a file.
			const hff::IndexFilter& filter = *itf;
			string outdir = flt::get_model_dir(filter, fitAlg_);
			ForceDirectory(outdir);

			string filename = fitAlg_ + itos(idate) + ".txt";

			ofstream ofsMap( (outdir + "\\mapping.txt").c_str(), ios::app );
			ofsMap << idate << "\t" << idate << "\t" << filename << endl;

			ofstream ofsModel( (outdir + "\\" + filename).c_str() );
			ofsModel << ar_[iFilter];
		}
	}
}

void HFitIndexAR::endJob()
{
}
