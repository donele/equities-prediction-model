#include <MSignal/MMakeSampleBB.h>
//#include <MSignal/SigSet.h>
#include <MSignal/SignalCalculatorTest.h>
#include <MSignal/writeSig.h>
#include <MSignal/writeSig_at.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_signal/HedgeNF.h>
#include <gtlib_signal/HedgeNFadj.h>
#include <gtlib_signal/HedgeMarket.h>
#include <gtlib_signal/HedgeFullLen.h>
#include <gtlib_signal/HedgeFullLenNF.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <thread>
#include <algorithm>
using namespace std;
using namespace hff;
using namespace sig;
using namespace gtlib;

MMakeSampleBB::MMakeSampleBB(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	sample_all_(false),
	dayOK_(false),
	threadWrite_(false),
	multHedgedTarget_(false),
	irregularSampling_(false),
	preload_(true),
	verbose_(0),
	iHedgeAlg_(0),
	iTargetAlg_(1),
	nHedgeErrDay_(0),
	desiredSamplesOm_(0),
	desiredSamplesTm_(0),
	weight_source_("db"), // db, debugdb, disk, calculate
	write_ombin_reg_(false),
	write_ombin_tevt_(false),
	write_ombin_eevt_(false),
	write_ombin_cevt_(false),
	write_tmbin_reg_(false),
	write_tmbin_tevt_(false),
	write_tmbin_eevt_(false),
	write_tmbin_cevt_(false),
	cntDay_(0),
	minMsecs_(0),
	maxMsecs_(86400000),
	filterHorizon_(0),
	filterLag_(0),
	dpMilli_(nullptr),
	dataProvider_(nullptr),
	pEstTime_(nullptr)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("sampleAll") )
		sample_all_ = conf.find("sampleAll")->second == "true";
	if( conf.count("threadWrite") )
		threadWrite_ = conf.find("threadWrite")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("multHedgedTarget") )
		multHedgedTarget_ = conf.find("multHedgedTarget")->second == "true";
	if( conf.count("irregularSampling") )
		irregularSampling_ = conf.find("irregularSampling")->second == "true";
	if( conf.count("preload") )
		preload_ = conf.find("preload")->second == "true";
	if( conf.count("iHedgeAlg") )
		iHedgeAlg_ = atoi( conf.find("iHedgeAlg")->second.c_str() );
	if( conf.count("iTargetAlg") )
		iTargetAlg_ = atoi( conf.find("iTargetAlg")->second.c_str() );
	if( conf.count("wmodel") )
		wmodel_ = conf.find("wmodel")->second;
	if( conf.count("weightSource") )
		weight_source_ = conf.find("weightSource")->second;
	if( conf.count("writeOmBinReg") )
		write_ombin_reg_ = conf.find("writeOmBinReg")->second == "true";
	if( conf.count("writeOmBinTevt") )
		write_ombin_tevt_ = conf.find("writeOmBinTevt")->second == "true";
	if( conf.count("writeOmBinEevt") )
		write_ombin_eevt_ = conf.find("writeOmBinEevt")->second == "true";
	if( conf.count("writeOmBinCevt") )
		write_ombin_cevt_ = conf.find("writeOmBinCevt")->second == "true";
	if( conf.count("writeTmBinReg") )
		write_tmbin_reg_ = conf.find("writeTmBinReg")->second == "true";
	if( conf.count("writeTmBinTevt") )
		write_tmbin_tevt_ = conf.find("writeTmBinTevt")->second == "true";
	if( conf.count("writeTmBinEevt") )
		write_tmbin_eevt_ = conf.find("writeTmBinEevt")->second == "true";
	if( conf.count("writeTmBinCevt") )
		write_tmbin_cevt_ = conf.find("writeTmBinCevt")->second == "true";
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	write_om_ = write_ombin_reg_ || write_ombin_tevt_ || write_ombin_eevt_ || write_ombin_cevt_;
}

MMakeSampleBB::~MMakeSampleBB()
{}

