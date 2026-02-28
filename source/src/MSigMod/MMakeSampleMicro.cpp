#include <MSigMod/MMakeSampleMicro.h>
#include <MSigMod/SignalCalculatorMicro.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_signal/HedgeNF.h>
#include <gtlib_signal/HedgeNFadj.h>
#include <gtlib_signal/HedgeMarket.h>
#include <gtlib_signal/HedgeFullLen.h>
#include <gtlib_signal/HedgeFullLenNF.h>
#include <gtlib_signal/sigFtns.h>
#include <gtlib_signal/writeFtns.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <thread>
#include <algorithm>
using namespace std;
using namespace hff;
using namespace gtlib;

MMakeSampleMicro::MMakeSampleMicro(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	debugSigCal_(false),
	sample_all_(false),
	dayOK_(false),
	writeSigmoid_(false),
	preload_(true),
	earlySelectSig_(true),
	smoothTarget_(false),
	discardOutliers_(true),
	removeRestricted_(false),
	doUniversalLinearModel_(true),
	doErrorCorrectionModel_(true),
	usUnivOverride_(false),
	removeHardToBorrow_(false),
	sampling_("reg"),
	schedType_(0),
	verbose_(0),
	nHedgeErrDay_(0),
	desiredSamplesOm_(0),
	desiredSamplesTm_(0),
	weight_source_("calculate"), // db, debugdb, disk, calculate
	use_sigmoid_(false),
	write_ombin_reg_(false),
	write_ombin_tevt_(false),
	write_ombin_nevt_(false),
	write_tmbin_reg_(false),
	write_tmbin_tevt_(false),
	write_tmbin_nevt_(false),
	openDelay_(30),
	cntDay_(0),
	minMsecs_(0),
	maxMsecs_(86400000),
	filterHorizon_(0),
	filterLag_(0),
	dpMicro_(nullptr),
	dataProvider_(nullptr),
	pEstTime_(nullptr)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugSigCal") )
		debugSigCal_ = conf.find("debugSigCal")->second == "true";
	if( conf.count("sampleAll") )
		sample_all_ = conf.find("sampleAll")->second == "true";
	if( conf.count("writeSigmoid") )
		writeSigmoid_ = conf.find("writeSigmoid")->second == "true";
	if( conf.count("useSigmoid") )
		use_sigmoid_ = conf.find("useSigmoid")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("sampling") )
		sampling_ = conf.find("sampling")->second;
	if( conf.count("schedType") )
		schedType_ = atoi(conf.find("schedType")->second.c_str());
	if( conf.count("openDelay") )
		openDelay_ = atoi(conf.find("openDelay")->second.c_str());
	if( conf.count("smoothTarget") )
		smoothTarget_ = conf.find("smoothTarget")->second == "true";
	if( conf.count("discardOutliers") )
		discardOutliers_ = conf.find("discardOutliers")->second == "true";
	if( conf.count("doUniversalLinearModel") )
		doUniversalLinearModel_ = conf.find("doUniversalLinearModel")->second == "true";
	if( conf.count("doErrorCorrectionModel") )
		doErrorCorrectionModel_ = conf.find("doErrorCorrectionModel")->second == "true";
	if( conf.count("usUnivOverride") )
		usUnivOverride_ = conf.find("usUnivOverride")->second == "true";
	if( conf.count("removeHardToBorrow") )
		removeHardToBorrow_ = conf.find("removeHardToBorrow")->second == "true";
	if( conf.count("preload") )
		preload_ = conf.find("preload")->second == "true";
	if( conf.count("removeRestricted") )
		removeRestricted_ = conf.find("removeRestricted")->second == "true";
	if( conf.count("wmodel") )
		wmodel_ = conf.find("wmodel")->second;
	if( conf.count("weightSource") )
		weight_source_ = conf.find("weightSource")->second;
	if( conf.count("writeOmBinReg") )
		write_ombin_reg_ = conf.find("writeOmBinReg")->second == "true";
	if( conf.count("writeOmBinTevt") )
		write_ombin_tevt_ = conf.find("writeOmBinTevt")->second == "true";
	if( conf.count("writeOmBinNevt") )
		write_ombin_nevt_ = conf.find("writeOmBinNevt")->second == "true";
	if( conf.count("writeTmBinReg") )
		write_tmbin_reg_ = conf.find("writeTmBinReg")->second == "true";
	if( conf.count("writeTmBinTevt") )
		write_tmbin_tevt_ = conf.find("writeTmBinTevt")->second == "true";
	if( conf.count("writeTmBinNevt") )
		write_tmbin_nevt_ = conf.find("writeTmBinNevt")->second == "true";
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	write_om_ = write_ombin_reg_ || write_ombin_tevt_;
	write_reg_ = write_ombin_reg_ || write_tmbin_reg_;
	write_evt_ = write_ombin_tevt_ || write_ombin_nevt_ || write_tmbin_tevt_ || write_tmbin_nevt_;
}

MMakeSampleMicro::~MMakeSampleMicro()
{}

