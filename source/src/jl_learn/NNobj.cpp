#include <jl_learn/NNobj.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <algorithm>
#include <functional>
using namespace std;

ostream& operator <<(ostream& os, NNinputInfo& obj)
{
	os << obj.n << endl;

	char buf[100];
	for( int i=0; i<obj.n; ++i )
	{
		sprintf(buf, "%15.10f\t", obj.mean[i]);
		os << buf;
	}
	os << endl;
	for( int i=0; i<obj.n; ++i )
	{
		sprintf(buf, "%15.10f\t", obj.stdev[i]);
		os << buf;
	}
	os << endl;

	return os;
}

NNsample::NNsample(const NNsample& ns)
{
	input = ns.input;
	target = ns.target;
	quote = ns.quote;
}

NNinputInfo::NNinputInfo(const NNinputInfo& nni)
{
	n = nni.n;
	mean = nni.mean;
	stdev = nni.stdev;
}

NNinputInfo::NNinputInfo(string filename)
{
	ifstream ifs(filename.c_str());
	ifs >> n;
	mean = vector<double>(n);
	stdev = vector<double>(n);
	for( int i = 0; i < n; ++i )
		ifs >> mean[i];
	for( int i = 0; i < n; ++i )
		ifs >> stdev[i];
}

void NNperformance::add(double v)
{
	++bufn;
	buf1 += v;
	return;
}

void NNperformance::add(double v, double s)
{
	++bufn;
	buf1 += v;
	buf2 += s;
	return;
}

double NNperformance::get_n()
{
	return n;
}

double NNperformance::get_sum2()
{
	return sum2;
}

double NNperformance::endofdaysum()
{
	double ret = buf1;

	sum1 += buf1;
	buf1 = 0;
	return ret;
}

double NNperformance::endofdayerr()
{
	double ret = 0;
	if( fabs(bufn) > 1e-100 )
		ret = sqrt(buf1 / bufn);

	n += bufn;
	sum1 += buf1;
	bufn = 0;
	buf1 = 0;
	return ret;
}

double NNperformance::endofdayavg()
{
	double ret = 0;
	if( fabs(bufn) > 1e-100 )
		ret = buf1 / bufn;

	n += bufn;
	sum1 += buf1;
	bufn = 0;
	buf1 = 0;
	return ret;
}

double NNperformance::endofdayrat()
{
	double ret = 0;
	if( fabs(buf2) > 1e-100 )
		ret = buf1 / buf2 * 10000;

	sum1 += buf1;
	sum2 += buf2;
	buf1 = 0;
	buf2 = 0;
	return ret;
}

double NNperformance::resultsum()
{
	double ret = sum1;
	clear();
	return ret;
}

double NNperformance::resulterr()
{
	double ret = 0;
	if( fabs(n) > 1e-100 )
		ret = sqrt(sum1 / n);
	clear();
	return ret;
}

double NNperformance::resultavg()
{
	double ret = 0;
	if( fabs(n) > 1e-100 )
		ret = sum1 / n;
	clear();
	return ret;
}

double NNperformance::resultrat()
{
	double ret = 0;
	if( fabs(sum2) > 1e-100 )
		ret = sum1 / sum2 * 10000;
	clear();
	return ret;
}