void MMakeSampleBB::beginJob()
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
			string covDir = get_cov_dir(baseDir, model);
			string weightDir = get_weight_dir(baseDir, model);
			int minPts = linMod.minPts(model);
			biLinMod_ = BiLinearModel(covDir, weightDir,
					linMod.om_univ_fit_days, linMod.om_err_fit_days, minPts,
					linMod.nHorizon, linMod.num_time_slices, om_num_sig_,
					om_num_err_sig_, ridgeUniv_, ridgeSS_, uids_, allIdates);
		}
		else
		{
			biLinModW_ = BiLinearModelWeights(linMod.nHorizon, linMod.num_time_slices, om_num_sig_, om_num_err_sig_);
		}

		vCorr_.resize(linMod.nHorizon);
		vCorrTot_.resize(linMod.nHorizon);
	}

	//if( write_ombin_reg_ || write_tmbin_reg_ )
	//{
		//int step = linMod.om_bin_sample_freq * 1000;
		//for ( int msecs = linMod.openMsecs + step; msecs < linMod.closeMsecs; msecs += step )
			//sampleMsecsVect_.push_back(msecs);
	//}
}

void MMakeSampleBB::beginDay()
{
	nHedgeErrDay_ = 0;
	++cntDay_;
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel nonLinMod = MEnv::Instance()->nonLinearModel;
	const string& model = MEnv::Instance()->model;
	mTickerUid_ = get_uid_map(markets, idate, uids_);
	const string& baseDir = MEnv::Instance()->baseDir;

	pSig_ = new vector<hff::SigC>();
	pHedge_ = getHedgeObject(model, idate);

	// okECNs.
	okECNs_.clear();
	auto ecns = linMod.get_ecns();
	for( auto it = ecns.begin(); it != ecns.end(); ++it )
	{
		string market = *it;
		if( "US" == market || mto::dataOK(market, idate) )
			okECNs_.push_back(market);
	}

	bool doAfterHours = false;
	string market = markets[0];
	longTicker_ = mto::longTicker(market);

	if( cntDay_ == 1 )
		pEstTime_ = new gtlib::EstTime(mTickerUid_.size());

	// Read the linear model.
	if( write_om_ )
	{
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
			biLinModW_.read_weights(idate, get_weight_dir(MEnv::Instance()->baseDir, wmodel_));
		else if( weight_source_ == "calculate" )
			biLinMod_.beginDay(idate);
	}
}

gtlib::Hedge* MMakeSampleBB::getHedgeObject(const string& model, int idate)
{
	if( iHedgeAlg_ == 1 )
	{
		string model02 = model.substr(0, 2);
		if( model02 == "US" )
			return new HedgeNF(idate);
		else
			return new HedgeMarket();
	}
	else if( iHedgeAlg_ == 2 )
	{
		string model02 = model.substr(0, 2);
		if( model02 == "US" )
			return new HedgeFullLenNF(idate);
		else
			return new HedgeFullLen();
	}
	else if( iHedgeAlg_ == 3 ) // force market hedging for US.
		return new HedgeMarket();
	else if( iHedgeAlg_ == 4 )
	{
		string model02 = model.substr(0, 2);
		if( model02 == "US" )
			return new HedgeNFadj(idate);
		else
			return new HedgeMarket();
	}

	return nullptr;
}

bool MMakeSampleBB::enough_day_ombin()
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	bool enough_day_ombin = weight_source_ != "calculate" ||
		biLinMod_.goodModel();
	return enough_day_ombin || debug_;
}

void MMakeSampleBB::beginMarket()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	const vector<string>& tickers = MEnv::Instance()->tickers;
	idate_ = idate;
	idate_p_ = prevClose(market, idate_);
	idate_pp_ = prevClose(market, idate_p_);
	idate_n_ = nextClose(market, idate_);
	dayOK_ = mto::dataOK(market, idate_);
	longTicker_ = mto::longTicker(market);
	sessions_ = MSessions(market, idate_);
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

void MMakeSampleBB::closeDBConnections(const string& market)
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

