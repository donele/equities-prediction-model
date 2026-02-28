#include <jl_lib/OneSecRet.h>
#include <optionlibs/TickData.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <string>
#include <map>
#include <algorithm>
using namespace std;

OneSecRet::OneSecRet()
{}

OneSecRet::OneSecRet(const string& market, int idate)
:market_(market),
idate_(idate),
msecOpen_(mto::msecOpen(market, idate)),
msecClose_(mto::msecClose(market, idate))
{}

OneSecRet::~OneSecRet()
{}

void OneSecRet::add_quote(string symbol, const QuoteInfo& quote)
{
	int msecs = quote.msecs;
	if( msecs > msecOpen_ && msecs < msecClose_ )
	{
		double bid = quote.bid;
		double ask = quote.ask;

		if( "CL" == market_ )
			symbol = symbol.substr(0,3) + "_RET";

		mvMsec_[symbol].push_back(msecs);
		mvBid_[symbol].push_back(bid);
		mvAsk_[symbol].push_back(ask);
	}

	return;
}

void OneSecRet::calculate_each_return()
{
	// Divide a day into 1 second intervals.
	vector<int> vTime;
	int msec = msecOpen_;
	while( msec <= msecClose_ )
	{
		vTime.push_back(msec);
		msec += 1000;
	}
	int vts = vTime.size();
	vector<double> vPrice(vts-1);
	int vps = vPrice.size();

	for( auto it = begin(mvMsec_); it != end(mvMsec_); ++it )
	{
		string symbol = it->first;
		unsigned vs = mvMsec_[symbol].size();
		vector<double>& vMsec = mvMsec_[symbol];
		vector<double>& vBid = mvBid_[symbol];
		vector<double>& vAsk = mvAsk_[symbol];

		// Find latest mid prices before each interval
		unsigned ii = 0;
		for( unsigned p=0; p<vps; ++p )
		{
			vPrice[p] = -1;
			if( ii > 0 )
				--ii;

			while( ii < vs && vMsec[ii] <= vTime[p+1] )
			{
				double bid = vBid[ii];
				double ask = vAsk[ii];

				double mid = get_mid( bid, ask );
				if( mid > 0 )
				{
					// cross (ask < bid) is allowed.
					bool valid_spread = fabs( (ask - bid)/mid ) < 0.05;
					if( valid_spread )
						vPrice[p] = mid;
				}
				++ii;
			}
		}

		vector<double> temp_vRet;
		temp_vRet.reserve( vts );
		temp_vRet.push_back(0); // market opening
		temp_vRet.push_back(0); // 1 second after market opening
		bool valid_change = false;
		for( unsigned p=1; p<vps; ++p )
		{
			double ret = 0;
			bool valid_price = vPrice[p-1] > 0 && vPrice[p] > 0 && vPrice[p] != vPrice[p-1];
			if( valid_price )
			{
				ret = (vPrice[p] - vPrice[p-1])/vPrice[p-1];
				valid_change = fabs( ret ) < 0.02;

				if( !valid_change )
					ret = 0;
			}
			temp_vRet.push_back( ret );
			double temp = *(temp_vRet.rbegin());
		}
		mvRet_[symbol] = temp_vRet;

		vector<double>().swap(vMsec);
		vector<double>().swap(vBid);
		vector<double>().swap(vAsk);
	}

	mvMsec_.clear();
	mvBid_.clear();
	mvAsk_.clear();

	return;
}


void OneSecRet::calculate_each_return_paneu()
{
	for( auto it = begin(mvMsec_); it != end(mvMsec_); ++it )
	{
		string symbol = it->first;
		unsigned vs = mvMsec_[symbol].size();
		vector<double>& vMsec = mvMsec_[symbol];
		vector<double>& vBid = mvBid_[symbol];
		vector<double>& vAsk = mvAsk_[symbol];

		// Divide a day into 1 second intervals. Market dependent.
		string market = (string)"E" + symbol.substr(0, 1);
		int msecOpen = mto::msecOpen(market, idate_);
		int msecClose = mto::msecClose(market, idate_);
		vector<int> vTime;
		int msec = msecOpen;
		while( msec <= msecClose )
		{
			vTime.push_back(msec);
			msec += 1000;
		}
		int vts = vTime.size();
		vector<double> vPrice(vts-1);
		int vps = vPrice.size();

		// Find latest mid prices before each interval
		unsigned ii = 0;
		for( unsigned p=0; p<vps; ++p )
		{
			vPrice[p] = -1;
			if( ii > 0 )
				--ii;

			while( ii < vs && vMsec[ii] <= vTime[p+1] )
			{
				double bid = vBid[ii];
				double ask = vAsk[ii];

				double mid = get_mid( bid, ask );
				if( mid > 0 )
				{
					// cross (ask < bid) is allowed.
					bool valid_spread = fabs( (ask - bid)/mid ) < 0.05;
					if( valid_spread )
						vPrice[p] = mid;
				}
				++ii;
			}
		}

		vector<double> temp_vRet;
		temp_vRet.reserve( vts );
		temp_vRet.push_back(0); // market opening
		temp_vRet.push_back(0); // 1 second after market opening
		bool valid_change = false;
		for( unsigned p=1; p<vps; ++p )
		{
			double ret = 0;
			bool valid_price = vPrice[p-1] > 0 && vPrice[p] > 0 && vPrice[p] != vPrice[p-1];
			if( valid_price )
			{
				ret = (vPrice[p] - vPrice[p-1])/vPrice[p-1];
				valid_change = fabs( ret ) < 0.02;

				if( !valid_change )
					ret = 0;
			}
			temp_vRet.push_back( ret );
			double temp = *(temp_vRet.rbegin());
		}
		mvRet_[symbol] = temp_vRet;

		vector<double>().swap(vMsec);
		vector<double>().swap(vBid);
		vector<double>().swap(vAsk);
	}

	mvMsec_.clear();
	mvBid_.clear();
	mvAsk_.clear();

	return;
}

