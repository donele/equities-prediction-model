#include "optionlibs/TickData.h"
#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
using namespace std;
using std::string;

void usage()
{
	cout << "merge_us_book.exe <iBook> <date_from> <date_to>\n";
	cout << "	1 arcabook\n";
	cout << "	2 batsbook\n";
	cout << "	3 bx\n";
	cout << "	4 byx\n";
	cout << "	5 edga\n";
	cout << "	6 edgx\n";
	cout << "	7 inetbook\n";
	cout << "	8 nysebook\n";
	cout << "	9 nyseultrabook\n";
	cout << "	10 psx\n";
	exit(1);
}

int main(int argc, char* argv[])
{
	vector<string> subdirs;
	subdirs.push_back("arcabook");
	subdirs.push_back("batsbook");
	subdirs.push_back("bx");
	subdirs.push_back("byx");
	subdirs.push_back("edga");
	subdirs.push_back("edgx");
	subdirs.push_back("inetbook");
	subdirs.push_back("nysebook");
	subdirs.push_back("nyseultrabook");
	subdirs.push_back("psx");
	int nBook = subdirs.size();

	if(argc < 4)
		usage();

	int iBook = atoi(argv[1]);
	int idate_from = atoi(argv[2]);
	int idate_to = atoi(argv[3]);
	if(iBook < 1 || iBook > nBook || idate_from < 20070101 || idate_from > 20160101 || idate_to < 20070101 || idate_to > 20160101 || idate_from > idate_to)
		usage();

	string subdir = subdirs[iBook - 1];
	string dir_from = "/mnt/gf0/book_us/" + subdir;
	string dir_to = "/mnt/gf0/book_us_merged/" + subdir;

	namespace fs = boost::filesystem;
	fs::path fsdir_to(dir_to);
	if( !fs::is_directory(fsdir_to) )
		fs::create_directories(fsdir_to);

	vector<string> dirs = FindAllSubdirs(dir_from);
	TickAccessMulti<OrderData> tao;
	TickAccessMulti<TradeInfo> tat;
	for( vector<string>::iterator it = dirs.begin(); it != dirs.end(); ++it )
	{
		tao.AddRoot(*it);
		tat.AddRoot(*it);
	}

	Exchange ex("US");
	int idate = (int)ex.NextOpen(QuoteTime(idate_from, 040000, "ET")).Date();
	for(; idate <= idate_to; idate = (int)ex.NextOpen(QuoteTime(idate, 200000, "ET")).Date())
	{
		// Orders.
		{
			TickStorage<OrderData> tso(2000, 2.);
			vector<string> names;
			tao.GetNames(idate, &names);
			if( !names.empty() )
			{
				TickSeries<OrderData> ts;
				for(vector<string>::iterator it = names.begin(); it != names.end(); ++it )
				{
					string ticker = *it;
					tao.GetTickSeries(ticker, idate, &ts);
					tso.Import(ticker, ts);
				}
			}
			tso.WriteBIN(idate, dir_to);
		}

		// Trades.
		{
			TickStorage<TradeInfo> tst(2000, 2.);
			vector<string> names;
			tat.GetNames(idate, &names);
			if( !names.empty() )
			{
				TickSeries<TradeInfo> ts;
				for(vector<string>::iterator it = names.begin(); it != names.end(); ++it )
				{
					string ticker = *it;
					tat.GetTickSeries(ticker, idate, &ts);
					tst.Import(ticker, ts);
				}
			}
			tst.WriteBIN(idate, dir_to);
		}

		cout << subdir << "\t" << idate << endl;
	}
}