void MMakeSampleBB::beginTicker(const string& ticker)
{
	if( !dayOK_ )
		return;
	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		boost::mutex::scoped_lock lock(est_mutex_);
		pEstTime_->beginTicker();
	}

	string market = MEnv::Instance()->market;
	string model = MEnv::Instance()->model;
	string model2 = model.substr(0, 2);
	int idate = MEnv::Instance()->idate;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;

	auto itT = mTickerUid_.find(ticker);
	if( itT != mTickerUid_.end() )
	{
		string uid = itT->second;
		SigC sig;

		// Read from stockcharacteristics table.
		bool charaok = read_chara_data(sig, model, market, uid, idate_, idate_p_, idate_pp_, idate_n_);
		if( debug_ || charaok && sig.avgDlyVolume > 0. && sig.avgDlyVolat > 0. )
		{
			const vector<OrderQty>* tradeQty = static_cast<const vector<OrderQty>*>(MEvent::Instance()->get(ticker, "tradeQty"));

			// SignalCalculator.
			SignalCalculatorTest sigcal(uid, ticker, sig, &sessions_, linMod.openMsecs, linMod.closeMsecs, tradeQty, linMod.exploratoryDelay);

			double desiredQuoteProb = (sig.medNquotes > 0) ? (desiredSamplesOm_ / sig.medNquotes) : -1.;
			double desiredTradeProb = (sig.medNtrades > 0) ? (desiredSamplesOm_ / sig.medNtrades) : -1.;
			double desiredBooktradeProb = (sig.medNbooktrades > 0) ? (desiredSamplesOm_ / sig.medNbooktrades) : -1.;

			sigcal.regularSampleProbability = 1.;
			if( !write_ombin_reg_ && !write_tmbin_reg_ )
				sigcal.regularSampleProbability = -1.;
			sigcal.quoteSampleProbability = -1.;
			sigcal.tradeSampleProbability = -1.;

			sigcal.booktradeSampleProbability = -1.;
			if( write_ombin_tevt_ || write_ombin_eevt_ || write_ombin_cevt_ )
				sigcal.booktradeSampleProbability = 1.;

			TCM_classic tcmc(ticker, &sigcal, linMod.openMsecs, linMod.closeMsecs, auctionMkts_);
			tcmc.SetRegularCB(getSampleMsecs(idate, ticker));
			tcmc.SetIdxFutPred(&idxfp_);

			TickProviderMulti<UsecsTime, OrderDataMicro> provider;
			provider.AddConsumer(&tcmc);
			{
				boost::mutex::scoped_lock lock(load_mutex_);
				provider.LoadData(idate, ticker, dataProvider_);
			}

			provider.Run();
			sigcal.endTicker();

			if( pHedge_ != nullptr )
			{
				boost::mutex::scoped_lock lock(hedge_mutex_);
				if( iHedgeAlg_ == 0 ) // Use tcmc.
					pHedge_->updateTicker(ticker, sig.inUnivFit, sig.tarON, linMod.openMsecs, linMod.closeMsecs, sigcal.getFirstTrade(), tcmc);
				else if( iHedgeAlg_ == 1 || iHedgeAlg_ == 3 || iHedgeAlg_ == 4 ) // Use my sample.
					pHedge_->updateTicker(ticker, sig.inUnivFit, sig.tarON, linMod.openMsecs, linMod.closeMsecs, sigcal.getFirstTrade(), sigcal.getQuotes());
				else if( iHedgeAlg_ == 2 ) // Pass linMod and nonLinMod.
					pHedge_->updateTicker(linMod, nonLinMod, ticker, sig.inUnivFit, sig.tarON, sigcal.getFirstTrade(), sigcal.getQuotes());
			}

			if( iTargetAlg_ == 0 ) // Use TickProvider.NbboAt()
				sigcal.calculate_targets(sig, tcmc);
			else if( iTargetAlg_ == 1 ) // Use my own sample.
				sigcal.calculate_targets(sig, sigcal.getQuotes());

			get_prediction_at(sig, uid, ticker, idate);
			{
				boost::mutex::scoped_lock lock(sig_mutex_);
				(*pSig_).push_back(std::move(sig));
			}
		}
	}

	// with MT
}

void MMakeSampleBB::endTicker(const string& ticker)
{
}

void MMakeSampleBB::endMarket()
{
	if( dpMilli_ != nullptr )
	{
		delete dpMilli_;
		dpMilli_ = nullptr;
	}
}

void MMakeSampleBB::endDay()
{
	if( cntDay_ == 1 && pEstTime_ != nullptr )
		pEstTime_->beginEndDay();

	if( nHedgeErrDay_ > 0 )
		cout << "WARNING MMakeSampleBB::endDay(): " << nHedgeErrDay_ << " tickers could not be hedged.\n";

	if( threadWrite_ )
	{
		for( auto& t : vWriteThread_ )
			t.join();
		vWriteThread_.push_back(thread(&MMakeSampleBB::write_signal, this, pHedge_, pSig_, enough_day_ombin()));
	}
	else
		write_signal(pHedge_, pSig_, enough_day_ombin());

	if( write_om_ )
		linearModelEndDay();

	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		pEstTime_->endDay();
		delete pEstTime_;
	}
}

