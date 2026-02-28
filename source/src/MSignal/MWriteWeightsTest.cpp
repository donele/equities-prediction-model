#include <MSignal/MWriteWeightsTest.h>
#include <MSignal/SigSet.h>
#include <MSignal.h>
#include <gtlib_model/pathFtns.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
using namespace std;
using namespace hff;
using namespace sig;
using namespace gtlib;

MWriteWeightsTest::MWriteWeightsTest(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
debug_noss_(false),
freeze_univ_(false),
udate_is_trading_day_(true),
verbose_(0),
write_weights_(true),
write_weights_debug_(true),
write_weights_horizon_(0),
write_weights_lag_(0),
iHorizon_(0),
nThreads_(1),
db_offset_(3)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugNoss") )
		debug_noss_ = conf.find("debugNoss")->second == "true";
	if( conf.count("freezeUniv") )
		freeze_univ_ = conf.find("freezeUniv")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("writeWeights") )
		write_weights_ = conf.find("writeWeights")->second == "true";
	if( conf.count("writeWeightsDebug") )
		write_weights_debug_ = conf.find("writeWeightsDebug")->second == "true";
	if( conf.count("writeWeightsHorizon") )
		write_weights_horizon_ = atoi(conf.find("writeWeightsHorizon")->second.c_str());
	if( conf.count("writeWeightsLag") )
		write_weights_lag_ = atoi(conf.find("writeWeightsLag")->second.c_str());
	if( conf.count("nThreads") )
		nThreads_ = atoi( conf.find("nThreads")->second.c_str() );
	if( conf.count("dbOffset") )
		db_offset_ = atoi(conf.find("dbOffset")->second.c_str());
}

MWriteWeightsTest::~MWriteWeightsTest()
{}

void MWriteWeightsTest::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	biLinMod_ = BiLinearModelWeights(linMod.nHorizon, linMod.num_time_slices, om_num_sig_, om_num_err_sig_);

	if( MEnv::Instance()->multiThreadTicker )
		nThreads_ = MEnv::Instance()->nMaxThreadTicker;
	SigSet::Instance()->resize(nThreads_);

	int indx = 0;
	for( vector<pair<int, int> >::const_iterator it = linMod.vHorizonLag.begin(); it != linMod.vHorizonLag.end(); ++it, ++indx )
	{
		if( it->first == write_weights_horizon_ && it->second == write_weights_lag_ )
		{
			iHorizon_ = indx;
			break;
		}
	}
}

void MWriteWeightsTest::beginDay()
{
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel nonLinMod = MEnv::Instance()->nonLinearModel;
	string model = MEnv::Instance()->model;

	int weight_date = nextClose( markets[0], idate );
	biLinMod_.read_weights(weight_date, get_weight_dir(MEnv::Instance()->baseDir, model));

	udate_is_trading_day_ = true;
	if( idate != prevClose(markets[0], nextClose(markets[0], idate)) ) // holiday.
		udate_is_trading_day_ = false;

	if( udate_is_trading_day_ && write_weights_ )
	{
		// Do this dayok or not.
		int dbdate = idate;
		for( int i = 0; i < db_offset_; ++i )
			dbdate = nextClose( markets[0], dbdate );
		cout << "Writing weights for " << dbdate << endl;
		biLinMod_.write_weights_beginDay(idate, dbdate, model, linMod.markets, write_weights_debug_, linMod.gpts);
	}
}

void MWriteWeightsTest::beginMarket()
{
	if( udate_is_trading_day_ )
	{
		string model02 = MEnv::Instance()->model.substr(0, 2);
		string market = MEnv::Instance()->market;
		const vector<string>& tickers = MEnv::Instance()->tickers;
		idate_ = MEnv::Instance()->idate;
		idate_p_ = prevClose(market, idate_);

		idate_p_good_ = get_good_idate(model02, market, idate_p_); // Using idate_p_ for compatibility with Jeff's code. Fitting includes idate_'s data, so use the value from idate_p_.
		get_uid_map(market, idate_p_good_);
	}
}

