#include <HNews.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <optionlibs/TickData.h>
#include <map>
#include <jl_lib.h>
//#include <jl_lib/GODBC.h>
//#include <jl_lib/mto.h>
//#include <jl_lib/jlutil.h>
#include <string>
#include "TFile.h"
using namespace std;

HNewsScope::HNewsScope(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
verbose_(0),
input_dir_("\\\\smrc-nas09\\gf1\\tickR\\NewsScope\\data")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("input_file") )
		input_file_ = conf.find("input_file")->second;
	if( conf.count("outdir") )
		outdir_ = conf.find("outdir")->second;
}

HNewsScope::~HNewsScope()
{}

void HNewsScope::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	assert_loopingOrder_dmt();

	mExtMarket_["N"] = "US";
	mExtMarket_["O"] = "US";
	mExtMarket_["TO"] = "CJ";
	mExtMarket_["V"] = "CJ";
	mExtMarket_["AS"] = "EA";
	mExtMarket_["BR"] = "EB";
	mExtMarket_["LS"] = "EI";
	mExtMarket_["PA"] = "EP";
	mExtMarket_["F"] = "EF";
	mExtMarket_["L"] = "EL";
	mExtMarket_["MA"] = "ED";
	mExtMarket_["MI"] = "EM";
	mExtMarket_["S"] = "EZ";
	mExtMarket_["OL"] = "EO";
	mExtMarket_["VX"] = "EX";
	mExtMarket_["CO"] = "EC";
	mExtMarket_["ST"] = "EW";
	mExtMarket_["HE"] = "EY";
	mExtMarket_["T"] = "AT";
	mExtMarket_["HK"] = "AH";
	mExtMarket_["AX"] = "AS";
	mExtMarket_["TW"] = "AW";
	mExtMarket_["TWO"] = "AW";

	ifstream ifs("\\\\smrc-nas09\\gf1\\tickR\\NewsScope\\data\\dss-sample2.csv");
	string line = "";
	while( getline(ifs, line) )
	{
		vector<string> sl = splitN(line, ',');
		string ticker = sl[0];
		string ric = sl[7];
		if( ric.find(".OQ") != string::npos )
		{
			int n = ric.size();
			ric = ric.substr(0, n-1);
		}
		mRICticker_[ric] = ticker;
	}

	return;
}

void HNewsScope::beginDay()
{
	int idate = HEnv::Instance()->idate();

	lines_.clear();
	if( !temp_line_.empty() )
		lines_.push_back(temp_line_);
	temp_line_ = "";

	if( !ifs_.is_open() )
	{
		char filename[200];
		int yyyymm1 = 0;
		int yyyymm2 = 0;
		if( idate >= 20080101 && idate <= 20081231 )
		{
			yyyymm1 = 200801;
			yyyymm2 = 200812;
		}
		else if( idate >= 20090101 && idate <= 20090731 )
		{
			yyyymm1 = 200901;
			yyyymm2 = 200907;
		}
		sprintf(filename, "%s\\RNSE.%d-%d.EQU.txt", input_dir_.c_str(), yyyymm1, yyyymm2);
		//sprintf(filename, "%s\\20080102.txt", input_dir_.c_str());
		ifs_.open(filename);
	}

	string line = "";
	while( getline(ifs_, line) )
	{
		int line_idate = get_idate(line);
		if( line_idate < idate )
			continue;
		else if( line_idate == idate )
			lines_.push_back(line);
		else if( line_idate > idate )
		{
			temp_line_ = line;
			break;
		}
	}

	if( ifs_.eof() )
	{
		ifs_.close();
		ifs_.clear();
	}

	return;
}

