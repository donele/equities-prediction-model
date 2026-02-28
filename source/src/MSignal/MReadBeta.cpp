#include <MSignal/MReadBeta.h>
#include <MSignal.h>
#include <MFramework.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/IndexBetaInfo.h>
#include <map>
#include <MSignal/sig.h>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
using namespace sig;
using namespace std;
using namespace flt;

MReadBeta::MReadBeta(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	verbose_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("bmodel") )
		bmodel_ = conf.find("bmodel")->second;
}

MReadBeta::~MReadBeta()
{}

// beginJob() ------------------------------------------------------------------

void MReadBeta::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	string model = MEnv::Instance()->model;
	if( bmodel_.empty() )
		bmodel_ = model;

	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf)
	{
		const hff::IndexFilter& filter = *itf;
		string targetName = filter.names[0];
		indexNames_.insert(targetName);
	}
}

// beginDay() ------------------------------------------------------------------

void MReadBeta::beginDay()
{
	vector<string> markets = MEnv::Instance()->markets;
	int idate = MEnv::Instance()->idate;

	vector<IndexBetaInfo> vIndexBeta;
	for( auto& indexName : indexNames_ )
	{
		IndexBetaInfo bInfo(indexName);
		string beta_path = get_beta_path(indexName, bmodel_, idate);
		ifstream ifs(beta_path.c_str());
		ifs >> bInfo;
		vIndexBeta.push_back(bInfo);
	}
	MEvent::Instance()->add<vector<IndexBetaInfo>>("", "beta", vIndexBeta);
}

// endDay() --------------------------------------------------------------------

void MReadBeta::endDay()
{
}

// endJob() --------------------------------------------------------------------

void MReadBeta::endJob()
{
}
