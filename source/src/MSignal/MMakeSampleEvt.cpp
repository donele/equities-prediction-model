#include <MSignal/MMakeSampleEvt.h>
#include <MSignal/SigSet.h>
#include <MSignal.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
using namespace std;
using namespace hff;
using namespace sig;
using namespace gtlib;

MMakeSampleEvt::MMakeSampleEvt(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	debugFlashSell_(false),
	sample_all_(false),
	dayOK_(false),
	verbose_(0),
	nErrDay_(0),
	sample_max_(0),
	calculate_booksig_(true),
	weight_source_("db"), // db, debugdb, disk, calculate
	write_ombin_reg_(false),
	write_ombin_tevt_(false),
	write_ombin_nevt_(false),
	write_ombin_eevt_(false),
	write_ombin_cevt_(false),
	write_tmbin_reg_(false),
	write_tmbin_tevt_(false),
	write_tmbin_eevt_(false),
	write_tmbin_cevt_(false),
	txt_corr_(true),
	cntDay_(0),
	nThreads_(1),
	minMsecs_(0),
	maxMsecs_(86400000),
	filterHorizon_(0),
	filterLag_(0),
	cntOmBin_reg_(0),
	cntOmBin_tevt_(0),
	cntOmBin_nevt_(0),
	cntOmBin_eevt_(0),
	cntOmBin_cevt_(0),
	cntTmBin_reg_(0),
	cntTmBin_tevt_(0),
	cntTmBin_eevt_(0),
	cntTmBin_cevt_(0),
	tp_(0),
	pEstTime_(nullptr)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugFlashSell") )
		debugFlashSell_ = conf.find("debugFlashSell")->second == "true";
	if( conf.count("sampleAll") )
		sample_all_ = conf.find("sampleAll")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("wmodel") )
		wmodel_ = conf.find("wmodel")->second;
	if( conf.count("calculateBooksig") )
		calculate_booksig_ = conf.find("calculateBooksig")->second == "true";
	if( conf.count("weightSource") )
		weight_source_ = conf.find("weightSource")->second;
	if( conf.count("writeOmBinReg") )
		write_ombin_reg_ = conf.find("writeOmBinReg")->second == "true";
	if( conf.count("writeOmBinTevt") )
		write_ombin_tevt_ = conf.find("writeOmBinTevt")->second == "true";
	if( conf.count("writeOmBinNevt") )
		write_ombin_nevt_ = conf.find("writeOmBinNevt")->second == "true";
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
	if( conf.count("txtCorr") )
		txt_corr_ = conf.find("txtCorr")->second == "true";
	if( conf.count("nThreads") )
		nThreads_ = atoi( conf.find("nThreads")->second.c_str() );
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	write_om_ = write_ombin_reg_ || write_ombin_tevt_ || write_ombin_nevt_ || write_ombin_eevt_ || write_ombin_cevt_;
}

MMakeSampleEvt::~MMakeSampleEvt()
{}

void MMakeSampleEvt::beginJob()
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

	if( MEnv::Instance()->multiThreadTicker )
		nThreads_ = MEnv::Instance()->nMaxThreadTicker;
	SigSet::Instance()->resize(nThreads_);

	if( write_ombin_reg_ )
	{
		int stepMsecsSigs = linMod.gridInterval * 1000;
		for ( int msecs = linMod.openMsecs + stepMsecsSigs; msecs < linMod.closeMsecs; msecs += stepMsecsSigs )
			sampleMsecsVect_.push_back(msecs);
	}

	if( debugFlashSell_ )
	{
		sampleMsecsVect_.clear();
		for( int msecs = minMsecs_; msecs < maxMsecs_; msecs += 100 )
			sampleMsecsVect_.push_back(msecs);
	}
}

