#include <jl_lib/CrossStat.h>
#include "optionlibs/TickData.h"
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <vector>
#include <numeric>
#include <string>
#include <algorithm>
using namespace std;

CrossStat::CrossStat()
:crossed_(false),
lenCross_(0),
max_cross_(0),
sum_cross_(0),
afterOpen_skip_(15*60*1000),
afterBreak_skip_(5*60*1000),
n_cross_(0),
there_was_a_valid_quote_(false)
{}

CrossStat::CrossStat(const string& market, int idate)
:crossed_(false),
lenCross_(0),
max_cross_(0),
sum_cross_(0),
afterOpen_skip_(15*60*1000),
afterBreak_skip_(5*60*1000),
n_cross_(0),
there_was_a_valid_quote_(false)
{
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
	vBreaks_ = mto::breaks(market, afterBreak_skip_);
}

void CrossStat::reset(const string& market, int idate, bool there_was_a_valid_quote)
{
	lenCross_ = 0;
	max_cross_ = 0;
	sum_cross_ = 0;
	n_cross_ = 0;
	there_was_a_valid_quote_ = false;
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
	vBreaks_ = mto::breaks(market, afterBreak_skip_);
	there_was_a_valid_quote_ = there_was_a_valid_quote;
}

void CrossStat::beginDay()
{
	beginTicker();
	vector<int>().swap(vMaxCross_);
	vector<int>().swap(vSumCross_);
	vector<int>().swap(vNCross_);
	return;
}

void CrossStat::beginTicker(bool there_was_a_valid_quote)
{
	crossed_ = false;
	lenCross_ = 0;
	vector<int>().swap(vLenCross_);
	vector<int>().swap(vCrossMsecs_);
	max_cross_ = 0;
	sum_cross_ = 0;
	n_cross_ = 0;
	there_was_a_valid_quote_ = there_was_a_valid_quote;

	return;
}

void CrossStat::endTicker(bool there_was_a_valid_quote)
{
	if( crossed_ )
	{
		vCrossMsecs_.push_back(crossMsecs_);
		crossMsecs_ = 0;
		vLenCross_.push_back(lenCross_);
	}

	max_cross_ = 0;
	sum_cross_ = 0;
	n_cross_ = 0;
	if( !vLenCross_.empty() )
	{
		vector<int> temp_vLenCross(vLenCross_);
		sort(temp_vLenCross.begin(), temp_vLenCross.end());
		max_cross_ = *(temp_vLenCross.rbegin());
		sum_cross_ = accumulate(temp_vLenCross.begin(), temp_vLenCross.end(), 0);
		n_cross_ = temp_vLenCross.size();
	}
	vMaxCross_.push_back(max_cross_);
	vSumCross_.push_back(sum_cross_);
	vNCross_.push_back(n_cross_);
	there_was_a_valid_quote_ = there_was_a_valid_quote;

	return;
}

void CrossStat::endDay()
{
	sort(vMaxCross_.begin(), vMaxCross_.end());
	sort(vSumCross_.begin(), vSumCross_.end());
	sort(vNCross_.begin(), vNCross_.end());

	return;
}

void CrossStat::add_quote(const QuoteInfo& quote)
{
	// cross
	int msecs = quote.msecs;
	if( msecs > msecOpen_ + afterOpen_skip_ && msecs < msecClose_ )
	{
		bool is_crossed = quote.bid >= quote.ask;
		bool is_break = find_if(vBreaks_.begin(), vBreaks_.end(), bind2nd(inRange(), msecs)) != vBreaks_.end();

		if( !there_was_a_valid_quote_ && !is_crossed && !is_break )
			there_was_a_valid_quote_ = true;

		if( there_was_a_valid_quote_ )
		{
			if( !crossed_ && is_crossed && !is_break ) // new cross
			{
				crossed_ = true;
				lenCross_ = 0;
				crossMsecs_ = msecs;
			}
			else if( crossed_ && is_crossed && !is_break ) // continuing cross
			{
				lenCross_ += msecs - lastMsecs_;
			}
			else if( crossed_ && (!is_crossed || is_break) ) // end of cross
			{
				lenCross_ += msecs - lastMsecs_;
				vCrossMsecs_.push_back(crossMsecs_);
				crossMsecs_ = 0;
				vLenCross_.push_back(lenCross_);

				lenCross_ = 0;
				crossed_ = false;
			}
		}
	}
	lastMsecs_ = msecs;
	return;
}

int CrossStat::get_max_cross()
{
	return max_cross_;
}

int CrossStat::get_sum_cross()
{
	return sum_cross_;
}

int CrossStat::get_n_cross()
{
	return n_cross_;
}

int CrossStat::get_n_crossed_tickers()
{
	vector<int>::iterator uBound = upper_bound(vNCross_.begin(), vNCross_.end(), 0);
	int ret = distance(uBound, vNCross_.end());
	return ret;
}

void CrossStat::get_eff_maxCross( int& n, vector<double>& x, vector<double>& y )
{
	get_eff(n, x, y, vMaxCross_);
	return;
}

void CrossStat::get_eff_sumCross( int& n, vector<double>& x, vector<double>& y )
{
	get_eff(n, x, y, vSumCross_);
	return;
}

void CrossStat::get_eff( int& n, vector<double>& x, vector<double>& y, vector<int>& v )
{
	x.clear();
	y.clear();
	vector<double> w;
	for(vector<int>::iterator it = v.begin(); it != v.end(); ++it )
		w.push_back(*it);
	vector<double>::iterator w1 = upper_bound(w.begin(), w.end(), 0.1);
	x = vector<double>(w1, w.end());

	int n_symbol = v.size();
	int n_symbol_lost = x.size();
	for( vector<double>::iterator it = x.begin(); it != x.end(); ++it )
	{
		double eff = (n_symbol - (--n_symbol_lost)) / (double)n_symbol;
		if( y.size() < 5 || (*it) < 3600 || eff < 0.99 )
			y.push_back(eff);
		else
			break;
	}
	n = y.size();

	return;
}

void CrossStat::print_cross(ostream& os)
{
	char buf[400];
	sprintf(buf, "%5d crosses, %15.3f%15.3f", n_cross_, max_cross_/1000.0, sum_cross_/1000.0);
	os << buf << endl;

	for( int i=0; i<n_cross_; ++i )
	{
		os << vCrossMsecs_[i] << "\t" << vLenCross_[i] << endl;
	}

	return;
}
