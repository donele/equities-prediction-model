#include <jl_lib/stat_util.h>
#include <math.h>
#include <algorithm>
#include <numeric>
using namespace std;

float mean(const vector<float>& v)
{
	float ret = 0.;
	if( !v.empty() )
	{
		vector<double> w(v.begin(), v.end());
		ret = mean(w);
	}
	return ret;
}

double mean(const vector<double>& v)
{
	float ret = 0.;
	if( !v.empty() )
		ret = accumulate(v.begin(), v.end(), 0.) / v.size();
	return ret;
}

float variance(const vector<float>& v)
{
	float ret = 0.;
	if( !v.empty() )
	{
		vector<double> v2(v.begin(), v.end());

		vector<double> w(v.size());
		transform(v2.begin(), v2.end(), w.begin(), sqr);
		double mean2 = mean(w);
		double mean0 = mean(v2);
		ret = mean2 - mean0 * mean0;
	}
	if( ret < 0. )
		ret = 0.;
	return ret;
}

double variance(const vector<double>& v)
{
	double ret = 0.;
	if( !v.empty() )
	{
		vector<double> w(v.size());
		transform(v.begin(), v.end(), w.begin(), sqr);
		double mean2 = mean(w);
		double mean0 = mean(v);
		ret = mean2 - mean0 * mean0;
	}
	if( ret < 0. )
		ret = 0.;
	return ret;
}

float stdev(const vector<float>& v)
{
	double var = variance(v);
	if( var > 0. )
		return sqrt(var);
	return 0.;
}

double stdev(const vector<double>& v)
{
	double var = variance(v);
	if( var > 0. )
		return sqrt(var);
	return 0.;
}

float corr(const vector<float>& v1, const vector<float>& v2)
{
	float ret = 0.;
	int size1 = v1.size();
	int size2 = v2.size();
	if( size1 > 0 && size1 == size2 )
	{
		// Mean of the product.
		double sumProd = 0.;
		auto itv1 = v1.begin();
		auto itv2 = v2.begin();
		auto itv1End = v1.end();
		for( ; itv1 != itv1End; ++itv1, ++itv2 )
			sumProd += *itv1 * *itv2;
		double meanProd = sumProd / size1;

		// stdev of v1 and v2.
		double stdev1 = stdev(v1);
		double stdev2 = stdev(v2);

		if( stdev1 > 0 && stdev2 > 0 )
		{
			double mean1 = mean(v1);
			double mean2 = mean(v2);
			ret = (meanProd - mean1 * mean2) / (stdev1 * stdev2);
		}
	}
	return ret;
}

double corr(const vector<double>& v1, const vector<double>& v2)
{
	float ret = 0.;
	int size1 = v1.size();
	int size2 = v2.size();
	if( size1 > 0 && size1 == size2 )
	{
		// Mean of the product.
		double sumProd = 0.;
		auto itv1 = v1.begin();
		auto itv2 = v2.begin();
		auto itv1End = v1.end();
		for( ; itv1 != itv1End; ++itv1, ++itv2 )
			sumProd += *itv1 * *itv2;
		double meanProd = sumProd / size1;

		// stdev of v1 and v2.
		double stdev1 = stdev(v1);
		double stdev2 = stdev(v2);

		if( stdev1 > 0 && stdev2 > 0 )
		{
			double mean1 = mean(v1);
			double mean2 = mean(v2);
			ret = (meanProd - mean1 * mean2) / (stdev1 * stdev2);
		}
	}
	return ret;
}

double sqr(double x)
{
	return x * x;
}
