#include <MSignal/MFitIndexAR.h>
#include <MFramework.h>
#include <MSignal/flt.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
using namespace std;
using namespace gtlib;

MFitIndexAR::MFitIndexAR(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
single_thread_(true),
nDays_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

MFitIndexAR::~MFitIndexAR()
{}

void MFitIndexAR::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MFitIndexAR::beginDay()
{
	++nDays_;

	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	int NF = filters.size();

	// Create AR objects.
	mar_.clear();
	for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf )
	{
		const hff::IndexFilter& filter = *itf;
		if( filter.fitDays <= nDays_ )
		{
			string title = filter.title();
			mar_[title] = ARMulti(filter.names.size(), filter.length, filter.horizon, filter.targetLag);
		}
	}

	// Loop filters and add data.
	if( single_thread_ )
	{
		for( int i = 0; i < NF; ++i )
			add_data(i);
	}
	else
	{
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int i = 0; i < NF; ++i )
		{
			listThread.push_back(
				boost::shared_ptr<boost::thread>(
					new boost::thread(boost::bind(&MFitIndexAR::add_data, this, i))
				)
			);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
	}
}

void MFitIndexAR::beginTicker(const string& ticker)
{
}

void MFitIndexAR::endTicker(const string& ticker)
{
}

void MFitIndexAR::endDay()
{
	// Loop filters.
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	int cntWrite = 0;
	for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf )
	{
		string title = itf->title();
		if( mar_.count(title) )
		{
			// Calculate the coefficients.
			mar_[title].getCoeffs();
			if( mar_[title].coeffsGood() )
			{
				// Write to a file.
				const hff::IndexFilter& filter = *itf;
				string outdir = flt::get_filter_dir(filter, model);
				mkd(outdir);

				string filename = "flt" + itos(idate) + ".txt";
				string path = xpf(outdir + "\\" + filename).c_str();
				ofstream ofsModel( path.c_str() );
				ofsModel << mar_[title];
				++cntWrite;
			}
		}
	}
	if( cntWrite > 0 )
		printf("Filters written to %d file(s).\n", cntWrite);
}

void MFitIndexAR::endJob()
{
}

void MFitIndexAR::add_data(int iFilter)
{
	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	const hff::IndexFilter& filter = filters[iFilter];

	string title = filter.title();
	if( mar_.count(title) )
	{
		map<string, ARMulti>::iterator itm = mar_.find(title);

		// Loop dates.
		vector<int> data_idates = flt::get_idates(filter.fitDays, idate);
		for( vector<int>::iterator itd = data_idates.begin(); itd != data_idates.end(); ++itd )
		{
			// Get the data.
			int data_idate = *itd;
			vector<const vector<ReturnData>*> vp = flt::get_data(filter.names, data_idate);

			//  Add the data.
			if( flt::valid_data(vp) )
				itm->second.add(filter, vp);
		}
	}
}
