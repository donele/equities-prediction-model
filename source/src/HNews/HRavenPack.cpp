#include <HNews.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <optionlibs/TickData.h>
#include <map>
#include <jl_lib.h>
#include <string>
#include "TFile.h"
using namespace std;

HRavenPack::HRavenPack(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
verbose_(0),
input_dir_("\\\\smrc-nas09\\gf1\\tickR\\news\\E\\")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("input_file") )
		input_file_ = conf.find("input_file")->second;
	if( conf.count("outdir") )
		outdir_ = conf.find("outdir")->second;
}

HRavenPack::~HRavenPack()
{}

void HRavenPack::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	assert_loopingOrder_dmt();

	return;
}

void HRavenPack::beginDay()
{
	int idate = HEnv::Instance()->idate();

	lines_.clear();
	if( !temp_line_.empty() )
		lines_.push_back(temp_line_);
	temp_line_ = "";

	if( !ifs_.is_open() )
	{
		char filename[200];
		int yyyy = idate / 10000;
		int mm = idate / 100 % 100;
		sprintf(filename, "%s\\%d\\%d-%02d-equities.csv", input_dir_.c_str(), yyyy, yyyy, mm);
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

void HRavenPack::beginMarket()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	if( verbose_ > 1 )
		cout << market << "\t" << idate << "\n";

	string country = mto::country(market);
	vector<string> tickers = get_tickers(market, idate);
	map<string, TickSeries<QuoteInfo> >& mTs = mmTs_[market];

	QuoteTime nowUTC = QuoteTime(idate, 0, "UTC") - RealTime( 1/(24.0*60*60) );
	QuoteTime nextOpenUTC = GEX::Instance()->get(market)->NextOpen(nowUTC);
	QuoteTime nextCloseUTC = GEX::Instance()->get(market)->NextClose(nowUTC);

	map<string, int> mTickerNotMatched;

	int msecsFirstData = 24*60*60*1000;
	for( vector<string>::iterator it = lines_.begin(); it != lines_.end(); ++it )
	{
		string line = *it;
		vector<string> sl = splitN(line, ',');

		// time.
		int msecs;
		QuoteTime lineTimeUTC = get_lineTime(sl[0]);
		if( lineTimeUTC > nextCloseUTC ) // write tickdata.
		{
			write_tickdata(mTs, market, nextCloseUTC, msecsFirstData);

			msecsFirstData = 24*60*60*1000;
			nextOpenUTC = GEX::Instance()->get(market)->NextOpen(lineTimeUTC);
			nextCloseUTC = GEX::Instance()->get(market)->NextClose(lineTimeUTC);
		}

		// country.
		string lineCountry = sl[27].substr(0, 2);
		if( lineCountry == country )
		{
			// ticker.
			string ticker = get_ticker(sl[27], market, idate);

			bool preOpen = nextOpenUTC < nextCloseUTC && lineTimeUTC < nextOpenUTC;
			int msecs = preOpen? get_msecs(nextOpenUTC, market): get_msecs(lineTimeUTC, market);
			if( msecs_valid(market, msecs, sl) )
			{
				if( msecs < msecsFirstData )
					msecsFirstData = msecs;

				if( binary_search(tickers.begin(), tickers.end(), ticker) )
				{
					if( preOpen ) // pre open.
						add_data(mTs, ticker, msecs - 1, sl);
					else if( lineTimeUTC <= nextCloseUTC ) // during trading hours.
						add_data(mTs, ticker, msecs, sl);
					else // error.
						cerr << "HRavenPack::beginMarket() ERROR.\n";
				}
				else
					++mTickerNotMatched[ticker];
			}
		}
	}

	for( map<string, int>::iterator it = mTickerNotMatched.begin(); it != mTickerNotMatched.end(); ++it )
	{
		if( it->second > 5 )
			printf( " ticker %s appeared %d times but not matched.\n", it->first.c_str(), it->second );
	}

	return;
}

void HRavenPack::endMarket()
{
	return;
}

void HRavenPack::endDay()
{
	return;
}

void HRavenPack::endJob()
{
	return;
}

bool HRavenPack::msecs_valid(string market, int msecs, vector<string>& sl)
{
	bool valid = true;
	if( "U" == mto::region(market) || "C" == mto::region(market) )
	{
		if( (msecs >= 56400000 && msecs <= 56520000) || (msecs >= 57000000 && msecs <= 57300000) )
		{
			int rel = atoi(sl[3].c_str());
			if( rel != 100 )
				valid = false; // filter out the close imbalance reports.
		}
	}

	return valid;
}

int HRavenPack::get_idate(string& line)
{
	int idate = 0;
	if( line.size() > 10 )
		idate = atoi(line.substr(0,4).c_str())*10000 + atoi(line.substr(5,2).c_str())*100 + atoi(line.substr(8,2).c_str());
	return idate;
}

vector<string> HRavenPack::get_tickers(string market, int idate)
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

string HRavenPack::get_ticker(string sl1, string market, int idate)
{
	int sl1_size = sl1.size();
	string ticker = sl1.substr(3, sl1_size - 3);

	if( ("EX" == market || "CA" == market) && sl1.find(".V") != string::npos )
	{
		//int size = ticker.size();
		//ticker = ticker.substr(0, size - 2);
	}
	else if( "AH" == market )
	{
		ticker = HK_ticker(ticker, idate);
	}
	else if( "EC" == market || "EW" == market )
	{
		ticker = QHtoBloomberg(market, ticker);
	}
	return ticker;
}

QuoteTime HRavenPack::get_lineTime(string& t)
{
	int idate = get_idate(t);
	int hhmmss = atoi( (t.substr(11,2) + t.substr(14,2) + t.substr(17,2)).c_str() );
	int mmm = atoi( t.substr(20,3).c_str() );
	QuoteTime lineTime(idate, hhmmss, "UTC");
	lineTime += RealTime( mmm/86400000.0 );
	return lineTime;
}

void HRavenPack::write_tickdata(map<string, TickSeries<QuoteInfo> >& mTs, string market, QuoteTime nextCloseUTC, int msecsFirstData)
{
	nextCloseUTC.SetTimeZone(mto::tz(market));
	int idate = (int)nextCloseUTC.Date();
	if( msecsFirstData < mto::msecOpen(market, idate) + 60*60*1000 ) // Need to update
	{
		string outdir = "";
		if( !outdir_.empty() )
			outdir = outdir_ + "\\" + market;
		else if( "US" == market )
			outdir = "\\\\smrc-nas09\\gf1\\tickC\\us\\news";
		else if( "C" == mto::region(market) )
			outdir = "\\\\smrc-nas09\\gf1\\tickC\\ca\\news";
		else if( mto::isInternational(market) )
			outdir = "\\\\smrc-nas09\\gf1\\tickC\\" + mto::region_long(market) + "\\news\\" + mto::code(market);

		if( !outdir.empty() )
		{
			TickStorage<QuoteInfo> tsQ;
			map<string, TickSeries<QuoteInfo> >::iterator itQ;
			for(itQ = mTs.begin(); itQ != mTs.end(); ++itQ)
			{
				string ticker = itQ->first;
				string compTicker = mto::compTicker(ticker, market, idate);
				tsQ.Import(compTicker, itQ->second);
			}
			if( verbose_ >= 1 )
				cout << "writing at " << outdir << "\n";
			tsQ.WriteBIN(idate, outdir, mto::longTicker(market));
			mTs.clear();
		}
	}
	return;
}

int HRavenPack::get_msecs(QuoteTime qt, string market)
{
	qt.SetTimeZone(mto::tz(market));
	int idate = (int)qt.Date();
	QuoteTime midnight(idate, 0, mto::tz(market));
	int msecs = ROUND(86400000. * (qt - midnight).Days());
	return msecs;
}

void HRavenPack::add_data(map<string, TickSeries<QuoteInfo> >& mTs, string ticker, int msecs, vector<string>& sl)
{
	if( false )
	{
		int rel = atoi(sl[3].c_str()); // Relevance. Over 75 is considered significantly relevant.
		int cat = newsCatRP_.cat2i(sl[4]);
		int css = atoi(sl[5].c_str()); // Composite Sentiment Score. 0 to 100.
		int wle = atoi(sl[6].c_str());
		int pcm = atoi(sl[7].c_str());
		int ecm = atoi(sl[8].c_str());
		int rcm = atoi(sl[9].c_str());
		int vcm = atoi(sl[10].c_str());
		int nip = atoi(sl[11].c_str()); // News Impact Projection. 0 to 100. 50 means  unable to determine if the story will have any impact.
		int mean = (wle + pcm + ecm + rcm + vcm) / 5.0;

		QuoteInfo quote;
		quote.msecs = msecs;
		quote.bidEx = rel;
		quote.bidSize = css;
		quote.bid = mean;
		quote.ask = nip;
		quote.askSize = 0;
		quote.askEx = cat;
		quote.quflags = 0;

		map<string, TickSeries<QuoteInfo> >::iterator it_symbol = mTs.find(ticker);
		if( it_symbol == mTs.end() ) // new symbol
		{
			mTs[ticker] = TickSeries<QuoteInfo>(3, 1.1);
			it_symbol = mTs.find(ticker);
		}
		it_symbol->second.Write(quote);
	}
	else if( true ) // v 3.0
	{
		int rel = atoi(sl[5].c_str()); // relevance.
		int cat = newsCatRP_.cat2i(sl[11]); // Category.
		int ens = atoi(sl[18].c_str()); // novelty. (G_ENS)
		int ess = atoi(sl[12].c_str()); // sentiment. (ESS)
		int aes = atoi(sl[13].c_str()); // aggregate event sentimet.
		int aev = atoi(sl[14].c_str()); // aggregate event volume.
		int isDJ = (sl[26].size() > 2 && sl[26].substr(0, 2) == "DJ") ? 1000 : 0;
		ess += isDJ;

		QuoteInfo quote;
		quote.msecs = msecs;
		quote.bidEx = rel; // char
		quote.bidSize = cat; // int
		quote.bid = aes; // float
		quote.ask = aev; // float
		quote.askSize = ess; // int
		quote.askEx = ens; // char

		map<string, TickSeries<QuoteInfo> >::iterator it_symbol = mTs.find(ticker);
		if( it_symbol == mTs.end() ) // new symbol
		{
			mTs[ticker] = TickSeries<QuoteInfo>(3, 1.1);
			it_symbol = mTs.find(ticker);
		}
		it_symbol->second.Write(quote);
	}

	return;
}