void MMakeSampleMicro::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = MEnv::Instance()->model;
	if( wmodel_.empty() )
		wmodel_ = model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const string& baseDir = MEnv::Instance()->baseDir;
	const vector<int>& allIdates = MEnv::Instance()->allIdates;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
	cout << uids_.size() << " instruments." << endl;
	tSources_.read(mto::region(MEnv::Instance()->markets[0]), MEnv::Instance()->sourceFlag);

	if( write_om_ )
	{
		if( weight_source_ == "calculate" )
		{
			double ridgeReg = 100 * 20. / linMod.gridInterval;
			double ridgeUniv_ = ridgeReg;
			double ridgeSS_ = ridgeReg;
			string weightDir = get_weight_dir(baseDir, model);
			string covDir = get_cov_dir(baseDir, model);
			int minPts = linMod.minPts(model);
			biLinMod_ = BiLinearModel(covDir, weightDir,
					linMod.om_univ_fit_days, linMod.om_err_fit_days, minPts,
					linMod.nHorizon, linMod.num_time_slices, om_num_sig_,
					om_num_err_sig_, ridgeUniv_, ridgeSS_, uids_, allIdates);
			if(writeSigmoid_)
				biLinMod_.setWriteSigmoid(true);
			if( use_sigmoid_ )
				biLinMod_.useSigmoid();
		}
		else
		{
			biLinModW_ = BiLinearModelWeights(linMod.nHorizon, linMod.num_time_slices, om_num_sig_, om_num_err_sig_);
			if( use_sigmoid_ )
				biLinModW_.useSigmoid();
		}

		vCorr_.resize(linMod.nHorizon);
		vCorrTot_.resize(linMod.nHorizon);
	}
}

void MMakeSampleMicro::beginDay()
{
	cout << moduleName_ << "::beginDay()" << endl;
	nHedgeErrDay_ = 0;
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

	if( sampling_ == "vol" )
		mercVol_.beginDay(model, idate);
	else if( sampling_ == "order" )
	{
		string market = markets[0];
		char cmd[1000];
		sprintf(cmd, "select symbol, ordermsecs from hforders where"
				" orderschedtype = %d and idate = %d"
				" order by symbol, ordermsecs", schedType_, idate);
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, vv);

		mSampleMsecs_.clear();
		vector<int> vMsecs;
		string prev_ticker;
		int prev_msecs = 0;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if(!prev_ticker.empty() && ticker != prev_ticker)
			{
				mSampleMsecs_[prev_ticker] = vMsecs;
				vMsecs.clear();
			}
			int msecs = atoi((*it)[1].c_str());
			if(msecs != prev_msecs)
				vMsecs.push_back(msecs);
			prev_msecs = msecs;
			prev_ticker = ticker;
		}
		if(!vMsecs.empty())
			mSampleMsecs_[prev_ticker] = vMsecs;
	}
	else if( sampling_ == "trade" )
	{
		string market = markets[0];
		char cmd[1000];
		sprintf(cmd, "select o.symbol, o.ordermsecs from hforders o inner join hforderevents e"
				" on o.orderid = e.orderid and o.idate = %d and e.idate = %d "
				" where o.orderschedtype = %d and e.qty > 0"
				" order by o.symbol, o.ordermsecs", idate, idate, schedType_);
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, vv);

		mSampleMsecs_.clear();
		vector<int> vMsecs;
		string prev_ticker;
		int prev_msecs = 0;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if(!prev_ticker.empty() && ticker != prev_ticker)
			{
				mSampleMsecs_[prev_ticker] = vMsecs;
				vMsecs.clear();
			}
			int msecs = atoi((*it)[1].c_str());
			if(msecs != prev_msecs)
				vMsecs.push_back(msecs);
			prev_msecs = msecs;
			prev_ticker = ticker;
		}
		if(!vMsecs.empty())
			mSampleMsecs_[prev_ticker] = vMsecs;
	}
	else if( sampling_ == "notrade" )
	{
		string market = markets[0];
		char cmd[1000];
		sprintf(cmd, "select o.symbol, o.ordermsecs from hforders o inner join hforderevents e"
				" on o.orderid = e.orderid and o.idate = %d and e.idate = %d"
				" where o.orderschedtype = %d"
				" group by o.symbol, o.ordermsecs"
				" having sum(e.qty) is null"
				" order by o.symbol, o.ordermsecs",
				idate, idate, schedType_);
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, vv);

		mSampleMsecs_.clear();
		vector<int> vMsecs;
		string prev_ticker;
		int prev_msecs = 0;
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			if(!prev_ticker.empty() && ticker != prev_ticker)
			{
				mSampleMsecs_[prev_ticker] = vMsecs;
				vMsecs.clear();
			}
			int msecs = atoi((*it)[1].c_str());
			if(msecs != prev_msecs)
				vMsecs.push_back(msecs);
			prev_msecs = msecs;
			prev_ticker = ticker;
		}
		if(!vMsecs.empty())
			mSampleMsecs_[prev_ticker] = vMsecs;
	}

	pSig_ = new vector<hff::SigC>();
	pHedge_ = getHedgeObject(model, idate);

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

	// Read the linear model.
	if( write_om_ )
	{
		// Read ranges
		if( use_sigmoid_ ) {
			if( weight_source_ == "calculate" )
				biLinMod_.read_range(idate);
			else
				biLinModW_.read_range(idate, get_weight_dir(MEnv::Instance()->baseDir, wmodel_));
		}

		// Read weights
		if( weight_source_ == "db" )
		{
			string dbname = mto::hfpar(market, idate);
			biLinModW_.read_db_weights(model, market, idate, linMod.mCode[0], mTickerUid_, linMod.gpts, dbname);
		}
		else if( weight_source_ == "debugdb" )
		{
			string dbname = mto::hfdbg(market);
			biLinModW_.read_db_weights(model, market, idate, linMod.mCode[0], mTickerUid_, linMod.gpts, dbname);
		}
		else if( weight_source_ == "disk" )
		{
			biLinModW_.read_weights(idate, get_weight_dir(MEnv::Instance()->baseDir, wmodel_));
		}
		else if( weight_source_ == "calculate" )
		{
			biLinMod_.beginDay(idate);
		}
	}
}

