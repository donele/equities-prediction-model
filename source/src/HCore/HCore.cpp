#include <jl_lib/jlutil.h>
#include <TH1.h>
#include <map>
#include <string>
#include <fstream>
#include <vector>
#include <boost/thread.hpp>
#include <HCore/HCore.h>
#include <HLib.h>
#include <HOrders.h>
#include <HTickSeries.h>
#ifdef _WIN32
#include <HMod.h>
#include <HModDataOK.h>
#include <HSecRet.h>
#include <HModMon.h>
#endif
using namespace std;

HCore::HCore(string filename)
{
	vector<pair<ModuleInfo, multimap<string, string> > > conf;
	ifstream is(filename.c_str());
	if( is.is_open() )
		conf_ = HConfig(is);
}

HCore::HCore(istream& is)
{
	conf_ = HConfig(is);
}

HCore::HCore(HConfig& conf)
{
	conf_ = conf;
}

void HCore::run()
{
	printf("Process ID: %u.\n", get_pid());
	init(conf_.get_block());
	runImpl();
}

void HCore::init(vector<pair<ModuleInfo, multimap<string, string> > > block)
{
	// 'block' is a vector of pairs of module name and option map.
	// An option map is a multimap of pairs of option and value.
	// block[n].first is the name of module.
	// block[n].second is the multimap for the module.

	cout << block;

	modules_.clear();

	// Other modules.
	int execOrder = -1;
	int nMods = block.size();
	for( int i=0; i<nMods; ++i )
	{
		HModule* mod = 0;
		string className = block[i].first.className;
		string moduleName = block[i].first.moduleName;
		int temp_execOrder = block[i].first.execOrder;
		if( temp_execOrder >= 0 )
		{
			if( execOrder <= temp_execOrder )
				execOrder = temp_execOrder;
			else
			{
				cerr << "HCore:Error execOrder of module " << moduleName << endl;
				exit(5);
			}
		}
		else
			++execOrder;

		if( 0 )
			;
#ifdef __HLib_Headers__
		else if( "HInit" == className )
			mod = new HInit(moduleName, block[0].second);
		//else if( "HInitModel" == className )
		//	mod = new HInitModel(moduleName, block[0].second);
#endif
#ifdef __HMod_Headers__
		//else if( "HChixQuality" == className )
		//	mod = new HChixQuality(moduleName, block[i].second);
		//else if( "HChixOrders" == className )
		//	mod = new HChixOrders(moduleName, block[i].second);
		//else if( "HChixEffect_test" == className )
		//	mod = new HChixEffect_test(moduleName, block[i].second);
		//else if( "HChixEffect" == className )
		//	mod = new HChixEffect(moduleName, block[i].second);
		//else if( "HChixEffectByTicker" == className )
		//	mod = new HChixEffectByTicker(moduleName, block[i].second);
		//else if( "HChixEffectBadness" == className )
		//	mod = new HChixEffectBadness(moduleName, block[i].second);
		//else if( "HChiXTokyoDelay" == className )
		//	mod = new HChiXTokyoDelay(moduleName, block[i].second);
		else if( "HBookTradeStat" == className )
			mod = new HBookTradeStat(moduleName, block[i].second);
		//else if( "HContHKW" == className )
		//	mod = new HContHKW(moduleName, block[i].second);
		//else if( "HDarkVolume" == className )
		//	mod = new HDarkVolume(moduleName, block[i].second);
		//else if( "HDarkFilter" == className )
		//	mod = new HDarkFilter(moduleName, block[i].second);
		//else if( "HDarkFilterOOS" == className )
		//	mod = new HDarkFilterOOS(moduleName, block[i].second);
		//else if( "HDarkFilterScale" == className )
		//	mod = new HDarkFilterScale(moduleName, block[i].second);
		else if( "HExample" == className )
			mod = new HExample(moduleName, block[i].second);
		else if( "HFillImbBook" == className )
			mod = new HFillImbBook(moduleName, block[i].second);
		//else if( "HIndexFutureTest" == className )
		//	mod = new HIndexFutureTest(moduleName, block[i].second);
		//else if( "HIndexFutureFit" == className )
		//	mod = new HIndexFutureFit(moduleName, block[i].second);
		//else if( "HInitHKW" == className )
		//	mod = new HInitHKW(moduleName, block[i].second);
		else if( "HMarketSumm" == className )
			mod = new HMarketSumm(moduleName, block[i].second);
		else if( "HMinimumResponse" == className )
			mod = new HMinimumResponse(moduleName, block[i].second);
		else if( "HNbboAna" == className )
			mod = new HNbboAna(moduleName, block[i].second);
		else if( "HNbboCross" == className )
			mod = new HNbboCross(moduleName, block[i].second);
		else if( "HQuoteTime" == className )
			mod = new HQuoteTime(moduleName, block[i].second);
		else if( "HStocksCorr" == className )
			mod = new HStocksCorr(moduleName, block[i].second);
		else if( "HStocksCorrTest" == className )
			mod = new HStocksCorrTest(moduleName, block[i].second);
		else if( "HUSvolume" == className )
			mod = new HUSvolume(moduleName, block[i].second);
#endif
#ifdef __HModDataOK_Headers__
		else if( "HDataOK" == className )
			mod = new HDataOK(moduleName, block[i].second);
#endif
#ifdef __HModMon_Headers__
		else if( "HMon" == className )
			mod = new HMon(moduleName, block[i].second);
		//else if( "HDiffusion" == className )
			//mod = new HDiffusion(moduleName, block[i].second);
#endif
#ifdef __HOrders_Headers__
		else if( "HFillTime" == className )
			mod = new HFillTime(moduleName, block[i].second);
		else if( "HFillTimeHist" == className )
			mod = new HFillTimeHist(moduleName, block[i].second);
		else if( "HOrderSummRead_test" == className )
			mod = new HOrderSummRead_test(moduleName, block[i].second);
		else if( "HOrderSummRead" == className )
			mod = new HOrderSummRead(moduleName, block[i].second);
		else if( "HEurexRead" == className )
			mod = new HEurexRead(moduleName, block[i].second);
		else if( "HOrderSumm" == className )
			mod = new HOrderSumm(moduleName, block[i].second);
		else if( "HOrderAna" == className )
			mod = new HOrderAna(moduleName, block[i].second);
		else if( "HMarketMakingSpeed" == className )
			mod = new HMarketMakingSpeed(moduleName, block[i].second);
		//else if( "HOrdersCouldHaveUsedNews" == className )
			//mod = new HOrdersCouldHaveUsedNews(moduleName, block[i].second);
		else if( "HTradeAna" == className )
			mod = new HTradeAna(moduleName, block[i].second);
		else if( "HTradeAnaGainSig" == className )
			mod = new HTradeAnaGainSig(moduleName, block[i].second);
		else if( "HTradeAnaGainSigMult" == className )
			mod = new HTradeAnaGainSigMult(moduleName, block[i].second);
		else if( "HReturnAna" == className )
			mod = new HReturnAna(moduleName, block[i].second);
		else if( "HCloseToCloseAna" == className )
			mod = new HCloseToCloseAna(moduleName, block[i].second);
		else if( "HTradeAnaProfile" == className )
			mod = new HTradeAnaProfile(moduleName, block[i].second);
		else if( "HTradeAnaVolume" == className )
			mod = new HTradeAnaVolume(moduleName, block[i].second);
		else if( "HMercatorTrades" == className )
			mod = new HMercatorTrades(moduleName, block[i].second);
#endif
#ifdef __HSecRet_Headers__
		else if( "HSecRet" == className )
			mod = new HSecRet(moduleName, block[i].second);
		else if( "HSecRetMult" == className )
			mod = new HSecRetMult(moduleName, block[i].second);
		else if( "HSecRetMultWrite" == className )
			mod = new HSecRetMultWrite(moduleName, block[i].second);
		else if( "HSecRetMultCap" == className )
			mod = new HSecRetMultCap(moduleName, block[i].second);
		else if( "HSecRetIndexFutures" == className )
			mod = new HSecRetIndexFutures(moduleName, block[i].second);
#endif
#ifdef __HTickSeries_Headers__
		else if( "HQuoteSample" == className )
			mod = new HQuoteSample(moduleName, block[i].second);
		else if( "HTickRead" == className )
			mod = new HTickRead(moduleName, block[i].second);
		else if( "HTickReadM" == className )
			mod = new HTickReadM(moduleName, block[i].second);
		else if( "HTickReadIndex" == className )
			mod = new HTickReadIndex(moduleName, block[i].second);
		else if( "HTickLoad" == className )
			mod = new HTickLoad(moduleName, block[i].second);
		else if( "HTickSeries" == className )
			mod = new HTickSeries(moduleName, block[i].second);
		else if( "HTickSeriesQuote" == className )
			mod = new HTickSeriesQuote(moduleName, block[i].second);
		else if( "HTickSeriesTrade" == className )
			mod = new HTickSeriesTrade(moduleName, block[i].second);
		else if( "HTickSeriesNews" == className )
			mod = new HTickSeriesNews(moduleName, block[i].second);
#endif
#ifdef _WIN32
		//else if( "HNNSample" == className )
		//	mod = new HNNSample(moduleName, block[i].second);
		//else if( "HNNSampleCommodity" == className )
		//	mod = new HNNSampleCommodity(moduleName, block[i].second);
		//else if( "HNNSampleNews" == className )
		//	mod = new HNNSampleNews(moduleName, block[i].second);
		//else if( "HNNSampleNewsFile" == className )
		//	mod = new HNNSampleNewsFile(moduleName, block[i].second);
		//else if( "HNNSampleRead" == className )
		//	mod = new HNNSampleRead(moduleName, block[i].second);
		//else if( "HSignalNewsFile" == className )
		//	mod = new HSignalNewsFile(moduleName, block[i].second);
		//else if( "HSignalNewsBin" == className )
		//	mod = new HSignalNewsBin(moduleName, block[i].second);
		//else if( "HSignalRead" == className )
		//	mod = new HSignalRead(moduleName, block[i].second);
		//else if( "HSignalSubset" == className )
		//	mod = new HSignalSubset(moduleName, block[i].second);
		//else if( "HBinSigRead" == className )
		//	mod = new HBinSigRead(moduleName, block[i].second);
		//else if( "HNNReadRoot" == className )
		//	mod = new HNNReadRoot(moduleName, block[i].second);
		//else if( "HNNTrainer" == className )
		//	mod = new HNNTrainer(moduleName, block[i].second);
		//else if( "HNNTester" == className )
		//	mod = new HNNTester(moduleName, block[i].second);
		//else if( "HNNTrader" == className )
		//	mod = new HNNTrader(moduleName, block[i].second);
		//else if( "HRavenPack" == className )
		//	mod = new HRavenPack(moduleName, block[i].second);
		//else if( "HRavenPackGlobal" == className )
		//	mod = new HRavenPackGlobal(moduleName, block[i].second);
		//else if( "HRavenPackAgg" == className )
		//	mod = new HRavenPackAgg(moduleName, block[i].second);
		//else if( "HRavenPackAggMarketRet" == className )
		//	mod = new HRavenPackAggMarketRet(moduleName, block[i].second);
		//else if( "HRavenPackRead" == className )
		//	mod = new HRavenPackRead(moduleName, block[i].second);
		//else if( "HNewsScope" == className )
		//	mod = new HNewsScope(moduleName, block[i].second);
		//else if( "HNewsRead" == className )
		//	mod = new HNewsRead(moduleName, block[i].second);
		//else if( "HNewsReadGlobal" == className )
		//	mod = new HNewsReadGlobal(moduleName, block[i].second);
		//else if( "HNewsComp" == className )
		//	mod = new HNewsComp(moduleName, block[i].second);
		//else if( "HReturnsCorr" == className )
		//	mod = new HReturnsCorr(moduleName, block[i].second);
		//else if( "HMakeSampleIndex" == className )
		//	mod = new HMakeSampleIndex(moduleName, block[i].second);
		//else if( "HBiLinModTest" == className )
		//	mod = new HBiLinModTest(moduleName, block[i].second);
		//else if( "HMakeSampleTarget" == className )
		//	mod = new HMakeSampleTarget(moduleName, block[i].second);
		//else if( "HMarketSumm" == className )
		//	mod = new HMarketSumm(moduleName, block[i].second);
		//else if( "HCompSig" == className )
		//	mod = new HCompSig(moduleName, block[i].second);
#endif
		else
		{
			cerr << " module " << className << " is unknown to HCore." << endl;
			exit(3);
		}

		if( mod != 0 )
			modules_.insert( make_pair(execOrder, mod) );
	}
}