void MMakeSampleEvt::beginDay()
{
	++cntDay_;
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel nonLinMod = MEnv::Instance()->nonLinearModel;
	mTickerUid_ = get_uid_map(markets, idate, uids_);
	sample_max_ = 2 * (linMod.closeMsecs - linMod.openMsecs) / 30000 + 1;

	// okECNs.
	okECNs_.clear();
	auto ecns = linMod.get_ecns();
	for( auto it = ecns.begin(); it != ecns.end(); ++it )
	{
		string market = *it;
		if( "US" == market || mto::dataOK(market, idate) )
			okECNs_.push_back(market);
	}

	string market = markets[0];
	longTicker_ = mto::longTicker(market);

	read_linear_model();
	open_sig_files();


	pvIndexBeta_ = static_cast<const vector<IndexBetaInfo>*>(MEvent::Instance()->get("", "beta"));
}

void MMakeSampleEvt::read_linear_model()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	string market = MEnv::Instance()->markets[0];
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;

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

void MMakeSampleEvt::open_sig_files()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	string market = MEnv::Instance()->markets[0];
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const string& baseDir = MEnv::Instance()->baseDir;

	// bin.
	if( write_ombin_reg_ && enough_day_ombin() )
	{
		omBin_reg_.open( get_sig_path(baseDir, model, idate, "om", "reg").c_str(), ios::out | ios::binary );
		omBinTxt_reg_.open( get_sigTxt_path(baseDir, model, idate, "om", "reg").c_str(), ios::out );
		writeSig::write_omBin_header(omBin_reg_, omBinTxt_reg_);
	}
	if( write_ombin_tevt_ )
	{
		omBin_tevt_.open( get_sig_path(baseDir, model, idate, "om", "tevt").c_str(), ios::out | ios::binary );
		omBinTxt_tevt_.open( get_sigTxt_path(baseDir, model, idate, "om", "tevt").c_str(), ios::out );
		writeSig::write_omBin_header(omBin_tevt_, omBinTxt_tevt_);
	}
	if( write_ombin_nevt_ )
	{
		omBin_nevt_.open( get_sig_path(baseDir, model, idate, "om", "nevt").c_str(), ios::out | ios::binary );
		omBinTxt_nevt_.open( get_sigTxt_path(baseDir, model, idate, "om", "nevt").c_str(), ios::out );
		writeSig::write_omBin_header(omBin_nevt_, omBinTxt_nevt_);
	}
	if( write_ombin_eevt_ )
	{
		omBin_eevt_.open( get_sig_path(baseDir, model, idate, "om", "eevt").c_str(), ios::out | ios::binary );
		omBinTxt_eevt_.open( get_sigTxt_path(baseDir, model, idate, "om", "eevt").c_str(), ios::out );
		writeSig::write_omBin_header(omBin_eevt_, omBinTxt_eevt_);
	}
	if( write_ombin_cevt_ )
	{
		omBin_cevt_.open( get_sig_path(baseDir, model, idate, "om", "cevt").c_str(), ios::out | ios::binary );
		omBinTxt_cevt_.open( get_sigTxt_path(baseDir, model, idate, "om", "cevt").c_str(), ios::out );
		writeSig::write_omBin_header(omBin_cevt_, omBinTxt_cevt_);
	}
	if( write_tmbin_reg_ )
	{
		tmBin_reg_.open( get_sig_path(baseDir, model, idate, "tm", "reg").c_str(), ios::out | ios::binary );
		tmBinTxt_reg_.open( get_sigTxt_path(baseDir, model, idate, "tm", "reg").c_str(), ios::out );
		writeSig::write_tmBin_header(tmBin_reg_, tmBinTxt_reg_);
	}
	if( write_tmbin_tevt_ )
	{
		tmBin_tevt_.open( get_sig_path(baseDir, model, idate, "tm", "tevt").c_str(), ios::out | ios::binary );
		tmBinTxt_tevt_.open( get_sigTxt_path(baseDir, model, idate, "tm", "tevt").c_str(), ios::out );
		writeSig::write_tmBin_header(tmBin_tevt_, tmBinTxt_tevt_);
	}
	if( write_tmbin_eevt_ )
	{
		tmBin_eevt_.open( get_sig_path(baseDir, model, idate, "tm", "eevt").c_str(), ios::out | ios::binary );
		tmBinTxt_eevt_.open( get_sigTxt_path(baseDir, model, idate, "tm", "eevt").c_str(), ios::out );
		writeSig::write_tmBin_header(tmBin_eevt_, tmBinTxt_eevt_);
	}
	if( write_tmbin_cevt_ )
	{
		tmBin_cevt_.open( get_sig_path(baseDir, model, idate, "tm", "cevt").c_str(), ios::out | ios::binary );
		tmBinTxt_cevt_.open( get_sigTxt_path(baseDir, model, idate, "tm", "cevt").c_str(), ios::out );
		writeSig::write_tmBin_header(tmBin_cevt_, tmBinTxt_cevt_);
	}
}