void HNewsScope::beginMarket()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	if( verbose_ > 1 )
		cout << market << "\t" << idate << "\n";

	map<string, TickSeries<QuoteInfo> >& mTs = mmTs_[market];

	QuoteTime nowUTC = QuoteTime(idate, 0, "UTC") - RealTime( 1/(24.0*60*60) );
	QuoteTime nextOpenUTC = GEX::Instance()->get(market)->NextOpen(nowUTC);
	QuoteTime nextCloseUTC = GEX::Instance()->get(market)->NextClose(nowUTC);

	map<string, int> mTickerNotMatched;

	int msecsFirstData = 24*60*60*1000;
	for( vector<string>::iterator it = lines_.begin(); it != lines_.end(); ++it )
	{
		string line = *it;
		vector<string> sl = splitN(line, '\t');

		int msecs;
		QuoteTime lineTimeUTC = get_lineTime(sl[0]);
		if( lineTimeUTC > nextCloseUTC ) // write tickdata.
		{
			write_tickdata(mTs, market, nextCloseUTC, msecsFirstData);

			msecsFirstData = 24*60*60*1000;
			nextOpenUTC = GEX::Instance()->get(market)->NextOpen(lineTimeUTC);
			nextCloseUTC = GEX::Instance()->get(market)->NextClose(lineTimeUTC);
		}

		string lineMarket = get_market(sl[2]);
		if( lineMarket == market )
		{
			string RIC = get_RIC(sl[1]);
			bool preOpen = nextOpenUTC < nextCloseUTC && lineTimeUTC < nextOpenUTC;
			int msecs = preOpen? get_msecs(nextOpenUTC, market): get_msecs(lineTimeUTC, market);

			if( msecs_valid(market, msecs) )
			{
				if( msecs < msecsFirstData )
					msecsFirstData = msecs;

				if( preOpen ) // pre open.
				{
					add_data(mTs, RIC, msecs - 1, sl);
				}
				else if( lineTimeUTC <= nextCloseUTC ) // during trading hours.
				{
					add_data(mTs, RIC, msecs, sl);
				}
				else // error.
				{
					cerr << "HNewsScope::beginMarket() ERROR.\n";
				}
			}
		}
	}

	return;
}

void HNewsScope::endMarket()
{
	return;
}

void HNewsScope::endDay()
{
	return;
}

void HNewsScope::endJob()
{
	return;
}

bool HNewsScope::msecs_valid(string market, int msecs)
{
	bool valid = true;
	if( "U" == mto::region(market) || "C" == mto::region(market) )
	{
		if( (msecs >= 56400000 && msecs <= 56580000) || (msecs >= 57000000 && msecs <= 57360000) )
		{
			valid = false; // filter out the close imbalance reports.
		}
	}

	return valid;
}

void HNewsScope::split_newsScope(vector<string>& sl, string& line)
{
	static const int N = 14;
	static int lengths[N] = {24, 10, 14, 64, 17, 15, 17, 17, 17, 15, 15, 15, 15, 15};

	int offset = 0;
	for( int i=0; i<N; ++i )
	{
		int len = lengths[i];
		sl.push_back(line.substr(offset, len));
		offset += len;
	}
	return;
}

int HNewsScope::get_idate(string& line)
{
	int idate = 0;
	if( line.size() > 10 )
		idate = atoi(line.substr(7,4).c_str() )*10000 + MMMtomm(line.substr(3,3))*100 + atoi(line.substr(0,2).c_str());
	return idate;
}

vector<string> HNewsScope::get_tickers(string market, int idate)
{
	QuoteTime today(idate, 040000, mto::tz(market));
	int idate_prev = (int)GEX::Instance()->get(market)->PrevClose(today).Date();
	string selChara = mto::selChara(market, idate_prev);
	if( market == "Ak" )
		selChara = " idate = " + itos(idate_prev) + " and (market = 'AK' or market = 'AQ') ";

	string cmd = (string)" select " + mto::ts(market) + " from stockcharacteristics "
		+ " where " + mto::selChara(market, idate_prev)
		+ " and " + mto::selType(market);
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
	vector<string> tickers;
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		tickers.push_back(ticker);
	}
	sort(tickers.begin(), tickers.end());
	return tickers;
}

string HNewsScope::get_RIC(string sl1)
{
	string RIC = trim(sl1);
	return RIC;
}