//
//	Run the Modules.
//

void HCore::runImpl()
{
	TH1::AddDirectory(kFALSE);

	{
		// Create singletons in a single thread mode.
		HEnv::Instance();
		HEvent::Instance();

		for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			it->second->beginJobBase();

		// Repeat the markets MR times. Usually MR is one.
		int MR = HEnv::Instance()->marketRep();
		for( int mr = 0; mr < MR; ++mr )
		{
			HEnv::Instance()->iMarketRep(mr);

			if( HEnv::Instance()->loopingOrder() == "mdt" )
				loop_markets_dates();
			else if( HEnv::Instance()->loopingOrder() == "dmt" )
				loop_dates_markets();
		}

		for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			it->second->endJobBase();

		HEnv::Instance()->clear();
		HEvent::Instance()->clear();
	}

	return;
}

void HCore::loop_markets_dates()
{
	// Loop markets.
	vector<string> markets = HEnv::Instance()->markets();
	for( vector<string>::iterator itm = markets.begin(); itm != markets.end(); ++itm )
	{
		string market = *itm;
		HEnv::Instance()->market(market);

		for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			it->second->beginMarketBase();

		// Loop dates.
		vector<int> idates = HEnv::Instance()->idates();
		for( vector<int>::iterator itd = idates.begin(); itd != idates.end(); ++itd )
		{
			int idate = *itd;
			HEnv::Instance()->idate(idate);

			for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			{
				if( it->second->validIdate(idate) )
				{
					it->second->beginDayBase();
					it->second->beginMarketDayBase();
				}
			}

			// Loop tickers.
			loop_tickers();

			for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			{
				if( it->second->validIdate(idate) )
				{
					it->second->endMarketDay();
					it->second->endDay();
				}
			}
		}

		for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			it->second->endMarket();
	}
	return;
}

