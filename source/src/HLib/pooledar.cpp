#include <HLib/PooledAR.h>
#include <iostream>
using namespace std;

PooledAR::PooledAR(int filterLen, bool scale)
:filterLen_(filterLen),
scale_(scale),
acc_(0)
{
	ar_ = AR(filterLen_);
}

void PooledAR::add(string ticker, double v)
{
	mTickerAcc_[ticker].add(v);
	return;
}

void PooledAR::add(int lag, double v0, double v1)
{
	if( tickerStdev_ > 0 )
	{
		double s0 = (v0 - tickerMean_) / tickerStdev_;
		double s1 = (v1 - tickerMean_) / tickerStdev_;
		ar_.add(lag, s0, s1);
	}
	return;
}

vector<double> PooledAR::getCoeffs()
{
	return ar_.getCoeffs();
}

void PooledAR::beginTicker(const string& ticker)
{
	if( !mTickerAcc_.empty() )
		calculate_mean();

	if( mTickerMean_.count(ticker) )
	{
		tickerMean_ = mTickerMean_[ticker];
		tickerStdev_ = mTickerStdev_[ticker];
	}
	else
	{
		double zero = 0;
		tickerMean_ = 1.0 / zero;
		tickerStdev_ = 1.0 / zero;
	}

	return;
}

void PooledAR::endTicker(const string& ticker)
{
	double zero = 0;
	tickerMean_ = 1.0 / zero;
	tickerStdev_ = 1.0 / zero;
	return;
}

void PooledAR::calculate_mean()
{
	for( map<string, Acc>::iterator it = mTickerAcc_.begin(); it != mTickerAcc_.end(); ++it )
	{
		mTickerMean_[it->first] = it->second.mean();
		if( scale_ )
			mTickerStdev_[it->first] = it->second.stdev();
		else
			mTickerStdev_[it->first] = 1.0;
	}
	mTickerAcc_.clear();
	return;
}

double PooledAR::pred(string ticker, vector<double>& v)
{
	double mean = mTickerMean_[ticker];
	double stdev = mTickerStdev_[ticker];
	double y = ar_.pred(v);
	return mean + y * stdev;
}

ostream& operator <<(ostream& os, const PooledAR& obj)
{
	os << obj.ar_;

	for( map<string, double>::const_iterator it = obj.mTickerMean_.begin(); it != obj.mTickerMean_.end(); ++it )
	{
		double stdev = obj.mTickerStdev_.find(it->first)->second;
		os << it->first << "\t" << it->second << "\t" << stdev << "\t";
	}
	os << endl;

	return os;
}

istream& operator >>(std::istream& is, PooledAR& obj)
{
	is >> obj.ar_;

	string ticker;
	double mean;
	double stdev;
	while( is >> ticker )
	{
		if( is >> mean >> stdev )
		{
			obj.mTickerMean_[ticker] = mean;
			obj.mTickerStdev_[ticker] = stdev;
		}
	}

	return is;
}