bool MMakeSampleEvt::enough_day_ombin()
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	bool enough_day_ombin = weight_source_ != "calculate" ||
		//cntDay_ >= linMod.om_univ_fit_days + linMod.om_err_fit_days;
		biLinMod_.goodModel();
	return enough_day_ombin || debug_;
}

void MMakeSampleEvt::beginMarket()
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
	if( cntDay_ == 1 )
		pEstTime_ = new gtlib::EstTime(MEnv::Instance()->tickers.size());

	//
	// Event.
	//
	// Idx.
	idxfp_.LoadData(*GEX::Instance()->get(market), idate, mto::bindirReturn(market, idate), mto::longTicker(market));
	idxfp_.LoadFilters(idate, mto::hfpar(market, idate), *GEX::Instance()->get(market));

	// TickProviderClassic.
	init_tp(tp_);
}

void MMakeSampleEvt::beginMarketDay()
{
	string market = MEnv::Instance()->market;
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

void MMakeSampleEvt::beginTicker(const string& ticker)
{
	if( !dayOK_ )
		return;
	if( cntDay_ == 1 && pEstTime_ != nullptr )
		pEstTime_->beginTicker();

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

		int iSig = -1;
		int desiredSamplesOm = (linMod.closeMsecs - linMod.openMsecs) / (linMod.om_bin_sample_freq * 1000);
		int desiredSamplesTm = (linMod.closeMsecs - linMod.openMsecs) / (linMod.tm_bin_sample_freq * 1000);
		if( sample_all_ )
		{
			desiredSamplesOm = max_int_;
			desiredSamplesTm = max_int_;
		}

		SigC& sig = SigSet::Instance()->get_sig_object(iSig, 0/*sampleMsecsVect_.size() + desiredSamples * 6*/);

		// Read from stockcharacteristics table.
		bool charaok = read_chara_data(sig, model, market, uid, idate_, idate_p_, idate_pp_, idate_n_);

		// Read from the tick data.
		const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
		const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));
		const vector<QuoteInfo>* news = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "news"));
		const vector<QuoteInfo>* sip = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "sip"));

		if( debug_ || charaok && sig.avgDlyVolume > 0. && sig.avgDlyVolat > 0.
				&& quotes != 0 && trades != 0 && quotes->size() >= min_quotes_ && trades->size() >= min_trades_ )
		{
			// SignalCalculator.
			SignalCalculator sigcal(ticker, sig, quotes, trades, &sessions_, linMod.openMsecs, linMod.closeMsecs, linMod.exploratoryDelay, sip);

			double desiredQuoteProb = (sig.medNquotes > 0) ? (desiredSamplesOm / sig.medNquotes) : -1.;
			double desiredTradeProb = (sig.medNtrades > 0) ? (desiredSamplesOm / sig.medNtrades) : -1.;
			double desiredBooktradeProb = (sig.medNbooktrades > 0) ? (desiredSamplesOm / sig.medNbooktrades) : -1.;

			sigcal.regularSampleProbability = 1.;
			if( !write_ombin_reg_ && !write_tmbin_reg_ )
				sigcal.regularSampleProbability = -1.;
			sigcal.quoteSampleProbability = -1.;
			sigcal.tradeSampleProbability = -1.;

			sigcal.booktradeSampleProbability = -1.;
			if( write_ombin_tevt_ || write_ombin_nevt_ || write_ombin_eevt_ || write_ombin_cevt_ )
				sigcal.booktradeSampleProbability = 1.;

			TickProviderClassic<TradeInfo, QuoteInfo, OrderData>* tp = 0;
			if( MEnv::Instance()->multiThreadTicker )
				init_tp(tp);
			else
				tp = tp_;

			// TickProviderClassic.
			tp->SetConsumer(&sigcal);
			tp->StartSymbol(ticker, max_double_, sampleMsecsVect_);

			// end of NYSE auction.
			if( "US" == market )
				tp->SetAutoEndOfAuction('N', 1);

			// Run.
			tp->Run();
			if( MEnv::Instance()->multiThreadTicker )
				delete tp;

			// Calculate predictions and update the linear model.
			get_prediction_at(sig, uid, ticker, idate);

			write_signal(sig, uid, ticker, desiredSamplesOm, desiredSamplesTm);
		}

		SigSet::Instance()->free_sig_object(iSig);
	}
}