void OneSecRet::calculate_average_return(const set<string>& symbols)
{
	vAvgSecRet_.clear();
	vAvgSecRetSig_.clear();
	int n = (msecClose_ - msecOpen_)/1000;
	for( int i=0; i<=n; ++i )
	{
		//double totRet = 0;
		//int univSize = 0;
		Acc acc;
		for( map<string, vector<double> >::iterator it = mvRet_.begin(); it != mvRet_.end(); ++it )
		{
			string symbol = it->first;
			if( (symbols.empty() || symbols.count(symbol)) && i < it->second.size() )
			{
				//++univSize;
				//totRet += (it->second)[i];
				acc.add((it->second)[i]);
			}
		}
		//double avgSecRet = 0;
		//if( univSize != 0 )
		//	avgSecRet = totRet/(double)univSize;

		vAvgSecRet_.push_back( acc.mean() );
		vAvgSecRetSig_.push_back( (acc.stdev() > 1e-10)? acc.mean() / acc.stdev(): 0. );
	}

	return;
}

void OneSecRet::insert_each_return( TickStorage<ReturnData>& tsR )
{
	ReturnData rd;
	for( map<string, vector<double> >::iterator it = mvRet_.begin(); it != mvRet_.end(); ++it )
	{
		string symbol = it->first;
		vector<double>& vRet = it->second;
		int msecs = msecOpen_;
		for( vector<double>::iterator it2 = vRet.begin(); it2 != vRet.end(); ++it2 )
		{
			rd.msecs = msecs;
			rd.ret = *it2;
			tsR.Insert(symbol.c_str(), rd);
			msecs += 1000;
		}
		vector<double>().swap(vRet);
	}
	mvRet_.clear();
	return;
}

void OneSecRet::insert_return( const string& symbol, TickSeries<ReturnData>& ts )
{
	ReturnData rd;

	vector<double>& vRet = mvRet_[symbol];
	int msecs = msecOpen_;
	for( vector<double>::iterator it2 = vRet.begin(); it2 != vRet.end(); ++it2 )
	{
		rd.msecs = msecs;
		rd.ret = *it2;
		ts.Write(rd);
		msecs += 1000;
	}

	return;
}

void OneSecRet::insert_average_return( TickStorage<ReturnData>& tsR, const string& symbol )
{
	ReturnData rd;
	int msecs = msecOpen_;
	for( vector<double>::iterator it = vAvgSecRet_.begin(); it != vAvgSecRet_.end(); ++it )
	{
		rd.msecs = msecs;
		rd.ret = *it;
		tsR.Insert(symbol.c_str(), rd);
		msecs += 1000;
	}

	return;
}

void OneSecRet::insert_average_return( TickSeries<ReturnData>& tsR )
{
	ReturnData rd;
	int msecs = msecOpen_;
	for( vector<double>::iterator it = vAvgSecRet_.begin(); it != vAvgSecRet_.end(); ++it )
	{
		rd.msecs = msecs;
		rd.ret = *it;
		tsR.Write(rd);
		msecs += 1000;
	}

	return;
}

void OneSecRet::insert_average_return_sig( TickSeries<ReturnData>& tsR )
{
	ReturnData rd;
	int msecs = msecOpen_;
	for( vector<double>::iterator it = vAvgSecRetSig_.begin(); it != vAvgSecRetSig_.end(); ++it )
	{
		rd.msecs = msecs;
		rd.ret = *it;
		tsR.Write(rd);
		msecs += 1000;
	}

	return;
}
