#include <MSignal/MFitIndexOLSTest.h>
#include <MFramework.h>
#include <MSignal/flt.h>
#include <jl_lib/MovWinLinMod.h>
#include <map>
#include <string>
#include <numeric>
#include <iterator>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
using namespace std;

MFitIndexOLSTest::MFitIndexOLSTest(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
single_thread_(true),
nDays_(0),
lag0_(1),
lagMult_(1),
sampleRate_(1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("lag0") )
		lag0_ = atoi( conf.find("lag0")->second.c_str() );
	if( conf.count("lagMult") )
		lagMult_ = atoi( conf.find("lagMult")->second.c_str() );
	if( conf.count("sampleRate") )
		sampleRate_ = atoi( conf.find("sampleRate")->second.c_str() );
}

MFitIndexOLSTest::~MFitIndexOLSTest()
{}

void MFitIndexOLSTest::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	iim_ = IndexInputMaker(lag0_, lagMult_);

	// Create fitter objects.
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf )
	{
		const hff::IndexFilter& filter = *itf;
		string title = filter.title();
		mLinMod_[title] = MovWinLinMod(iim_.getNinputs(filter.length), 0.);
	}
}

void MFitIndexOLSTest::beginDay()
{
	++nDays_;

	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	int idate = MEnv::Instance()->idate;
	int NF = filters.size();

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
					new boost::thread(boost::bind(&MFitIndexOLSTest::add_data, this, i))
				)
			);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
	}
}

void MFitIndexOLSTest::beginTicker(const string& ticker)
{
}

void MFitIndexOLSTest::endTicker(const string& ticker)
{
}

void MFitIndexOLSTest::endDay()
{
	// Loop filters.
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf )
	{
		const hff::IndexFilter& filter = *itf;
		string title = filter.title();
		if( mLinMod_.count(title) )
		{
			vector<int> data_idates = flt::get_idates(filter.fitDays, idate);
			int idateFirst = 0;
			if( !data_idates.empty() )
				idateFirst = data_idates[0];

			// Calculate and Write to a file.
			string path = get_weight_path(filter, model, idate, lag0_, lagMult_);
			bool doWrite = false;
			if( itf->fitDays == data_idates.size() )
				doWrite = true;
			mLinMod_[title].endDay(idate, idateFirst, path, doWrite);
		}
	}
}

void MFitIndexOLSTest::endJob()
{
}

string MFitIndexOLSTest::get_weight_path(const hff::IndexFilter& filter, const string& model, int idate, int lag0, int lagMult)
{
	string outdir = flt::get_filter_dir(filter, model);
	mkd(outdir);
	string filename;
	if( lag0 == 0 && lagMult == 0 )
		filename = "weight" + itos(idate) + ".txt";
	else
		filename = "weight" + itos(idate) + "_" + itos(lag0) + "_" + itos(lagMult) + ".txt";
	string path = xpf(outdir + "\\" + filename).c_str();
	return path;
}

void MFitIndexOLSTest::add_data(int iFilter)
{
	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	const hff::IndexFilter& filter = filters[iFilter];

	string title = filter.title();
	if( mLinMod_.count(title) )
	{
		map<string, MovWinLinMod>::iterator itm = mLinMod_.find(title);
		vector<const vector<ReturnData>*> vp = flt::get_data(filter.names, idate);

		//  Add the data.
		if( flt::valid_data(vp) )
		{
			int maxT = vp[0]->size() - filter.horizon;
			for( int t = filter.skip; t < maxT; t += sampleRate_ )
			{
				float target = iim_.createTarget(t, vp[0], filter.horizon);
				vector<float> input(itm->second.getNInputs(), 0.);
				iim_.createInput(input, t, vp[1], filter.length);
				itm->second.add(input, target);
			}
		}
	}
}

