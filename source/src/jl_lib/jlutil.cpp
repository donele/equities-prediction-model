#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GEX.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GTH.h>
#include <jl_lib/sort_util.h>
#include <string>
#include <vector>
#include <set>
#include <numeric>
#include <sstream>
#include <map>
#include <chrono>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <algorithm>
#include <stdio.h>
#include <cctype>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#ifdef _WIN32
#include <psapi.h>
#include <windows.h>
#endif

using namespace std;

const double ltmb_ = 5e-5; // a number less than minimum bid increment of any known market.
const double basis_pts_ = 1e4;
#ifdef _WIN32
//const double min_double_ = DBL_MIN;
const double max_double_ = DBL_MAX;
//const float min_float_ = FLT_MIN;
const float max_float_ = FLT_MAX;
const int max_int_ = INT_MAX;
#else
//const double min_double_ = std::numeric_limits<double>::min();
const double max_double_ = std::numeric_limits<double>::max();
//const float min_float_ = std::numeric_limits<float>::min();
const float max_float_ = std::numeric_limits<float>::max();
const int max_int_ = std::numeric_limits<int>::max();
#endif
const std::string tickC_dir_ = "\\\\smrc-nas09\\gf1\\tickC";
const std::string tickR_dir_ = "\\\\smrc-nas09\\gf1\\tickR";
const std::string tickMon_dir_= "\\\\smrc-nas10\\l\\tickMon";

void mkd(const std::string path)
{
	if( !(boost::filesystem::is_directory(path)) )
		boost::filesystem::create_directories(path);
}

string xpf(const string& path)
{
	string ret = path;
#ifndef _WIN32
	// Replace the backslashes.
	{
		int position = 0;
		while( (position = ret.find("\\", position)) != string::npos )
			ret.replace(position++, 1, "/");
	}
	
	// Do not allow double slashes except for the beginning.
	{
		int position = 1;
		while( (position = ret.find("//", position)) != string::npos )
			ret.replace(position, 2, "/");
	}
	
// 	// Replace the semicolons.
// 	{
// 		int position = 0;
// 		while( (position = ret.find(";", position)) != string::npos )
// 		{
// 			ret.replace(position, 1, "\\;");
// 			position += 2;
// 		}
// 	}
	
	// Replace the hostname.
	{
		int position1 = ret.find("//");
		if( position1 == 0 )
		{
		  int position2 = ret.find("/", 2);
		  if( position2 > position1 )
		  {
			ret.replace(position1, position2 - position1, "/mnt");
		  }
		}
	}
// 	if( ret.substr(0, 16) == "//smrc-nas09" )
// 		ret.replace(0, 16, "/mnt");
// 	if( ret.substr(0, 16) == "//smrc-ltc-mrct40" )
// 		ret.replace(0, 16, "/mnt");
#endif
	return ret;
}

bool is_number(const string& s)
{
	string::const_iterator it = s.begin();
	while( it != s.end() && std::isdigit(*it) ) ++it;
	return !s.empty() && it == s.end();
}

bool isPowerOfTwo(int n)
{
	if( n < 0 )
		return false;
	while(((n % 2) == 0) && n > 1)
		n /= 2;
	return (n == 1);
}

int sign(double d)
{
	int s = 0;
	if( d > 0. )
		s = 1;
	else if( d < 0. )
		s = -1;
	return s;
}

double clip_off(double d, double clip)
{
	if( clip < 0. )
	{
		cerr << "ERROR: negative clip. " << clip << "\n";
		exit(5);
	}
	if( d != d ) // indeterminate.
		d = 0.;
	else
	{
		if( d > clip )
			d = clip;
		else if( d < -clip )
			d = - clip;
	}
	return d;
}

void clip(double& d, double clip)
{
	if( clip < 0. )
	{
		cerr << "ERROR: negative clip. " << clip << "\n";
		exit(5);
	}
	if( d != d ) // indeterminate.
		d = 0.;
	else
	{
		if( d > clip )
			d = clip;
		else if( d < -clip )
			d = - clip;
	}
}

void clip(float& d, float clip)
{
	if( clip < 0. )
	{
		cerr << "ERROR: negative clip. " << clip << "\n";
		exit(5);
	}
	if( d != d ) // indeterminate.
		d = 0.;
	else
	{
		if( d > clip )
			d = clip;
		else if( d < -clip )
			d = - clip;
	}
}

string itos(int i)
{
	ostringstream s;
	s << i;
	return s.str();
}

string dtos(double d, int p)
{
	char buf[100];
	sprintf( buf, "%.*f", p, d );
	return buf;
}

string dtosN0n(double d, int p)
{
	char buf[100];
	if( d > 1e-200 ) // positive
		sprintf( buf, "%.*f", p, d );
	else
		sprintf( buf, "NULL" );
	return buf;
}

string itoq(int idate)
{
	char buf[100];
	sprintf( buf, "%d/%d/%d", idate/100%100, idate%100, idate/10000 );
	string ret = buf;
	return ret;
}

int qtoi(const string& qdate)
{
	int pos1 = qdate.find("-");
	int yyyy = atoi( qdate.substr(0, pos1).c_str() );
	int pos2 = qdate.find("-", pos1 + 1);
	int mm = atoi( qdate.substr(pos1 + 1, pos2 - pos1).c_str() );
	int pos3 = qdate.size();
	int dd = atoi( qdate.substr(pos2 + 1, pos3 - pos2).c_str() );
	int idate = yyyy * 10000 + mm * 100 + dd;
	return idate;
}

string quote(const string& s)
{
	string ret = (string)"'" + s + "'";
	return ret;
}

string quoteN(const string& s)
{
	string ret = "NULL";
	if( !s.empty() )
		ret = (string)"'" + s + "'";

	return ret;
}

string unquote(const string& s)
{
	string ret = s;
	int n = s.size();
	if( n >= 2 )
	{
		if( (s[0] == '\'' && s[n-1] == '\'') || (s[0] == '\"' && s[n-1] == '\"') )
			ret = s.substr(1, n-2);
	}
	return ret;
}

string trim( const string& s )
{
	//if( !s.empty() && s[s.size() - 1] == ' ' )
		//return trim(s.substr(0, s.size() - 1));
	//return s;
	string ret = s;
	boost::trim(ret);
	return ret;
}

multimap<string, string> get_amm(int argc, char* argv[])
{
	multimap<string, string> amm;
	
	for( int i=0; i<argc; ++i )
	{
		// find a key.
		string this_arg = argv[i];
		int lenArg = this_arg.size();
		if( lenArg > 1 && this_arg[0] == '-' )
		{
			// count the number of values for the key.
			int nval = 0;
			int j = i;
			while( ++j < argc )
			{
				if( argv[j][0] == '-' )
					break;
				else
					++nval;
			}

			// insert the values for the key.
			if( nval == 0 )
				amm.insert(make_pair(this_arg.substr(1, lenArg-1), ""));
			else
			{
				for( int k=1; k<=nval; ++k )
					amm.insert(make_pair(this_arg.substr(1, lenArg-1), argv[i+k]));
			}
		}
	}

	return amm;
}

