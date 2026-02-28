#include <HLib/QProfile.h>
#include <algorithm>
#include <jl_lib/jlutil.h>
//#include <jl_lib.h>
using namespace std;

QProfile::QProfile()
{}

QProfile::QProfile(string name, string title, double min1, double max1, double min2, double max2)
:min1_(min1),
max1_(max1),
min2_(min2),
max2_(max2),
name_(name),
title_(title)
{}

void QProfile::Fill(float f1, float f2)
{
	if( (min1_ > max1_ || (min1_ <= f1 && f1 <= max1_)) && (min2_ > max2_ || (min2_ <= f2 && f2 <= max2_)) )
		v_.push_back(make_pair(f1, f2));
}

TProfile QProfile::getObject(int nbins, int nbinsGraph, string mode)
{
	sort(v_.begin(), v_.end());
	int nEntries = v_.size();
	int interval = nEntries / nbins;

	// mean of x of each bin.
	vector<Acc> vAcc(nbins);
	{
		int n = 0;
		for( vector<pair<float, float> >::iterator it = v_.begin(); it != v_.end(); ++it, ++n )
		{
			int indx = min(n / interval, nbins - 1);
			vAcc[indx].add(it->first);
		}
	}

	// bin center.
	vector<float> binC;
	for( vector<Acc>::iterator it = vAcc.begin(); it != vAcc.end(); ++it )
		binC.push_back(it->mean());

	// Create an object.
	double pLow = binC[0];
	double pHigh = binC[nbins-1];
	if( mode == "log" )
	{
		pLow = log(pLow) - 0.5;
		pHigh = log(pHigh) + 0.5;
	}
	else if( mode == "linear" )
	{
		double margin = (binC[nbins-1] - binC[0]) * 0.1;
		pLow -= margin;
		pHigh += margin;
	}
	else if( mode == "eq" )
	{
		pLow = 0;
		pHigh = nbins;
		nbinsGraph = nbins;
	}
	TProfile p(name_.c_str(), title_.c_str(), nbinsGraph, pLow, pHigh);

	// Fill the object.
	int n = 0;
	for( vector<pair<float, float> >::iterator it = v_.begin(); it != v_.end(); ++it, ++n )
	{
		int indx = min(n / interval, nbins - 1);
		double x = binC[indx];
		if( mode == "log" )
			x = log(x);
		else if( mode == "eq" )
			x = indx;
		p.Fill(x, it->second);
	}

	// X-axis label.
	int indx = 0;
	for( vector<float>::iterator it = binC.begin(); it != binC.end(); ++it, ++indx )
	{
		double binCenter = *it;
		double x = binCenter;
		if( mode == "log" )
			x = log(x);
		else if( mode == "eq" )
			x = indx;

		for( int i=1; i<=nbinsGraph; ++i )
		{
			if( p.GetXaxis()->GetBinLowEdge(i) >= x )
			{
				char label[100];
				if( binCenter > 10 )
					sprintf(label, "%.0f", binCenter);
				else
					sprintf(label, "%.1f", binCenter);
				p.GetXaxis()->SetBinLabel(i, label);
				break;
			}
		}
	}
	return p;
}