void MWriteWeightsTest::beginTicker(const string& ticker)
{
	if( udate_is_trading_day_ )
	{
		string model = MEnv::Instance()->model;
		string market = MEnv::Instance()->market;
		int idate = MEnv::Instance()->idate;
		const hff::LinearModel linMod = MEnv::Instance()->linearModel;

		auto itT = mTickerUid_.find(ticker);
		if( itT != mTickerUid_.end() )
		{
			string uid = itT->second;

			int iSig = -1;
			SigC& sig = SigSet::Instance()->get_sig_object(iSig);

			// Read from stockcharacteristics table.
			if( read_chara_weight(sig, model, market, uid, idate_p_good_) )
			{
				get_prediction(sig, uid, ticker, idate);
				if( write_weights_ )
				{
					biLinMod_.write_weights_ticker(model, market, uid, baseTicker(ticker), iHorizon_,
						sig.avgDlyVolume, sig.adjPrevClose, sig.avgDlyVolat, sig.medSprd, sig.inUnivChara, sig.northRST, sig.northTRD, sig.northBP,
						sig.sectype, sig.exchangeChar, debug_noss_);
				}
			}
			SigSet::Instance()->free_sig_object(iSig);
		}
	}
}

void MWriteWeightsTest::endTicker(const string& ticker)
{
}

void MWriteWeightsTest::endMarket()
{
}

void MWriteWeightsTest::endDay()
{
	if( udate_is_trading_day_ && write_weights_ )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		string model = MEnv::Instance()->model;
		string market = MEnv::Instance()->markets[0];

		// Do this dayok or not.
		biLinMod_.write_weights_endDay(model, market, linMod.mCode[0], freeze_univ_);
	}
}

void MWriteWeightsTest::endJob()
{
}

int MWriteWeightsTest::get_good_idate(const string& model02, const string& market, int idate)
{
	int ret_default = idate;

	for( int cnt = 0; cnt < 20; ++cnt )
	{
		int nuniv_today = get_nuniv(model02, market, idate);

		vector<int> past5days;
		{
			int idate_prev = prevClose(market, idate);
			for( int i = 0; i < 5; ++i )
			{
				past5days.push_back(idate_prev);
				idate_prev = prevClose(market, idate_prev);
			}
		}

		int max_nuniv = 0;
		for( vector<int>::iterator it = past5days.begin(); it != past5days.end(); ++it )
		{
			int idate_prev = *it;
			int nuniv_prev = get_nuniv(model02, market, idate_prev);
			if( nuniv_prev > max_nuniv )
				max_nuniv = nuniv_prev;
		}
		bool enough_univ = nuniv_today > 0.7 * max_nuniv;

		if( enough_univ )
			return idate;
		else
			idate = prevClose(market, idate);
	}

	return ret_default;
}

int MWriteWeightsTest::get_nuniv(const string& model02, const string& market, int idate)
{
	string requirements;
	if( model02 == "UF" )
		requirements = " and sectype = 'F' ";
	else if( model02 == "US" )
		requirements = " and sectype != 'F' ";

	char cmd[1000];
	sprintf( cmd, "select %s, uniqueID from stockcharacteristics "
		" where inuniverse = 1 and %s and uniqueID is not null %s",
		mto::compTicker(market).c_str(),
		mto::selChara(market, idate).c_str(),
		requirements.c_str() );
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
	return vv.size();
}

bool MWriteWeightsTest::get_uid_map(const string& market, int idate)
{
	mTickerUid_.clear();

	char cmd[1000];
	sprintf(cmd, "select %s, uniqueID from stockcharacteristics "
		" where %s and uniqueID is not null ",
		mto::compTicker(market).c_str(),
		mto::selChara(market, idate).c_str());
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);

	if( !vv.empty() )
	{
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			string uid = trim((*it)[1]);
			if( !ticker.empty() )
				mTickerUid_[ticker] = uid;
		}
		return true;
	}
	return false;
}

void MWriteWeightsTest::get_prediction(SigC& sig, const string& uid, const string& ticker, int idate)
{
	// Hedge object.
	const SigH* psigh = 0;
	const vector<SigH>* pvSigH = static_cast<const vector<SigH>*>(MEvent::Instance()->get("", "vSigH"));
	if( pvSigH != 0 )
	{
		for( vector<SigH>::const_iterator it = pvSigH->begin(); it != pvSigH->end(); ++it )
		{
			if( it->ticker == ticker )
			{
				psigh = &(*it);
				break;
			}
		}
	}

	// Hedge signal.
	if( psigh != 0 )
	{
		sig.northBP = psigh->northBP;
		sig.northRST = psigh->northRST;
		sig.northTRD = psigh->northTRD;
		sig.tarON = psigh->tarON;
	}
}
