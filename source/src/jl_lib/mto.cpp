#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GFee.h>
#include <optionlibs/TickData.h>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

namespace mto {
string ex(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		string reg = region(m);
		if( "U" == reg )
			ret = "US";
		else if( "C" == reg )
			ret = "CA";

		else if( "EA" == m || "EB" == m || "EI" == m || "EP" == m )
			ret = "FR";
		else if( "EF" == m )
			ret = "DE";
		else if( "EL" == m )
			ret = "GB";

		else if( "ED" == m ) // Madrid
			ret = "ES";
		else if( "EM" == m ) // Milan
			ret = "IT";
		else if( "EZ" == m ) // Zurich
			ret = "CH";
		else if( "EO" == m ) // Oslo
			ret = "NO";
		else if( "EX" == m ) // Virt-X
			ret = "CH";
		else if( "EC" == m ) // Copenhagen
			ret = "DK";
		else if( "EW" == m ) // Stockholm
			ret = "SE";
		else if( "EY" == m ) // Helsinki
			ret = "FI";

		else if( "E" == reg )
			ret = "FR";

		else if( "EU" == m ) // Europe
			ret = "FR";
		else if( "EH" == m ) // Chi-X
			ret = "FR";
		else if( "ET" == m ) // Turquoise
			ret = "FR";
		else if( "EG" == m ) // BATS
			ret = "FR";

		else if( "AT" == m || "AC" == m || "AN" ==m ) // Japan
			ret = "JP";
		else if( "AH" == m )
			ret = "HK";
		else if( "AS" == m || "AX" == m ) // Australia
			ret = "AU";
		else if( "AG" == m )
			ret = "SG";
		else if( "AK" == m || "AQ" == m )
			ret = "KR";
		else if( "AW" == m )
			ret = "TW";
		else if( "AA" == m )
			ret = "CN";

		else if( "MJ" == m )
			ret = "ZA";
		else if( "SS" == m ) 
			ret = "BR";
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string country(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		string reg = region(m);
		if( "U" == reg )
			ret = "US";
		else if( "C" == reg )
			ret = "CA";

		else if( "EA" == m )
			ret = "NL";
		else if( "EB" == m )
			ret = "BE";
		else if( "EI" == m )
			ret = "PT";
		else if( "EP" == m )
			ret = "FR";
		else if( "EF" == m )
			ret = "DE";
		else if( "EL" == m )
			ret = "GB";

		else if( "ED" == m ) // Madrid
			ret = "ES";
		else if( "EM" == m ) // Milan
			ret = "IT";
		else if( "EZ" == m ) // Zurich
			ret = "CH";
		else if( "EO" == m ) // Oslo
			ret = "NO";
		else if( "EX" == m ) // Virt-X
			ret = "CH";
		else if( "EC" == m ) // Copenhagen
			ret = "DK";
		else if( "EW" == m ) // Stockholm
			ret = "SE";
		else if( "EY" == m ) // Helsinki
			ret = "FI";

		else if( "AT" == m )
			ret = "JP";
		else if( "AH" == m )
			ret = "HK";
		else if( "AS" == m )
			ret = "AU";
		else if( "AG" == m )
			ret = "SG";
		else if( "AK" == m || "AQ" == m )
			ret = "KR";
		else if( "AW" == m )
			ret = "TW";
		else if( "AA" == m )
			ret = "CN";

		else if( "MJ" == m )
			ret = "ZA";

		else if( "SS" == m )
			ret = "BR";

	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string exRIC(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		string reg = region(m);
		if( "U" == reg )
			ret = "";
		else if( "C" == reg )
			ret = "";
		else if( "EA" == m )
			ret = "AEX";
		else if( "EB" == m )
			ret = "BRU";
		else if( "EI" == m )
			ret = "LIS";
		else if( "EP" == m )
			ret = "PAR";
		else if( "EF" == m )
			ret = "GER";
		else if( "EL" == m )
			ret = "LSE";

		else if( "ED" == m ) // Madrid
			ret = "MCE";
		else if( "EM" == m ) // Milan
			ret = "MIL";
		else if( "EZ" == m ) // Zurich
			ret = "SWX";
		else if( "EO" == m ) // Oslo
			ret = "OSL";
		else if( "EX" == m ) // Virt-X
			ret = "VTX";
		else if( "EC" == m ) // Copenhagen
			ret = "CPH";
		else if( "EW" == m ) // Stockholm
			ret = "STO";
		else if( "EY" == m ) // Helsinki
			ret = "HEX";

		else if( "AT" == m )
			ret = "TYO";
		else if( "AH" == m )
			ret = "HKG";
		else if( "AS" == m )
			ret = "ASX";
		else if( "AG" == m )
			ret = "SES";
		else if( "AK" == m )
			ret = "KSC";
		else if( "AQ" == m )
			ret = "KOE";
		else if( "AW" == m )
			ret = "TAI";
		else if( "MJ" == m )
			ret = "JNB";
		else if( "SS" == m ) // Sao Paulo
			ret = ""; 
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string MIC(const string& m)
{
	string ret;

	if( m.size() == 2 )
	{
		string reg = region(m);
		if( "C" == reg )
			ret = "";

		else if( "EA" == m )
			ret = "AEX";
		else if( "EB" == m )
			ret = "BRU";
		else if( "EI" == m )
			ret = "LIS";
		else if( "EP" == m )
			ret = "PAR";
		else if( "EF" == m )
			ret = "GER";
		else if( "EL" == m )
			ret = "LSE";

		else if( "ED" == m ) // Madrid
			ret = "MCE";
		else if( "EM" == m ) // Milan
			ret = "MIL";
		else if( "EZ" == m ) // Zurich
			ret = "SWX";
		else if( "EO" == m ) // Oslo
			ret = "OSL";
		else if( "EX" == m ) // Virt-X
			ret = "VTX";
		else if( "EC" == m ) // Copenhagen
			ret = "CPH";
		else if( "EW" == m ) // Stockholm
			ret = "STO";
		else if( "EY" == m ) // Helsinki
			ret = "HEX";

		else if( "AT" == m )
			ret = "TYO";
		else if( "AH" == m )
			ret = "HKG";
		else if( "AS" == m )
			ret = "ASX";
		else if( "AG" == m )
			ret = "SES";
		else if( "AK" == m )
			ret = "KSC";
		else if( "AQ" == m )
			ret = "KOE";
		else if( "AW" == m )
			ret = "TAI";
		else if( "MJ" == m )
			ret = "JNB";
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string tz(const string& m, int idate)
{
	string ret = "";

	if( m.size() == 2 )
	{
		string reg = region(m);
		if( "C" == reg || "U" == reg )
			ret = "ET";

		else if( "E" == reg )
			ret = "GMT";

		else if( "MJ" == m )
			ret = "SAST";

		else if( "AT" == m || "AC" == m || "AN" == m )
			ret = "JPT";
		else if( "AH" == m || "AA" == m )
			ret = "HKT";
		else if( "AS" == m || "AX" == m )
			ret = "AET";
		else if( "AG" == m )
			ret = "HKT";
		else if( "AK" == m || "AQ" == m )
			ret = "JPT";
		else if( "AW" == m )
			ret = "HKT";
		else if( "SS" == m )
		{
			if( idate < 20120701 )
				ret = "BRXT";
			else
				ret = "BRT";
		}
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string region(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		string reg = m.substr(0,1);
		if( "A" == reg || "E" == reg || "C" == reg || "U" == reg || "M" == reg || "S" == reg )
			ret = reg;
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

bool isInternational(const string& m)
{
	string reg = region(m);
	if( "E" == reg || "A" == reg || "M" == reg || "S" == reg)
		return true;
	return false;
}

string region_long(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		string region = m.substr(0,1);
		if( "A" == region )
			ret = "asia";
		else if( "E" == region )
			ret = "eu";
		else if( "M" == region )
			ret = "ma";
		else if( "C" == region )
			ret = "ca";
		else if( "U" == region )
			ret = "us";
		else if( "S" == region)
			ret = "sa";
		else
			ret = region;
	}
	if( ret.empty() )
		exit(2);

	return ret;
}

string code(const string& m)
{
	string ret = "";
	if( m.size() == 2 )
	{
		if( "US" == m )
			ret = "";
		else if( "EU" == m )
			ret = "";
		else
			ret = m.substr(1,1);
	}
	return ret;
}

int exFlag(const string& m)
{
	int ret = -1;
	string reg = region(m);

	if( reg == "U" )
	{
		if( m == "UP" )
			ret = 3;
		else if( m == "UQ" )
			ret = 2;
		else if( m == "UN" )
			ret = 1;
		else if( m == "UC" )
			ret = 194;
		else if( m == "UD" )
			ret = 196;
		else if( m == "UJ" )
			ret = 195;
	}
	else
		ret = m[1];
	return ret;
}

string uidHead(const string& m)
{
	string ret = "";
	if( "CJ" == m )
		ret = "CA";
	else
	{
		string reg = region(m);
		if( "A" == reg || "E" == reg || "M" == reg || "C" == reg || "S" == reg )
		{
			if( m.size() == 1 )
				ret = reg + m;
			else if( m.size() == 2 )
				ret = m;
		}
	}

	if( ret.size() != 2 )
		exit(2);

	return ret;
}

bool longTicker(const string& m)
{
	bool ret = false;
	if( region(m) == "E" )
		ret = true;
	return ret;
}

string hf(const string& m)
{
	string ret = "";
	string reg = m;
	if( m.size() > 1 )
		reg = region(m);

	if( "C" == reg )
		ret = "hfStockTsx";
	else if( "U" == reg )
		ret = "hfStock";
	else if( reg == "E" )
		ret = "hfstockeu";
	else if( reg == "M" )
		ret = "hfstockma";
	else if( reg == "A" )
		ret = "hfStockAsia";
	else if( reg == "S" )
		ret = "hfstocksa";
	if( ret.empty() )
		exit(2);

	return ret;
}

string hfpar(const string& m, int idate)
{
	if( (region(m) == "A" || m == "KR") && idate < 20130301 )
		return "hfstockasia_chi";
	else if( region(m) == "C" && idate < 20110101 )
	{
		if( idate < 20081201 )
			return "hfstocktsx_20081130";
		else if( idate < 20110101 )
			return "hfstocktsx_hist";
	}
	else if( region(m) == "E" && idate < 20110100 )
		return "hfstockeu_hist";
	else if( region(m) == "U" && idate < 20130701 )
		return "hfstock_old";
	else
		return hf(m);
	return "";
}

string hfdbg(const string& m)
{
	string ret = "";
	string reg = m;
	if( m.size() > 1 )
		reg = region(m);

	if( "C" == reg )
		ret = "hfStockTsx_debug";
	else if( "U" == reg )
		ret = "hfstock_debug";
	else if( reg == "E" )
		ret = "hfstockeu_debug";
	else if( reg == "M" )
		ret = "hfstockma_debug";
	else if( reg == "A" )
		ret = "hfStockAsia_debug";
	else if( reg == "S" )
		ret = "hfstocksa_debug";
	if( ret.empty() )
		exit(2);

	return ret;
}

string hfo(const string& m, int idate)
{
	string ret = "";

	if( "U" == region(m) )
	{
		if( idate >= 20051110 && idate < 20070702 )
			ret = "hfStock_US_20070825";
		else if( idate >= 20070702 && idate <= 20080212 )
			ret = "hfStock_US_20080201";
		else if (idate >= 20080213 && idate <= 20081219 )
			ret = "hfstock_us_20081220";
		else if (idate >= 20081220 && idate <= 20090401 )
			ret = "hfstock_us_20090410";
		else if( idate > 20090401 && idate < 20130701 )
			ret = "hfstock_old";
		else if( idate >= 20130701 )
			ret = "hfStock";
	}
	else
		ret = hf(m);

	if( ret.empty() )
		exit(2);

	return ret;
}

string ok(const string& m)
{
	string ret = "";
	if( "U" == region(m) )
		ret = "tickDataElvOK";
	else
		ret = "tickDataOK";
	return ret;
}

string th(const string& m)
{
	string ret = "";
	string reg = region(m);

	if( "C" == reg )
		ret = "tickerHistoryTsx";
	else if( "U" == reg )
		ret = "tickerHistory";
	else if( reg == "E" )
		ret = "tickerHistoryEU";
	else if( reg == "M" )
		ret = "tickerHistoryma";
	else if( reg == "A" )
		ret = "tickerHistoryAsia";
	else if( reg == "S" )
		ret = "tickerHistorySA";
	if( ret.empty() )
		exit(2);

	return ret;
}

string bindir(const string& m, int idate)
{
	string bindir = "";
	string reg = region(m);
	string table = "tickDataSources";

	if( "U" == reg )
	{
		exit(2);

		string db = "equityData";
		string field = "";
		if( idate < 20071001 )
		{
			if( "UQ" == m )
				field = "nbboNDQ";
			else
				field = "stocksDirectory";
		}
		else
			field = "nbboBook";
	}
	else if( "CL" == m )
	{
		bindir = bindirFuture("CJ", idate);
	}
	else
	{
		string db = hf(m);
		string field = "stocksDirectory";

		vector<vector<string> > vvd;
		string cmd = (string)"select " + field
			+ " from " + table
			+ " where switchDate <= " + itos(idate)
			+ andMarketSel(m)
			+ " order by switchDate desc ";

		GODBC::Instance()->read( db, cmd, vvd );

		if( !vvd.empty() )
			bindir = xpf(trim(vvd[0][0]));
	}

	return bindir;
}

string bindirBook(const string& m, int idate)
{
	string bindir = "";
	if( "U" != region(m) )
	{
		vector<vector<string> > vvd;
		string cmd = (string)"select bookDirectory "
			+ " from tickDataSources "
			+ " where switchDate <= " + itos(idate)
			+ andMarketSel(m)
			+ " order by switchDate desc ";
		GODBC::Instance()->read(hf(m), cmd, vvd);

		if( !vvd.empty() )
			bindir = xpf(trim(vvd[0][0]));
	}
	else
	{
	}

	return bindir;
}

string bindirReturn(const string& m, int idate)
{
	string bindir = "";

	string db = hf(m);
	string reg = region(m);
	if( "U" == reg )
		db = "equityData";

	string table = "tickDataSources";
	string field = "stocksDirectory";
	string retMarket = m;
	if( "C" == reg )
		retMarket = "CJ";

	vector<vector<string> > vvd;
	string cmd = (string)"select " + field
		+ " from " + table
		+ " where switchDate <= " + itos(idate)
		+ andMarketSel(m)
		+ " order by switchDate desc ";

	GODBC::Instance()->read(db, cmd, vvd);

	if( !vvd.empty() )
		bindir = xpf(trim(vvd[0][0]));

	if( bindir.empty() )
		exit(2);

	return bindir;
}

string bindirFuture(const string& m, int idate)
{
	string bindir = "";
	if( "U" != region(m) )
	{
		vector<vector<string> > vvd;
		string cmd = (string)" select futuresdirectory "
			+ " from tickdatasources "
			+ " where switchdate <= " + itos(idate)
			+ andMarketSel(m)
			+ " order by switchdate desc ";

		GODBC::Instance()->read(mto::hf(m), cmd, vvd);

		if( !vvd.empty() )
			bindir = xpf(trim(vvd[0][0]));
	}
	return bindir;
}

vector<string> dests(const string& m)
{
	vector<string> ret;
	if( m == "US" )
		ret = usDests();
	else if( m != "EU" && m[0] == 'E' )
	{
		ret = {m};
		vector<string> ecns = euEcns();
		ret.insert(end(ret), begin(ecns), end(ecns));
	}
	else if( m[0] == 'C' )
	{
		ret = {"CJ"};
		vector<string> ecns = caEcns();
		ret.insert(end(ret), begin(ecns), end(ecns));
	}
	else
		ret = {m};
	return ret;
}

string nbbodir(const string& m, int idate)
{
	string bindir = "";
	string reg = region(m);
	string colName = "";
	if( "E" == reg )
		colName = "nbbodirectory";
	else if( "A" == reg || "M" == reg )
		colName = "stocksdirectory";

	vector<vector<string> > vvd;
	string cmd = (string)" select " + colName
		+ " from tickdatasources "
		+ " where switchdate <= " + itos(idate)
		+ andMarketSel(m)
		+ " order by switchdate desc ";

	GODBC::Instance()->read(mto::hf(m), cmd, vvd);

	if( !vvd.empty() )
		bindir = xpf(trim(vvd[0][0]));
	return bindir;
}

vector<string> nbbodirs(const string& m, int idate)
{
	vector<string> bindir;

	string reg = region(m);
	string colName = "";
	if( "E" == reg || "C" == reg )
		colName = "nbbodirectory";
	else if( "A" == reg || "M" == reg || "S" == reg )
		colName = "stocksdirectory";

	if( "U" != reg )
	{
		vector<vector<string> > vvd;
		string cmd = (string)" select " + colName
			+ " from tickdatasources "
			+ " where switchdate <= " + itos(idate)
			+ andMarketSel(m)
			+ " order by switchdate desc ";

		GODBC::Instance()->read(mto::hf(m), cmd, vvd);

		if( !vvd.empty() )
			bindir.push_back(xpf(trim(vvd[0][0])));
	}
	else // nbbo
	{
		string dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\nbbo\\");
		bindir.push_back( xpf(dir + "1") );
		bindir.push_back( xpf(dir + "2") );
	}
	return bindir;
}

vector<string> bindirs(const string& m, int idate)
{
	vector<string> bindir;
	string reg = region(m);

	if( "CL" == m )
	{
		bindir.push_back(xpf(bindirFuture("CJ", idate)));
	}
	else if( "US" == m ) // Return nbbo directory.
	{
		if( idate >= 20071001 )
		{
			string dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\nbbo");
			bindir.push_back(xpf(dir + xpf("\\1")));
			bindir.push_back(xpf(dir + xpf("\\2")));
		}
		else
		{
			string db = "equityData";
			string field = "stocksDirectory, nbboNDQ";
			string table = "tickDataSources";
			vector<vector<string> > vvd;
			string cmd = (string)"select " + field
				+ " from " + table
				+ " where switchDate <= " + itos(idate)
				+ andMarketSel(m)
				+ " order by switchDate desc ";
			GODBC::Instance()->read(db, cmd, vvd);
			if( !vvd.empty() )
			{
				bindir.push_back(xpf(trim(vvd[0][0])));
				bindir.push_back(xpf(trim(vvd[0][1])));
			}
		}
	}
	else if( "U" == reg ) // tob
	{
		string dir = xpf("\\\\smrc-nas09\\gf1\\tickC\\us\\book\\tob\\") + code(m);
		bindir.push_back(dir);
	}
	else
	{
		string db = hf(m);
		string table = "tickDataSources";
		string field = "stocksDirectory";

		vector<vector<string> > vvd;
		string cmd = (string)"select " + field
			+ " from " + table
			+ " where switchDate <= " + itos(idate)
			+ andMarketSel(m)
			+ " order by switchDate desc ";

		GODBC::Instance()->read( db, cmd, vvd );

		if( !vvd.empty() )
			bindir.push_back(xpf(trim(vvd[0][0])));
	}

	return bindir;
}

vector<string> bindirsBook(const string& m, int idate)
{
	vector<string> bindir;
	if( "U" == region(m) )
	{
		string subdir = "";
		if( "UP" == m )
			subdir = "arcabook";
		else if( "UQ" == m )
			subdir = "inetbook";
		else if( "UN" == m )
			if( idate < 20080602 )
				subdir = "nysebook";
			else
				subdir = "nyseultrabook";
		else if( "UC" == m )
			subdir = "batsbook";
		else if( "UD" == m )
			subdir = "edgx";
		else if( "UX" == m )
			subdir = "psx";
		else if( "UB" == m )
			subdir = "bx";
		else if( "UY" == m )
			subdir = "byx";
		else if( "UJ" == m )
			subdir = "edga";
		string base = xpf("\\\\smrc-nas09\\gf0\\book_us\\") + subdir;
		vector<string> dirs = FindAllSubdirs(base);
		for( vector<string>::iterator it = dirs.begin(); it != dirs.end(); ++it )
			bindir.push_back(xpf(*it));
	}
	else
	{
		vector<vector<string> > vvd;
		string cmd = (string)"select bookDirectory "
			+ " from tickDataSources "
			+ " where switchDate <= " + itos(idate)
			+ andMarketSel(m)
			+ " order by switchDate desc ";

		GODBC::Instance()->read( hf(m), cmd, vvd );

		if( !vvd.empty() )
			bindir.push_back(xpf(trim(vvd[0][0])));
	}

	return bindir;
}

string extel(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		if( "CJ" == m )
			ret = "NCT";
		else if( "CL" == m )
			ret = "";
		else if( "CC" == m )
			ret = "";
		else if( "CH" == m )
			ret = "";
		else if( "CP" == m )
			ret = "";
		else if( "CO" == m )
			ret = "";

		else if( "EA" == m )
			ret = "ENA";
		else if( "EB" == m )
			ret = "EBB";
		else if( "EI" == m )
			ret = "EPL";
		else if( "EP" == m )
			ret = "EFC";
		else if( "EF" == m )
			ret = "EDF";
		else if( "EL" == m )
			ret = "EXL";

		else if( "ED" == m ) // Madrid
			ret = "EEC";
		else if( "EM" == m ) // Milan
			ret = "EIC";
		else if( "EZ" == m ) // Zurich
			ret = "ESE";
		else if( "EO" == m ) // Oslo
			ret = "SNO";
		else if( "EX" == m ) // Virt-X
			ret = "ESV";
		else if( "EC" == m ) // Copenhagen
			ret = "SDC";
		else if( "EW" == m ) // Stockholm
			ret = "SSS";
		else if( "EY" == m ) // Helsinki
			ret = "SFH";

		else if( "AT" == m || "AC" == m || "AN" == m )
			ret = "FJT";
		else if( "AH" == m )
			ret = "FHH";
		else if( "AS" == m || "AX" == m )
			ret = "AAS";
		else if( "AK" == m || "AQ" == m )
			ret = "FKS";
		else if( "AG" == m )
			ret = "FMS";
		else if( "AW" == m )
			ret = "FAT";

		else if( "MJ" == m )
			ret = "KSJ";

		else if( "SS" == m )
			ret = "LBS";
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string extelCountry(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		if( "CJ" == m )
			ret = "NC";
		else if( "CL" == m )
			ret = "NC";
		else if( "CC" == m )
			ret = "NC";
		else if( "CH" == m )
			ret = "NC";
		else if( "CP" == m )
			ret = "NC";
		else if( "CO" == m )
			ret = "NC";

		else if( "EA" == m )
			ret = "EN";
		else if( "EB" == m )
			ret = "EB";
		else if( "EI" == m )
			ret = "EP";
		else if( "EP" == m )
			ret = "EF";
		else if( "EF" == m )
			ret = "ED";
		else if( "EL" == m )
			ret = "EX";

		else if( "ED" == m ) // Madrid
			ret = "EE";
		else if( "EM" == m ) // Milan
			ret = "EI";
		else if( "EZ" == m ) // Zurich
			ret = "ES";
		else if( "EO" == m ) // Oslo
			ret = "SN";
		else if( "EX" == m ) // Virt-X
			ret = "ES";
		else if( "EC" == m ) // Copenhagen
			ret = "SD";
		else if( "EW" == m ) // Stockholm
			ret = "SS";
		else if( "EY" == m ) // Helsinki
			ret = "SF";

		else if( "AT" == m || "AC" == m || "AN" == m )
			ret = "FJ";
		else if( "AH" == m )
			ret = "FH";
		else if( "AS" == m || "AX" == m )
			ret = "AA";
		else if( "AK" == m || "AQ" == m )
			ret = "FK";
		else if( "AG" == m )
			ret = "FM";
		else if( "AW" == m )
			ret = "FA";

		else if( "MJ" == m ) // Johannesburg
			ret = "KS";

		else if( "SS" == m ) // Brazil
			ret = "LB";
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string city(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		if( "US" == m )
			ret = "USA";
		else if( "CJ" == m )
			ret = "Toronto";
		else if( "CL" == m )
			ret = "Montreal";
		else if( "CC" == m )
			ret = "Chi-X";
		else if( "CH" == m )
			ret = "Alpha";
		else if( "CP" == m )
			ret = "Pure";
		else if( "CO" == m )
			ret = "Omega";

		else if( "EA" == m )
			ret = "Amsterdam";
		else if( "EB" == m )
			ret = "Brussels";
		else if( "EI" == m )
			ret = "Lisbon";
		else if( "EP" == m )
			ret = "Paris";
		else if( "EF" == m )
			ret = "Frankfurt";
		else if( "EL" == m )
			ret = "London";

		else if( "ED" == m ) // Madrid
			ret = "SIBE";
		else if( "EM" == m ) // Milan
			ret = "Milan";
		else if( "EZ" == m ) // Zurich
			ret = "Zurich";
		else if( "EO" == m ) // Oslo
			ret = "Oslo";
		else if( "EX" == m ) // Virt-X
			ret = "Virt-X";
		else if( "EC" == m ) // Copenhagen
			ret = "Copenhagen";
		else if( "EW" == m ) // Stockholm
			ret = "Stockholm";
		else if( "EY" == m ) // Helsinki
			ret = "Helsinki";

		else if( "EU" == m ) // Europe
			ret = "Europe";

		else if( "AT" == m || "AC" == m || "AN" == m )
			ret = "Tokyo";
		else if( "AH" == m )
			ret = "HongKong";
		else if( "AS" == m || "AX" == m )
			ret = "Sydney";
		else if( "AK" == m || "AQ" == m )
			ret = "Seoul";
		else if( "AG" == m )
			ret = "Singapore";
		else if( "AW" == m )
			ret = "Taiwan";

		else if( "MJ" == m )
			ret = "Johannesburg";

		else if( "SS" == m )
			ret = "Sao Paulo";

	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string currQAI(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		if( "U" == region(m) )
			ret = "1031";

		else if( "C" == region(m) )
			ret = "1032";

		else if( "EA" == m || "EB" == m || "EI" == m || "EP" == m || "EF" == m )
			ret = "1454";
		else if( "EL" == m )
			ret = "1023";

		else if( "ED" == m ) // Madrid
			ret = "1454";
		else if( "EM" == m ) // Milan
			ret = "1454";
		else if( "EZ" == m ) // Zurich
			ret = "1092";
		else if( "EO" == m ) // Oslo
			ret = "1076";
		else if( "EX" == m ) // Virt-X
			ret = "1092";
		else if( "EC" == m ) // Copenhagen
			ret = "1072";
		else if( "EW" == m ) // Stockholm
			ret = "1079";
		else if( "EY" == m ) // Helsinki
			ret = "1454";

		else if( "AT" == m || "AC" == m || "AN" == m ) // Tokyo
			ret = "1091";
		else if( "AH" == m ) // HongKong
			ret = "1050";
		else if( "AS" == m || "AX" == m ) // Sydney
			ret = "1040";
		else if( "AK" == m || "AQ" == m ) // Seoul
			ret = "1100";
		else if( "AW" == m ) // Taiwan
			ret = "1101";
		else if( "AG" == m ) // Singapore
			ret = "1052";

		else if( "MJ" == m ) // Johannesburg
			ret = "1059";

		else if( "SS" == m ) // Brazil
			ret = "1087"; // Brazil Real
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

string currISO(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		if( "C" == region(m) )
			ret = "CAD";
		else if( "U" == region(m) )
			ret = "USD";

		else if( "EA" == m || "EB" == m || "EI" == m || "EP" == m || "EF" == m )
			ret = "EUR";
		else if( "EL" == m )
			ret = "GBP";

		else if( "ED" == m ) // Madrid
			ret = "EUR";
		else if( "EM" == m ) // Milan
			ret = "EUR";
		else if( "EZ" == m ) // Zurich
			ret = "CHF";
		else if( "EO" == m ) // Oslo
			ret = "NOK";
		else if( "EX" == m ) // Virt-X
			ret = "CHF";
		else if( "EC" == m ) // Copenhagen
			ret = "DKK";
		else if( "EW" == m ) // Stockholm
			ret = "SEK";
		else if( "EY" == m ) // Helsinki
			ret = "EUR";

		else if( "AT" == m || "AC" == m || "AN" == m )
			ret = "JPY";
		else if( "AH" == m )
			ret = "HKD";
		else if( "AS" == m || "AX" == m )
			ret = "AUD";
		else if( "AK" == m || "AQ" == m )
			ret = "KRW";
		else if( "AG" == m )
			ret = "SGD";
		else if( "AW" == m )
			ret = "TWD";
		else if( "MJ" == m )
			ret = "ZAR";
		else if( "SS" == m )
			ret = "BRL";
	}

	return ret;
}

string currChara(const string& m)
{
	string ret = "";

	if( m.size() == 2 )
	{
		if( "C" == region(m) )
			ret = "CAD";

		else if( "EA" == m || "EB" == m || "EI" == m || "EP" == m || "EF" == m )
			ret = "EUR";
		else if( "EL" == m )
			ret = "GBX";

		else if( "ED" == m ) // Madrid
			ret = "EUR";
		else if( "EM" == m ) // Milan
			ret = "EUR";
		else if( "EZ" == m ) // Zurich
			ret = "CHF";
		else if( "EO" == m ) // Oslo
			ret = "NOK";
		else if( "EX" == m ) // Virt-X
			ret = "CHF";
		else if( "EC" == m ) // Copenhagen
			ret = "DKK";
		else if( "EW" == m ) // Stockholm
			ret = "SEK";
		else if( "EY" == m ) // Helsinki
			ret = "EUR";

		else if( "AT" == m )
			ret = "100YEN";
		else if( "AH" == m )
			ret = "HKD";
		else if( "AS" == m )
			ret = "AUD";
		else if( "AK" == m || "AQ" == m )
			ret = "100KRW";
		else if( "AG" == m )
			ret = "SGD";
		else if( "AW" == m )
			ret = "TWD";
		else if( "AA" == m )
			ret = "CNY";

		else if( "MJ" == m )
			ret = "ZAC";
		else if( "SS" == m )
			ret = "BRL";
	}

	if( ret.empty() )
		exit(2);

	return ret;
}

double currWgt(const string& m) // last update: May 10 2013.
{
	double ret = 0;

	if( m.size() == 2 )
	{
		if( "U" == region(m) )
			ret = 1.;
		else if( "C" == region(m) )
			ret = 1.01;

		else if( "EA" == m || "EB" == m || "EI" == m || "EP" == m || "EF" == m )
			ret = 1.30;
		else if( "EL" == m )
			ret = 1.53;

		else if( "ED" == m ) // Madrid
			ret = 1.30;
		else if( "EM" == m ) // Milan
			ret = 1.30;
		else if( "EZ" == m ) // Zurich
			ret = 1.00;
		else if( "EO" == m ) // Oslo
			ret = 0.17;
		else if( "EX" == m ) // Virt-X
			ret = 1.00;
		else if( "EC" == m ) // Copenhagen
			ret = 0.17;
		else if( "EW" == m ) // Stockholm
			ret = 0.15;
		else if( "EY" == m ) // Helsinki
			ret = 1.30;

		else if( "AT" == m )
			ret = 0.01;
		else if( "AH" == m )
			ret = 0.13;
		else if( "AS" == m )
			ret = 1.00;
		else if( "AK" == m || "AQ" == m )
			ret = 0.00099;
		else if( "AG" == m )
			ret = 0.73;
		else if( "AW" == m )
			ret = 0.03;
		else if( "MJ" == m )
			ret = 0.11;
		else if( "SS" == m )
			ret = 0.49;
	}

	if( ret < 1e-7 )
		exit(2);

	return ret;
}

double currWgtMerc(const string& m) // last update: Jul 2019
{
	double ret = 0;

	if( m.size() == 2 )
	{
		if( "U" == region(m) )
			ret = 1.;
		else if( "C" == region(m) )
			ret = 0.77;

		else if( "EA" == m || "EB" == m || "EI" == m || "EP" == m || "EF" == m )
			ret = 1.13;
		else if( "EL" == m )
			ret = 0.0125;

		else if( "ED" == m ) // Madrid
			ret = 1.13;
		else if( "EM" == m ) // Milan
			ret = 1.13;
		else if( "EZ" == m ) // Zurich
			ret = 1.02;
		else if( "EO" == m ) // Oslo
			ret = 0.12;
		else if( "EX" == m ) // Virt-X
			ret = 1.02;
		else if( "EC" == m ) // Copenhagen
			ret = 0.15;
		else if( "EW" == m ) // Stockholm
			ret = 0.11;
		else if( "EY" == m ) // Helsinki
			ret = 1.13;

		else if( "AT" == m )
			ret = 0.93;
		else if( "AH" == m )
			ret = 0.13;
		else if( "AS" == m )
			ret = 0.70;
		else if( "AA" == m )
			ret = 0.15;
		else if( "AK" == m || "AQ" == m )
			ret = 0.085;
		else if( "AG" == m )
			ret = 0.73;
		else if( "AW" == m )
			ret = 0.03;
		else if( "MJ" == m )
			ret = 0.11;
		else if( "SS" == m )
			ret = 0.49;
	}

	if( ret < 1e-7 )
		exit(2);

	return ret;
}

double fee_bpt(const string& m, double price)
{
	string m2 = m.substr(0, 2);
	double ret = 1.;
	if( "US" == m2 )
	{
		if( price == 0. )
			ret = 1.;
		else
			//ret = 36. / price + 0.092;
			ret = 36. / price;
	}
	else if( "C" == m2.substr(0, 1) )
	{
		if( price == 0. )
			ret = 1.;
		else
			ret = 40. / price + 0.1056;
	}
	else if( "AT" == m2 )
		ret = 1.;
	else if( "AH" == m2 )
		ret = 12.3;
	else if( "AS" == m2 )
		ret = 1.5;
	else if( "AK" == m2 || "AQ" == m2 || "KR" == m2 )
		ret = 16.5;
	else if( "AA" == m2 )
		ret = 20.;
	else if( "MJ" == m2 )
		ret = 5.;
	else if( "SS" == m2 )
		ret = 6.;
	return ret;
}

double fee_bpt(const string& m, char primex, double price)
{
	double fb = GFee::Instance().feeBpt(m, primex, price);
	return fb;
}

string selChara(int idate)
{
	string ret = (string)" idate = " + itos(idate);
	return ret;
}

string selChara(const string& m, int idate)
{
	string ret = "";
	if( "C" == region(m) || "U" == region(m) )
		ret = (string)" idate = " + itos(idate);
	else if( isInternational(m) )
	{
		if( isECN(m) )
			ret = (string)" idate = " + itos(idate);
		else
			ret = (string)" idate = " + itos(idate) + " and market = " + quote(code(m));
	}
	if( ret.empty() )
		exit(2);
	return ret;
}

string selChara(const string& m, int idate0, int idate1)
{
	string ret = "";
	if( "C" == region(m) || "U" == region(m) )
		ret = (string)" (idate >= " + itos(idate0) + " and idate <= " + itos(idate1) + ") ";
	else if( isInternational(m) )
	{
		if( isECN(m) )
			ret = (string)" (idate >= " + itos(idate0) + " and idate <= " + itos(idate1) + ") ";
		else
			ret = (string)" (idate >= " + itos(idate0) + " and idate <= " + itos(idate1) + ") and market = " + quote(code(m));
	}
	if( ret.empty() )
		exit(2);
	return ret;
}

string selOrder(const string& m, int idate)
{
	string ret = "";
	if( "C" == region(m) || "U" == region(m) )
		ret = (string)" idate = " + itos(idate);
	else if( isInternational(m) )
	{
		if( isECN(m) )
			ret = (string)" idate = " + itos(idate);
		else
			ret = (string)" idate = " + itos(idate) + " and exchange = " + quote(code(m));
	}
	if( ret.empty() )
		exit(2);
	return ret;
}

string selTradeTime(const string& m, int idate)
{
	string ret = "";
	if( "C" == region(m) || "U" == region(m) )
		ret = (string)" o.idate = " + itos(idate) + " and e.idate = " + itos(idate);
	else if( isInternational(m) )
	{
		if( isECN(m) )
			ret = (string)" o.idate = " + itos(idate) + " and e.idate = " + itos(idate);
		else
			ret = (string)" o.idate = " + itos(idate) + " and e.idate = " + itos(idate) + " and o.exchange = " + quote(code(m));
	}
	if( ret.empty() )
		exit(2);
	return ret;
}

string selVal(const string& m)
{
	string ret = "";
	string extel = mto::extel(m);
	if( extel.empty() )
		exit(2);

	if( "EF" == m )
		ret = (string)" (v.Exchange = 'EDA' or v.Exchange = 'EDF') ";
	else if( "EX" == m )
		ret = (string)" (v.Exchange = 'ESV' or v.Exchange = 'ESE') ";
	else if( "AT" == m )
		ret = (string)"(v.Exchange = 'FJT' or v.Exchange = 'FJJ' or v.Exchange = 'FJO')";
	else
		ret = (string)" v.Exchange = " + quote(extel);

	return ret;
}

string selInfo(const string& m)
{
	string ret = "";
	string extel = mto::extel(m);
	if( extel.empty() )
		exit(2);

	if( extel.size() == 3 )
	{
		if( "EX" == m )
			ret = (string)" i.QuoteCtry = 'ES' and (i.Exchange = 'V' or i.Exchange = 'E') ";
		else if( "AT" == m )
			ret = (string)" i.QuoteCtry = 'FJ' and (i.Exchange = 'T' or i.Exchange = 'J' or i.Exchange = 'O') ";
		else
			ret = (string)" i.QuoteCtry = " + quote(extel.substr(0, 2))
				+ " and i.Exchange = " + quote(extel.substr(2, 1));
	}

	return ret;
}

string selType(const string& m)
{
	string ret = "";
	if( "C" == region(m) )
		ret = " (secType = '7' or sectype = 'C' or sectype = 'F' or sectype = 'I' or sectype = 'U') ";
	else if( "U" == region(m) )
		ret = " (secType = 'A' or secType = 'U' or secType = 'W' or secType = 'F' or secType = 'C' or secType = '7' or secType = 'P' or secType = 'S' or secType = 'I' or secType = 'T') ";
	else if( "E" == region(m) )
		ret = " (secType = '09' or secType = '10' or sectype = '31' or sectype = '34' or sectype = '54' or sectype = '5G' or sectype = '6C' or sectype = '6D') ";
	else if( isInternational(m) && "E" != region(m) )
		ret = " (secType = '09' or secType = '10' or sectype = '31' or sectype = '34' or sectype = '54' or sectype = '6C' or sectype = '6D') ";
	if( ret.empty() )
		exit(2);

	return ret;
}

string selTypeTight(const string& m)
{
	string ret = "";
	if( "C" == region(m) )
		ret = " (sectype = 'C') ";
	else if( "U" == region(m) )
		ret = " (secType = 'A' or secType = 'C') ";
	else if( isInternational(m) )
		ret = " (secType = '09' or secType = '10') ";
	if( ret.empty() )
		exit(2);

	return ret;
}

string ts(const string& m)
{
	string ret = "";
	if( "C" == region(m) || "U" == region(m) )
		ret = "ticker";
	else if( isInternational(m) )
		ret = "symbol";

	return ret;
}

string compTicker(const string& m)
{
	string ret = "";
	if( "C" == region(m) || "U" == region(m) )
		ret = "ticker";
	else if( isInternational(m) )
	{
		ret = "market + ':' + symbol";
	}

	return ret;
}

string compTicker(const string& m, int idate)
{
	string ret = "";
	if( "C" == region(m) || "U" == region(m) )
		ret = "ticker";
	else if( isInternational(m) )
	{
		if( idate < 20090706 )
			ret = "symbol";
		else
			ret = "market + ':' + symbol";
	}

	return ret;
}

string compTicker(const string& ticker, const string& market)
{
	string ret = ticker;
	if( isInternational(market) )
	{
		if( ticker.find(":") == string::npos )
			ret = code(market) + ":" + ticker;
	}
	return ret;
}

string compTicker(const string& ticker, const string& market, int idate)
{
	string ret = ticker;
	if( isInternational(market) )
	{
		if( ticker.find(":") == string::npos ) // Input is a base ticker.
		{
			if( idate >= 20090706 )
				ret = code(market) + ":" + ticker;
		}
		else // Input is a compound ticker.
		{
			if( idate < 20090706 )
			{
				int tickerSize = ticker.size();
				if( tickerSize > 2 && ticker.find(":") == 1 )
				{
					ret = ticker.substr(2, tickerSize - 2);
				}
			}
		}
	}
	return ret;
}

vector<string> compTickers(const vector<string>& tickers, const string& market)
{
	vector<string> ret;
	for( auto it = begin(tickers); it != end(tickers); ++it )
	{
		string ticker = *it;
		string cTicker = compTicker(ticker, market);
		ret.push_back(cTicker);
	}
	return ret;
}

string retName(const string& m)
{
	string ret = "";
	if( "C" == region(m) )
		ret = "TSX_RET";
	else if( "US" == m )
		ret = "SPX_RET";
	else if( "E" == region(m) )
		ret = "PANEU_RET";
	else if( "A" == region(m) || "M" == region(m) )
		ret = code(m) + "_RET";
	else if( "S" == region(m) )
		ret = m + "_RET";
	else
		exit(5);
	return ret;
}

string andMarketSel(const string& m)
{
	string ret = "";
	if( region(m) == "E" || region(m) == "A" || region(m) == "M" || region(m) == "S" || region(m) == "C")
		ret = (string)" and market = " + quote(code(m));
	return ret;
}

int msecOpen( const string& m, int idate )
{
	int ret = 0;
	if( m.size() == 2 )
	{
		string reg = region(m);
		if( "C" == reg || "U" == reg )
			ret = 9.5*60*60*1000;
		else if( "E" == reg )
			ret = 8*60*60*1000;
		else if( "AT" == m || "AC" == m || "AN" == m )
			ret = 9*60*60*1000;
		else if( "AH" == m )
		{
			if ( idate >= 20110307 )
				ret = 9*60*60*1000 + 30*60*1000;
			else
				ret = 10*60*60*1000;
		}
		else if( "AS" == m || "AX" == m )
			ret = 10*60*60*1000 + 10*60*1000;
		else if( "AG" == m )
			ret = 9*60*60*1000;
		else if( "AK" == m || "AQ" == m )
			ret = 9*60*60*1000;
		else if( "AW" == m )
			ret = 9*60*60*1000;
		else if( "AA" == m )
			ret = 9.5*60*60*1000;

		else if( "MJ" == m )
			ret = 9*60*60*1000;
		else if( "SS" == m) 
			ret = 10*60*60*1000;
	}
	return ret;
}

int msecClose( const string& m, int idate )
{
	/*
	 * Euronext		-> 20070216 16:25, 20070219 -> 16:30 (London time)
	 * Oslo			-> 20080829 15:20, 20080901 -> 16:20 (London time)
	 * Stockholm		-> 20100205 16:20, 20100208 -> 16:25
	 * Helsinki		-> 20100205 16:20, 20100208 -> 16:25
	 * Copenhagen	-> 20100205 15:50, 20100208 -> 15:55
	 * Taiwan		-> 9:00am to 1:25pm
	 * Korea		-> 20160731 15:00, 20160801 -> 15:30
	 */
	int ret = 0;
	if( m.size() == 2 )
	{
		string reg = region(m);
		if( "U" == reg )
			ret = 16*60*60*1000;
		if( "C" == reg && "CL" != m )
			ret = 16*60*60*1000;
		else if( "CL" == m )
			ret = 16*60*60*1000 + 15*60*1000;
		else if( "EZ" == m || "EX" == m )
			ret = 16*60*60*1000 + 20*60*1000;
		else if( "EW" == m || "EY" == m )
		{
			if( idate <= 20100205 )
				ret = 16*60*60*1000 + 20*60*1000;
			else
				ret = 16*60*60*1000 + 25*60*1000;
		}
		else if( "EM" == m )
		{
			if(idate < 20151123)
				ret = 16*60*60*1000 + 25*60*1000;
			else
				ret = 16*60*60*1000 + 30*60*1000;
		}
		else if( "EA" == m || "EB" == m || "EI" == m || "EP" == m )
		{
			if( idate <= 20070216 )
				ret = 16*60*60*1000 + 25*60*1000;
			else
				ret = 16*60*60*1000 + 30*60*1000;
		}
		else if( "EF" == m || "EL" == m || "ED" == m )
			ret = 16*60*60*1000 + 30*60*1000;
		else if( "EO" == m )
		{
			if( idate <= 20080829 || idate >= 20120806 )
				ret = 15*60*60*1000 + 20*60*1000;
			else
				ret = 16*60*60*1000 + 20*60*1000;
		}
		else if( "EC" == m )
		{
			if( idate <= 20100205 )
				ret = 15*60*60*1000 + 50*60*1000;
			else
				ret = 15*60*60*1000 + 55*60*1000;
		}
		else if( "EU" == m || "EH" == m || "ET" == m || "EG" == m )
			ret = 16*60*60*1000 + 30*60*1000;

		else if( "AT" == m || "AC" == m || "AN" == m )
			ret = 15*60*60*1000;
		else if( "AH" == m )
			ret = 16*60*60*1000;
		else if( "AS" == m || "AX" == m )
			ret = 16*60*60*1000;
		else if( "AG" == m )
			ret = 17*60*60*1000;
		else if( "AK" == m || "AQ" == m )
		{
			if( idate < 20160801 )
				ret = 14*60*60*1000 + 50*60*1000;
			else if( idate >= 20160801 )
				ret = 15*60*60*1000 + 20*60*1000;
		}
		else if( "AW" == m )
			ret = 13*60*60*1000+25*60*1000;
		else if( "AA" == m )
			ret = 15*60*60*1000;

		else if( "MJ" == m )
			ret = 16*60*60*1000 + 50*60*1000;
		else if( "SS" == m ) // In BRXT time zone.
		{
			if( idate >= 20121203 && idate <= 20130707 )
				ret = 17*60*60*1000 + 25*60*1000;
			else if( idate > 20130707 && idate < 20151221 )
				ret = 16*60*60*1000 + 55*60*1000;
			else if( idate >= 20151221 && idate < 20160314 )
				ret = 17*60*60*1000 + 55*60*1000;
			else if( idate >= 20160314 )
				ret = 16*60*60*1000 + 55*60*1000;
		}
	}
	return ret;
}

vector<pair<int, int> > breaks(const string& market, int delay, int idate)
{
	vector<pair<int, int> > v;
	int H2ms = 60 * 60 * 1000;
	int M2ms = 60 * 1000;
	if( "EF" == market )
		v.push_back(make_pair( 12*H2ms, 12*H2ms + 3*M2ms + delay*M2ms ));
	else if( "EC" == market || "EY" == market || "EW" == market )
	{
		if( idate >= 20131209 )
			v.push_back(make_pair( 12*H2ms + 30*M2ms, 12*H2ms + 35*M2ms ));
	}
	else if("EL" == market)
	{
		if(idate >= 20160321)
			v.push_back(make_pair(12*H2ms, 12*H2ms + 2*M2ms));
	}
	else if( "AT" == market )
	{
		if ( idate >= 20111121 )
			v.push_back(make_pair( 11*H2ms + .5*H2ms, 12*H2ms + .5*H2ms + delay*M2ms));
		else
			v.push_back(make_pair( 11*H2ms, 12*H2ms + .5*H2ms + delay*M2ms ));
	}
	else if( "AH" == market )
	{
		if( idate >= 20120305 )
			v.push_back(make_pair( 12*H2ms, 13*H2ms + delay*M2ms ));
		else if ( idate >= 20110307 && idate < 20120305 )
			v.push_back(make_pair( 12*H2ms, 13*H2ms + .5*H2ms + delay*M2ms ));
		else if( idate < 20110307 )
			v.push_back(make_pair( 12*H2ms + .5*H2ms, 14*H2ms + .5*H2ms + delay*M2ms ));
	}
	else if( "AG" == market )
		v.push_back(make_pair( 12*H2ms + .5*H2ms, 14*H2ms + delay*M2ms ));
	else if( "AA" == market )
		v.push_back(make_pair( 11*H2ms + .5*H2ms, 13*H2ms + delay*M2ms ));
	return v;
}

vector<pair<int, int> > sessions(const string& market, int delayOpen, int delayClose, int delayOther, int idate)
{
	vector<pair<int, int> > b = breaks(market, 0, idate);
	vector<pair<int, int> > v;
	int nSessions = b.size() + 1;
	for( int s = 0; s < nSessions; ++s )
	{
		int delay1 = 0;
		int delay2 = 0;

		// Beginning of session
		int msec1 = 0;
		if( s == 0 ) // first session
		{
			msec1 = mto::msecOpen(market, idate);
			delay1 = delayOpen;
		}
		else
		{
			msec1 = b[s-1].second;
			delay1 = delayOther;
		}

		// End of session
		int msec2 = 0;
		if( s == nSessions - 1 ) // last session
		{
			msec2 = mto::msecClose(market, idate);
			delay2 = delayClose;
		}
		else
		{
			msec2 = b[s].first;
			delay2 = delayOther;
		}

		v.push_back(make_pair( msec1 + delay1 * 60 * 1000, msec2 - delay2 * 60 * 1000 ));
	}
	return v;
}

bool dataOK( const string& market, int idate )
{
	string table = ok(market);
	bool dataok = false;
	vector<vector<string> > vvOK;

	if( "U" == region(market) )
	{
		string db = "equitydata";
		if( "US" == market )
		{
			char cmd[1000];
			sprintf(cmd, "select count(*) "
					" from %s "
					" where idate = %d "
					" and dataOK = 1 "
					//" and (arcaOK = 1 or arcaOK is null) "
					" and (arcaOK = 1 or idate < 20070208) "
					//" and (inetOK = 1 or inetOK is null) "
					" and (inetOK = 1 or idate < 20070208) "
					//" and (nyseOK = 1 or nyseOK is null) "
					" and (nyseOK = 1 or idate < 20070312) "
					//" and (batsOK = 1 or batsOK is null) "
					" and (batsOK = 1 or idate < 20071120) "
					//" and (edgaOK = 1 or edgaOK is null) "
					//" and (edgxOK = 1 or edgxOK is null) ",
					" and (edgxOK = 1 or idate < 20080709) ",
					table.c_str(),
					idate
				   );

			GODBC::Instance()->read(db, cmd, vvOK);
			dataok = !vvOK.empty() && atoi(vvOK[0][0].c_str()) == 1;
		}
		else
		{
			string okField = "";
			if( "UP" == market )
				okField = "arcaOK";
			else if( "UQ" == market )
				okField = "inetOK";
			else if( "UN" == market )
				okField = "nyseOK";
			else if( "UC" == market )
				okField = "batsOK";
			else if( "UD" == market )
				okField = "edgxOK";
			else if( "UJ" == market )
				okField = "edgaOK";
			else if( "UB" == market )
				okField = "bxok";
			else if( "UY" == market )
				okField = "byxok";
			else if( "UX" == market )
				okField = "psxok";
			char cmd[1000];
			sprintf(cmd, "select count(*) "
					" from %s "
					" where idate = %d "
					" and %s = 1",
					table.c_str(),
					idate,
					okField.c_str()
				   );

			GODBC::Instance()->read(db, cmd, vvOK);
			dataok = !vvOK.empty() && atoi(vvOK[0][0].c_str()) == 1;
		}
	}
	else
	{
		string db = hf(market);
		string cmd = (string)"SELECT dataOK "
			+ " FROM " + table
			+ " WHERE idate = " + itos( idate )
			+ andMarketSel(market);

		GODBC::Instance()->read(db, cmd, vvOK );
		dataok = !vvOK.empty() && atoi( vvOK[0][0].c_str() ) == 1;
	}

	return dataok;
}

vector<int> dates( const string& market, int d1, int d2 )
{
	vector<int> vd;
	Exchange ex(market);
	int idate = (int)ex.NextOpen( QuoteTime(d1, 040000, mto::tz(market)) ).Date();
	for( ; idate <= d2; idate = (int)ex.NextOpen( QuoteTime(idate, 200000, mto::tz(market)) ).Date() )
	{
		vd.push_back(idate);
	}
	return vd;
}

int lotSize( const string& market, int idate )
{
	int ret = 1;
	string reg = region(market);
	if( reg == "U" && idate < 20070208 )
		ret = 100;
	return ret;
}

double convertPriceQAItoChara( double qai_price, const string& currQAI )
{
	double chara_price = qai_price;
	if( currQAI == "1023" || currQAI == "1059" ) // GBP to GBX, ZAR to ZAC
		chara_price = qai_price * 100;
	else if( currQAI == "1091" || currQAI == "1100" ) // JPY/KRW to 100YEN/100KRW
		chara_price = qai_price / 100.0;

	return chara_price;
}
}
