#include <MSigMod/MTradeQuoteLatency.h>
#include <MSigMod/TradeQuoteLatencyCalculator.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_signal/sigFtns.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <thread>
#include <algorithm>
using namespace std;
using namespace hff;
using namespace gtlib;

MTradeQuoteLatency::MTradeQuoteLatency(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	debugSigCal_(false),
	preload_(true),
	usUnivOverride_(false),
	samplePosMaxPos_(false),
	removeRestricted_(true),
	allowNegSize_(false),
	allowNegSizeFrom_(0),
	verbose_(0),
	openDelay_(30),
	cntDay_(0),
	minMsecs_(0),
	maxMsecs_(86400000),
	dpMilli_(nullptr),
	dataProvider_(nullptr),
	pEstTime_(nullptr)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugSigCal") )
		debugSigCal_ = conf.find("debugSigCal")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("openDelay") )
		openDelay_ = atoi(conf.find("openDelay")->second.c_str());
	if( conf.count("preload") )
		preload_ = conf.find("preload")->second == "true";
	if( conf.count("removeRestricted") )
		removeRestricted_ = conf.find("removeRestricted")->second == "true";
	if( conf.count("allowNegSize") )
		allowNegSize_ = conf.find("allowNegSize")->second == "true";
	if( conf.count("allowNegSizeFrom") )
		allowNegSizeFrom_ = atoi(conf.find("allowNegSizeFrom")->second.c_str());
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
}

MTradeQuoteLatency::~MTradeQuoteLatency()
{}

void MTradeQuoteLatency::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const string& baseDir = MEnv::Instance()->baseDir;
	const vector<int>& allIdates = MEnv::Instance()->allIdates;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
	cout << uids_.size() << " instruments." << endl;
	tSources_.read(mto::region(MEnv::Instance()->markets[0]), MEnv::Instance()->sourceFlag);
}

void MTradeQuoteLatency::beginDay()
{
	cout << moduleName_ << "::beginDay()" << endl;
	++cntDay_;
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel nonLinMod = MEnv::Instance()->nonLinearModel;
	const string& model = MEnv::Instance()->model;
	mTickerUid_ = get_uid_map(markets, idate, uids_);
	const string& baseDir = MEnv::Instance()->baseDir;
	if( linMod.halt_tickers )
		statusChange_.beginDay(idate);
	if(removeRestricted_)
		set_restricted(model, idate);

	// okECNs.
	okECNs_.clear();
	auto ecns = linMod.get_ecns(idate);
	for( auto it = ecns.begin(); it != ecns.end(); ++it )
	{
		string market = *it;
		if( "US" == market || mto::dataOK(market, idate) )
			okECNs_.push_back(market);
	}

	bool doAfterHours = false;
	string market = markets[0];

	if( cntDay_ == 1 )
		pEstTime_ = new gtlib::EstTime(mTickerUid_.size());
}

void MTradeQuoteLatency::set_restricted(const string& model, int idate)
{
	vector<string> markets = MEnv::Instance()->markets;
	restricted_.clear();
	string model02 = model.substr(0, 2);
	if(model02 == "US" || model02 == "UF")
	{
		char cmd[200];
		sprintf(cmd, "select distinct symbol from hforderparams where idate = %d and maxposition = 0", idate);
		vector<vector<string>> vv;
		GODBC::Instance()->read(mto::hfo(markets[0], idate), cmd, vv);
		for(auto& v : vv)
		{
			string ticker = trim(v[0]);
			if(!ticker.empty())
				restricted_.push_back(ticker);
		}
		sort(restricted_.begin(), restricted_.end());
	}
}

void MTradeQuoteLatency::beginMarket()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	const vector<string>& tickers = MEnv::Instance()->tickers;
	idate_ = idate;
	idate_p_ = prevClose(market, idate_);
	idate_p2_ = prevClose(market, idate_p_);
	idate_p3_ = prevClose(market, idate_p2_);
	idate_p4_ = prevClose(market, idate_p3_);
	idate_p5_ = prevClose(market, idate_p4_);
	idate_p6_ = prevClose(market, idate_p5_);
	idate_p7_ = prevClose(market, idate_p6_);
	idate_p8_ = prevClose(market, idate_p7_);
	idate_n_ = nextClose(market, idate_);
	sessions_ = Sessions(market, idate_);
	auctionMkts_ = "";
	if( "US" == market )
		auctionMkts_ = "N";

	initTickProvider();

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	closeDBConnections(market);
}

void MTradeQuoteLatency::closeDBConnections(const string& market)
{
	GODBC::Instance()->close(mto::hf(market));

	// This is to save the database memory.
	if( "U" == mto::region(market) )
	{
		string db = "equitydata";
		GODBC::Instance()->close(db);
	}
	else
	{
		string db = mto::hf(market);
		GODBC::Instance()->close(db);
	}

}