void HCore::loop_dates_markets()
{
	vector<string> markets = HEnv::Instance()->markets();
	vector<int> idates = HEnv::Instance()->idates();

	if( !markets.empty() && !idates.empty() )
	{
		// Loop dates.
		for( vector<int>::iterator itd = idates.begin(); itd != idates.end(); ++itd )
		{
			int idate = *itd;
			HEnv::Instance()->idate(idate);

			for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			{
				if( it->second->validIdate(idate) )
					it->second->beginDayBase();
			}

			// Loop markets.
			for( vector<string>::iterator itm = markets.begin(); itm != markets.end(); ++itm )
			{
				string market = *itm;

				HEnv::Instance()->market(market);

				for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
				{
					if( it->second->validIdate(idate) )
					{
						it->second->beginMarketBase();
						it->second->beginMarketDayBase();
					}
				}

				// Loop tickers.
				loop_tickers();

				for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
				{
					if( it->second->validIdate(idate) )
					{
						it->second->endMarketDay();
						it->second->endMarket();
					}
				}
			}

			for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			{
				if( it->second->validIdate(idate) )
					it->second->endDay();
			}
		}
	}
	return;
}

void HCore::loop_tickers()
{
	vector<string> tickers = HEnv::Instance()->tickers();
	//copy(tickers.begin(), tickers.end(), ostream_iterator<string>(cout, " "));
	//
	// Multi thread tickers.
	//
	if( HEnv::Instance()->multiThreadTicker() )
	{
		int nMaxThreadTicker = HEnv::Instance()->nMaxThreadTicker();
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int iThread = 0; iThread < nMaxThreadTicker; ++iThread )
		{
			listThread.push_back(
				boost::shared_ptr<boost::thread>(
					new boost::thread(boost::bind(&HCore::loop_tickers_thread, this, iThread))
				)
			);
		}

		// join ticker threads.
		for( list<boost::shared_ptr<boost::thread> >::iterator it = listThread.begin(); it != listThread.end(); ++it )
			(*it)->join();
	}
	//
	// Single thread tickers.
	//
	else
	{
		for( vector<string>::const_iterator itt = tickers.begin(); itt != tickers.end(); ++itt )
		{
			string ticker = *itt;
			loopModule(ticker);
		}
	}
	return;
}