map<string, string> get_am( int argc, char* argv[] )
{
	map<string, string> am;
	
	for( int i=0; i<argc; ++i )
	{
		string this_arg = argv[i];
		int lenArg = this_arg.size();
		if( lenArg > 1 && this_arg[0] == '-' )
		{
			if( i < argc - 1 )
			{
				string next_arg = argv[i+1];
				if( !next_arg.empty() )
				{
					if( next_arg[0] != '-' )
					{
						am[this_arg.substr(1,lenArg-1)] = next_arg;
					}
					else
					{
						am[this_arg.substr(1,lenArg-1)] = "";
					}
				}
			}
			else
				am[this_arg.substr(1,lenArg-1)] = "";
		}
	}

	return am;
}

int itoday(const string& market)
{
	char syyyymmdd[128];
	char shhmmss[128];
	struct tm* now;
	time_t ltime;
	time( &ltime );
	now = gmtime( &ltime );
	strftime( syyyymmdd, 128, "%Y%m%d", now );
	strftime( shhmmss, 128, "%H%M%S", now );
	int utc_yyyymmdd = atoi(syyyymmdd);
	int utc_hhmmss = atoi(shhmmss);

	QuoteTime qt(utc_yyyymmdd, utc_hhmmss, "UTC");
	qt.SetTimeZone(mto::tz(market));
	int idate = (int)qt.Date();
	return idate;
}

int itoday()
{
	char tmpbuf[128];
	struct tm* today;
	time_t ltime;
	time( &ltime );
	today = localtime( &ltime );
	strftime( tmpbuf, 128, "%Y%m%d", today );
	int ret = atoi(tmpbuf);
	return ret;
}

int getFinalSigDay(const std::string& market)
{
	int nValidDay = 0;
	int idate = itoday();
	for(int cnt = 0; ++cnt < 20; idate = prevClose(market, idate))
	{
		if( get_univ_tickers(market, idate).size() > 0 )
		{
			if( ++nValidDay >= 2 )
				return idate;
		}
	}
	return 0;
}

string timestamp()
{
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	return buf;
}

vector<string> split( const string& s )
{
	vector<string> ret;
	typedef string::size_type string_size;
	string_size i = 0;

	while( i != s.size() )
	{
		while( i != s.size() && isspace(s[i]) )
			++i;

		string_size j = i;
		while( j != s.size() && !isspace(s[j]) )
			++j;

		if( i != j )
		{
			ret.push_back(s.substr(i, j-i));
			i=j;
		}
	}

	return ret;
}

vector<string> split( const string& s, const char d )
{
	vector<string> ret;
	typedef string::size_type string_size;
	string_size i = 0;

	while( i != s.size() )
	{
		while( i != s.size() && (isspace(s[i]) || s[i] == d) )
			++i;

		string_size j = i;
		while( j != s.size() && s[j] != d )
			++j;

		if( i != j )
		{
			ret.push_back(s.substr(i, j-i));
			i=j;
		}
	}

	return ret;
}

vector<string> splitN( const string& s, const char d )
{
	int pos = -1;
	vector<int> positions(1,pos);
	while( 1 )
	{
		pos = s.find(d, pos+1);
		if( pos != string::npos )
			positions.push_back(pos);
		else
		{
			positions.push_back(s.size());
			break;
		}
	}

	vector<string> ret;
	for( int i=0; i<positions.size()-1; ++i )
	{
		int len = positions[i+1]-positions[i]-1;
		if( !( i == positions.size()- 2 && len == 0 ) ) // don't add a null string when the line ends with a delimiter.
			ret.push_back( s.substr( positions[i]+1, len ) );
	}

	return ret;
}

int MMMtomm(const string& MMM)
{
	if( "JAN" == MMM )
		return 1;
	else if( "FEB" == MMM )
		return 2;
	else if( "MAR" == MMM )
		return 3;
	else if( "APR" == MMM )
		return 4;
	else if( "MAY" == MMM )
		return 5;
	else if( "JUN" == MMM )
		return 6;
	else if( "JUL" == MMM )
		return 7;
	else if( "AUG" == MMM )
		return 8;
	else if( "SEP" == MMM )
		return 9;
	else if( "OCT" == MMM )
		return 10;
	else if( "NOV" == MMM )
		return 11;
	else if( "DEC" == MMM )
		return 12;
	return 0;
}

int Mmmtomm(const string& Mmm)
{
	if( "Jan" == Mmm )
		return 1;
	else if( "Feb" == Mmm )
		return 2;
	else if( "Mar" == Mmm )
		return 3;
	else if( "Apr" == Mmm )
		return 4;
	else if( "May" == Mmm )
		return 5;
	else if( "Jun" == Mmm )
		return 6;
	else if( "Jul" == Mmm )
		return 7;
	else if( "Aug" == Mmm )
		return 8;
	else if( "Sep" == Mmm )
		return 9;
	else if( "Oct" == Mmm )
		return 10;
	else if( "Nov" == Mmm )
		return 11;
	else if( "Dec" == Mmm )
		return 12;
	return 0;
}

double get_mid(const double& bid, const double& ask)
{
	if( bid < ltmb_ && ask > ltmb_ ) // only ask is valid.
		return ask;
	else if( bid > ltmb_ && ask < ltmb_ ) // only bid is valid.
		return bid;
	else if( bid > ltmb_ && ask > ltmb_ ) // both prices valid.
		return (ask + bid)/2.0;
	return 0.;
}

double get_mid(const QuoteInfo& quote)
{
	if( quote.bid < ltmb_ && quote.ask > ltmb_ ) // only ask is valid.
		return quote.ask;
	else if( quote.bid > ltmb_ && quote.ask < ltmb_ ) // only bid is valid.
		return quote.bid;
	else if( quote.bid > ltmb_ && quote.ask > ltmb_ ) // both prices valid.
		return (quote.ask + quote.bid)/2.0;
	return 0.;
}

double get_sprd(double bid, double ask)
{
	double sprd = 2.0 * (ask - bid) / (ask + bid);
	return sprd;
}

int prevClose(const string& market, int idate)
{
	int prev = (int)GEX::Instance()->get(market)->PrevClose( QuoteTime(idate, 120000, mto::tz(market)) ).Date();
	return prev;
}

int nextClose(const string& market, int idate)
{
	int next = (int)GEX::Instance()->get(market)->NextClose( QuoteTime(idate, 200000, mto::tz(market)) ).Date();
	return next;
}