void MTradeQuoteLatency::beginTicker(const string& ticker)
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	string model = MEnv::Instance()->model;
	string model2 = model.substr(0, 2);

	// If model is for multiple markets, skip the tickers from other markets.
	if(mto::isInternational(market) && market[1] != ticker[0])
		return;

	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		boost::mutex::scoped_lock lock(est_mutex_);
		pEstTime_->beginTicker();
	}
	bool isRestricted = binary_search(restricted_.begin(), restricted_.end(), ticker);
	if(isRestricted)
		return;

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	auto itT = mTickerUid_.find(ticker);
	if( itT != mTickerUid_.end() )
	{
		string uid = itT->second;
		SigC sig;

		// Read from stockcharacteristics table.
		bool requireTickValid = !(idate >=20180912 && idate <= 20190610); // tickvalid on stochchara is not updated for new data.
		const hff::CharaContainer* ch = static_cast<const hff::CharaContainer*>(MEvent::Instance()->get("", "CharaContainer"));
		bool charaok = read_chara_data(ch, sig, model, market, uid, linMod.on_target_clip, samplePosMaxPos_, requireTickValid,
				idate_, idate_p_, idate_p2_, idate_p3_, idate_p4_, idate_p5_, idate_p6_, idate_p7_, idate_p8_,  idate_n_);
		if( charaok && sig.avgDlyVolume > 0. && sig.avgDlyVolat > 0. )
		{
			TradeQuoteLatencyCalculator sigcal(idate, uid, ticker, &sessions_, linMod.openMsecs, linMod.closeMsecs);
			if(debugSigCal_)
				sigcal.setDebug();
			if( linMod.halt_tickers )
				sigcal.setStatusChange(&statusChange_);

			TCM_classic tcmc(ticker, &sigcal, linMod.openMsecs, linMod.closeMsecs, auctionMkts_);
			if(idate <= linMod.allowNegSizeTo)
			{
				TCS_classic* tcsc = tcmc.SingleConverter();
				if(tcsc != nullptr) tcmc.SingleConverter()->SetAllowNegSize(true);
			}

			TickProviderMulti<UsecsTime, OrderDataMicro> provider;
			provider.AddConsumer(&tcmc);
			{
				boost::mutex::scoped_lock lock(load_mutex_);
				provider.LoadData(idate, ticker, dataProvider_);
			}

			provider.Run();
			sigcal.endTicker();
		}
	}

	// with MT
}

void MTradeQuoteLatency::endTicker(const string& ticker)
{
}

void MTradeQuoteLatency::endMarket()
{
	if( dpMilli_ != nullptr )
	{
		delete dpMilli_;
		dpMilli_ = nullptr;
	}
}

void MTradeQuoteLatency::endDay()
{
	if( cntDay_ == 1 && pEstTime_ != nullptr )
		pEstTime_->beginEndDay();

	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		pEstTime_->endDay();
		delete pEstTime_;
	}
}

void MTradeQuoteLatency::endJob()
{
}

void MTradeQuoteLatency::initTickProvider()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	if( dpMilli_ != nullptr )
	{
		delete dpMilli_;
		dpMilli_ = nullptr;
	}
	dpMilli_ = new TickDataProviderMilli<TradeInfo, OrderDataMicro>();

	// nbbodir.
	// linMod.primaryOnly
	vector<string> dirs = (linMod.primary_only) ? tSources_.bookdirectory(market, idate) : tSources_.nbbodirectory(market, idate);
	for( auto& dir : dirs )
		dpMilli_->AddTradeRoot(dir);

	// bookdir.
	if( "US" == market )
	{
		auto ecns = linMod.get_ecns(idate);
		for( auto& ecn : ecns )
		{
			vector<string> dirs = tSources_.bookdirectory(ecn, idate);
			for( auto& dir : dirs )
				dpMilli_->AddBookRoot(ecn[1], dir);
		}
	}
	else
	{
		// main market.
		vector<string> dirs = tSources_.bookdirectory(market, idate);
		if(idate >= 20180912 && idate <= 20190610)
		{
			string backfill = "/mnt/gf1/tickC/eu_backfill/" + mto::code(market);
			dpMilli_->AddBookRoot(mto::code(market)[0], backfill, false);
			for( auto& dir : dirs )
				dpMilli_->AddBookTradeRoot(mto::code(market)[0], dir);
		}
		else
		{
			for( auto& dir : dirs )
				dpMilli_->AddBookRoot(mto::code(market)[0], dir);
		}

		// ecns.
		// linMod.primaryOnly // redundant.
		if(!linMod.primary_only)
		{
			for( auto& ecn : okECNs_ )
			{
				vector<string> dirs = tSources_.bookdirectory(ecn, idate);
				if(idate >= 20180912 && idate <= 20190610 && ecn == "ET")
				{
					string backfill = "/mnt/gf1/tickC/eu_backfill/" + ecn.substr(1,1);
					dpMilli_->AddBookRoot(ecn[1], backfill, false);
					for( auto& dir : dirs )
						dpMilli_->AddBookTradeRoot(ecn[1], dir);
				}
				else
				{
					for( auto& dir : dirs )
						dpMilli_->AddBookRoot(ecn[1], dir);
				}
			}
		}
	}
	if( preload_ )
	{
		if( cntDay_ == 1 )
			cout << market << " Begin preload " << getTimerInfoSimple() << endl;
		dpMilli_->PreloadData(idate, MEnv::Instance()->tickers);
		if( cntDay_ == 1 )
			cout << market << " End preload " << getTimerInfoSimple() << endl;
	}
	dataProvider_ = dpMilli_;
}
