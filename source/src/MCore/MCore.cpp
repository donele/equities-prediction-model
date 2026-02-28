#include <MCore/MCore.h>
#include <MCore/ModuleFactory.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <fstream>
#include <vector>
#include <MFramework.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
using namespace std;

MCore::MCore(const string& confDir, const string& filename, bool print, const vector<string>& lines)
:print_(print)
{
	string path = filename;
	if( !boost::filesystem::exists(filename) && !confDir.empty() )
		path = confDir + '/' + filename;

	ifstream is(path.c_str());
	if( is.is_open() )
		conf_ = HConfig(is, lines, confDir);

#ifdef _WIN32
	_setmaxstdio(2048);
#endif
}

MCore::MCore(istream& is, bool print, const vector<string>& lines)
:print_(print)
{
	conf_ = HConfig(is, lines);
}

MCore::MCore(HConfig& conf)
{
	conf_ = conf;
}

void MCore::run()
{
	printf("Process ID: %u.\n", get_pid());
	vector<pair<ModuleInfo, multimap<string, string> > > block = conf_.get_block();
	init(block);
	if( !print_ )
		runImpl();
}

void MCore::init(vector<pair<ModuleInfo, multimap<string, string> > >& block)
{
	ModuleFactory* mFactory = new ModuleFactory();
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
		string className = block[i].first.className;
		string moduleName = block[i].first.moduleName;
		int temp_execOrder = block[i].first.execOrder;
		if( temp_execOrder >= 0 )
		{
			if( execOrder <= temp_execOrder )
				execOrder = temp_execOrder;
			else
			{
				cerr << "MCore:Error execOrder of module " << moduleName << endl;
				exit(5);
			}
		}
		else
			++execOrder;

		MModuleBase* mod = mFactory->getModule(className, moduleName, block[i].second);
		if( mod != nullptr )
			modules_.insert( make_pair(execOrder, mod) );
	}
}

//
//	Run the Modules.
//

void MCore::runImpl()
{
	// Create singletons in a single thread mode.
	MEnv::Instance();
	MEvent::Instance();

	for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
		it->second->beginJob();

	if( MEnv::Instance()->loopingOrder == "mdt" )
		loop_markets_dates();
	else if( MEnv::Instance()->loopingOrder == "dmt" )
		loop_dates_markets();

	for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
		it->second->endJob();
	for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
	{
		delete it->second;
	}

	return;
}

void MCore::loop_markets_dates()
{
	// Loop markets.
	vector<string> markets = MEnv::Instance()->markets;
	for( vector<string>::iterator itm = markets.begin(); itm != markets.end(); ++itm )
	{
		string market = *itm;
		MEnv::Instance()->market = market;

		for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			it->second->beginMarket();

		// Loop dates.
		vector<int> idates = MEnv::Instance()->idates;
		for( vector<int>::iterator itd = idates.begin(); itd != idates.end(); ++itd )
		{
			int idate = *itd;
			MEnv::Instance()->idate = idate;

			for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			{
				it->second->beginDay();
				it->second->beginMarketDay();
			}

			// Loop tickers.
			loop_tickers();

			for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			{
				it->second->endMarketDay();
			}

			loop_endDay(idate);
		}

		for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			it->second->endMarket();
	}
	return;
}

void MCore::loop_dates_markets()
{
	vector<string>& markets = MEnv::Instance()->markets;
	vector<int>& idates = MEnv::Instance()->idates;

	if( !markets.empty() && !idates.empty() )
	{
		// Loop dates.
		for( vector<int>::iterator itd = idates.begin(); itd != idates.end(); ++itd )
		{
			int idate = *itd;
			MEnv::Instance()->idate = idate;

			for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			{
				it->second->beginDay();
			}

			// Loop markets.
			for( vector<string>::iterator itm = markets.begin(); itm != markets.end(); ++itm )
			{
				string market = *itm;

				MEnv::Instance()->market = market;

				for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
				{
					it->second->beginMarket();
					it->second->beginMarketDay();
				}

				// Loop tickers.
				loop_tickers();

				for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
				{
					it->second->endMarketDay();
					it->second->endMarket();
				}
			}

			loop_endDay(idate);
		}
	}
	return;
}

void MCore::loop_tickers()
{
	//
	// Multi thread tickers.
	//
	if( MEnv::Instance()->multiThreadTicker )
	{
		int nMaxThreadTicker = MEnv::Instance()->nMaxThreadTicker;
		list<boost::shared_ptr<boost::thread> > listThread;
		for( int iThread = 0; iThread < nMaxThreadTicker; ++iThread )
		{
			listThread.push_back(
				boost::shared_ptr<boost::thread>(
					new boost::thread(boost::bind(&MCore::loop_tickers_thread, this, iThread))
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
		vector<string> tickers = MEnv::Instance()->tickersDebug;
		if( tickers.empty() )
			tickers = MEnv::Instance()->tickers;
		for( vector<string>::const_iterator itt = tickers.begin(); itt != tickers.end(); ++itt )
		{
			string ticker = *itt;
			loopModule(ticker);
		}
	}
	return;
}

void MCore::loop_tickers_thread(int iThread)
{
	vector<string> tickers = MEnv::Instance()->tickersDebug;
	if( tickers.empty() )
		tickers = MEnv::Instance()->tickers;
	int nTickers = tickers.size();
	int nMaxThreadTicker = MEnv::Instance()->nMaxThreadTicker;
	int nTickerEachThread = ceil((float)nTickers / nMaxThreadTicker);
	vector<string>::iterator itBeg = tickers.begin();

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

			for( vector<string>::iterator it = itFrom; it != itTo; ++it )
			{
				string ticker = *it;
				loopModule(ticker);
			}
		}
	}
}

void MCore::loopModule(const string& ticker)
{
	for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
	{
		it->second->beginTicker(ticker);
	}

	for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
	{
		it->second->endTicker(ticker);
	}

	return;
}

void MCore::loop_endDay(int idate)
{
/*
*	Multi thread modules.
*/
	if( MEnv::Instance()->multiThreadModule )
	{
		// Sort out the execution orders of the modules.
		set<int> sExecOrder;
		for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
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
					pitmod.first->second->endDay();
				else if( nThreads > 1 ) // Multi thread.
				{
					boost::thread_group threads;
					for( itmod itm = pitmod.first; itm != pitmod.second; ++itm )
						threads.create_thread(boost::bind(&MModuleBase::endDay, itm->second));
					threads.join_all();
				}
			}
		}
	}
/*
*	Single thread modules.
*/
	else
	{
		for( multimap<int, MModuleBase*>::iterator it = modules_.begin(); it != modules_.end(); ++it )
			it->second->endDay();
	}
}