void MMakeSampleEvt::endTicker(const string& ticker)
{
}

void MMakeSampleEvt::endMarket()
{
	if( cntDay_ == 1 && pEstTime_ != nullptr )
	{
		pEstTime_->endDay();
		delete pEstTime_;
	}
}

void MMakeSampleEvt::endDay()
{
	finish_file(write_ombin_reg_ && enough_day_ombin(), omBin_reg_, omBinTxt_reg_, cntOmBin_reg_);
	finish_file(write_ombin_tevt_, omBin_tevt_, omBinTxt_tevt_, cntOmBin_tevt_);
	finish_file(write_ombin_nevt_, omBin_nevt_, omBinTxt_nevt_, cntOmBin_nevt_);
	finish_file(write_ombin_eevt_, omBin_eevt_, omBinTxt_eevt_, cntOmBin_eevt_);
	finish_file(write_ombin_cevt_, omBin_cevt_, omBinTxt_cevt_, cntOmBin_cevt_);

	finish_file(write_tmbin_reg_, tmBin_reg_, tmBinTxt_reg_, cntTmBin_reg_);
	finish_file(write_tmbin_tevt_, tmBin_tevt_, tmBinTxt_tevt_, cntTmBin_tevt_);
	finish_file(write_tmbin_eevt_, tmBin_eevt_, tmBinTxt_eevt_, cntTmBin_eevt_);
	finish_file(write_tmbin_cevt_, tmBin_cevt_, tmBinTxt_cevt_, cntTmBin_cevt_);

	finish_linear_model();

	nErrDay_ = 0;
}

void MMakeSampleEvt::finish_linear_model()
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	int idate = MEnv::Instance()->idate;

	if( write_om_ )
	{
		if( weight_source_ == "calculate" )
			biLinMod_.endDay(idate);

		for( int iT = 0; iT < linMod.nHorizon; ++iT )
		{
			cout << itos(linMod.vHorizonLag[iT].first) + ";" + itos(linMod.vHorizonLag[iT].second) << "\t";
			cout << "corr " << vCorr_[iT].corr() << " corrTot " << vCorrTot_[iT].corr() << endl;
		}
	}
}

void MMakeSampleEvt::endJob()
{
	if( write_om_ )
		if( weight_source_ == "calculate" )
			biLinMod_.endJob();
}

void MMakeSampleEvt::finish_file(bool do_write, ofstream& ofbin, ofstream& ofbintxt, int& cnt)
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