int nextOpen(const string& market, int idate, int n)
{
	int next = idate;
	for( int i = 0; i < n; ++i )
		next = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(next, 200000, mto::tz(market)) ).Date();
	return next;
}

int advance_idate(const string& market, int idate, int dd)
{
	int ret = idate;
	if( dd > 0 )
	{
		for( int i = 0; i < dd; ++i )
			ret = (int)GEX::Instance()->get(market)->NextClose( QuoteTime(ret, 200000, mto::tz(market)) ).Date();
	}
	else if( dd < 0 )
	{
		for( int i = 0; i < - dd; ++i )
			ret = (int)GEX::Instance()->get(market)->NextClose( QuoteTime(ret, 200000, mto::tz(market)) ).Date();
	}
	return ret;
}

void print_mr(MultiRegress& mr)
{
	cout << "R2 summary\n";

	cout << "nPoints: " << mr.NPoints() << endl;

	int ndim = mr.NDim();
	double r2 = mr.R2(ndim - 1);
	cout << "R^2: " << r2 << endl;

	int iTarget = ndim - 1;
	vector<double> coeffs = mr.ForecastCoeffs(iTarget);
	vector<double> sensi = mr.Sensitivities(iTarget);
	vector<string> labels = mr.Labels();
	vector<double> vMean;
	vector<double> vVar;
	vector<double> vPR2;

	for( int i=0; i<ndim; ++i )
	{
		vMean.push_back(mr.Mean(i));
		vVar.push_back(mr.Delta2(i));
		double pR2 = 0.;
		if( i < ndim - 1 )
		{
			vector<unsigned> inOut;
			for( int j=0; j<ndim; ++j )
				if( j != i )
					inOut.push_back(j);
			pR2 = r2 - mr.R2(inOut);
		}
		vPR2.push_back(pR2);
	}

	printf("%2s %10s %7s %7s %7s %7s %7s\n", "n", "label", "mean", "var", "coeff", "sensi", "r2");
	for( int i=0; i<=ndim; ++i )
	{
		if( i == 0 ) // const
			printf("%2d %10s %7s %7s %7.3f\n", i, "const", " ", " ", coeffs[i]);
		else if( i == ndim ) // Target
			printf("%2d %10s %7.3f %7.3f\n", i, labels[i-1].c_str(), vMean[i-1], vVar[i-1]);
		else
			printf("%2d %10s %7.3f %7.3f %7.3f %7.3f %7.3f\n", i, labels[i-1].c_str(), vMean[i-1], vVar[i-1], coeffs[i], sensi[i-1], vPR2[i-1]);
	}

	printf("\n");
	for( int i=0; i<ndim; ++i )
	{
		{
			for( int j=0; j<ndim; ++j )
			{
				printf("%10s %10s %9.3f\n", labels[i].c_str(), labels[j].c_str(), mr.Corr(i, j));
			}
		}
	}

	return;
}

bool inRange::operator()(const pair<int, int>& p, const int& t) const
{
	return t >= p.first && t <= p.second;
}

bool inRangeStrict::operator()(const pair<int, int>& p, const int& t) const
{
	return t > p.first && t < p.second;
}

void getLotSizeCA( std::map<std::string, int>& m, int idate )
{
	m.clear();
	string market = "CA";

	QuoteTime today(idate, 040000, mto::tz(market));
	int last_date = (int)GEX::Instance()->get(mto::ex(market))->PrevClose(today).Date();
	// 0.10 <= price <= 0.99 -> 500
	{
		vector<vector<string> > vvChara;
		string cmd = (string)"SELECT ticker "
			+ " FROM stockCharacteristics "
			+ " where idate = " + itos(last_date)
			+ " and 0.1 <= close_ and close_ <= 0.99 ";
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vvChara);

		for( vector<vector<string> >::iterator it = vvChara.begin(); it != vvChara.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			m.insert( make_pair(ticker, 500) );
		}
	}
	// price < 0.10 -> 1000
	{
		vector<vector<string> > vvChara;
		string cmd = (string)"SELECT ticker "
			+ " FROM stockCharacteristics "
			+ " where idate = " + itos(last_date)
			+ " and close_ < 0.1 ";
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vvChara);

		for( vector<vector<string> >::iterator it = vvChara.begin(); it != vvChara.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			m.insert( make_pair(ticker, 1000) );
		}
	}
	// price >= 1.0 -> 100
	{
		vector<vector<string> > vvChara;
		string cmd = (string)"SELECT ticker "
			+ " FROM stockCharacteristics "
			+ " where idate = " + itos(last_date)
			+ " and close_ >= 1.0 ";
		GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vvChara);

		for( vector<vector<string> >::iterator it = vvChara.begin(); it != vvChara.end(); ++it )
		{
			string ticker = trim((*it)[0]);
			m.insert( make_pair(ticker, 100) );
		}
	}

	return;
}

string HK_4digit( const string& ticker )
{
	int iTicker = atoi( ticker.c_str() );
	if( iTicker > 0 && iTicker <= 9999 )
	{
		char ticker_4digit[10];
		sprintf( ticker_4digit, "%04d", iTicker );
		return ticker_4digit;
	}
	return ticker;
}

string HK_QAI( const string& ticker )
{
	int iTicker = atoi( ticker.c_str() );
	if( iTicker > 0 && iTicker <= 9999)
	{
		char ticker_QAI[10];
		sprintf( ticker_QAI, "%d", iTicker );
		return ticker_QAI;
	}
	return ticker;
}

string HK_ticker( const string& ticker, int idate )
{
	if( idate < 20080407 )
		return HK_4digit(ticker);
	else
		return HK_QAI(ticker);
	return ticker;
}

string QHtoBloomberg( const string& market, const string& ticker )
{
	// Only for the small European exchanges.

	// Drop "FO-" at the beginning of the ticker, where market = "EC" or "EW" or "EY". (1)
	// Drop "-xxx" at the end of the ticker, where xxx = USD, EUR, etc. (2)
	// Convert "-x" to ".x" where x is a char, and market is "EZ". (3)
	// Remove "-" in the middle of the ticker, like in "xxx-A", "xxx-B", "xxx-SEK-B", "SAGA-PREF", etc. (4a-c)

	string ret = ticker;
	if( "EZ" == market || "EX" == market || "EC" == market || "EW" == market || "EY" == market )
	{
		int pos = 0;
		while( (pos = ret.find('-')) >= 0 )
		{
			if( pos == string::npos )
				break;
			int z = ret.size();
			if( pos == 2 && z > 3 && ret.substr(0, 2) == "FO" ) // (1)
				ret = ret.substr(3, z - 3);
			else if( pos == z - 4 && isalnum(ret[z - 3]) && isalnum(ret[z - 2]) && isalnum(ret[z - 1]) ) // (2)
				ret = ret.substr(0, z - 4);
			else if( "EZ" == market && pos == z - 2 && isalpha(ret[z - 1]) ) // (3)
				ret = ret.substr(0, z - 2) + "." + ret.substr(z - 1, 1);
			else if( pos == z - 1 ) // (4a)
				ret = ret.substr(0, z - 1);
			else if( pos == 0 ) // (4b)
				ret = ret.substr(1, z - 1);
			else // (4c)
				ret = ret.substr(0, pos) + ret.substr(pos + 1, z - pos + 1);
		}
	}

	return ret;
}