void MMakeSampleMicro::set_restricted(const string& model, int idate)
{
	vector<string> markets = MEnv::Instance()->markets;
	restricted_.clear();
	string model02 = model.substr(0, 2);
	if(model02 == "US")
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

gtlib::Hedge* MMakeSampleMicro::getHedgeObject(const string& model, int idate)
{
	string model02 = model.substr(0, 2);
	if( model02 == "US" )
		return new HedgeFullLenNF(idate);
	else
		return new HedgeFullLen();

	return nullptr;
}

bool MMakeSampleMicro::enough_day_ombin()
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	bool enough_day_ombin = weight_source_ != "calculate" ||
		biLinMod_.goodModel();
	return enough_day_ombin;
}

void MMakeSampleMicro::beginMarket()
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
	dayOK_ = mto::dataOK(market, idate_);
	sessions_ = Sessions(market, idate_);
	auctionMkts_ = "";
	if( "US" == market )
		auctionMkts_ = "N";

	idxfp_.LoadData(*GEX::Instance()->get(market), idate, mto::bindirReturn(market, idate), mto::longTicker(market));
	idxfp_.LoadFilters(idate, mto::hfpar(market, idate), *GEX::Instance()->get(market));
	initTickProvider();

	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	desiredSamplesOm_ = (linMod.closeMsecs - linMod.openMsecs) / (linMod.om_bin_sample_freq * 1000);
	desiredSamplesTm_ = (linMod.closeMsecs - linMod.openMsecs) / (linMod.tm_bin_sample_freq * 1000);
	if( sample_all_ )
	{
		desiredSamplesOm_ = max_int_;
		desiredSamplesTm_ = max_int_;
	}

	closeDBConnections(market);
}

void MMakeSampleMicro::closeDBConnections(const string& market)
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

void MMakeSampleMicro::beginTicker(const string& ticker)
{
	int idate = MEnv::Instance()->idate;

	if( !dayOK_ )
		return;
	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		boost::mutex::scoped_lock lock(est_mutex_);
		pEstTime_->beginTicker();
	}
	bool isRestricted = binary_search(restricted_.begin(), restricted_.end(), ticker);
	if(isRestricted)
		return;

	string market = MEnv::Instance()->market;
	string model = MEnv::Instance()->model;
	string model2 = model.substr(0, 2);
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	auto itT = mTickerUid_.find(ticker);
	if( itT != mTickerUid_.end() )
	{
		string uid = itT->second;
		SigC sig;

		// Read from stockcharacteristics table.
		bool requireTickValid = true;
		const hff::CharaContainer* ch = static_cast<const hff::CharaContainer*>(MEvent::Instance()->get("", "CharaContainer"));
		bool charaok = read_chara_data(ch, sig, model, market, uid, linMod.on_target_clip, removeHardToBorrow_, requireTickValid,
				idate_, idate_p_, idate_p2_, idate_p3_, idate_p4_, idate_p5_, idate_p6_, idate_p7_, idate_p8_,  idate_n_);
		if( charaok && sig.avgDlyVolume > 0. && sig.avgDlyVolat > 0. )
		{
			const vector<OrderQty>* tradeQty = static_cast<const vector<OrderQty>*>(MEvent::Instance()->get(ticker, "tradeQty"));
			SignalCalculatorMicro sigcal(idate, uid, ticker, sig, &sessions_, linMod.openMsecs, linMod.closeMsecs, auctionMkts_, tradeQty);
			if(debugSigCal_)
				sigcal.setDebug();
			if(!discardOutliers_)
				sigcal.setDiscardOutliers(false);
			if( linMod.halt_tickers )
				sigcal.setStatusChange(&statusChange_);
			if(openDelay_ > 0)
				sigcal.setOpenDelay(openDelay_);

			vector<int> vSampleMsecs;
			if( write_ombin_reg_ || write_tmbin_reg_ )
			{
				vSampleMsecs = getSampleMsecsTicker(idate, ticker, sig.avgDlyVolat / basis_pts_);
				int tmSampleScale = getTmSampleScale();
				sigcal.setTmSampleMsecs(vSampleMsecs, tmSampleScale);
			}
			vector<UsecsTime> regTime;
			for(int msecs : vSampleMsecs)
				regTime.push_back(UsecsTime(msecs));

			// Booktrade sampling.
			if( write_evt_ )
			{
				if(write_ombin_nevt_ || write_tmbin_nevt_)
				{
					const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
					int qStep = quotes->size() / desiredSamplesOm_ / 2;
					sigcal.setQuoteSampleStep(max(1, qStep));
				}

				if(write_ombin_tevt_ || write_tmbin_tevt_)
				{
					const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));
					int tStep = trades->size() / desiredSamplesOm_ / 3;
					sigcal.setBooktradeSampleStep(max(1, tStep));
				}
			}

			sigcal.setTradeSampleStep(0);
			TCM_micro tcm;
			tcm.SetIdxFutPred(&idxfp_);//for index/future data (if needed)
			tcm.InsertSymbol(&sigcal);
			tcm.SetRegularCB(regTime);

			TickProviderMulti<UsecsTime, OrderDataMicro> provider;
			provider.AddConsumer(&tcm);
			{
				boost::mutex::scoped_lock lock(load_mutex_);
				provider.LoadData(idate, ticker, dataProvider_);
			}

			provider.Run();
			sigcal.endTicker();

			if( pHedge_ != nullptr )
			{
				boost::mutex::scoped_lock lock(hedge_mutex_);
				pHedge_->updateTicker(linMod, nonLinMod, ticker, sig.inUnivFit, sig.close, sig.tarON, sig.tarClcl, sigcal.getFirstTrade(), sigcal.getQuotes(), smoothTarget_);
			}

			// Use local quotes sample. Do not use TickProvider.NbboAt().
			sigcal.calculate_targets(sig, sigcal.getQuotes(), smoothTarget_);

			get_prediction(sig, uid, ticker, idate);

			{
				boost::mutex::scoped_lock lock(sig_mutex_);
				if( earlySelectSig_ )
					selectSig(sig);
				(*pSig_).push_back(std::move(sig));
			}
		}
	}

	// with MT
}

