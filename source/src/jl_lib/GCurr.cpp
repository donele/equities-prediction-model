#include <jl_lib/GCurr.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/mto.h>
#include <string>
#include <vector>
using namespace std;

GCurr* GCurr::instance_ = 0;

GCurr* GCurr::Instance()
{
	if( instance_ == 0 )
		instance_ = new GCurr();
	return instance_;
}

GCurr::GCurr()
{}

void GCurr::set_idate(int idate)
{
	if( idate_ != idate )
	{
		idate_ = idate;
		vector<vector<string> > vvRet;
		string cmd = (string)"SELECT name, rate "
			+ " FROM currency"
			+ " where idate = (select max(idate) from currency where idate <=" + quote(itos(idate)) + ") ";
		GODBC::Instance()->get("equitydata")->ReadTable(cmd, &vvRet);

		for( vector<vector<string> >::iterator it = vvRet.begin(); it != vvRet.end(); ++it )
		{
			string curr = trim( (*it)[0] );
			if( !curr.empty() )
			{
				double rate = atof( (*it)[1].c_str() );
				m_[curr] = 1./ rate;
			}
		}
	}
	return;
}

double GCurr::convert(const string& marketTo, const string& marketFrom, double price)
{
	string currFrom = mto::currISO(marketFrom);
	string currTo = mto::currISO(marketTo);
	double ret = price;
	if( currFrom != currTo )
	{
		ret = m_[currTo] / m_[currFrom] * price;
		if( marketFrom == "EL" || marketFrom == "MJ" )
			ret /= 100.0;
		else if( marketFrom == "AK" || marketFrom == "AQ" || marketFrom == "AT" )
			ret *= 100.0;
	}
	return ret;
}

double GCurr::exchrat(const string& marketTo, const string& marketFrom)
{
	string currFrom = mto::currISO(marketFrom);
	string currTo = mto::currISO(marketTo);
	double ret = 1.;
	if( currFrom != currTo )
	{
		ret = m_[currTo] / m_[currFrom];
		if( marketFrom == "EL" || marketFrom == "MJ" )
			ret /= 100.0;
		else if( marketFrom == "AK" || marketFrom == "AQ" || marketFrom == "AT" )
			ret *= 100.0;
	}
	return ret;
}