string removeColon(string ticker )
{
	while( ticker.find(":") != string::npos )
	{
		int posColon = ticker.find(":");
		ticker.replace( posColon, 1, "_" );
	}
	return ticker;
}

string baseTicker(string ticker)
{
	int pos = ticker.find(":");
	while( pos != string::npos )
	{
		ticker = ticker.substr(pos + 1, ticker.size() - pos - 1);
		pos = ticker.find(":");
	}
	return ticker;
}

string currCharaToQAI( const string& currChara )
{
	string ret = "";

	if( currChara == "CAD" )
		ret = "1032";

	else if( currChara == "EUR" )
		ret = "1454";
	else if( currChara == "GBX" )
		ret = "1023";

	else if( currChara == "CHF" )
		ret = "1092";
	else if( currChara == "NOK" )
		ret = "1076";
	else if( currChara == "DKK" )
		ret = "1072";
	else if( currChara == "SEK" )
		ret = "1079";

	else if( currChara == "100YEN" )
		ret = "1091";
	else if( currChara == "HKD" )
		ret = "1050";
	else if( currChara == "AUD" )
		ret = "1040";
	else if( currChara == "100KRW" )
		ret = "1100";
	else if( currChara == "SGD" )
		ret = "1052";

	return ret;
}

vector<string> base_names( const vector<string>& tickers )
{
	vector<string> temp;
	for( auto ticker : tickers )
	{
		int size = ticker.size();
		if( size > 2 && ticker.find(":") == 1 )
			temp.push_back(ticker.substr(2, size - 2));
		else
			temp.push_back(ticker);
	}
	return temp;
}

string base_name( const string& ticker )
{
	string ret = ticker;
	int size = ticker.size();
	if( size > 2 && ticker.find(":") == 1 )
		ret = ticker.substr(2, size - 2);

	return ret;
}

char ExecFeesPrimex(const string& model, const string& ticker)
{
	char primex = '\0';
	if(model[0] == 'U')
		primex = 'N';
	else if(model[0] == 'C')
		primex = 'J';
	else if(ticker.find(":") != std::string::npos)
		return primex = ticker[0];
	return primex;
}

vector<string> comp_ticker( const vector<string>& tickers, const string& market )
{
	vector<string> ret;

	string code = mto::code(market);
	string reg = mto::region(market);
	for( string ticker : tickers )
	{
		if( mto::isInternational(market) )
		{
			if( ticker.find(":") == string::npos && !isECN_EU(market) )
				ticker = code + ":" + ticker;
		}
		else
		{
			ticker = base_name(ticker);
		}
		ret.push_back(ticker);
	}

	return ret;
}

vector<string> comp_ticker( const vector<string>& tickers, const string& market, int idate )
{
	vector<string> ret;

	string code = mto::code(market);
	string reg = mto::region(market);
	for( string ticker : tickers )
	{
		if( idate >= 20090706 )
		{
			if( mto::isInternational(market) && ticker.find(":") == string::npos && !isECN_EU(market) )
				ticker = code + ":" + ticker;
		}
		ret.push_back(ticker);
	}

	return ret;
}

vector<string> get_univ_tickers( const string& market, int idate )
{
	vector<string> ret;
	int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();
	string cmd = (string)" select uniqueID, market from stockCharacteristics "
		+ " where " + mto::selChara(market, idate_prev)
		+ " and inUniverse = 1 and uniqueID is not null ";
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);

	for(vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string uid = trim((*it)[0]);
		string code = trim((*it)[1]);

		string primary = "";
		if( mto::region(market) == "U" )
			primary = "US";
		else if( mto::region(market) == "C" )
			primary = "CA";
		else
			primary = mto::region(market) + code;

		GTH::Instance()->init(primary);
		string ticker = GTH::Instance()->get(primary)->UniqueToTicker(uid, idate);
		if( !ticker.empty() )
			ret.push_back(mto::compTicker(ticker, primary, idate));
	}
	sort(ret.begin(), ret.end());
	return ret;
}

vector<string> get_univ_tickers( const string& market, int idate1, int idate2 )
{
	vector<string> ret;
	int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate1, 040000, mto::tz(market))).Date();
	string cmd = (string)" select distinct uniqueID, market from stockCharacteristics "
		+ " where " + mto::selChara(market, idate_prev, idate2)
		+ " and inUniverse = 1 and uniqueID is not null ";
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);

	for(vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string uid = trim((*it)[0]);
		string code = trim((*it)[1]);

		string primary = "";
		if( mto::region(market) == "U" )
			primary = "US";
		else if( mto::region(market) == "C" )
			primary = "CA";
		else
			primary = mto::region(market) + code;

		GTH::Instance()->init(primary);
		string ticker = GTH::Instance()->get(primary)->UniqueToTicker(uid, idate2);
		if( !ticker.empty() )
			ret.push_back(mto::compTicker(ticker, primary, idate2));
	}
	sort(ret.begin(), ret.end());
	return ret;
}

vector<string> get_univ_tickers(const string& model2, int idate, const vector<int>& univ)
{
	string market;
	string selMarket;
	if( model2[0] == 'U' )
		market = "US";
	else
	{
		market = model2;
		selMarket = "exchange = '";
		selMarket += mto::code(market);
		selMarket += "'";
	}

	string selUniv = "inuniverse in (";
	for( auto it = begin(univ); it != end(univ); ++it )
	{
		if( it == univ.begin() )
			selUniv += itos(*it);
		else
		{
			selUniv += ",";
			selUniv += itos(*it);
		}
	}
	selUniv += ")";

	char cmdChara[2000];
	char cmdHfu[2000];
	if( model2 == "US" )
		sprintf( cmdHfu, "select symbol from hfuniverse where idate = %d and inuniverse > 0", idate );
	else
		sprintf( cmdHfu, "select symbol from hfuniverse where idate = %d and %s and %s", idate, selMarket.c_str(), selUniv.c_str());

	vector<string> tickers;
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmdHfu, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		if( !ticker.empty() )
			tickers.push_back(ticker);
	}
	return tickers;
}

void get_close( map<string, double>& m, const string& market, int idate)
{
	m.clear();
	vector<vector<string> > vvcl;
	{
		string cmd = (string)" select " + mto::compTicker(market) + ", close_ "
			+ " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate);
		GODBC::Instance()->read(mto::hf(market), cmd, vvcl);
	}
	for( auto it = vvcl.begin(); it != vvcl.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
		{
			double close = atof((*it)[1].c_str());
			m.insert( make_pair(symbol, close) );
		}
	}
}