vector<int> MMakeSampleBB::getSampleMsecs(int idate, const string& ticker)
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	int stepSec = linMod.om_bin_sample_freq;
	int step = 0;
	if( write_ombin_reg_ || write_tmbin_reg_ )
		step = stepSec * 1000;

	int seed = 0;
	if( irregularSampling_ )
	{
		string baseTicker = base_name(ticker);
		int N = baseTicker.size();
		double prod = idate / 10000.;
		for( int i = 0; i < N; ++i )
			prod = fmod(prod * baseTicker[i], stepSec);
		seed = 1000 * (static_cast<int>(ceil(prod)) % (stepSec * 2 / 3) - stepSec / 3);
	}

	vector<int> sampleMsecs;
	if( step > 0 && std::abs(seed) < step / 2 )
	{
		int noise = seed;
		int tMin = linMod.openMsecs + step;
		int tMax = linMod.closeMsecs;
		for( int msecs = tMin; msecs < tMax; msecs += step, noise *= -1 )
			sampleMsecs.push_back(msecs + noise);
	}
	return sampleMsecs;
}

void MMakeSampleBB::write_signal(gtlib::Hedge* pHedge, std::vector<hff::SigC>* pSig, bool enough_day_ombin)
{
	int idate = MEnv::Instance()->idate;
	const string& baseDir = MEnv::Instance()->baseDir;
	const string& model = MEnv::Instance()->model;
	const LinearModel& linMod = MEnv::Instance()->linearModel;

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
	if( write_ombin_eevt_ || write_tmbin_eevt_ )
		write_sample_type(write_ombin_eevt_, write_tmbin_eevt_, idate, "eevt", exploratory_event_, pSig);
	if( write_ombin_cevt_ || write_tmbin_cevt_ )
		write_sample_type(write_ombin_cevt_, write_tmbin_cevt_, idate, "cevt", trade_and_exploratory_event_, pSig);
	delete pHedge;
	delete pSig;
}

void MMakeSampleBB::write_sample_type(bool write_om, bool write_tm, int idate,
		const string& desc, int sampleType, vector<SigC>* pSig)
{
	const string& baseDir = MEnv::Instance()->baseDir;
	const string& model = MEnv::Instance()->model;
	for( auto& sig : (*pSig) )
	{
		if( sig.inUnivFit )
			select_events(sig.sI, sampleType);
	}
	if(write_om)
		write_signal_file(baseDir, model, idate, "om", desc, sampleType, pSig);
	if(write_tm)
		write_signal_file(baseDir, model, idate, "tm", desc, sampleType, pSig);
}

void MMakeSampleBB::write_signal_file(const string& baseDir, const string& model, int idate,
		const string& sigType, const string& desc, int sampleType, vector<SigC>* pSig)
{
	ofstream binReg( get_sig_path(baseDir, model, idate, sigType, desc).c_str(), ios::out | ios::binary );
	ofstream binTxtReg( get_sigTxt_path(baseDir, model, idate, sigType, desc).c_str(), ios::out );
	writeSig::write_bin_header(sigType, binReg, binTxtReg);
	int cnt = 0;
	for( auto& sig : (*pSig) )
	{
		if( sig.inUnivFit )
		{
			const string& ticker = sig.ticker;
			const string& uid = mTickerUid_[sig.ticker];
			cnt += writeSig::write_bin_evt(sigType, binReg, binTxtReg, sig, uid, ticker, minMsecs_, maxMsecs_, sampleType);
		}
	}
	finish_file(true, binReg, binTxtReg, cnt);
}

void MMakeSampleBB::linearModelEndDay()
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

void MMakeSampleBB::endJob()
{
	if( threadWrite_ )
	{
		for( auto& t : vWriteThread_ )
			t.join();
	}

	if( write_om_ )
	{
		if( weight_source_ == "calculate" )
			biLinMod_.endJob();
	}
}

void MMakeSampleBB::finish_file(bool do_write, ofstream& ofbin, ofstream& ofbintxt, int& cnt)
{
	if( do_write )
	{
		writeSig::update_ncols(ofbin, cnt);

		ofbin.close();
		ofbin.clear();

		ofbintxt.close();
		ofbintxt.clear();

		cnt = 0;
	}
}