void MMakeSampleMicro::endTicker(const string& ticker)
{
}

void MMakeSampleMicro::endMarket()
{
	if( dpMicro_ != nullptr )
	{
		delete dpMicro_;
		dpMicro_ = nullptr;
	}
}

void MMakeSampleMicro::endDay()
{
	if( cntDay_ == 1 && pEstTime_ != nullptr )
		pEstTime_->beginEndDay();

	if( nHedgeErrDay_ > 0 )
		cout << "WARNING MMakeSampleMicro::endDay(): " << nHedgeErrDay_ << " tickers could not be hedged.\n";

	write_signal(pHedge_, pSig_, enough_day_ombin());
	if( writeSigmoid_ )
		write_sigmoid_range();

	if( write_om_ )
		linearModelEndDay();

	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		pEstTime_->endDay();
		delete pEstTime_;
	}
	delete pHedge_;
	delete pSig_;
}

void MMakeSampleMicro::endJob()
{
	if( write_om_ )
	{
		if( weight_source_ == "calculate" )
			biLinMod_.endJob();
	}
}

vector<int> MMakeSampleMicro::getSampleMsecsTicker(int idate, const string& ticker, float medVolat)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;

	int stepSec = 0;
	if( write_ombin_reg_ )
		stepSec = linMod.om_bin_sample_freq;
	else if( write_tmbin_reg_ )
		stepSec = linMod.tm_bin_sample_freq;

	vector<int> sampleMsecs;
	if( stepSec > 0 && write_reg_ )
	{
		if( sampling_ == "irreg" )
			sampleMsecs = getSampleMsecsIrreg(ticker, idate, linMod.openMsecs, linMod.closeMsecs, stepSec);
		else if( sampling_ == "vol" )
			sampleMsecs = getSampleMsecs(mercVol_, ticker, linMod.openMsecs, linMod.closeMsecs, stepSec);
		else if( sampling_ == "volat" )
		{
			const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));
			sampleMsecs = getSampleMsecsVolat(linMod.openMsecs, linMod.closeMsecs, stepSec, medVolat, trades, sessions_);
		}
		else if( sampling_ == "order" || sampling_ == "trade" || sampling_ == "notrade")
			sampleMsecs = mSampleMsecs_[ticker];
		else if( sampling_ == "reg" )
			sampleMsecs = getSampleMsecs(linMod.openMsecs, linMod.closeMsecs, stepSec);
	}
	return sampleMsecs;
}

int MMakeSampleMicro::getTmSampleScale()
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	int scale = 1;
	if( write_ombin_reg_ )
		scale = linMod.tm_bin_sample_freq / linMod.om_bin_sample_freq;
	return scale;
}

//
// Which signal to write.
//

void MMakeSampleMicro::selectSig(SigC& sig)
{
	if( write_ombin_reg_ || write_tmbin_reg_ )
		select_sample_type("reg", regular_sample_, sig);
	if( write_ombin_tevt_ || write_tmbin_tevt_ )
		select_sample_type("tevt", book_trade_event_, sig);
	if( write_ombin_nevt_ || write_tmbin_nevt_ )
		select_sample_type("nevt", nbbo_event_, sig);

	// remove invalid sigs.
	sig.sI.erase(remove_if(sig.sI.begin(), sig.sI.end(),
				[](StateInfo x){return x.valid == 0 && x.validTm == 0;}), sig.sI.end());
}

void MMakeSampleMicro::select_sample_type(const string& desc, int sampleType, SigC& sig)
{
	if( sig.inUnivFit )
		select_events(sig.sI, sampleType);
}

void MMakeSampleMicro::select_events(vector<StateInfo>& sI, int sampleType)
{
	if( regular_sample_ & sampleType )
		select_events_reg(sI);
	else
		select_events_evt(sI, sampleType);
}