void HCore::loop_tickers_thread(int iThread)
{
	vector<string> tickers = HEnv::Instance()->tickers();
	int nTickers = tickers.size();
	int nMaxThreadTicker = HEnv::Instance()->nMaxThreadTicker();
	int nTickerEachThread = ceil((float)nTickers / nMaxThreadTicker);
	vector<string>::iterator itBeg = tickers.begin();

	//cout << "iThread " << iThread << endl;
	int offset1 = iThread * nTickerEachThread;
	if( offset1 < nTickers )
	{
		int offset2 = (iThread + 1) * nTickerEachThread;
		if( offset2 > nTickers || iThread == nMaxThreadTicker - 1 )
			offset2 = nTickers;

		if( offset2 > offset1 )
		{
			vector<string>::iterator itFrom = itBeg + offset1;
			vector<string>::iterator itTo = itBeg + offset2;

			//cout << "HCore::loop_tickers_thread() thread id is: " << boost::this_thread::get_id()
			//	<< "\n tickers from " << *itFrom << " to " << *(itTo-1) << endl;
			for( vector<string>::iterator it = itFrom; it != itTo; ++it )
			{
				string ticker = *it;
				loopModule(ticker);
			}
		}
	}
}
//
//void HCore::loopModule(string ticker)
//{
//	for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
//	{
//		if( it->second->validIdate(HEnv::Instance()->idate()) )
//			it->second->beginTickerBase(ticker);
//	}
//
//	for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
//	{
//		if( it->second->validIdate(HEnv::Instance()->idate()) )
//			it->second->endTicker(ticker);
//	}
//
//	return;
//}