void MMakeSampleBB::initTickProvider()
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
	vector<string> dirs = tSources_.nbbodirectory(market, idate);
	for( auto& dir : dirs )
		dpMilli_->AddTradeRoot(dir);

	// bookdir.
	if( "US" == market )
	{
		auto ecns = linMod.get_ecns();
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
		for( auto& dir : dirs )
			dpMilli_->AddBookRoot(mto::code(market)[0], dir);

		// ecns.
		for( auto& ecn : okECNs_ )
		{
			vector<string> dirs = tSources_.bookdirectory(ecn, idate);
			for( auto& dir : dirs )
				dpMilli_->AddBookRoot(ecn[1], dir);
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

void MMakeSampleBB::get_hedged_targets(SigC& sig, const TargetHedger* pHedger)
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
		sig.northBP = pHedger->getNorthBP();
		sig.northRST = pHedger->getNorthRST();
		sig.northTRD = pHedger->getNorthTRD();
		sig.tarON = 0.;

		for( int k = 0; k < Npts; ++k )
		{
			int sec = sI[k].msso / 1000;
			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			{
				int len = nonLinMod.vHorizonLag[iT].first;
				int lag = nonLinMod.vHorizonLag[iT].second;
				if( multHedgedTarget_ )
					sI[k].target[iT] = pHedger->getMultHedgedTarget(sI[k].target[iT], sec, iT, len, lag);
				else
					sI[k].target[iT] = pHedger->getHedgedTarget(sI[k].target[iT], sec, iT, len, lag);
			}

			{
				int secTo = (linMod.closeMsecs - linMod.openMsecs) / 1000;
				int horizon = secTo - sec;
				//sI[k].target60Intra = 0.;
				sI[k].targetClose = pHedger->getHedgedTargetClose(sI[k].targetClose, sec);

				if( horizon >= 60 && horizon < 600 )
					clip(sI[k].targetClose, hff::om_target_clip_);
				else if( horizon >= 600 && horizon < 3600 )
					clip(sI[k].targetClose, hff::tm_target_clip_);
				else if( horizon >= 3600 )
					clip(sI[k].targetClose, hff::tm_target_60_clip_);
			}

			sI[k].targetNextClose = 0.;
		}
	}
}

void MMakeSampleBB::get_hedged_targets(SigC& sig, const vector<hff::SigH>* pvSigH)
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
		sig.northBP = psigh->northBP;
		sig.northRST = psigh->northRST;
		sig.northTRD = psigh->northTRD;
		sig.tarON = psigh->tarON;
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

void MMakeSampleBB::get_prediction_at(SigC& sig, const string& uid, const string& ticker, int idate)
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

void MMakeSampleBB::batch_update_biLin(int k, BiLinearModel::LinRegStatTicker* linRegStat, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI)
{
	int iT = 0;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	if( k >= skip_first_secs_ / linMod.gridInterval /*&& sI.ask >= sI.bid*/ )
	{
		// Update univ model.
		if( sig.inUnivFit == 1 )
			linRegStat->olsmov[timeIdx].add(sI.om, sI.target[iT]);

		// Update err corr model.
		if( biLinMod_.goodUnivModel() )
			linRegStat->olsmovErr.add(sI.omErr, sI.targetErr[iT]);
	}
}

void MMakeSampleBB::select_events_reg(vector<StateInfo>& sI)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	int tmInterval = linMod.tm_bin_sample_freq * 1000;
	for( auto it = sI.begin(); it != sI.end(); ++it )
	{
		if( regular_sample_ & it->sampleType )
		{
			bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
			if( it->gptOK && sprdOK )
			{
				it->valid = 1;
				if( it->msso % tmInterval == 0 )
					it->validTm = 1;
			}
		}
	}
}

void MMakeSampleBB::select_events(vector<StateInfo>& sI, int sampleType)
{
	if( regular_sample_ & sampleType )
		select_events_reg(sI);
	else
	{
		vector<int> vIndexTot = getGoodDataIndex(sI, sampleType);
		vector<int> vIndexOmWrite = getSampledIndex(vIndexTot, desiredSamplesOm_);
		vector<int> vIndexTmWrite = getSampledIndex(vIndexOmWrite, desiredSamplesTm_);

		for( int i : vIndexOmWrite )
			sI[i].valid = 1;
		for( int i : vIndexTmWrite )
			sI[i].validTm = 1;
	}
}

vector<int> MMakeSampleBB::getGoodDataIndex(vector<StateInfo>& sI, int sampleType)
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
			if( it->gptOK && sprdOK && it->bidPersists && it->askPersists && it->msso > last_msso )
				vIndexTot.push_back(distance(sIBeg, it));
			last_msso = it->msso;
		}
	}
	return vIndexTot;
}

vector<int> MMakeSampleBB::getSampledIndex(const vector<int>& vIndexTot, int desiredSamples)
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

