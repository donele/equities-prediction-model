#include <MSignal/MMakeSampleEvt.h>
#include <MSignal/SigSet.h>
#include <MSignal.h>
#include <MFitObj.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
using namespace std;
using namespace hff;
using namespace sig;

MMakeSampleEvt::MMakeSampleEvt(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
debug_booktrade_signal_(false),
debug_booktrade_prob_(false),
debug_rz_signal_(false),
sampleAtWrite_(true),
sample_all_(false),
dayOK_(false),
debug_fillImbBook_(false),
verbose_(0),
nErrDay_(0),
sample_max_(0),
calculate_booksig_(true),
use_db_weights_(true),
write_omtxt_reg_(false),
write_ombin_reg_(false),
write_omtxt_tevt_(false),
write_ombin_tevt_(false),
write_ombin_nevt_(false),
write_ombin_eevt_(false),
write_tmtxt_reg_(false),
write_tmbin_reg_(false),
write_tmtxt_tevt_(false),
write_tmbin_tevt_(false),
write_tmbin_eevt_(false),
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
cntTmBin_reg_(0),
cntTmBin_tevt_(0),
cntTmBin_eevt_(0),
tp_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debugBooktradeSignal") )
		debug_booktrade_signal_ = conf.find("debugBooktradeSignal")->second == "true";
	if( conf.count("debugBooktradeProb") )
		debug_booktrade_prob_ = conf.find("debugBooktradeProb")->second == "true";
	if( conf.count("debugRZsignal") )
		debug_rz_signal_ = conf.find("debugRZsignal")->second == "true";
	if( conf.count("debugFillImbBook") )
		debug_fillImbBook_ = conf.find("debugFillImbBook")->second == "true";
	if( conf.count("sampleAtWrite") )
		sampleAtWrite_ = conf.find("sampleAtWrite")->second == "true";
	if( conf.count("sampleAll") )
		sample_all_ = conf.find("sampleAll")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("wmodel") )
		wmodel_ = conf.find("wmodel")->second;
	if( conf.count("calculateBooksig") )
		calculate_booksig_ = conf.find("calculateBooksig")->second == "true";
	if( conf.count("useDBWeights") )
		use_db_weights_ = conf.find("useDBWeights")->second == "true";
	if( conf.count("writeOmTxtReg") )
		write_omtxt_reg_ = conf.find("writeOmTxtReg")->second == "true";
	if( conf.count("writeOmBinReg") )
		write_ombin_reg_ = conf.find("writeOmBinReg")->second == "true";
	if( conf.count("writeOmTxtTevt") )
		write_omtxt_tevt_ = conf.find("writeOmTxtTevt")->second == "true";
	if( conf.count("writeOmBinTevt") )
		write_ombin_tevt_ = conf.find("writeOmBinTevt")->second == "true";
	if( conf.count("writeOmBinNevt") )
		write_ombin_nevt_ = conf.find("writeOmBinNevt")->second == "true";
	if( conf.count("writeOmBinEevt") )
		write_ombin_eevt_ = conf.find("writeOmBinEevt")->second == "true";
	if( conf.count("writeTmTxtReg") )
		write_tmtxt_reg_ = conf.find("writeTmTxtReg")->second == "true";
	if( conf.count("writeTmBinReg") )
		write_tmbin_reg_ = conf.find("writeTmBinReg")->second == "true";
	if( conf.count("writeTmTxtTevt") )
		write_tmtxt_tevt_ = conf.find("writeTmTxtTevt")->second == "true";
	if( conf.count("writeTmBinTevt") )
		write_tmbin_tevt_ = conf.find("writeTmBinTevt")->second == "true";
	if( conf.count("writeTmBinEevt") )
		write_tmbin_eevt_ = conf.find("writeTmBinEevt")->second == "true";
	if( conf.count("txtCorr") )
		txt_corr_ = conf.find("txtCorr")->second == "true";
	if( conf.count("nThreads") )
		nThreads_ = atoi( conf.find("nThreads")->second.c_str() );
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	write_om_ = write_omtxt_reg_ || write_ombin_reg_ || write_omtxt_tevt_ || write_ombin_tevt_ || write_ombin_nevt_ || write_ombin_eevt_;
}

MMakeSampleEvt::~MMakeSampleEvt()
{}