void MMakeSampleMicro::select_events_reg(vector<StateInfo>& sI)
{
	for( auto it = sI.begin(); it != sI.end(); ++it )
	{
		if( regular_sample_ & it->sampleType )
		{
			bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
			if( !discardOutliers_ || (it->gptOK && sprdOK) )
			{
				it->valid = 1;
				if( it->isTmSample )
					it->validTm = 1;
			}
		}
	}
}

void MMakeSampleMicro::select_events_evt(vector<StateInfo>& sI, int sampleType)
{
	vector<int> vIndexTot = getGoodDataIndex(sI, sampleType);
	vector<int> vIndexOmWrite = getSampledIndex(vIndexTot, desiredSamplesOm_);
	vector<int> vIndexTmWrite = getSampledIndex(vIndexOmWrite, desiredSamplesTm_);

	for( int i : vIndexOmWrite )
		sI[i].valid = 1;
	for( int i : vIndexTmWrite )
		sI[i].validTm = 1;
}

vector<int> MMakeSampleMicro::getGoodDataIndex(vector<StateInfo>& sI, int sampleType)
{
	vector<int> vIndexTot;
	auto sIBeg = begin(sI);
	int last_msso = 0;
	for( auto it = begin(sI); it != end(sI); ++it )
	{
		if( sampleType & it->sampleType )
		{
			it->valid = 0;
			it->validTm = 0;
			bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
			bool persists = it->bidPersists && it->askPersists;
			if( persists && it->msso > last_msso )
			{
				if(!discardOutliers_ || it->gptOK && sprdOK)
					vIndexTot.push_back(distance(sIBeg, it));
			}
			last_msso = it->msso;
		}
	}
	return vIndexTot;
}

vector<int> MMakeSampleMicro::getSampledIndex(const vector<int>& vIndexTot, int desiredSamples)
{
	vector<int> sampledIndex;
	double tot = vIndexTot.size();
	if( tot > 0. )
	{
		if( tot > desiredSamples )
		{
			double interval = tot / desiredSamples;
			for( int i = 0; i < desiredSamples; ++i )
				sampledIndex.push_back(vIndexTot[static_cast<int>(i * interval)]);
		}
		else
			sampledIndex = vIndexTot;
	}
	return sampledIndex;
}

//
// Write.
//

void MMakeSampleMicro::write_signal(gtlib::Hedge* pHedge, std::vector<hff::SigC>* pSig, bool enough_day_ombin)
{
	int idate = MEnv::Instance()->idate;
	const string& baseDir = MEnv::Instance()->baseDir;
	const string& model = MEnv::Instance()->model;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	if( pHedge != nullptr )
	{
		pHedge->calculateHedge(linMod.n1sec);
		for( auto& sig : (*pSig) )
		{
			if( sig.inUnivFit == 1 )
			{
				const TargetHedger* pHedger = pHedge->getTargetHedger(sig.ticker);
				get_hedged_targets(sig, pHedger);
			}
		}
	}

	if( (write_ombin_reg_ && enough_day_ombin) || write_tmbin_reg_ )
		write_sample_type(write_ombin_reg_, write_tmbin_reg_, idate, "reg", regular_sample_, pSig);
	if( write_ombin_tevt_ || write_tmbin_tevt_ )
		write_sample_type(write_ombin_tevt_, write_tmbin_tevt_, idate, "tevt", book_trade_event_, pSig);
	if( write_ombin_nevt_ || write_tmbin_nevt_ )
		write_sample_type(write_ombin_nevt_, write_tmbin_nevt_, idate, "nevt", nbbo_event_, pSig);
	if(debugSigCal_)
		printTakeRat();
}

void MMakeSampleMicro::printTakeRat()
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	int nTot = 0;
	int nPer = 0;
	int nTaken = 0;
	int nPerNotTaken = 0;
	for( auto& sig : (*pSig_) )
	{
		if( sig.inUnivFit )
		{
			int Npts = sig.sI.size();
			for(int j = 0; j < Npts; ++j)
			{
				const StateInfo& sI = sig.sI[j];
				if( sI.sampleType & book_trade_event_ )
				{
					int msecs = sI.msso + linMod.openMsecs;
					bool persist = sI.bidPersists && sI.askPersists;

					++nTot;
					if(persist)
						++nPer;
					if(sI.eventTakeRatMkt > 0.99)
						++nTaken;
					if(persist && sI.eventTakeRatMkt < 0.99)
						++nPerNotTaken;
				}
			}
		}
	}
	printf("nTot %d nPersist %d nAllTaken %d nPerNotAllTkn %d\n", nTot, nPer, nTaken, nPerNotTaken);
}

void MMakeSampleMicro::write_sample_type(bool write_om, bool write_tm, int idate,
		const string& desc, int sampleType, vector<SigC>* pSig)
{
	const string& baseDir = MEnv::Instance()->baseDir;
	const string& model = MEnv::Instance()->model;
	if( !earlySelectSig_ )
	{
		for( auto& sig : (*pSig) )
		{
			if( sig.inUnivFit )
				select_events(sig.sI, sampleType);
		}
	}
	if(write_om)
		write_signal_file(baseDir, model, idate, "om", desc, sampleType, pSig);
	if(write_tm)
		write_signal_file(baseDir, model, idate, "tm", desc, sampleType, pSig);
}