QuoteTime HNewsScope::get_lineTime(string& t)
{
	int idate = get_idate(t);
	int hhmmss = atoi( (t.substr(12,2) + t.substr(15,2) + t.substr(18,2)).c_str() );
	int mmm = atoi( t.substr(21,3).c_str() );
	QuoteTime lineTime(idate, hhmmss, "UTC");
	lineTime += RealTime( mmm/86400000.0 );
	return lineTime;
}

string HNewsScope::get_market(string& s)
{
	if( !s.empty() && s.find(".") != string::npos )
	{
		vector<string> sl = split(s, '.');
		if( sl.size() == 2 )
		{
			string ext = sl[1];
			string market = mExtMarket_[ext];
			return market;
		}
	}
	return "";
}

void HNewsScope::write_tickdata(map<string, TickSeries<QuoteInfo> >& mTs, string market, QuoteTime nextCloseUTC, int msecsFirstData)
{
	nextCloseUTC.SetTimeZone(mto::tz(market));
	int idate = (int)nextCloseUTC.Date();
	if( msecsFirstData < mto::msecOpen(market, idate) + 60*60*1000 )
	{
		string outdir = "";
		if( !outdir_.empty() )
			outdir = outdir_ + "\\" + market;
		else if( "US" == market )
			outdir = "\\\\smrc-nas10\\l\\tickC\\us\\newsRT";
		else if( "C" == mto::region(market) )
			outdir = "\\\\smrc-ltc-mrct16\\data00\\tickC\\ca\\newsRT";
		else if( mto::isInternational(market) )
			outdir = "\\\\smrc-ltc-mrct16\\data00\\tickC\\" + mto::region_long(market) + "\\newsRT\\" + mto::code(market);

		if( !outdir.empty() )
		{
			vector<string> tickers = get_tickers(market, idate);

			TickStorage<QuoteInfo> tsQ;
			map<string, TickSeries<QuoteInfo> >::iterator itQ;
			for(itQ = mTs.begin(); itQ != mTs.end(); ++itQ)
			{
				string RIC = itQ->first;
				string ticker = mRICticker_[RIC];

				if( binary_search(tickers.begin(), tickers.end(), ticker) )
				{
					string compTicker = mto::compTicker(ticker, market, idate);
					tsQ.Import(compTicker, itQ->second);
				}
			}
			if( verbose_ >= 1 )
				cout << "writing at " << outdir << "\n";
			tsQ.WriteBIN(idate, outdir, mto::longTicker(market));
			mTs.clear();
		}
	}
	return;
}

int HNewsScope::get_msecs(QuoteTime qt, string market)
{
	qt.SetTimeZone(mto::tz(market));
	int idate = (int)qt.Date();
	QuoteTime midnight(idate, 0, mto::tz(market));
	int msecs = ROUND(86400000. * (qt - midnight).Days());
	return msecs;
}

void HNewsScope::add_data(map<string, TickSeries<QuoteInfo> >& mTs, string RIC, int msecs, vector<string>& sl)
{
	double rel = atof(sl[4].c_str());
	double sent = atof(sl[5].c_str());

	int cnt1 = atoi(sl[10].c_str());
	int cnt2 = atoi(sl[11].c_str());
	int cnt3 = atoi(sl[12].c_str());
	int cnt4 = atoi(sl[13].c_str());
	int cnt5 = atoi(sl[14].c_str());

	//if( cnt1 == 0 && cnt2 == 0 && cnt3 == 0 && cnt4 == 0 && cnt5 == 0 )
	{
		QuoteInfo quote;
		quote.msecs = msecs;
		quote.bid = rel;
		quote.bidSize = sent;
		quote.bidEx = cnt1;
		quote.ask = cnt2;
		quote.askSize = cnt3;
		quote.askEx = cnt4;
		quote.quflags = 0;

		map<string, TickSeries<QuoteInfo> >::iterator it_symbol = mTs.find(RIC);
		if( it_symbol == mTs.end() ) // new symbol
		{
			mTs[RIC] = TickSeries<QuoteInfo>(3, 1.1);
			it_symbol = mTs.find(RIC);
		}
		it_symbol->second.Write(quote);
	}

	return;
}