void check_and_delete(const string& dbname, const string& chk, const string& cmd)
{
	vector<vector<string> > vv;
	GODBC::Instance()->get(dbname)->ReadTable(chk, &vv);
	if( atoi(vv[0][0].c_str()) > 0 )
		GODBC::Instance()->exec(dbname, cmd);
}

void check_and_insert_or_update(const string& dbname, const string& chk, const string& ins, const string& upt)
{
	vector<vector<string> > vv;
	GODBC::Instance()->read(dbname, chk, vv);
	if( atoi(vv[0][0].c_str()) > 0 ) // update.
		GODBC::Instance()->exec(dbname, upt);
	else // insert.
		GODBC::Instance()->exec(dbname, ins);
}

void check_and_exit(const string& dbname, const string& chk)
{
	vector<vector<string> > vv;
	GODBC::Instance()->read(dbname, chk, vv);
	if( atoi(vv[0][0].c_str()) > 0 )
	{
		cerr << "Implementation does not support overwritting the database " << dbname << ". Delete the entries and retry. (" << chk << ")\n";
		exit(5);
	}
}

vector<string> get_univ_uids( const string& market, int idate )
{
	vector<string> ret;
	int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(QuoteTime(idate, 040000, mto::tz(market))).Date();
	string cmd = (string)" select uniqueID from stockCharacteristics "
		+ " where " + mto::selChara(market, idate_prev)
		+ " and inUniverse = 1 ";
	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);

	for(auto it = vv.begin(); it != vv.end(); ++it )
	{
		string uid = trim((*it)[0]);
		ret.push_back(uid);
	}
	sort(ret.begin(), ret.end());
	return ret;
}

SampleCounter::SampleCounter(int step)
	:step_(step),
	cnt_(0)
{
}

void SampleCounter::update()
{
	++cnt_;
}

bool SampleCounter::doSample()
{
	return (step_ > 0 && cnt_ % step_ == 0);
}

void TickersVolat::read(int idate)
{
	mTickerVolat_.clear();

	int prevIdate = prevClose(market_, idate);
	char cmd[1000];
	sprintf(cmd, "select ticker, medVolatility from stockcharacteristics "
			" where inuniverse = 1 and idate = %d ",
			prevIdate);
	vector<vector<string>> vv;
	GODBC::Instance()->read("hfstock", cmd, vv);

	double sum = 0.;
	int cnt = 0;
	for( auto& v : vv )
	{
		string ticker = trim(v[0]);
		float volat = atof(v[1].c_str());
		if( !ticker.empty() && volat > 0. )
		{
			mTickerVolat_[ticker] = volat;
			sum += volat;
			++cnt;
		}
	}
	meanVolat_ = sum / cnt;
}

float TickersVolat::getVolat(const string& ticker) const
{
	float ret = meanVolat_;
	auto it = mTickerVolat_.find(ticker);
	if( it != mTickerVolat_.end() )
	{
		ret = it->second;
	}
	return ret;
}

map<string, float> readChara(const string& market, int idate, const string& col)
{
	map<string, float> m;
	vector<vector<string> > vv;
	{
		string cmd = (string)" select " + mto::compTicker(market) + ", " + col
			+ " from stockCharacteristics "
			+ " where " + mto::selChara(market, idate);
		GODBC::Instance()->read(mto::hf(market), cmd, vv);
	}
	for( auto it = vv.begin(); it != vv.end(); ++it )
	{
		string symbol = trim((*it)[0]);
		if( !symbol.empty() )
		{
			double val = atof((*it)[1].c_str());
			m.insert( make_pair(symbol, val) );
		}
	}
	return m;
}

vector<string> primaries()
{
	vector<string> markets;
	markets.push_back("EA");
	markets.push_back("EB");
	markets.push_back("EI");
	markets.push_back("EP");
	markets.push_back("EF");
	markets.push_back("EL");
	markets.push_back("ED");
	markets.push_back("EM");
	markets.push_back("EZ");
	markets.push_back("EO");
	markets.push_back("EX");
	markets.push_back("EC");
	markets.push_back("EW");
	markets.push_back("EY");
	markets.push_back("AT");
	markets.push_back("AH");
	markets.push_back("AS");
	markets.push_back("AK");
	markets.push_back("AQ");
	markets.push_back("AW");
	markets.push_back("AG");
	markets.push_back("MJ");
	return markets;
}

vector<string> euEcns()
{
	return vector<string>{"ET", "EH", "EG"};
}

vector<string> caEcns()
{
	return vector<string>{"CC", "CH", "CO", "CP"};
}

vector<string> asEcns()
{
	return vector<string>{"AC", "AX", "AN"};
}

vector<string> ecns()
{
	vector<string> ret = euEcns();
	vector<string> ca = caEcns();
	vector<string> as = asEcns();
	ret.insert(end(ret), begin(ca), end(ca));
	ret.insert(end(ret), begin(as), end(as));
	return ret;
}

vector<string> usDests()
{
	return vector<string>{"UP", "UQ", "UN", "UC", "UD", "UJ", "UB", "UY"};
}

bool isUSDest(const string& market)
{
	vector<string> usDests = ::usDests();
	return find(begin(usDests),end(usDests), market) != end(usDests);
}

bool isECN( const string& market )
{
	vector<string> ecns = ::ecns();
	return find(begin(ecns), end(ecns), market) != end(ecns);
	//static set<string> ecns;
	//if( ecns.empty() )
	//{
	//	// Europe.
	//	ecns.insert("ET");
	//	ecns.insert("EH");
	//	ecns.insert("EG");

	//	// Canada.
	//	ecns.insert("CC");
	//	ecns.insert("CH");
	//	ecns.insert("CO");
	//	ecns.insert("CP");

	//	// US.
	//	ecns.insert("UP");
	//	ecns.insert("UQ");
	//	ecns.insert("UN");
	//	ecns.insert("UC");
	//	ecns.insert("UD");
	//	ecns.insert("UJ");
	//	ecns.insert("UB");
	//	ecns.insert("UY");

	//	// Asia.
	//	ecns.insert("AC");
	//	ecns.insert("AX");
	//	ecns.insert("AN");
	//}
	//return ecns.count(market);
}

bool isECN_EU( const string& market )
{
	vector<string> ecns = euEcns();
	return find(begin(ecns), end(ecns), market) != end(ecns);
	//static set<string> ecns;
	//if( ecns.empty() )
	//{
	//	ecns.insert("ET");
	//	ecns.insert("EH");
	//	ecns.insert("EG");
	//}
	//return ecns.count(market);
}

