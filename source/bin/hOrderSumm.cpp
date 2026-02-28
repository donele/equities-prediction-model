#include <HCore/HCore.h>
#include <jl_lib.h>
#include <string>
using namespace std;

void exit_usage();
void exit_usage()
{
	cout << "hOrderSumm.exe -d yyyymmdd -r [eu|asia|ca|ma] -e DEST\n";
	cout << "hOrderSumm.exe -d1 yyyymmdd -d2 yyyymmdd -m MARKET -e DEST\n";
	exit(0);
	return;
}

int main(int argc, char* argv[])
{
	if( argc < 2 )
		exit_usage();

	Arg arg(argc, argv);
	HConfig conf;

	string basedir = tickMon_dir_ + "\\ordersumm";

	// HInit.
	{
		ModuleInfo mInfo("HInit", "HInit", 0);

		multimap<string, string> options;

		if( arg.isGiven("r") )
		{
			string region = arg.getVal("r");
			if( "eu" == region )
			{
				options.insert( make_pair("market", "EA") );
				options.insert( make_pair("market", "EB") );
				options.insert( make_pair("market", "EI") );
				options.insert( make_pair("market", "EP") );
				options.insert( make_pair("market", "EF") );
				options.insert( make_pair("market", "EL") );
				options.insert( make_pair("market", "ED") );
				options.insert( make_pair("market", "EM") );
				options.insert( make_pair("market", "EZ") );
				options.insert( make_pair("market", "EO") );
				options.insert( make_pair("market", "EX") );
				options.insert( make_pair("market", "EC") );
				options.insert( make_pair("market", "EW") );
				options.insert( make_pair("market", "EY") );
			}
			else if( "asia" == region )
			{
				options.insert( make_pair("market", "AT") );
				options.insert( make_pair("market", "AH") );
				options.insert( make_pair("market", "AS") );
			}
			else if( "ca" == region )
			{
				options.insert( make_pair("market", "CJ") );
			}
			else if( "ma" == region )
			{
				options.insert( make_pair("market", "MJ") );
			}
			else if( "sa" == region )
			{
				options.insert( make_pair("market", "SS") );
			}
		}
		else if( arg.isGiven("m") )
		{
			vector<string> markets = arg.getVals("m");
			for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
			{
				string market = *it;
				options.insert( make_pair("market", market) );
			}
		}

		if( arg.isGiven("d") )
		{
			string d = arg.getVal("d");
			options.insert( make_pair("dateFrom", d) );
			options.insert( make_pair("dateTo", d) );
		}
		else
		{
			if( arg.isGiven("d1") )
			{
				string d1 = arg.getVal("d1");
				options.insert( make_pair("dateFrom", d1) );
			}

			if( arg.isGiven("d2") )
			{
				string d2 = arg.getVal("d2");
				options.insert( make_pair("dateTo", d2) );
			}
		}

		options.insert( make_pair( "requireDataOK", "false" ) );
		options.insert( make_pair( "multiThreadModule", "true" ) );
		options.insert( make_pair( "multiThreadTicker", "true" ) );
		options.insert( make_pair( "nMaxThreadTicker", "8" ) );

		conf.add_module( make_pair( mInfo, options ) );
	}

	vector<string> dests;
	if( arg.isGiven("e") )
		dests = arg.getVals("e");

	// HOrderSummRead.
	for( vector<string>::iterator it = dests.begin(); it != dests.end(); ++it )
	{
		string dest = *it;

		ModuleInfo mInfo("HOrderSummRead", (string)"HOrderSummRead_" + dest, 1);

		multimap<string, string> options;

		options.insert( make_pair( "dest", dest ) );
		options.insert( make_pair( "tickSource", "order" ) );

		conf.add_module( make_pair( mInfo, options) );
	}

	// HOrderSumm.
	{
		vector<string> execTypes;
		execTypes.push_back("L");
		execTypes.push_back("A");

		vector<string> orderSchedTypes;
		orderSchedTypes.push_back("1");
		orderSchedTypes.push_back("2");

		for( vector<string>::iterator itd = dests.begin(); itd != dests.end(); ++itd )
		{
			string dest = *itd;

			for( vector<string>::iterator ite = execTypes.begin(); ite != execTypes.end(); ++ite )
			{
				string execType = *ite;

				for( vector<string>::iterator ito = orderSchedTypes.begin(); ito != orderSchedTypes.end(); ++ito )
				{
					string orderSchedType = *ito;

					{
						ModuleInfo mInfo("HOrderSumm", (string)"os_" + execType + "_" + orderSchedType, 2);

						multimap<string, string> options;

						options.insert( make_pair("dest", dest) );
						options.insert( make_pair("execType", execType) );
						options.insert( make_pair("orderSchedType", orderSchedType) );
						options.insert( make_pair("basedir", basedir) );
						if( arg.isGiven("v") )
							options.insert( make_pair( "verbose", arg.getVal("v") ) );

						conf.add_module( make_pair( mInfo, options) );
					}
				}
			}
		}
	}

	HCore hf(conf);
	hf.run();
}