void MMakeSampleEvt::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	pid_ = get_pid();

	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	uids_ = MEvent::Instance()->get<set<string> >("", "allUids");
	cout << uids_.size() << " instruments." << endl;
	tSources_.read(mto::region(MEnv::Instance()->markets[0]), MEnv::Instance()->sourceFlag);

	if( write_om_ )
	{
		biLinModW_ = BiLinearModelWeights(linMod.nHorizon, num_time_slices_, om_num_sig_, om_num_err_sig_);
		vCorr_.resize(linMod.nHorizon);
		vCorrTot_.resize(linMod.nHorizon);
	}

	if( MEnv::Instance()->multiThreadTicker )
		nThreads_ = MEnv::Instance()->nMaxThreadTicker;
	SigSet::Instance()->resize(nThreads_);

	int stepMsecsSigs = 30000;
	for ( int msecs = linMod.openMsecs + stepMsecsSigs; msecs < linMod.closeMsecs; msecs += stepMsecsSigs )
		sampleMsecsVect_.push_back(msecs);
}

void MMakeSampleEvt::beginDay()
{
	int idate = MEnv::Instance()->idate;
	vector<string> markets = MEnv::Instance()->markets;
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel nonLinMod = MEnv::Instance()->nonLinearModel;
	string model = MEnv::Instance()->model;
	mTickerUid_ = get_uid_map(markets, idate, uids_);
	sample_max_ = 2 * (linMod.closeMsecs - linMod.openMsecs) / 30000 + 1;

	// okECNs.
	okECNs_.clear();
	auto ecns = linMod.get_ecns();
	for( vector<string>::const_iterator it = ecns.begin(); it != ecns.end(); ++it )
	{
		string market = *it;
		if( "US" == market || mto::dataOK(market, idate) )
			okECNs_.push_back(market);
	}

	bool doAfterHours = false;
	string market = markets[0];
	longTicker_ = mto::longTicker(market);

	// txt.
	if( write_omtxt_reg_ )
	{
		omTxt_reg_.open( sigPath::get_omTxt_path(model, idate).c_str(), ios::out );
		writeSig::write_omTxt_header(omTxt_reg_);
	}
	if( write_omtxt_tevt_ )
	{
		omTxt_tevt_.open( sigPath::get_omTxt_tevt_path(model, idate).c_str(), ios::out );
		writeSig::write_omTxt_header(omTxt_tevt_);
	}
	//if( write_tmtxt_ )
	//{
	//	tmTxt_.open( sigPath::get_tmTxt_path(model, idate).c_str(), ios::out );
	//	writeSig::write_tmTxt_header(tmTxt_);
	//}

	// bin.
	if( write_ombin_reg_ )
	{
		omBin_reg_.open( sigPath::get_omBin_path(model, idate).c_str(), ios::out | ios::binary );
		omBinTxt_reg_.open( sigPath::get_omBinTxt_path(model, idate).c_str(), ios::out );
		writeSig::write_omBin_header(omBin_reg_, omBinTxt_reg_);
	}
	if( write_ombin_tevt_ )
	{
		omBin_tevt_.open( sigPath::get_omBin_tevt_path(model, idate).c_str(), ios::out | ios::binary );
		omBinTxt_tevt_.open( sigPath::get_omBinTxt_tevt_path(model, idate).c_str(), ios::out );
		writeSig::write_omBin_header(omBin_tevt_, omBinTxt_tevt_);
	}
	if( write_ombin_nevt_ )
	{
		omBin_nevt_.open( sigPath::get_omBin_nevt_path(model, idate).c_str(), ios::out | ios::binary );
		omBinTxt_nevt_.open( sigPath::get_omBinTxt_nevt_path(model, idate).c_str(), ios::out );
		writeSig::write_omBin_header(omBin_nevt_, omBinTxt_nevt_);
	}
	if( write_ombin_eevt_ )
	{
		omBin_eevt_.open( sigPath::get_omBin_eevt_path(model, idate).c_str(), ios::out | ios::binary );
		omBinTxt_eevt_.open( sigPath::get_omBinTxt_eevt_path(model, idate).c_str(), ios::out );
		writeSig::write_omBin_header(omBin_eevt_, omBinTxt_eevt_);
	}
	//if( write_tmbin_ )
	//{
	//	tmBin_.open( sigPath::get_tmBin_path(model, idate).c_str(), ios::out | ios::binary );
	//	tmBinTxt_.open( sigPath::get_tmBinTxt_path(model, idate).c_str(), ios::out );
	//	writeSig::write_tmBin_header(tmBin_, tmBinTxt_);
	//}
	if( write_tmbin_reg_ )
	{
		tmBin_reg_.open( sigPath::get_tmBin_path(model, idate).c_str(), ios::out | ios::binary );
		tmBinTxt_reg_.open( sigPath::get_tmBinTxt_path(model, idate).c_str(), ios::out );
		writeSig::write_tmBin_header(tmBin_reg_, tmBinTxt_reg_);
	}
	if( write_tmbin_tevt_ )
	{
		tmBin_tevt_.open( sigPath::get_tmBin_tevt_path(model, idate).c_str(), ios::out | ios::binary );
		tmBinTxt_tevt_.open( sigPath::get_tmBinTxt_tevt_path(model, idate).c_str(), ios::out );
		writeSig::write_tmBin_header(tmBin_tevt_, tmBinTxt_tevt_);
	}
	if( write_tmbin_eevt_ )
	{
		tmBin_eevt_.open( sigPath::get_tmBin_eevt_path(model, idate).c_str(), ios::out | ios::binary );
		tmBinTxt_eevt_.open( sigPath::get_tmBinTxt_eevt_path(model, idate).c_str(), ios::out );
		writeSig::write_tmBin_header(tmBin_eevt_, tmBinTxt_eevt_);
	}

	// Read the linear model.
	if( write_om_ )
	{
		if( use_db_weights_ )
		{
			string dbname = mto::hfpar(market, idate);
			biLinModW_.read_db_weights(model, market, idate, linMod.mCode[0], mTickerUid_, linMod.gpts, dbname);
		}
		else
			biLinModW_.read_weights(idate, sigPath::get_weight_dir(wmodel_));
	}

	if( "US" == market )
	{
		// NYSE names.
		nyse_names_.clear();
		char cmd[1000];
		sprintf(cmd, "select ticker from stockcharacteristics where idate = %d and market = 'N'", idate);
		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
		for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			nyse_names_.push_back(trim((*it)[0]));
		sort(nyse_names_.begin(), nyse_names_.end());

		TickAccessMulti<TradeInfo> ta;
		vector<string> dirs = tSources_.nbbodirectory(market, idate);
		ta.AddRoot(dirs[0]);
		GetFirstTradeTime(idate, 1, ta, nyse_names_, &first_trades_);

		// Get medNtrades from book trades.
		medNbooktradesmap_ = GetMedianBooktrades(idate);
	}
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

//
// Event.
//
	// Idx.
	idxfp_.LoadData(*GEX::Instance()->get(market), idate, mto::bindirReturn(market, idate), mto::longTicker(market));
	idxfp_.LoadFilters(idate, mto::hfpar(market, idate), *GEX::Instance()->get(market));

	// TickProviderClassic.
	init_tp(tp_);
}