void MMakeSampleMicro::write_signal_file(const string& baseDir, const string& model, int idate,
		const string& sigType, const string& desc, int sampleType, vector<SigC>* pSig)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	const NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	ofstream binReg( get_sig_path(baseDir, model, idate, sigType, desc).c_str(), ios::out | ios::binary );
	ofstream binTxtReg( get_sigTxt_path(baseDir, model, idate, sigType, desc).c_str(), ios::out );
	write_bin_header(sigType, binReg, binTxtReg, linMod, nonLinMod);
	int cnt = 0;

	bool sortTickers = false;
	if(sortTickers)
	{
		vector<string> wTickers;
		for( auto& sig : (*pSig) )
		{
			if( sig.inUnivFit )
			{
				const string& ticker = sig.ticker;
				wTickers.push_back(ticker);
			}
		}
		vector<int> sortedIndex;
		gsl_heapsort_index<int, string>(sortedIndex, wTickers);
		int n = sortedIndex.size();
		for(int i = 0; i < n; ++i)
		{
			auto& sig = (*pSig)[i];
			const string& ticker = sig.ticker;
			const string uid = mTickerUid_[ticker];
			//cnt += write_bin_evt(sigType, binReg, binTxtReg, sig, uid, ticker,
				//minMsecs_, maxMsecs_, sampleType, linMod, nonLinMod);
		}

	}
	else
		for( auto& sig : (*pSig) )
		{
			if( sig.inUnivFit )
			{
				const string& ticker = sig.ticker;
				const string& uid = mTickerUid_[sig.ticker];
				cnt += write_bin_evt(sigType, binReg, binTxtReg, sig, uid, ticker,
						minMsecs_, maxMsecs_, sampleType, linMod, nonLinMod);
			}
		}
	finish_file(true, binReg, binTxtReg, cnt);
}

void MMakeSampleMicro::linearModelEndDay()
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	int idate = MEnv::Instance()->idate;

	if( weight_source_ == "calculate" )
		biLinMod_.endDay(idate);

	for( int iT = 0; iT < linMod.nHorizon; ++iT )
	{
		cout << itos(linMod.vHorizonLag[iT].first) + ";" + itos(linMod.vHorizonLag[iT].second) << "\t";
		cout << "corr " << vCorr_[iT].corr() << " corrTot " << vCorrTot_[iT].corr() << endl;
	}
}

void MMakeSampleMicro::finish_file(bool do_write, ofstream& ofbin, ofstream& ofbintxt, int& cnt)
{
	if( do_write )
	{
		update_ncols(ofbin, cnt);

		ofbin.close();
		ofbin.clear();

		ofbintxt.close();
		ofbintxt.clear();

		cnt = 0;
	}
}

void MMakeSampleMicro::write_sigmoid_range()
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<vector<float>> vv(64, vector<float>());
	for( auto& sig : (*pSig_) )
	{
		int Npts = sig.sI.size();
		for(int j = 0; j < Npts; ++j)
		{
			const StateInfo& sI = sig.sI[j];
			if( sI.sampleType & regular_sample_ && sI.valid )
			{
				int msecs = sI.msso + linMod.openMsecs;
				for( int i = 0; i < om_num_basic_sig_; ++i )
				{
					int indx1 = i;
					int indx2 = i + om_num_basic_sig_;
					int indx3 = i + 2*om_num_basic_sig_;
					int indx4 = i + 3*om_num_basic_sig_;
					vv[indx1].push_back(sI.sig1[i]);
					vv[indx2].push_back(sig.logVolu * sI.sig1[i]);
					vv[indx3].push_back(sig.logPrice * sI.sig1[i]);
					vv[indx4].push_back(sI.absSprd * sI.sig1[i]);
				}
			}
		}
	}
	for( auto& v : vv )
		sort(begin(v), end(v));
	float Ndata = vv[0].size();
	int Ninput = vv.size();

	char buf[400];
	int idate = MEnv::Instance()->idate;
	string out_filename = biLinMod_.get_daily_range_file(idate);
	ofstream ofs(out_filename.c_str());
	for( int i = 0; i < Ninput; ++i )
	{
		float r1 = vv[i][int(Ndata * 0.005)];
		float r2 = vv[i][int(Ndata * 0.995)];
		float ravg = (r2 - r1) / 2.;
		sprintf(buf, "%.4f\n", i, ravg);
		ofs << buf;
	}
}

void MMakeSampleMicro::initTickProvider()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	if( dpMicro_ != nullptr )
	{
		delete dpMicro_;
		dpMicro_ = nullptr;
	}
	dpMicro_ = new TickDataProviderMicro<TradeInfo,OrderDataMicro>();

	bool initDefault = true;
	if(initDefault)
	{
		Exchange ex("US");
		dpMicro_->SetToDefault(ex);
	}
	else // tSources_ needs update for Micro data.
	{
		// nbbodir.
		vector<string> dirs = tSources_.nbbodirectory(market, idate);
		for( auto& dir : dirs )
			dpMicro_->AddTradeRoot(dir);

		// bookdir.
		if( "US" == market )
		{
			auto ecns = linMod.get_ecns(idate);
			for( auto& ecn : ecns )
			{
				vector<string> dirs = tSources_.bookdirectory(ecn, idate);
				for( auto& dir : dirs )
					dpMicro_->AddBookRoot(ecn[1], dir);
			}
		}
		else
		{
			// main market.
			vector<string> dirs = tSources_.bookdirectory(market, idate);
			for( auto& dir : dirs )
				dpMicro_->AddBookRoot(mto::code(market)[0], dir);

			// ecns.
			for( auto& ecn : okECNs_ )
			{
				vector<string> dirs = tSources_.bookdirectory(ecn, idate);
				for( auto& dir : dirs )
					dpMicro_->AddBookRoot(ecn[1], dir);
			}
		}
	}
	if( preload_ )
	{
		if( cntDay_ == 1 )
			cout << market << " Begin preload " << getTimerInfoSimple() << endl;
		dpMicro_->PreloadData(idate, MEnv::Instance()->tickers);
		if( cntDay_ == 1 )
			cout << market << " End preload " << getTimerInfoSimple() << endl;
	}
	dataProvider_ = dpMicro_;
}