double volatility_n( const vector<double>& vMsec, const vector<double>& vBid, const vector<double>& vAsk,
		const string& market, int n, int idate)
{
	int msec1 = mto::msecOpen(market, idate);
	int msec2 = mto::msecClose(market, idate);
	double interval = (msec2 - msec1) / (double)n;

	vector<double> vt;
	for( int t = msec1; t <= msec2; t += interval )
		vt.push_back(t);
	double volat = volatility_session( vMsec, vBid, vAsk, vt );
	return volat;
}

double volatility_day( const vector<double>& vMsec, const vector<double>& vBid, const vector<double>& vAsk,
		const string& market, int idate )
{
	vector<pair<int, int> > vSessions = mto::sessions(market, idate);
	double interval = 234000.0; // 100th of US trading hour.

	double sumVolat2 = 0;
	for(auto it = vSessions.begin(); it != vSessions.end(); ++it )
	{
		int msec1 = it->first;
		int msec2 = it->second;
		vector<double> vt;
		for( int t = msec1; t <= msec2; t += interval )
			vt.push_back(t);
		double volat = volatility_session( vMsec, vBid, vAsk, vt );
		sumVolat2 += volat*volat;
	}
	return sqrt(sumVolat2);
}

double volatility_session( const vector<double>& vMsec, const vector<double>& vBid, const vector<double>& vAsk,
		const vector<double>& vt )
{
	// Find the latest mid price before each interval
	vector<double> vPrice(vt.size()-1);
	int vps = vPrice.size();
	int i = 0;
	unsigned vs = vMsec.size();
	for( unsigned p=0; p<vps; ++p )
	{
		vPrice[p] = -1;
		if( i > 0 )
			--i;

		while( i < vs && vMsec[i] <= vt[p+1] )
		{
			double bid = vBid[i];
			double ask = vAsk[i];
			double mid = get_mid( bid, ask );
			if( mid > 0 )
			{
				bool valid_spread = fabs(ask-bid)/mid < 0.05;
				if( ask - bid > 0 && valid_spread )
					vPrice[p] = mid;
			}
			++i;
		}
	}

	vector<double> vR; // rate
	for( unsigned p=1; p<vPrice.size(); ++p )
	{
		bool valid_price = vPrice[p-1] > 0 && vPrice[p] > 0;
		bool valid_change = fabs(vPrice[p] - vPrice[p-1])/vPrice[p-1] < 0.03;
		if( valid_change && valid_price )
			vR.push_back( (vPrice[p] - vPrice[p-1])/vPrice[p-1] );
	}

	double volatTime = vt.size() - 2;
	double volat2 = 0;
	int vRsize = vR.size();
	if( vs > 10 && vRsize > 0.8 * vps && volatTime > 0 )
	{
		double sumR = 0;
		for( int i=0; i<vRsize; ++i )
			sumR += vR[i];
		double avgR = sumR/volatTime;

		for( int i=0; i<vRsize; ++i )
			volat2 += (vR[i] - avgR)*(vR[i] - avgR);
	}

	return sqrt(volat2);
}

string sedol6to7(const string& sedol6)
{
	int weights[6] = {1, 3, 1, 7, 3, 9};
	int sum = 0;
	for( int i=0; i<6; ++i )
	{
		int d = -1;
		char c = sedol6.substr(i, 1)[0];
		if( c >= 48 && c <= 57 ) // '0' = 48, '9' = 57.
			d = c - 48;
		else if( c >= 66 && c <= 90 ) // 'B' = 66, 'Z' = 90, 'A' is never used. 'B' -> 11
			d = c - 55;
		else if( c >= 98 && c <= 122 ) // 'b' = 98, 'z' = 122. 'b' -> 11
			d = c - 87;

		if( d >= 0 )
			sum += d * weights[i];
		else
			return "";
	}

	int check_digit = ( 10 - (sum % 10) ) % 10;
	char char_check_digit = check_digit + 48;
	string sedol7 = sedol6 + char_check_digit;
	return sedol7;
}

void get_intersection( const vector<string>& v1, const vector<string>& v2, vector<string>& v3 )
{
	set<string> s1(begin(v1), end(v1));
	set<string> s2(begin(v2), end(v2));
	set<string> s3;
	set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(s3, s3.begin()) );
	v3 = vector<string>(begin(s3), end(s3));
	return;
}

void get_union( const vector<string>& v1, const vector<string>& v2, vector<string>& v3 )
{
	set<string> s1(begin(v1), end(v1));
	set<string> s2(begin(v2), end(v2));
	set<string> s3;
	set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(s3, s3.begin()) );
	v3 = vector<string>(begin(s3), end(s3));
	return;
}

void get_ma(vector<double>& ma, const vector<double>& v, int n)
{
	ma.clear();
	int N = v.size();
	auto vBegin = v.begin();
	for( int i=0; i<N; ++i )
	{
		int offset1 = max(0, i - n + 1);
		int offset2 = min(N, i + 1);
		double sum = accumulate(vBegin + offset1, vBegin + offset2, 0.0);
		double mean = sum / (offset2 - offset1);
		ma.push_back(mean);
	}
	return;
}

string paste_symbol_string_sql( const vector<string>& vect_symbol, const vector<string>& vect_var, const string& var_type )
{
	int num = vect_symbol.size();
	if( num != vect_var.size() )
		cerr << "jlutil.cpp::paste_symbol_string_sql: two input arguments have different size." << endl;
	string paste;
	for( int i = 0; i != num; ++i )
	{
		string var = quote(vect_var[i]);
		if( var_type == "string")
			var = quote(vect_var[i]);
		else if( var_type == "numeric")
			var = vect_var[i];
		else
			cerr << "jlutil::paste_symbol_string_sql: var_type is not recognized." << endl;
		string row = "when symbol = " + quote(vect_symbol[i]) + " then " + var + " ";
		paste += row;
	}
	if( paste.size() > 0 )
		paste.erase( paste.size()-1 );
	return paste;
}

string paste_key_value_sql( const vector<string>& vect_key, const vector<string>& vect_value, 
						   const string& key_name, const string& value_type )
{
	/*
	This constructs a long string something like
	"when <key_name> = xxx then yyy when <key_name> = zzz then kkk ..."
	*/
	int num = vect_key.size();
	if( num != vect_value.size() )
		cerr << "jlutil.cpp::paste_symbol_string_sql: two input vectors have different size." << endl;
	string paste;
	for( int i = 0; i != num; ++i )
	{
		string row = "";
		if( !vect_key[i].empty() && vect_key[i] != "NULL" && !vect_value[i].empty() && vect_value[i] != "NULL" )
		{
			string value = quote(vect_value[i]);
			if( value_type == "string")
				value = quote(vect_value[i]);
			else if( value_type == "numeric")
				value = vect_value[i];
			else
				cerr << "jlutil::paste_symbol_string_sql: value_type is not recognized." << endl;
			row = "when " + key_name + " = " + quote(vect_key[i]) + " then " + value + " ";
		}
		paste += row;
	}
	if( paste.size() > 0 )
		paste.erase( paste.size()-1 );
	return paste;
}