void MMakeSampleEvt::beginTicker(const string& ticker)
{
	if( !dayOK_ )
		return;

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
		int desiredSamplesOm = (linMod.closeMsecs - linMod.openMsecs) / 30000;
		int desiredSamplesTm = (linMod.closeMsecs - linMod.openMsecs) / 300000;
		if( sample_all_ )
		{
			desiredSamplesOm = max_int_;
			desiredSamplesTm = max_int_;
		}

		SigC& sig = SigSet::Instance()->get_sig_object(iSig/*, 0/*sampleMsecsVect_.size() + desiredSamples * 6*/);

		// Read from stockcharacteristics table.
		bool charaok = read_chara_data(sig, model, market, uid, idate_, idate_p_, idate_pp_, idate_n_);

		// Read from the tick data.
		const vector<QuoteInfo>* quotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
		const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));
		const vector<QuoteInfo>* news = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "news"));
		const vector<QuoteInfo>* sip = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "sip"));

		if( (charaok || debug_rz_signal_) && sig.avgDlyVolume > 0. && sig.avgDlyVolat > 0.
			&& quotes != 0 && trades != 0 && quotes->size() >= min_quotes_ && trades->size() >= min_trades_ )
		{
			// SignalCalculator.
			SignalCalculator sigcal(ticker, sig, quotes, trades, &sessions_, linMod.openMsecs, linMod.closeMsecs, linMod.exploratoryDelay, sip);
			if( "US" == market )
			{
				if( idate < 20130307 )
					sig.medNbooktrades = medNbooktradesmap_[ticker];
				else
					sig.medNbooktrades = sig.medNtrades * 1.13;
			}
			else
				sig.medNbooktrades = sig.medNtrades;
			//sigcal.desiredSamplesOm = desiredSamplesOm;
			//sigcal.desiredSamplesTm = desiredSamplesTm;

			double desiredQuoteProb = (sig.medNquotes > 0) ? (desiredSamplesOm / sig.medNquotes) : -1.;
			double desiredTradeProb = (sig.medNtrades > 0) ? (desiredSamplesOm / sig.medNtrades) : -1.;
			double desiredBooktradeProb = (sig.medNbooktrades > 0) ? (desiredSamplesOm / sig.medNbooktrades) : -1.;

			sigcal.regularSampleProbability = 1.;
			if( !write_omtxt_reg_ && !write_ombin_reg_ && !write_tmtxt_reg_ && !write_tmbin_reg_ )
				sigcal.regularSampleProbability = -1.;
			//sigcal.quoteSampleProbability = (sig.medNquotes > 0) ? (desiredSamples / sig.medNquotes) : -1.;
			//sigcal.tradeSampleProbability = (sig.medNtrades > 0) ? (desiredSamples / sig.medNtrades) : -1.;
			//sigcal.booktradeSampleProbability = (sig.medNbooktrades > 0) ? (desiredSamples / sig.medNbooktrades) : -1.;
			sigcal.quoteSampleProbability = -1.;
			sigcal.tradeSampleProbability = -1.;
			sigcal.booktradeSampleProbability = desiredBooktradeProb;
			if( !write_omtxt_tevt_ && !write_ombin_tevt_ )
				sigcal.booktradeSampleProbability = -1.;
			if( debug_booktrade_signal_ || sampleAtWrite_ )
				sigcal.booktradeSampleProbability = 1.;

			if( debug_fillImbBook_ )
				sigcal.debug_fillImbBook(tSources_, linMod.ecns(), idate, ticker);
			if( debug_booktrade_prob_ )
				sigcal.debug_booktrade_prob();
			if( debug_rz_signal_ )
				sigcal.debug_rz_signal();

			TickProviderClassic<TradeInfo, QuoteInfo, OrderData>* tp = 0;
			if( MEnv::Instance()->multiThreadTicker )
				init_tp(tp);
			else
				tp = tp_;

			// TickProviderClassic.
			tp->SetConsumer(&sigcal);
			tp->StartSymbol(ticker, 0., sampleMsecsVect_);

			// end of NYSE auction.
			if( "US" == market )
			{
				if( binary_search(nyse_names_.begin(), nyse_names_.end(), ticker) )
				{
					map<string, int>::iterator it = first_trades_.find(ticker);
					if( it != first_trades_.end() )
						tp->SetEndOfAuction('N', it->second);
				}
			}

			// Set time for news events.
			sigcal.set_news(news);
			if( write_ombin_nevt_ && news != 0 && !news->empty() )
			{
				for( vector<QuoteInfo>::const_iterator it = news->begin(); it != news->end(); ++it )
				{
					//int msecs = it->msecs;
					if( it->msecs > linMod.openMsecs && it->msecs < linMod.closeMsecs )
						tp->SetTimeCB(it->msecs, (void*)cb_ref_nevt_);
				}
			}

			// Run.
			tp->Run();
			if( MEnv::Instance()->multiThreadTicker )
				delete tp;

			// Calculate predictions and update the linear model.
			get_prediction_at(sig, uid, ticker, idate);

			// Write signal files.
			//string write_ticker = ticker;
			//if( !linMod.writeCompTicker )
				//write_ticker = baseTicker(write_ticker);
			if( sampleAtWrite_ )
				write_signal(sig, uid, ticker, desiredSamplesOm, desiredSamplesTm);
			else
				write_signal(sig, uid, ticker);
		}

		SigSet::Instance()->free_sig_object(iSig);
	}
}