void MMakeSampleMicro::get_hedged_targets(SigC& sig, const TargetHedger* pHedger)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	// Hedge object.
	if( pHedger == nullptr )
		++nHedgeErrDay_;
	else
	{
		//if(!nfFromHfuniverse_)
		if(!linMod.nfFromHfuniv)
		{
			sig.northBP = pHedger->getNorthBP();
			sig.northRST = pHedger->getNorthRST();
			sig.northTRD = pHedger->getNorthTRD();
		}
		sig.tarONuh = sig.tarON;
		sig.tarON = pHedger->getHedgedTarON(sig.tarON);
		sig.tarClcluh = sig.tarClcl;
		sig.tarClcl = pHedger->getHedgedTarClcl(sig.tarClcl);

		for( int k = 0; k < Npts; ++k )
		{
			int sec = sI[k].msso / 1000;
			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			{
				int len = nonLinMod.vHorizonLag[iT].first;
				int lag = nonLinMod.vHorizonLag[iT].second;
				sI[k].target[iT] = pHedger->getHedgedTarget(sI[k].target[iT], sec, iT, len, lag);
			}

			{
				sI[k].target11Close = pHedger->getHedgedTarget11Close(sI[k].target11Close, sec);
				sI[k].target71Close = pHedger->getHedgedTarget71Close(sI[k].target71Close, sec);
				sI[k].targetClose = pHedger->getHedgedTargetClose(sI[k].targetClose, sec);
			}

			sI[k].targetNextClose = 0.;
		}
	}
}

void MMakeSampleMicro::get_hedged_targets(SigC& sig, const vector<hff::SigH>* pvSigH)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	// Hedge object.
	const hff::SigH* psigh = 0;
	if( pvSigH == 0 )
		++nHedgeErrDay_;
	else
	{
		for( auto it = pvSigH->begin(); it != pvSigH->end(); ++it )
		{
			if( it->ticker == sig.ticker )
			{
				psigh = &(*it);
				break;
			}
		}
	}

	// Hedge signal.
	if( psigh != 0 )
	{
		//if(!nfFromHfuniverse_)
		if(!linMod.nfFromHfuniv)
		{
			sig.northBP = psigh->northBP;
			sig.northRST = psigh->northRST;
			sig.northTRD = psigh->northTRD;
		}
		sig.tarON = psigh->tarON;
		sig.tarClcl = psigh->tarClcl;
	}

	for( int k = 0; k < Npts; ++k )
	{
		if( psigh != 0 )
		{
			int sec = sI[k].msso / 1000;
			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			{
				int len = nonLinMod.vHorizonLag[iT].first;
				int lag = nonLinMod.vHorizonLag[iT].second;
			}
		}
	}
}

