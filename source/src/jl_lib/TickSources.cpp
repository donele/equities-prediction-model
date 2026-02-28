#include <jl_lib/TickSources.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
using namespace std;

TickSources::TickSources()
{
}

TickSources::TickSources(string region)
{
	read(region);
}

void TickSources::read(string region, const string& flag)
{
	if( region.size() > 1 )
		region = mto::region(region);

	if( !mDir_.count(region) )
	{
		if( "E" == region || "C" == region )
		{
			char cmd[1000];
			sprintf(cmd, "select switchdate, market, stocksdirectory, bookdirectory, nbbodirectory "
				" from tickdatasources order by switchdate " );
			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(region), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				int switchDate = atoi( (*it)[0].c_str() );
				string market = region + trim( (*it)[1] );
				string dirmarket = market;

				Directories dirs;
				string stocksdir = xpf(trim((*it)[2]));
				string bookdir = xpf(trim((*it)[3]));
				string nbbodir = xpf(trim((*it)[4]));
				if( nbbodir.empty() )
					nbbodir = stocksdir;

				if( flag == "ca_test" )
				{
					if( nbbodir.find("/tickC/") != string::npos )
						nbbodir.replace(nbbodir.find("/tickC/"), 7, "/tickC_test/");
				}
				else if( flag == "CJ_CJonly" )
					nbbodir = stocksdir;
				else if( flag == "CJ_CConly" )
				{
					if( market == "CJ" )
						continue;
					else if( market == "CC" )
						dirmarket = "CJ";
				}
				else if( flag == "CJ_CHonly" )
				{
					if( market == "CJ" )
						continue;
					else if( market == "CH" )
						dirmarket = "CJ";
				}

				dirs.addstocksdir(stocksdir);
				dirs.addbookdir(bookdir);
				dirs.addnbbodir(nbbodir);
				mDir_[region][dirmarket][switchDate] = dirs;
			}
			if(flag == "ca_test")
			{
				for(string market : {"CD", "CE"})
				{
					Directories dirs;
					string stocksdir = "/mnt/gf1/tickC_test/ca/binLogL2/" + market.substr(1,1);
					dirs.addstocksdir(stocksdir);
					dirs.addbookdir(stocksdir);
					dirs.addnbbodir(stocksdir);
					mDir_[region][market][20180525] = dirs;
				}
			}
		}
		else if( "A" == region || "M" == region || "S" == region )
		{
			char cmd[1000];
			sprintf(cmd, "select switchdate, market, stocksdirectory, bookdirectory, nbbodirectory "
				" from tickdatasources order by switchdate " );
			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(region), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				int switchDate = atoi( (*it)[0].c_str() );
				string market = region + trim( (*it)[1] );
				string dirmarket = market;

				Directories dirs;
				string stocksdir = xpf(trim((*it)[2]));
				string bookdir = xpf(trim((*it)[3]));
				string nbbodir = xpf(trim((*it)[4]));
				if( nbbodir.empty() )
					nbbodir = stocksdir;

				if( flag == "AS_chixonly" )
				{
					if( market == "AS" ) // don't read the AS directories.
						continue;
					else if( market == "AX" ) // use AX data for AS.
						dirmarket = "AS";
				}

				dirs.addstocksdir(stocksdir);
				dirs.addbookdir(bookdir);
				dirs.addnbbodir(nbbodir);
				mDir_[region][dirmarket][switchDate] = dirs;
			}
		}
		else if( "U" == region )
		{
			char cmd[1000];
			sprintf(cmd, "select switchdate, tobbook, nbbobook, stocksdirectory, nbbondq "
				" from tickdatasources order by switchdate " );
			vector<vector<string> > vv;
			GODBC::Instance()->read("equitydata", cmd, vv);

			vector<string> markets;
			markets.push_back("P"); // arca
			markets.push_back("Q"); // nasdaq
			markets.push_back("N"); // nyse
			markets.push_back("C"); // bats-z
			markets.push_back("D"); // edgx
			markets.push_back("J"); // edga
			markets.push_back("B"); // bx
			//markets.push_back("X");
			markets.push_back("Y"); // byx, or bats-y
			for(int i = 0; i < vv.size(); ++i)
			{
				 int switchDate = atoi(vv[i][0].c_str());
				 if(switchDate == 20150511)
					 break;
				 else if(switchDate > 20150511)
				 {
					 vector<string> v = vv[i - 1];
					 v[0] = "20150511";
					 vv.insert(vv.begin() + i, v);
					 break;
				 }
				 else if(i == vv.size() - 1)
				 {
					 vector<string> v = vv[i];
					 v[0] = "20150511";
					 vv.push_back(v);
				 }
			}
			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				int switchDate = atoi( (*it)[0].c_str() );
				string bookbase = "/mnt/gf0/book_us/";
				if(switchDate < 20150511)
					bookbase = "/mnt/gf3/book_us/";
				for( vector<string>::iterator itm = markets.begin(); itm != markets.end(); ++itm )
				{
					string market = region + *itm;
					string subdir = "";
					if( "UP" == market )
						subdir = "arcabook";
					else if( "UQ" == market )
						subdir = "inetbook";
					else if( "UN" == market )
						subdir = "nyseultrabook";
					else if( "UC" == market )
						subdir = "batsbook";
					else if( "UD" == market )
						subdir = "edgx";
					else if( "UJ" == market )
						subdir = "edga";
					else if( "UB" == market )
						subdir = "bx";
					else if( "UY" == market )
						subdir = "byx";

					if( !subdir.empty() )
					{
						string stockdir = xpf(trim((*it)[1]) + "\\" + mto::code(market));
						string bookdir = xpf(bookbase + subdir);
						string nbbodir = xpf(trim((*it)[2]));
						string cqsdir = xpf(trim((*it)[3]));
						string uqdfdir = xpf(trim((*it)[4]));

						Directories dirs;
						dirs.addstocksdir(stockdir);
						dirs.bookdirectory = FindAllSubdirs(bookdir);
						dirs.addnbbodir(nbbodir + xpf("\\1"));
						dirs.addnbbodir(nbbodir + xpf("\\2"));
						dirs.addsipdir(cqsdir);
						dirs.addsipdir(uqdfdir);
						mDir_[region][market][switchDate] = dirs;
					}
				}
			}
		}
	}
}