void MMakeSampleEvt::init_tp(TickProviderClassic<TradeInfo,QuoteInfo,OrderData>*& tp)
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	if( tp != 0 )
		delete tp;
	tp = new TickProviderClassic<TradeInfo, QuoteInfo, OrderData>(idate_, linMod.openMsecs, linMod.closeMsecs, &idxfp_);
	tp->SkipTransient(-1);

	// nbbodir.
	vector<string> dirs = tSources_.nbbodirectory(market, idate);
	for( auto& dir : dirs )
	{
		tp->AddNbboRoot( dir, longTicker_ );
		tp->AddTradeRoot( dir, longTicker_ );
	}

	// bookdir.
	if( "US" == market )
	{
		auto ecns = linMod.get_ecns();
		for( auto& ecn : ecns )
		{
			vector<string> dirs = tSources_.bookdirectory(ecn, idate);
			for( auto& dir : dirs )
			{
				tp->AddBookRoot(ecn[1], dir);
				tp->AddBookTradeRoot(ecn[1], dir);
			}
		}
	}
	else
	{
		// main market.
		vector<string> dirs = tSources_.bookdirectory(market, idate);
		for( auto& dir : dirs )
		{
			tp->AddBookRoot( mto::code(market)[0], dir, longTicker_ );
			tp->AddBookTradeRoot( mto::code(market)[0], dir, longTicker_ );
		}

		// ecns.
		for( auto& ecn : okECNs_ )
		{
			vector<string> dirs = tSources_.bookdirectory(ecn, idate);
			for( auto& dir : dirs )
			{
				tp->AddBookRoot( ecn[1], dir, longTicker_ );
				tp->AddBookTradeRoot( ecn[1], dir, longTicker_ );
			}
		}
	}
}

map<string, double> MMakeSampleEvt::GetMedianBooktrades(int idate)
{
	map<string, double> m;

	ostringstream ss;
	ss << "L:\\booktradestats\\bts" << idate << ".txt";

	ifstream f(ss.str().c_str());
	int ln = 0;
	string line, symbol;
	double ntrades, medNtrades, avgNtrades, nDays;

	while(f.good())
	{
		if(ln)
		{
			f >> symbol >> ntrades >> medNtrades >> avgNtrades >> nDays;
			m[symbol] = medNtrades;
		}
		else
		{
			std::getline(f, line);
		}
		++ln;
	}

	f.close();

	return m;
}