void MMakeSampleEvt::endTicker(const string& ticker)
{
}

void MMakeSampleEvt::endMarket()
{
}

void MMakeSampleEvt::endDay()
{
	const LinearModel& linMod = MEnv::Instance()->linearModel;

	if( write_omtxt_reg_ )
	{
		omTxt_reg_.close();
		omTxt_reg_.clear();
	}

	if( write_ombin_reg_ )
	{
		writeSig::update_ncols(omBin_reg_, cntOmBin_reg_);

		omBin_reg_.close();
		omBin_reg_.clear();

		omBinTxt_reg_.close();
		omBinTxt_reg_.clear();

		cntOmBin_reg_ = 0;
	}

	if( write_omtxt_tevt_ )
	{
		omTxt_tevt_.close();
		omTxt_tevt_.clear();
	}

	if( write_ombin_tevt_ )
	{
		writeSig::update_ncols(omBin_tevt_, cntOmBin_tevt_);

		omBin_tevt_.close();
		omBin_tevt_.clear();

		omBinTxt_tevt_.close();
		omBinTxt_tevt_.clear();

		cntOmBin_tevt_ = 0;
	}

	if( write_ombin_nevt_ )
	{
		writeSig::update_ncols(omBin_nevt_, cntOmBin_nevt_);

		omBin_nevt_.close();
		omBin_nevt_.clear();

		omBinTxt_nevt_.close();
		omBinTxt_nevt_.clear();

		cntOmBin_nevt_ = 0;
	}

	if( write_ombin_eevt_ )
	{
		writeSig::update_ncols(omBin_eevt_, cntOmBin_eevt_);

		omBin_eevt_.close();
		omBin_eevt_.clear();

		omBinTxt_eevt_.close();
		omBinTxt_eevt_.clear();

		cntOmBin_eevt_ = 0;
	}

	if( write_tmbin_eevt_ )
	{
		writeSig::update_ncols(tmBin_eevt_, cntTmBin_eevt_);

		tmBin_eevt_.close();
		tmBin_eevt_.clear();

		tmBinTxt_eevt_.close();
		tmBinTxt_eevt_.clear();

		cntTmBin_eevt_ = 0;
	}

	if( write_tmtxt_reg_ )
	{
		tmTxt_reg_.close();
		tmTxt_reg_.clear();
	}

	if( write_tmbin_reg_ )
	{
		writeSig::update_ncols(tmBin_reg_, cntTmBin_reg_);

		tmBin_reg_.close();
		tmBin_reg_.clear();

		tmBinTxt_reg_.close();
		tmBinTxt_reg_.clear();

		cntTmBin_reg_ = 0;
	}

	if( write_tmtxt_tevt_ )
	{
		tmTxt_tevt_.close();
		tmTxt_tevt_.clear();
	}

	if( write_tmbin_tevt_ )
	{
		writeSig::update_ncols(tmBin_tevt_, cntTmBin_tevt_);

		tmBin_tevt_.close();
		tmBin_tevt_.clear();

		tmBinTxt_tevt_.close();
		tmBinTxt_tevt_.clear();

		cntTmBin_tevt_ = 0;
	}

	//if( write_tmtxt_ )
	//{
	//	tmTxt_.close();
	//	tmTxt_.clear();
	//}

	//if( write_tmbin_ )
	//{
	//	writeSig::update_ncols(tmBin_, cntTmBin_);

	//	tmBin_.close();
	//	tmBin_.clear();

	//	tmBinTxt_.close();
	//	tmBinTxt_.clear();

	//	cntTmBin_ = 0;
	//}

	if( write_om_ )
	{
		for( int iT = 0; iT < linMod.nHorizon; ++iT )
		{
			cout << itos(linMod.vHorizonLag[iT].first) + ";" + itos(linMod.vHorizonLag[iT].second) << "\t";
			cout << "corr " << vCorr_[iT].corr() << " corrTot " << vCorrTot_[iT].corr() << endl;
		}
	}

	++cntDay_;
	nErrDay_ = 0;
}