Acc::Acc()
:sum(0), sum2(0), n(0)
{}

Acc::Acc(const vector<float>& vv, const vector<int>& vi)
:sum(0), sum2(0), n(0)
{
	for( int indx : vi )
	{
		float x = vv[indx];
		sum += x;
		sum2 += x * x;
		++n;
	}
}

void Acc::add(double x)
{
	sum += x;
	sum2 += x*x;
	++n;
	return;
}

double Acc::mean() const
{
	return (n > 0)? sum / n: 0;
}

double Acc::var() const
{
	return (n > 0) ? sum2 / n - (sum / n) * (sum / n): 0;
}

double Acc::stdev() const
{
	return (n > 0) ? sqrt(sum2 / n - (sum / n) * (sum / n)) : 0;
}

double Acc::RMS() const
{
	return (n > 0) ? sqrt(sum2 / n) : 0;
}

void Acc::clear()
{
	sum = 0;
	sum2 = 0;
	n = 0;
	return;
}

Acc& Acc::operator+=(const Acc &rhs)
{
	sum += rhs.sum;
	sum2 += rhs.sum2;
	n += rhs.n;
	return *this;
}

const Acc Acc::operator+(const Acc &rhs) const
{
	Acc result = *this;
	result += rhs;
	return result;
}

ostream& operator <<(ostream& os, const Acc& obj)
{
	os << obj.sum << "\t" << obj.sum2 << "\t" << obj.n << "\n";
	return os;
}

istream& operator >>(std::istream& is, Acc& obj)
{
	is >> obj.sum >> obj.sum2 >> obj.n;
	return is;
}

WAcc::WAcc()
:sum(0), sum2(0), wsum(0), n(0)
{}

void WAcc::add(double x, double w)
{
	sum += w*x;
	sum2 += w*x*x;
	wsum += w;
	++n;
	return;
}

double WAcc::mean() const
{
	return (wsum > 0)? sum / wsum: 0;
}

double WAcc::var() const
{
	return (wsum > 0) ? sum2 / wsum - (sum / wsum) * (sum / wsum): 0;
}

double WAcc::stdev() const
{
	return (wsum > 0) ? sqrt(sum2 / wsum - (sum / wsum) * (sum / wsum)) : 0;
}

double WAcc::RMS() const
{
	return (wsum > 0) ? sqrt(sum2 / wsum) : 0;
}

void WAcc::clear()
{
	sum = 0;
	sum2 = 0;
	wsum = 0;
	n = 0;
	return;
}

Corr::Corr(const vector<float>& v1, const vector<float>& v2, const vector<int>& vi)
{
	for( int indx : vi )
	{
		float x = v1[indx];
		float y = v2[indx];
		accX.add(x);
		accY.add(y);
		accXY.add(x * y);
	}
}

void Corr::add(double x, double y)
{
	accX.add(x);
	accY.add(y);
	accXY.add(x * y);
	return;
}

double Corr::cov() const
{
	double cov = accXY.mean() - accX.mean() * accY.mean();
	return cov;
}

double Corr::corr() const
{
	double cov = accXY.mean() - accX.mean() * accY.mean();
	double ret = 0;
	double stdevX = accX.stdev();
	double stdevY = accY.stdev();
	if( stdevX > 0 && stdevY > 0 )
		ret = cov / (stdevX * stdevY);
	return ret;
}

void Corr::clear()
{
	accX.clear();
	accY.clear();
	accXY.clear();
	return;
}

Corr& Corr::operator+=(const Corr &rhs)
{
	accX += rhs.accX;
	accY += rhs.accY;
	accXY += rhs.accXY;
	return *this;
}

const Corr Corr::operator+(const Corr &rhs) const
{
	Corr result = *this;
	result += rhs;
	return result;
}

ostream& operator <<(ostream& os, const Corr& obj)
{
	os << obj.accX << obj.accY << obj.accXY;
	return os;
}

istream& operator >>(std::istream& is, Corr& obj)
{
	is >> obj.accX >> obj.accY >> obj.accXY;
	return is;
}

CorrInfo::CorrInfo(int nInputs)
{
	vCorr = vector<Corr>(nInputs);
	vvCorr = vector<vector<Corr> >(nInputs);
	for( int i = 0; i < nInputs; ++i )
		vvCorr[i] = vector<Corr>(i + 1);
	vAcc = vector<Acc>(nInputs);
}

void CorrInfo::clear()
{
	for( vector<Corr>::iterator it = vCorr.begin(); it != vCorr.end(); ++it )
		it->clear();
	for( vector<vector<Corr> >::iterator it1 = vvCorr.begin(); it1 != vvCorr.end(); ++it1 )
		for( vector<Corr>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2 )
			it2->clear();
	for( vector<Acc>::iterator it = vAcc.begin(); it != vAcc.end(); ++it )
		it->clear();
	accY.clear();
}

void CorrInfo::add(const vector<float>& input, float target)
{
	int N = input.size();
	for( int i = 0; i < N; ++i )
	{
		vCorr[i].add( input[i], target );
		for( int j = 0; j <= i; ++j )
			vvCorr[i][j].add( input[i], input[j] );
		vAcc[i].add( input[i] );
	}
	accY.add( target );
}

void EconVal::addBottom(double target, double pred)
{
	accTargetBottom.add(target);
	accPredBottom.add(pred);
	return;
}

void EconVal::addTop(double target, double pred)
{
	accTargetTop.add(target);
	accPredTop.add(pred);
	return;
}

double EconVal::econVal()
{
	double ret = (accTargetTop.mean() - accTargetBottom.mean()) / 2.0;
	return ret;
}

double EconVal::bias()
{
	double biasTop = accPredTop.mean() - accTargetTop.mean();
	double biasBottom = accPredBottom.mean() - accTargetBottom.mean();
	double ret = (biasTop - biasBottom) / 2.0;
	return ret;
}

double EconVal::malpred()
{
	double ret = (accPredTop.mean() - accPredBottom.mean()) / 2.0;
	return ret;
}

void EconVal::clear()
{
	accTargetBottom.clear();
	accPredBottom.clear();
	accTargetTop.clear();
	accPredTop.clear();
	return;
}

void CharaProfile::read(const string& market, int idate, const string& condVar, int nP, const string& symbolType)
{
	vector<string> markets;
	markets.push_back(market);
	read(markets, idate, condVar, nP, symbolType);
}