void MMakeSampleMicro::get_prediction(SigC& sig, const string& uid, const string& ticker, int idate)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	const vector<int>& useom = linMod.useom;
	const vector<int>& useomErr = linMod.useomErr;
	vector<hff::StateInfo>& sI = sig.sI;
	int Npts = sI.size();

	vector<int> vIndx;
	vIndx.push_back(4);
	vIndx.push_back(5);
	vIndx.push_back(6);
	vIndx.push_back(7);
	vIndx.push_back(12);
	vIndx.push_back(13);

	BiLinearModel::LinRegStatTicker* linRegStat = nullptr;
	if( weight_source_ == "calculate" && write_ombin_reg_ )
		linRegStat = biLinMod_.getNewLinRegStatTicker();

	int n1secSlice = ceil( float(linMod.n1sec - 1) / linMod.num_time_slices );
	for( int k = 0; k < Npts; ++k )
	{
		int sec = sI[k].msso / 1000;
		int timeIdx = sec / n1secSlice;

		if( sI[k].gptOK == 1 && (write_om_) )
		{
			for( int iT = 0; iT < linMod.nHorizon; ++iT )
			{
				// Set gSigs.
				for( auto it = vIndx.begin(); it != vIndx.end(); ++it )
					sI[k].sig1[*it] = 0.;

				// Construct predName from the filter with the matching horizon.
				int filterHorizon = 0;
				int filterLag = 0;
				if( filterHorizon_ > 0 )
				{
					filterHorizon = filterHorizon_;
					filterLag = filterLag_;
				}
				else
				{
					filterHorizon = linMod.vHorizonLag[iT].first;
					filterLag = linMod.vHorizonLag[iT].second;
				}
				int indxIndx = 0;
				const vector<hff::IndexFilter>& filters = MEnv::Instance()->indexFilters.filters;
				for( auto itf = filters.begin(); itf != filters.end() && indxIndx < 6; ++itf )
				{
					const hff::IndexFilter& filter = *itf;
					if( filterHorizon == filter.horizon && filterLag == filter.targetLag )
					{
						string predName = filter.title() + "_pred";
						const vector<float>* vPred = static_cast<const vector<float>*>(MEvent::Instance()->get("", predName));
						if( vPred != 0 )
						{
							sI[k].sig1[vIndx[indxIndx]] = (*vPred)[sec];
							++indxIndx;
						}
					}
				}

				// Get predictors.
				for( int i = 0; i < hff::om_num_basic_sig_; ++i )
				{
					int indx1 = i;
					int indx2 = i + om_num_basic_sig_;
					int indx3 = i + 2 * om_num_basic_sig_;
					int indx4 = i + 3 * om_num_basic_sig_;

					sI[k].om[indx1] = useom[indx1] * sI[k].sig1[i];
					sI[k].om[indx2] = useom[indx2] * sig.logVolu * sI[k].sig1[i];
					sI[k].om[indx3] = useom[indx3] * sig.logPrice * sI[k].sig1[i];
					sI[k].om[indx4] = useom[indx4] * sI[k].absSprd * sI[k].sig1[i];
				}

				for( int i = 0; i < om_num_err_sig_; ++i )
					sI[k].omErr[i] = useomErr[i] * sI[k].sig1[i];

				// Calculate predictions.
				double prIndex = 0.;
				double pr = 0.;
				double prErr = 0.;
				if( weight_source_ == "db" || weight_source_ == "debugdb" )
				{
					pr = biLinModW_.predDB(timeIdx, uid, sI[k].om);
				}
				else if( weight_source_ == "disk" )
				{
					prIndex = biLinModW_.predIndex(iT, timeIdx, sI[k].om) + biLinModW_.predErrIndex(iT, uid, sI[k].omErr);
					pr = biLinModW_.pred(iT, timeIdx, sI[k].om);
					prErr = biLinModW_.predErr(iT, uid, sI[k].omErr);
				}
				else if( weight_source_ == "calculate" )
				{
					prIndex = biLinMod_.predIndex(iT, timeIdx, sI[k].om) + biLinMod_.predErrIndex(iT, uid, sI[k].omErr);
					pr = biLinMod_.pred(iT, timeIdx, sI[k].om);
					prErr = biLinMod_.predErr(iT, uid, sI[k].omErr);
				}

				sI[k].predIndex[iT] = prIndex;
				sI[k].sigIndex1[iT] = sI[k].sig1[4];
				sI[k].sigIndex2[iT] = sI[k].sig1[5];
				sI[k].sigIndex3[iT] = sI[k].sig1[6];
				sI[k].sigIndex4[iT] = sI[k].sig1[7];
				sI[k].sigIndex5[iT] = sI[k].sig1[12];
				sI[k].sigIndex6[iT] = sI[k].sig1[13];

				sI[k].pred[iT] = pr;
				sI[k].predErr[iT] = prErr;

				// Get error targets.
				if( iT < linMod.nHorizon )
					sI[k].targetErr[iT] = sI[k].target[iT] - sI[k].pred[iT];
				else
					sI[k].targetErr[iT] = sI[k].target[iT];

				if( weight_source_ == "calculate" && write_ombin_reg_ )
					batch_update_biLin(k, linRegStat, timeIdx, uid, sig, sI[k]);
				if( k >= skip_first_secs_ / linMod.gridInterval )
				{
					vCorr_[iT].add(sI[k].pred[iT], sI[k].target[iT]);
					vCorrTot_[iT].add(sI[k].pred[iT] + sI[k].predErr[iT], sI[k].target[iT]);
				}
			}
			float indxPred1 = sI[k].sigIndex1[0];
			float indxPred2 = 0.;
			if(linMod.market[0] == 'U')
				indxPred2 = sI[k].sigIndex3[0];
			else
				indxPred2 = sI[k].sigIndex2[0];

			if( sI[k].sprd > 20. )
			{
				sI[k].indxPredWS1 = indxPred1;
				sI[k].indxPredWS2 = indxPred2;
			}
			float beta1 = 0.;
			float beta2 = 0.;
			sI[k].indxPred1Adj = indxPred1 * beta1;
			sI[k].indxPred2Adj = indxPred2 * beta2;
			sI[k].indxPred1Sprd = indxPred1 * sI[k].sprd;
			sI[k].indxPred2Sprd = indxPred2 * sI[k].sprd;
		}
	}
	if( weight_source_ == "calculate" && write_ombin_reg_ )
	{
		boost::mutex::scoped_lock lock(bilin_mutex_);
		biLinMod_.update(linRegStat, uid);
	}
	if( linRegStat != nullptr )
		delete linRegStat;
}

void MMakeSampleMicro::batch_update_biLin(int k, BiLinearModel::LinRegStatTicker* linRegStat, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI)
{
	int iT = 0;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	if( k >= skip_first_secs_ / linMod.gridInterval )
	{
		// Update univ model.
		if( sig.inUnivFit == 1 && doUniversalLinearModel_)
			linRegStat->olsmov[timeIdx].add(sI.om, sI.target[iT]);

		// Update err corr model.
		if( biLinMod_.goodUnivModel() && doErrorCorrectionModel_)
			linRegStat->olsmovErr.add(sI.omErr, sI.targetErr[iT]);
	}
}