void MMakeSampleEvt::get_prediction_at(SigC& sig, const string& uid, const string& ticker, int idate)
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

	// Hedge object.
	const hff::SigH* psigh = 0;
	const vector<hff::SigH>* pvSigH = static_cast<const vector<hff::SigH>*>(MEvent::Instance()->get("", "vSigH"));
	if( pvSigH == 0 )
	{
		if( nErrDay_++ < 1 )
			cout << "WARNING MMakeSampleEvt::get_prediction_at() Hedge info not available.\n";
	}
	else
	{
		for( auto it = pvSigH->begin(); it != pvSigH->end(); ++it )
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

	BiLinearModel::LinRegStatTicker* linRegStat = nullptr;
	if( weight_source_ == "calculate" && write_ombin_reg_ )
		linRegStat = biLinMod_.getNewLinRegStatTicker();

	int n1secSlice = ceil( float(linMod.n1sec - 1) / linMod.num_time_slices );
	for( int k = 0; k < Npts; ++k )
	{
		int sec = sI[k].msso / 1000;
		int timeIdx = sec / n1secSlice;

		if( psigh != 0 )
		{
			for( int iT = linMod.nHorizon; iT < nonLinMod.nHorizon; ++iT )
			{
				int len = nonLinMod.vHorizonLag[iT].first;
				int lag = nonLinMod.vHorizonLag[iT].second;
				sI[k].target[iT] = psigh->get_hedged_target(sec, len, lag);
			}
			//sI[k].target60Intra = 0.;
			sI[k].targetClose = 0.;
			sI[k].targetNextClose = 0.;
		}

		if( sI[k].gptOK == 1 && (write_om_) )
		{
			for( int iT = 0; iT < linMod.nHorizon; ++iT )
			{
				// Set gSigs.
				for( auto it = vIndx.begin(); it != vIndx.end(); ++it )
					sI[k].sig1[*it] = 0.;

				// Contruct predName from the filter with the matching horizon.
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
			float beta1 = (pvIndexBeta_ != nullptr) ? (*pvIndexBeta_)[0].getBeta(uid) : 1.;
			float beta2 = (pvIndexBeta_ != nullptr && pvIndexBeta_->size() > 1) ? (*pvIndexBeta_)[1].getBeta(uid) : 1.;
			sI[k].indxPred1Adj = indxPred1 * beta1;
			sI[k].indxPred2Adj = indxPred2 * beta2;
			sI[k].indxPred1Sprd = indxPred1 * sI[k].sprd;
			sI[k].indxPred2Sprd = indxPred2 * sI[k].sprd;
		}
	}
	if( weight_source_ == "calculate" && write_ombin_reg_ )
	{
		boost::mutex::scoped_lock lock(biLinMod_mutex_);
		biLinMod_.update(linRegStat, uid);
	}
	if( linRegStat != nullptr )
		delete linRegStat;
}

void MMakeSampleEvt::batch_update_biLin(int k, BiLinearModel::LinRegStatTicker* linRegStat, int timeIdx, const std::string& uid, const hff::SigC& sig, const hff::StateInfo& sI)
{
	int iT = 0;
	const LinearModel& linMod = MEnv::Instance()->linearModel;
	if( k >= skip_first_secs_ / linMod.gridInterval /*&& sI.ask >= sI.bid*/ )
	{
		// Update univ model.
		if( sig.inUnivFit == 1 )
			linRegStat->olsmov[timeIdx].add(sI.om, sI.target[iT]);

		// Update err corr model.
		//if( cntDay_ > linMod.om_univ_fit_days )
		if( biLinMod_.goodUnivModel() )
			linRegStat->olsmovErr.add(sI.omErr, sI.targetErr[iT]);
	}
}

void MMakeSampleEvt::write_signal(SigC& sig, const string& uid, const string& ticker, int desiredSamplesOm, int desiredSamplesTm)
{
	// Write signals.
	if( sig.inUnivFit )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		boost::mutex::scoped_lock lock(om_mutex_);

		if( write_ombin_reg_ || write_tmbin_reg_ )
		{
			int cnt = 0;
			int tmSample = desiredSamplesOm / desiredSamplesTm;
			for( auto it = sig.sI.begin(); it != sig.sI.end(); ++it )
			{
				if( regular_sample_ & it->sampleType )
				{
					bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
					if( it->gptOK && sprdOK )
					{
						it->valid = 1;
						if( ++cnt % tmSample == 1 )
							it->validTm = 1;
					}
				}
			}
		}
		if( write_ombin_reg_ && enough_day_ombin() )
			cntOmBin_reg_ += writeSig::write_omBin_evt(omBin_reg_, omBinTxt_reg_, sig, uid, ticker, minMsecs_, maxMsecs_, regular_sample_);
		if( write_tmbin_reg_ )
			cntTmBin_reg_ += writeSig::write_tmBin_evt(tmBin_reg_, tmBinTxt_reg_, sig, uid, ticker, minMsecs_, maxMsecs_, regular_sample_);

		if( write_ombin_tevt_ || write_tmbin_tevt_ )
			select_events(sig.sI, book_trade_event_, desiredSamplesOm, desiredSamplesTm);
		if( write_ombin_tevt_ )
			cntOmBin_tevt_ += writeSig::write_omBin_evt(omBin_tevt_, omBinTxt_tevt_, sig, uid, ticker, minMsecs_, maxMsecs_, book_trade_event_);
		if( write_tmbin_tevt_ )
			cntTmBin_tevt_ += writeSig::write_tmBin_evt(tmBin_tevt_, tmBinTxt_tevt_, sig, uid, ticker, minMsecs_, maxMsecs_, book_trade_event_);

		if( write_ombin_eevt_ || write_tmbin_eevt_ )
		{
			select_events(sig.sI, exploratory_event_, desiredSamplesOm, desiredSamplesTm);
		}
		if( write_ombin_eevt_ )
			cntOmBin_eevt_ += writeSig::write_omBin_evt(omBin_eevt_, omBinTxt_eevt_, sig, uid, ticker, minMsecs_, maxMsecs_, exploratory_event_);
		if( write_tmbin_eevt_ )
			cntTmBin_eevt_ += writeSig::write_tmBin_evt(tmBin_eevt_, tmBinTxt_eevt_, sig, uid, ticker, minMsecs_, maxMsecs_, exploratory_event_);

		if( write_ombin_cevt_ || write_tmbin_cevt_ )
		{
			select_events(sig.sI, trade_and_exploratory_event_, desiredSamplesOm, desiredSamplesTm);
		}
		if( write_ombin_cevt_ )
			cntOmBin_cevt_ += writeSig::write_omBin_evt(omBin_cevt_, omBinTxt_cevt_, sig, uid, ticker, minMsecs_, maxMsecs_, trade_and_exploratory_event_);
		if( write_tmbin_cevt_ )
			cntTmBin_cevt_ += writeSig::write_tmBin_evt(tmBin_cevt_, tmBinTxt_cevt_, sig, uid, ticker, minMsecs_, maxMsecs_, trade_and_exploratory_event_);
	}
}

