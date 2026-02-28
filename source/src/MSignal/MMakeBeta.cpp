#include <MSignal/MMakeBeta.h>
#include <MSignal.h>
#include <MFramework.h>
#include <jl_lib/jlutil_tickdata.h>
#include <jl_lib/CorrDaily.h>
#include <jl_lib/IndexBetaInfo.h>
#include <map>
#include <MSignal/sig.h>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
using namespace sig;
using namespace std;

MMakeBeta::MMakeBeta(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	verbose_(0),
	horizon_(60),
	cntDays_(0),
	MALen_(20)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("fmodel") )
		fmodel_ = conf.find("fmodel")->second;
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("MALen") )
		MALen_ = atoi( conf.find("MALen")->second.c_str() );

	if( fmodel_.empty() )
		fmodel_ = MEnv::Instance()->model;
}

MMakeBeta::~MMakeBeta()
{}

// beginJob() ------------------------------------------------------------------

void MMakeBeta::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
}

// beginDay() ------------------------------------------------------------------

void MMakeBeta::beginDay()
{
	vector<string> markets = MEnv::Instance()->markets;
	int idate = MEnv::Instance()->idate;
	mTickerUid_ = get_uid_map(markets, idate, uids_);
	read_index_targets();

	if( debug_ )
	{
		for( auto& kv : mvPredIndex_ )
			cout << boost::format("%15.4f ") % kv.first;
		cout << "\n";
		int N = mvPredIndex_.begin()->second.size();
		for( int i = 0; i < N; ++i )
		{
			for( auto& kv : mvPredIndex_ )
				cout << boost::format("%8.4f ") % kv.second[i];
			cout << "\n";
		}
	}
}

void MMakeBeta::read_index_targets()
{
	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	set<string> seenTargetNames;
	for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf)
	{
		const hff::IndexFilter& filter = *itf;
		string targetName = filter.names[0];
		if( !seenTargetNames.count(targetName) )
		{
			vector<float> pred1s = flt::get_data(targetName, idate); // pred1s[0] = ret(0 -> 1s)

			int N = pred1s.size();
			vector<float> pred60s(N);
			for( int i = 0; i < N; ++i )
				pred60s[i] = accumulate(begin(pred1s) + i, begin(pred1s) + std::min(i + horizon_, N), 0.);

			mvPredIndex_[targetName] = pred60s;
			seenTargetNames.insert(targetName);
		}
	}
}

// beginTicker() ---------------------------------------------------------------

void MMakeBeta::beginTicker(const string& ticker)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
	if( quote != nullptr && quotes->size() > 0 )
	{
		vector<float> vRetTicker = get_return_1s_bpt(quotes, linMod.openMsecs, linMod.closeMsecs);
		update_corr(vRetTicker, ticker);
	}
}

void MMakeBeta::update_corr(const vector<float>& vRetTicker, const string& ticker)
{
	int N = vRetTicker.size();
	for( auto& kv : mvPredIndex_ )
	{
		string retName = kv.first;
		CorrDaily& corr = getCorr(retName, ticker);
		for( int i = horizon_; i < N - horizon_; i += horizon_ / 2 )
		{
			double bmRet = kv.second[i];
			double tickerRet = accumulate(vRetTicker.begin() + i, vRetTicker.begin() + i + horizon_, 0.);
			corr.add(bmRet, tickerRet);
		}
	}
}

CorrDaily& MMakeBeta::getCorr(const string& retName, const string& ticker)
{
	boost::mutex::scoped_lock lock(corr_mutex_);
	if( !mFilterTickerCorr_[retName].count(ticker) )
		mFilterTickerCorr_[retName][ticker] = CorrDaily(MALen_);
	return mFilterTickerCorr_[retName][ticker];
}

// endTicker() -----------------------------------------------------------------

void MMakeBeta::endTicker(const string& ticker)
{
}

// endDay() --------------------------------------------------------------------

void MMakeBeta::endDay()
{
	for( auto& kvf : mFilterTickerCorr_ )
	{
		for( auto& kvt : kvf.second )
			kvt.second.endDay();
	}

	if( ++cntDays_ >= MALen_ )
		write_beta();
}

void MMakeBeta::write_beta()
{
	string model = MEnv::Instance()->model;
	int idate = MEnv::Instance()->idate;
	for( auto& kvf : mFilterTickerCorr_ )
	{
		string indexName = kvf.first;
		IndexBetaInfo bInfo(indexName);
		for( auto& kvt : kvf.second )
		{
			const string& retName = kvf.first;
			const string& ticker = kvt.first;
			const CorrDaily& corr = kvt.second;
			double correlation = corr.corr();
			double stdevX = corr.stdevX();
			double stdevY = corr.stdevY();
			double beta = correlation * stdevY / stdevX;
			string uid = mTickerUid_[ticker];
			if( !uid.empty() )
				bInfo.add(uid, beta);
			if( debug_ )
			{
				boost::mutex::scoped_lock lock(MEnv::Instance()->io_mutex);
				cout << boost::format("%16s [%6s] %7.4f %7.4f %6.4f %6.4f\n") % retName % uid % beta % correlation % stdevX % stdevY;
			}
		}

		string beta_dir = flt::get_beta_dir(indexName, model);
		mkd(beta_dir);

		string beta_path = beta_dir + "/beta" + to_string(idate) + ".txt";
		ofstream ofs(beta_path.c_str());
		ofs << bInfo;
	}
}

// endJob() --------------------------------------------------------------------

void MMakeBeta::endJob()
{
}