void CharaProfile::read(const vector<string>& markets, int idate, const string& condVar, int nP, const string& symbolType)
{
	// clear.
	v.clear();
	mSymbolIndx.clear();

	string symbolCol;
	if( symbolType == "ticker" )
	{
		char buf[100];
		sprintf(buf, "%s", mto::ts(markets[0]).c_str());
		symbolCol = (string)buf;
	}
	else if( symbolType == "uid" )
	{
		symbolCol = "uniqueID";
	}

	int indx = 0;
	//for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
	for( string market : markets )
	{
		//string market = *it;

		// read db.
		char cmd[2000];
		if( market == "US" || market == "CJ" )
		{
			sprintf(cmd, "select %s, %s "
				" from stockcharacteristics "
				" where %s and uniqueID is not null and inuniverse = 1 "
				" and sectype != 'P' and sectype != 'F' and sectype != 'X' ",
				symbolCol.c_str(),
				condVar.c_str(),
				mto::selChara(market, idate).c_str());
		}
		else
		{
			sprintf(cmd, "select %s, %s "
				" from stockcharacteristics "
				" where %s and uniqueID is not null and inuniverse = 1 ",
				symbolCol.c_str(),
				condVar.c_str(),
				mto::selChara(market, idate).c_str());
		}
		vector<vector<string> > vv;
		GODBC::Instance()->read(mto::hf(market), cmd, vv);

		for( auto it = vv.begin(); it != vv.end(); ++it )
		{
			string symbol = trim((*it)[0]);
			if( !symbol.empty() )
			{
				float var = atof((*it)[1].c_str());
				mSymbolIndx[symbol] = indx++;
				v.push_back(var);
			}
		}
	}

	int N = v.size();
	int d = ceil((double)N / nP);

	vector<int> tempIndx(N);
	gsl_heapsort_index<int, float>(tempIndx, v);

	vIndx.resize(N);
	for( int i = 0; i < N; ++i )
		vIndx[tempIndx[i]] = i / d;
}

int CharaProfile::get_indx(const string& symbol)
{
	return vIndx[mSymbolIndx[symbol]];
}

void printMemoryInfoSimple()
{
	cout << getMemoryInfoSimple() << endl;
}

string getMemoryInfoSimple()
{
	static size_t mem_prev = 0;
	size_t mem_now = getCurrentRSS();
	double fmem_change = ((double)mem_now - (double)mem_prev) / 1024. / 1024.;
	double fmem_now = mem_now / 1024. / 1024.;
	mem_prev = mem_now;

	size_t mem_peak = getPeakRSS();
	double fmem_peak = mem_peak / 1024. / 1024.;

	char buf[200];
    sprintf(buf, "Mem %.1fM(%+.1f) peak %.1fM", fmem_now, fmem_change, fmem_peak);
	return buf;
}

string getTimerInfoSimple()
{
	const static auto start = std::chrono::system_clock::now();
	static auto prev = start;
	auto end = std::chrono::system_clock::now();
	auto sinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	auto sincePrev = std::chrono::duration_cast<std::chrono::milliseconds>(end - prev);
	prev = end;

	std::chrono::hours hh = std::chrono::duration_cast<std::chrono::hours>(sinceStart);
	std::chrono::minutes mm = std::chrono::duration_cast<std::chrono::minutes>(sinceStart % chrono::hours(1));
	std::chrono::seconds ss = std::chrono::duration_cast<std::chrono::seconds>(sinceStart % chrono::minutes(1));
	std::chrono::milliseconds msec = std::chrono::duration_cast<std::chrono::milliseconds>(sinceStart % chrono::seconds(1));

	std::chrono::minutes mmSincePrev = std::chrono::duration_cast<std::chrono::minutes>(sincePrev);
	std::chrono::seconds ssSincePrev = std::chrono::duration_cast<std::chrono::seconds>(sincePrev % chrono::minutes(1));

	char buf[200];
	sprintf(buf, "Timer %02d:%02d:%02d(%+02d:%02d)", hh.count(), mm.count(), ss.count(), mmSincePrev.count(), ssSincePrev.count());
	return buf;
}

void PrintMemoryInfoSimple()
{
	static int mem_prev = 0;
	int mem_now = getCurrentRSS();
	int mem_peak = getPeakRSS();
	int mem_change = mem_now - mem_prev;
	char buf[200];
    sprintf(buf, "Mem current: %.1f M, change: %.1f M, peak: %.1f M\n",
			mem_now / 1024. / 1024., mem_change / 1024. / 1024., mem_peak / 1024. / 1024.);
	cout << buf << flush;
}

unsigned long get_pid()
{
	unsigned long pid = 0;
#ifdef _WIN32
	pid = GetCurrentProcessId();
#else
	pid = getpid();
#endif
	return pid;
}

int change_timezone( int msecs, const string& tzFrom, const string& tzTo, int idate )
{
	int ret = msecs;
	double diff = (QuoteTime(idate, 120000, tzFrom) - QuoteTime(idate, 120000, tzTo)).DHHMMSS();
	int dhh = (int)(diff * 100) % 100;

	int input_hhmmss = msecs / 1000 / 60 / 60 * 10000 + msecs / 1000 / 60 % 60 * 100 + msecs / 1000 % 60;
	int MMM = msecs%1000;
	QuoteTime input_qt(idate, input_hhmmss, tzFrom.c_str());

	double converted = (input_qt + RealTime(dhh / 24.0)).Date();
	int ret_hhmmss = ceil( (converted - (int)converted)*1000000 - 0.5 );

	int hh = ret_hhmmss / 10000 % 100;
	int mm = ret_hhmmss / 100 % 100;
	int ss = ret_hhmmss % 100;
	ret = hh*60*60*1000 + mm*60*1000 + ss*1000 + MMM;

	return ret;
}

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#pragma comment(lib, "psapi.lib")
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t getPeakRSS( )
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
    return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
    /* AIX and Solaris ------------------------------------------ */
    struct psinfo psinfo;
    int fd = -1;
    if ( (fd = open( "/proc/self/psinfo", O_RDONLY )) == -1 )
        return (size_t)0L;      /* Can't open? */
    if ( read( fd, &psinfo, sizeof(psinfo) ) != sizeof(psinfo) )
    {
        close( fd );
        return (size_t)0L;      /* Can't read? */
    }
    close( fd );
    return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage( RUSAGE_SELF, &rusage );
#if defined(__APPLE__) && defined(__MACH__)
    return (size_t)rusage.ru_maxrss;
#else
    return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L;          /* Unsupported. */
#endif
}

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS( )
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
    return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------ */
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
        (task_info_t)&info, &infoCount ) != KERN_SUCCESS )
        return (size_t)0L;      /* Can't access? */
    return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE* fp = NULL;
    if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
        return (size_t)0L;      /* Can't open? */
    if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
    {
        fclose( fp );
        return (size_t)0L;      /* Can't read? */
    }
    fclose( fp );
    return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE); // _SC_PAGESIZE = size of page in bytes

#else
    /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
    return (size_t)0L;          /* Unsupported. */
#endif
}


