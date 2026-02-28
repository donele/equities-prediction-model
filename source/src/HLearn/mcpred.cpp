#include <HLearn/MCPred.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <jl_lib/GTH.h>
using namespace std;

MCPred::MCPred()
{}

MCPred::MCPred(string pred_dir, string market, int idate)
:market_(market),
idate_(idate),
N_((mto::msecClose(market, idate) - mto::msecOpen(market, idate))/1000)
{
	char path[200];
	sprintf(path, "%s\\%d.txt", pred_dir.c_str(), idate);
	ifstream ifs(path);

	if( ifs.is_open() )
	{
		vector<double> vPred1m(N_, 0);
		vector<double> vPred40m(N_, 0);

		string uid = "";
		string line = "";
		while( !ifs.eof() )
		{
			getline(ifs, line);
			vector<string> v = split(line);
			if( v.size() == 6 )
			{
				if( uid != v[0] )
				{
					if( !uid.empty() )
						flush_pred(uid, vPred1m, vPred40m);
					uid = v[0];
				}
				add_line(v, vPred1m, vPred40m);
			}
		}
	}

	GTH::Instance()->init(market);
}

void MCPred::flush_pred(string uid, vector<double>& vPred1m, vector<double>& vPred40m)
{
	string ticker = mto::compTicker(GTH::Instance()->get(market_)->UniqueToTicker(uid, idate_), market_);
	mTickerPred1m_[ticker] = vPred1m;
	mTickerPred40m_[ticker] = vPred40m;
	vPred1m = vector<double>(N_, 0);
	vPred40m = vector<double>(N_, 0);
	return;
}

void MCPred::add_line(vector<string>& v, vector<double>& vPred1m, vector<double>& vPred40m)
{
	int n = atoi(v[1].c_str()) / 1000 / 60 - 1;
	if( n >= 0 && n < N_ )
	{
		double pred1m = atof(v[2].c_str());
		double pred40m = atof(v[4].c_str());
		vPred1m[n] = pred1m;
		vPred40m[n] = pred40m;
	}
	return;
}

double MCPred::get_pred1m(std::string ticker, int msecs)
{
	return get_pred(mTickerPred1m_, ticker, msecs);
}

double MCPred::get_pred40m(std::string ticker, int msecs)
{
	return get_pred(mTickerPred40m_, ticker, msecs);
}

double MCPred::get_pred41m(std::string ticker, int msecs)
{
	return get_pred(mTickerPred1m_, ticker, msecs) + get_pred(mTickerPred40m_, ticker, msecs);
}

double MCPred::get_pred(std::map<std::string, std::vector<double> >& m, std::string ticker, int msecs)
{
	double ret = 1e6;
	int n = ( msecs - mto::msecOpen(market_, idate_) ) / 1000 /60 - 1;
	map<string, vector<double> >::iterator it = m.find(ticker);
	if( it != m.end() )
		ret = it->second[n];

	return ret;
}

