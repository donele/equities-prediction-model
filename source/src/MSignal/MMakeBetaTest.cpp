#include <MSignal/MMakeBetaTest.h>
#include <MSignal.h>
#include <MFramework.h>
#include <jl_lib/jlutil_tickdata.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
using namespace std;

MMakeBetaTest::MMakeBetaTest(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
verbose_(0),
	len_(60),
	ndays_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("fmodel") )
		fmodel_ = conf.find("fmodel")->second;
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );

	if( fmodel_.empty() )
		fmodel_ = MEnv::Instance()->model;
}

MMakeBetaTest::~MMakeBetaTest()
{}

// beginJob() ------------------------------------------------------------------

void MMakeBetaTest::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

// beginDay() ------------------------------------------------------------------

void MMakeBetaTest::beginDay()
{
	read_index_preds();
	read_index_targets();
	get_ticker_volrank();

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

void MMakeBetaTest::read_index_preds()
{
	mvPredIndex_.clear();

	int idate = MEnv::Instance()->idate;
	const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
	for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end(); ++itf)
	{
		const hff::IndexFilter& filter = *itf;
		string predName = filter.title() + "_pred";
		const vector<float>* vPred = static_cast<const vector<float>*>(MEvent::Instance()->get("", predName));
		if( vPred != nullptr )
		{
			int pos1 = predName.find(";", 0);
			int pos2 = predName.find(";", pos1 + 1);
			string predNameSimple = predName.substr(0, pos2);
			mvPredIndex_[predNameSimple] = *vPred;
		}
	}
}

void MMakeBetaTest::read_index_targets()
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
				pred60s[i] = accumulate(begin(pred1s) + i, begin(pred1s) + std::min(i + len_, N), 0.);

			mvPredIndex_[targetName] = pred60s;
			seenTargetNames.insert(targetName);
		}
	}
}

void MMakeBetaTest::get_ticker_volrank()
{
	mTickerVolrank_.clear();

	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->markets[0];

	map<string, double> mTickerVol = read_volume(market, idate);
	vector<int> volrank = get_volrank(mTickerVol);
	fill_ticker_volrank(mTickerVol, volrank);
}

map<string, double> MMakeBetaTest::read_volume(const string& market, int idate)
{
	int okDate = idate;
	vector<vector<string>> vv;
	string cmd = boost::str(boost::format("select %s, shareout * close_ from stockcharacteristics where idate = %d and sectype >= 'A' and sectype <= 'Z' and sectype != 'P' and sectype != 'F' and sectype != 'X'") % mto::ts(market) % okDate);
	GODBC::Instance()->read(mto::hf(market), cmd, vv);

	map<string, double> mTickerVol;
	for( auto it = begin(vv); it != end(vv); ++it )
	{
		string ticker = (*it)[0];
		boost::trim(ticker);
		double vol = atof((*it)[1].c_str());
		mTickerVol[ticker] = vol;
	}
	return mTickerVol;
}

vector<int> MMakeBetaTest::get_volrank(const map<string, double>& mTickerVol)
{
	int rankSize = mTickerVol.size();
	vector<double> vol(rankSize);
	vector<int> volrank(rankSize);

	int cnt = 0;
	for( auto& kv : mTickerVol )
		vol[cnt++] = kv.second;
	gsl_rank(volrank, vol);

	for( auto& rank : volrank )
		rank = rankSize - rank + 1;
	return volrank;
}

void MMakeBetaTest::fill_ticker_volrank(const map<string, double>& mTickerVol, const vector<int>& volrank)
{
	int cnt = 0;
	for( auto it = begin(mTickerVol); it != end(mTickerVol); ++it, ++cnt )
	{
		string ticker = it->first;
		int rank = volrank[cnt];
		mTickerVolrank_[ticker] = rank;
	}
}

// beginTicker() ---------------------------------------------------------------

void MMakeBetaTest::beginTicker(const string& ticker)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
	if( quote != nullptr && quotes->size() > 0 )
	{
		vector<float> vRetTicker = get_return_1s_bpt(quotes, linMod.openMsecs, linMod.closeMsecs);
		update_corr(vRetTicker, ticker);
	}
}

void MMakeBetaTest::update_corr(const vector<float>& vRetTicker, const string& ticker)
{
	int N = vRetTicker.size();
	for( auto& kv : mvPredIndex_ )
	{
		string retName = kv.first;
		Corr* corr = nullptr;
		{
			boost::mutex::scoped_lock lock(corr_mutex_);
			corr = &mFilterTickerCorr_[retName][ticker];
		}
		if( corr != nullptr )
		{
			for( int i = len_; i < N - len_; i += len_ / 2 )
			{
				double bmRet = kv.second[i];
				double tickerRet = accumulate(vRetTicker.begin() + i, vRetTicker.begin() + i + len_, 0.);
				corr->add(bmRet, tickerRet);
			}
		}
	}
}

// endTicker() -----------------------------------------------------------------

void MMakeBetaTest::endTicker(const string& ticker)
{
	//if( ndays_ % 5 == 0 )
	{	
		boost::mutex::scoped_lock lock(MEnv::Instance()->io_mutex);
		for( auto& kv : mvPredIndex_ )
		{
			string retName = kv.first;
			print_summary(retName, ticker);
		}
	}
}

void MMakeBetaTest::print_summary(const string& retName, const string& ticker)
{
	int rankMax = mTickerVolrank_.size();
	Corr& corr = mFilterTickerCorr_[retName][ticker];
	double correlation = corr.corr();
	double beta = correlation * corr.accY.stdev() / corr.accX.stdev();
	int volRank = mTickerVolrank_[ticker];
	if( volRank == 0 )
		volRank = rankMax;
	if( corr.accX.n > (ndays_ + 1) * 100 )
		cout << boost::format("%16s %6s %4d %7.4f %7.4f %6.4f %6.4f\n") % retName % ticker % volRank % beta % correlation % corr.accX.stdev() % corr.accY.stdev();
}

// endDay() --------------------------------------------------------------------

void MMakeBetaTest::endDay()
{
	++ndays_;
}

// endJob() --------------------------------------------------------------------

void MMakeBetaTest::endJob()
{
}