void MMakeSampleEvt::select_events(vector<StateInfo>& sI, int sampleType, int desiredSamplesOm, int desiredSamplesTm)
{
	if( 0 )
		select_events_random(sI, sampleType, desiredSamplesOm, desiredSamplesTm);
	else
		select_events_deterministic(sI, sampleType, desiredSamplesOm, desiredSamplesTm);
}

void MMakeSampleEvt::select_events_random(vector<StateInfo>& sI, int sampleType, int desiredSamplesOm, int desiredSamplesTm)
{
	// Calculate total number.
	int last_msso = 0;
	double typetot = 0.;
	double typeok = 0.;
	double tot = 0.;
	for( auto it = sI.begin(); it != sI.end(); ++it )
	{
		if( sampleType & it->sampleType )
		{
			++typetot;
			it->valid = 0;
			it->validTm = 0;
			if( it->gptOK )
			{
				++typeok;
				if( it->bidPersists && it->askPersists )
				{
					if( it->msso > last_msso )
						++tot;
					last_msso = it->msso;
				}
			}
		}
	}

	// Calculate the probabilities.
	double omProb = 0.;
	double tmProb = 0.;
	if( tot > 0. )
	{
		if( tot > desiredSamplesOm )
			omProb = desiredSamplesOm / tot;
		else
			omProb = 1.;

		if( tot > desiredSamplesOm )
			tmProb = (double)desiredSamplesTm / desiredSamplesOm;
		else if( tot > desiredSamplesTm )
			tmProb = (double)desiredSamplesTm / tot;
		else
			tmProb = 1.;
	}

	// Select the data to write.
	last_msso = 0;
	int lastTimeWritten = 0;
	int totWritten = 0;
	for( auto it = sI.begin(); it != sI.end(); ++it )
	{
		bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
		if( sampleType & it->sampleType && it->gptOK && sprdOK && it->bidPersists && it->askPersists )
		{
			if( totWritten < sample_max_ && Random::Instance()->select(omProb) )
			{
				if( it->msso > last_msso )
				{
					it->valid = 1;
					lastTimeWritten = it->msso;
					++totWritten;

					if( Random::Instance()->select(tmProb) )
						it->validTm = 1;
				}
			}
			last_msso = it->msso;
		}
	}
}

void MMakeSampleEvt::select_events_deterministic(vector<StateInfo>& sI, int sampleType, int desiredSamplesOm, int desiredSamplesTm)
{
	vector<int> vIndexTot = getGoodDataIndex(sI, sampleType);
	vector<int> vIndexOmWrite = getSampledIndex(vIndexTot, desiredSamplesOm);
	vector<int> vIndexTmWrite = getSampledIndex(vIndexOmWrite, desiredSamplesTm);

	for( int i : vIndexOmWrite )
		sI[i].valid = 1;
	for( int i : vIndexTmWrite )
		sI[i].validTm = 1;
}

vector<int> MMakeSampleEvt::getGoodDataIndex(vector<StateInfo>& sI, int sampleType)
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

vector<int> MMakeSampleEvt::getSampledIndex(const vector<int>& vIndexTot, int desiredSamples)
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
