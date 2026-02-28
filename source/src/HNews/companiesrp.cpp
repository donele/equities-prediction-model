#include<HNews.h>
#include<jl_lib.h>
using namespace std;

CompaniesRP::CompaniesRP()
{}

CompaniesRP::CompaniesRP(string market)
:market_(market),
country_(mto::country(market))
{
	ifstream ifs("\\\\smrc-ltc-mrct16\\data00\\tickC\\eu\\RIC\\RavenPack_companies_v.1.4.csv");
	string line;
	while( getline(ifs, line) )
	{
		vector<string> sl = splitN(line, ',');
		int lineSize = sl.size();
		if( lineSize >= 8 && sl[0] != "RP_COMPANY_ID" )
		{
			string country = sl[5];
			if( country == country_ )
			{
				if( country_ == "CA" || country_ == "US" )
				{
					string company_id = sl[0];
					string country_ticker = sl[4];
					int country_ticker_size = country_ticker.size();
					if( country_ticker_size > 3 )
					{
						string ticker = country_ticker.substr(3, country_ticker_size - 3);
						mIdTicker_[company_id] = ticker;
					}
				}
				else
				{
					string company_id = sl[0];
					string isin = sl[3];
					mIdIsin_[company_id] = isin;
				}
			}
		}
	}
}

string CompaniesRP::ticker(string company_id)
{
	if( country_ == "CA" || country_ == "US" )
	{
		map<string, string>::iterator it = mIdTicker_.find(company_id);
		if( it != mIdTicker_.end() )
		{
			string ticker = it->second;
			return ticker;
		}
	}
	else
	{
		map<string, string>::iterator it = mIdIsin_.find(company_id);
		if( it != mIdIsin_.end() )
		{
			string isin = it->second;
			map<string, string>::iterator it2 = mIsinTicker_.find(isin);
			if( it2 != mIsinTicker_.end() )
			{
				string ticker = mto::compTicker(it2->second, market_);
				return ticker;
			}
		}
	}
	return "";
}

void CompaniesRP::beginDay(int idate)
{
	if( country_ != "CA" && country_ != "US" )
	{
		char cmd[400];
		sprintf(cmd, "select isin, symbol from stockcharacteristics \
					 where market = '%s' \
					 and idate = (select max(idate) from stockcharacteristics where market = '%s' and idate <= %d)",
					 mto::code(market_).c_str(),
					 mto::code(market_).c_str(),
					 idate);
		vector<vector<string> > vv;
		GODBC::Instance()->get(mto::hf(market_))->ReadTable(cmd, &vv);
		if( !vv.empty() )
		{
			mIsinTicker_.clear();
			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string isin = trim((*it)[0]);
				string symbol = trim((*it)[1]);
				if( !isin.empty() && !symbol.empty() )
					mIsinTicker_.insert(make_pair(isin, symbol));
			}
		}
	}
	return;
}