vector<string> TickSources::getDirs(const string& type, const string& market, int idate)
{
	if( type == "order" || type == "booktrade" )
		return bookdirectory(market, idate);
	else if( type == "nbbo" || type == "trade" || (type == "return" && market.substr(0, 1) == "U") )
		return nbbodirectory(market, idate);
	else if( type == "quote" || (type == "return" && market.substr(0, 1) != "U") )
		return stocksdirectory(market, idate);
}

vector<string> TickSources::stocksdirectory(const string& market, int idate)
{
	vector<string> ret;
	string region = mto::region(market);
	if( mDir_.count(region) && mDir_[region].count(market) )
	{
		for( map<int, Directories>::iterator it = mDir_[region][market].begin(); it != mDir_[region][market].end(); ++it )
		{
			int swdate = it->first;
			if( swdate <= idate )
				ret = it->second.stocksdirectory;
		}
	}

	return ret;
}

vector<string> TickSources::bookdirectory(const string& market, int idate)
{
	vector<string> ret;
	string region = mto::region(market);
	if( mDir_.count(region) && mDir_[region].count(market) )
	{
		for( map<int, Directories>::iterator it = mDir_[region][market].begin(); it != mDir_[region][market].end(); ++it )
		{
			int swdate = it->first;
			if( swdate <= idate )
			{
				ret = it->second.bookdirectory;
				if( market[0] == 'U' && idate > 20150508 )
				{
					vector<string> temp_ret = ret;
					for( vector<string>::iterator itd = temp_ret.begin(); itd != temp_ret.end(); )
					{
						if( itd->find("lfs") == string::npos )
							temp_ret.erase(itd);
						else
							++itd;
					}
					ret = temp_ret;
				}
				if( "UN" == market )
				{
					if( idate >= 20080602 )
					{
						for( vector<string>::iterator it = ret.begin(); it != ret.end(); ++it )
						{
							string& dir = *it;
							int pos = dir.find("nysebook");
							if( pos != string::npos )
								dir.replace(pos, 8, "nyseultrabook");
						}
					}
					else
					{
						for( vector<string>::iterator it = ret.begin(); it != ret.end(); ++it )
						{
							string& dir = *it;
							int pos = dir.find("nyseultrabook");
							if( pos != string::npos )
								dir.replace(pos, 13, "nysebook");
						}
					}
				}
			}
		}
	}

	return ret;
}

vector<string> TickSources::nbbodirectory(string market, int idate)
{
	vector<string> ret;
	if( market == "US" )
		market = "UP";
	string region = mto::region(market);
	if( mDir_.count(region) && mDir_[region].count(market) )
	{
		for( map<int, Directories>::iterator it = mDir_[region][market].begin(); it != mDir_[region][market].end(); ++it )
		{
			int swdate = it->first;
			if( swdate <= idate )
				ret = it->second.nbbodirectory;
		}
	}

	return ret;
}

vector<string> TickSources::sipdirectory(string market, int idate)
{
	vector<string> ret;
	if( market == "US" )
		market = "UP";
	string region = mto::region(market);
	if( mDir_.count(region) && mDir_[region].count(market) )
	{
		for( map<int, Directories>::iterator it = mDir_[region][market].begin(); it != mDir_[region][market].end(); ++it )
		{
			int swdate = it->first;
			if( swdate <= idate )
				ret = it->second.sipdirectory;
		}
	}

	return ret;
}

void TickSources::Directories::addstocksdir(const string& dir)
{
	stocksdirectory.push_back(dir);
}

void TickSources::Directories::addbookdir(const string& dir)
{
	bookdirectory.push_back(dir);
}

void TickSources::Directories::addnbbodir(const string& dir)
{
	nbbodirectory.push_back(dir);
}

void TickSources::Directories::addsipdir(const string& dir)
{
	sipdirectory.push_back(dir);
}