void HCore::loopModule(string ticker)
{
/*
*	beginTickerBase() Multi thread modules.
*/
	if( false && HEnv::Instance()->multiThreadModule() ) // Not tested yet.
	{
		// Sort out the execution orders of the modules.
		set<int> sExecOrder;
		for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			sExecOrder.insert(it->first);

		// Start multithreading according to the execution orders.
		for( set<int>::iterator it = sExecOrder.begin(); it != sExecOrder.end(); ++it )
		{
			int execOrder = *it;

			// Find the modules to run in parallel.
			int nThreads = 0;
			if( modules_.count(execOrder) )
			{
				pair<itmod, itmod> pitmod = modules_.equal_range(execOrder);
				nThreads = distance(pitmod.first, pitmod.second);

				if( nThreads == 1 ) // Single thread.
				{
					if( pitmod.first->second->validIdate(HEnv::Instance()->idate()) )
						pitmod.first->second->beginTickerBase(ticker);
				}
				else if( nThreads > 1 ) // Multi thread.
				{
					boost::thread_group threads;
					for( itmod itm = pitmod.first; itm != pitmod.second; ++itm )
					{
						if( itm->second->validIdate(HEnv::Instance()->idate()) )
							threads.create_thread(boost::bind(&HModule::beginTickerBase, itm->second, ticker));
					}
					threads.join_all();
				}
			}
		}
	}
/*
*	beginTickerBase() Single thread modules.
*/
	else
	{
		for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
		{
			if( it->second->validIdate(HEnv::Instance()->idate()) )
				it->second->beginTickerBase(ticker);
		}
	}

/*
*	endTicker() Single thread modules.
*/
	for( multimap<int, HModule*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
	{
		if( it->second->validIdate(HEnv::Instance()->idate()) )
			it->second->endTicker(ticker);
	}

	return;
}