void MMakeSampleEvt::endJob()
{
}

void MMakeSampleEvt::GetFirstTradeTimeBook(int idate, int openMsecs)
{
	first_trades_.clear();

	TickAccessMulti<TradeInfo> ta;
	vector<string> dirs = mto::bindirsBook("US", idate);
	for( vector<string>::iterator it = dirs.begin(); it != dirs.end(); ++it )
		ta.AddRoot(*it);

	for( vector<string>::iterator it = nyse_names_.begin(); it != nyse_names_.end(); ++it )
	{
		string symbol = *it;
		TickSeries<TradeInfo> ts;
		ta.GetTickSeries(symbol, idate, &ts);

		TradeInfo trade;
		int firstTradeMsecs = 0;
		while( ts.Read(&trade) )
		{
			if( trade.msecs > openMsecs )
			{
				firstTradeMsecs = trade.msecs;
				break;
			}
		}
		if( firstTradeMsecs > 0 )
			first_trades_[symbol] = firstTradeMsecs;
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
	for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
	{
		tp->AddNbboRoot( *itd, longTicker_ );
		tp->AddTradeRoot( *itd, longTicker_ );
	}

	// bookdir.
	if( "US" == market )
	{
		auto ecns = linMod.get_ecns();
		for( vector<string>::const_iterator ite = ecns.begin(); ite != ecns.end(); ++ite )
		{
			string ecn = *ite;
			vector<string> dirs = tSources_.bookdirectory(ecn, idate);
			for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
			{
				tp->AddBookRoot( ecn[1], *itd );
				tp->AddBookTradeRoot( ecn[1], *itd );
			}
		}
	}
	else
	{
		// main market.
		vector<string> dirs = tSources_.bookdirectory(market, idate);
		for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
		{
			tp->AddBookRoot( mto::code(market)[0], *itd, longTicker_ );
			tp->AddBookTradeRoot( mto::code(market)[0], *itd, longTicker_ );
		}

		// ecns.
		for( vector<string>::const_iterator ite = okECNs_.begin(); ite != okECNs_.end(); ++ite )
		{
			string ecn = *ite;
			vector<string> dirs = tSources_.bookdirectory(ecn, idate);
			for( vector<string>::iterator itd = dirs.begin(); itd != dirs.end(); ++itd )
			{
				tp->AddBookRoot( ecn[1], *itd, longTicker_ );
				tp->AddBookTradeRoot( ecn[1], *itd, longTicker_ );
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
		for( vector<hff::SigH>::const_iterator it = pvSigH->begin(); it != pvSigH->end(); ++it )
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

	int n1secSlice = ceil( float(linMod.n1sec - 1) / hff::num_time_slices_ );
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
			sI[k].target60Intra = 0.;
			sI[k].targetClose = 0.;
			sI[k].targetNextClose = 0.;
		}

		if( sI[k].gptOK == 1 && (write_om_) )
		{
			for( int iT = 0; iT < linMod.nHorizon; ++iT )
			{
				// Set gSigs.
				for( vector<int>::iterator it = vIndx.begin(); it != vIndx.end(); ++it )
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
				for( vector<hff::IndexFilter>::const_iterator itf = filters.begin(); itf != filters.end() && indxIndx < 6; ++itf )
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
				//if( write_weights_ )
				//{
				//	prIndex = biLinMod_.predIndex(iT, timeIdx, sI[k].om) + biLinMod_.predErrIndex(iT, uid, sI[k].omErr);
				//	pr = biLinMod_.pred(iT, timeIdx, sI[k].om);
				//	prErr = biLinMod_.predErr(iT, uid, sI[k].omErr);
				//}
				//else
				if( use_db_weights_ )
				{
					pr = biLinModW_.predDB(timeIdx, uid, sI[k].om);
				}
				else
				{
					prIndex = biLinModW_.predIndex(iT, timeIdx, sI[k].om) + biLinModW_.predErrIndex(iT, uid, sI[k].omErr);
					pr = biLinModW_.pred(iT, timeIdx, sI[k].om);
					prErr = biLinModW_.predErr(iT, uid, sI[k].omErr);
				}

				sI[k].predIndex[iT] = prIndex;
				sI[k].sigIndex1[iT] = sI[k].om[4];
				sI[k].sigIndex2[iT] = sI[k].om[5];
				sI[k].sigIndex3[iT] = sI[k].om[6];
				sI[k].sigIndex4[iT] = sI[k].om[7];
				sI[k].sigIndex5[iT] = sI[k].om[12];
				sI[k].sigIndex6[iT] = sI[k].om[13];

				sI[k].pred[iT] = pr;

				sI[k].predErr[iT] = prErr;

				// Get error targets.
				if( iT < linMod.nHorizon )
					sI[k].targetErr[iT] = sI[k].target[iT] - sI[k].pred[iT];
				else
					sI[k].targetErr[iT] = sI[k].target[iT];

				//if( write_weights_ )
				//	update_biLin(k, iT, timeIdx, uid, sig, sI[k]);
				if( k >= skip_first_secs_ / linMod.gridInterval )
				{
					vCorr_[iT].add(sI[k].pred[iT], sI[k].target[iT]);
					vCorrTot_[iT].add(sI[k].pred[iT] + sI[k].predErr[iT], sI[k].target[iT]);
				}
			}
		}
	}
}

//void MMakeSampleEvt::update_biLin(int k, int iT, int timeIdx, string uid, SigC& sig, StateInfo& sI)
//{
//	const LinearModel& linMod = MEnv::Instance()->linearModel;
//	if( k >= skip_first_secs_ / linMod.gridInterval )
//	{
//		vCorr_[iT].add(sI.pred[iT], sI.target[iT]);
//		vCorrTot_[iT].add(sI.pred[iT] + sI.predErr[iT], sI.target[iT]);
//
//		// Update univ model.
//		if( sig.inUnivFit == 1 && do_universal_linear_model_)
//		{
//			boost::mutex::scoped_lock lock(biLinMod_mutex_);
//			biLinMod_.update(iT, timeIdx, sI.om, sI.target[iT]);
//		}
//
//		// Update err corr model.
//		if( do_error_correction_model_ )
//		{
//			if( /*debugErrCorr_ ||*/ cntDay_ >= linMod.om_univ_fit_days() )
//			{
//				biLinMod_.updateErr(iT, uid, sI.omErr, sI.targetErr[iT]);
//			}
//		}
//	}
//}

void MMakeSampleEvt::write_signal(SigC& sig, const string& uid, const string& ticker, int desiredSamplesOm, int desiredSamplesTm)
{
	// Write signals.
	if( sig.inUnivFit )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		//int sample_max = 2 * (linMod.closeMsecs - linMod.openMsecs) / 30000 + 1;
		boost::mutex::scoped_lock lock(om_mutex_);

		if( write_omtxt_reg_ || write_ombin_reg_ || write_tmbin_reg_ )
		{
			int cnt = 0;
			int tmSample = desiredSamplesTm / desiredSamplesOm;
			for( vector<StateInfo>::iterator it = sig.sI.begin(); it != sig.sI.end(); ++it )
			{
				if( regular_sample_ & it->sampleType )
				{
					bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
					if( it->gptOK && sprdOK && sprdOK )
					{
						it->valid = 1;
						if( ++cnt % tmSample == 1 )
							it->validTm = 1;
					}
				}
			}
		}
		if( write_omtxt_reg_ )
			writeSig::write_omTxt_evt(omTxt_reg_, sig, uid, ticker, minMsecs_, maxMsecs_, regular_sample_);
		if( write_ombin_reg_ )
			cntOmBin_reg_ += writeSig::write_omBin_evt(omBin_reg_, omBinTxt_reg_, sig, uid, ticker, minMsecs_, maxMsecs_, regular_sample_);
		if( write_tmbin_reg_ )
			cntTmBin_reg_ += writeSig::write_tmBin_evt(tmBin_reg_, tmBinTxt_reg_, sig, uid, ticker, minMsecs_, maxMsecs_, regular_sample_);

		if( write_omtxt_tevt_ || write_ombin_tevt_ || write_tmbin_tevt_ )
		{
			select_events(sig.sI, book_trade_event_, desiredSamplesOm, desiredSamplesTm);
			//int last_msso = 0;
			//double tot = 0.;
			//double omProb = 0.;
			//double tmProb = 0.;
			//for( vector<StateInfo>::iterator it = sig.sI.begin(); it != sig.sI.end(); ++it )
			//{
			//	if( book_trade_event_ & it->sampleType && it->gptOK && it->bidPersists && it->askPersists )
			//	{
			//		if( it->msso > last_msso )
			//			++tot;
			//		last_msso = it->msso;
			//	}
			//}
			//if( tot > 0. )
			//{
			//	if( tot > desiredSamplesOm )
			//		omProb = desiredSamplesOm / tot;
			//	else
			//		omProb = 1.;

			//	if( tot > desiredSamplesOm )
			//		tmProb = (double)desiredSamplesTm / desiredSamplesOm;
			//	else if( tot > desiredSamplesTm )
			//		tmProb = (double)desiredSamplesTm / tot;
			//	else
			//		tmProb = 1.;
			//}
			//last_msso = 0;
			//int lastTimeWritten = 0;
			//int totWritten = 0;
			//for( vector<StateInfo>::iterator it = sig.sI.begin(); it != sig.sI.end(); ++it )
			//{
			//	bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
			//	if( book_trade_event_ & it->sampleType && it->gptOK && sprdOK && totWritten < sample_max && it->bidPersists && it->askPersists )
			//	{
			//		if( Random::Instance()->select(omProb) )
			//		{
			//			if( it->msso > last_msso )
			//			{
			//				it->valid = 1;
			//				lastTimeWritten = it->msso;
			//				++totWritten;

			//				if( Random::Instance()->select(tmProb) )
			//					it->validTm = 1;
			//			}
			//		}
			//		last_msso = it->msso;
			//	}
			//}
		}
		if( write_omtxt_tevt_ )
			writeSig::write_omTxt_evt(omTxt_tevt_, sig, uid, ticker, minMsecs_, maxMsecs_, book_trade_event_);
		if( write_ombin_tevt_ )
			cntOmBin_tevt_ += writeSig::write_omBin_evt(omBin_tevt_, omBinTxt_tevt_, sig, uid, ticker, minMsecs_, maxMsecs_, book_trade_event_);
		if( write_tmbin_tevt_ )
			cntTmBin_tevt_ += writeSig::write_tmBin_evt(tmBin_tevt_, tmBinTxt_tevt_, sig, uid, ticker, minMsecs_, maxMsecs_, book_trade_event_);

		//if( write_ombin_nevt_ )
		//{
		//}
		//if( write_ombin_nevt_ )
		//	cntOmBin_nevt_ += writeSig::write_omBin_evt(omBin_nevt_, omBinTxt_nevt_, sig, uid, ticker, minMsecs_, maxMsecs_, news_event_);

		if( write_ombin_eevt_ || write_tmbin_eevt_ )
		{
			select_events(sig.sI, exploratory_event_, desiredSamplesOm, desiredSamplesTm);
		}
		if( write_ombin_eevt_ )
			cntOmBin_eevt_ += writeSig::write_omBin_evt(omBin_eevt_, omBinTxt_eevt_, sig, uid, ticker, minMsecs_, maxMsecs_, exploratory_event_);
		if( write_tmbin_eevt_ )
			cntTmBin_eevt_ += writeSig::write_tmBin_evt(tmBin_eevt_, tmBinTxt_eevt_, sig, uid, ticker, minMsecs_, maxMsecs_, exploratory_event_);

		//if( write_tmtxt_ )
		//	writeSig::write_tmTxt_evt(tmTxt_, sig, uid, ticker, minMsecs_, maxMsecs_);
		//if( write_tmbin_ )
		//	cntTmBin_ += writeSig::write_tmBin_evt(tmBin_, tmBinTxt_, sig, uid, ticker, minMsecs_, maxMsecs_);


	}
}

void MMakeSampleEvt::select_events(vector<StateInfo>& sI, int sampleType, int desiredSamplesOm, int desiredSamplesTm)
{
	int last_msso = 0;
	double tot = 0.;
	double omProb = 0.;
	double tmProb = 0.;
	for( vector<StateInfo>::iterator it = sI.begin(); it != sI.end(); ++it )
	{
		if( sampleType & it->sampleType && it->gptOK && it->bidPersists && it->askPersists )
		{
			if( it->msso > last_msso )
				++tot;
			last_msso = it->msso;
		}
	}
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
	last_msso = 0;
	int lastTimeWritten = 0;
	int totWritten = 0;
	for( vector<StateInfo>::iterator it = sI.begin(); it != sI.end(); ++it )
	{
		bool sprdOK = fabs(it->sprd) >= om_tree_min_fit_sprd_ && fabs(it->sprd) < om_tree_max_fit_sprd_;
		if( sampleType & it->sampleType && it->gptOK && sprdOK && totWritten < sample_max_ && it->bidPersists && it->askPersists )
		{
			if( Random::Instance()->select(omProb) )
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
