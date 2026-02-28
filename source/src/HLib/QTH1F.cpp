#include <HLib/QTH1F.h>
#include <algorithm>
#include <jl_lib/jlutil.h>
//#include <jl_lib.h>
using namespace std;

QTH1F::QTH1F()
{}

QTH1F::QTH1F(string name, string title)
:name_(name),
title_(title)
{}

void QTH1F::Fill(float f)
{
	v_.push_back(f);
}

TH1F QTH1F::getObject(int nbins, int nbinsGraph, string mode)
{
	sort(v_.begin(), v_.end());
	int nEntries = v_.size();
	int interval = nEntries / nbins;

	// mean of x of each bin.
	vector<Acc> vAcc(nbins);
	{
		int n = 0;
		for( vector<float>::iterator it = v_.begin(); it != v_.end(); ++it, ++n )
		{
			int indx = min(n / interval, nbins - 1);
			vAcc[indx].add(*it);
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
	else
	{
		double margin = (binC[nbins-1] - binC[0]) * 0.1;
		pLow -= margin;
		pHigh += margin;
	}
	TH1F h(name_.c_str(), title_.c_str(), nbinsGraph, pLow, pHigh);

	// Fill the object.
	int n = 0;
	for( vector<float>::iterator it = v_.begin(); it != v_.end(); ++it, ++n )
	{
		int indx = min(n / interval, nbins - 1);
		double x = binC[indx];
		if( mode == "log" )
			x = log(x);
		h.Fill(x);
	}

	// X-axis label.
	for( vector<float>::iterator it = binC.begin(); it != binC.end(); ++it )
	{
		double binCenter = *it;
		double x = binCenter;
		if( mode == "log" )
			x = log(x);
		int nbin = 0;
		for( int i=1; i<=nbinsGraph; ++i )
		{
			if( h.GetXaxis()->GetBinLowEdge(i) > x )
			{
				char label[100];
				sprintf(label, "%.0f", binCenter);
				h.GetXaxis()->SetBinLabel(i, label);
				break;
			}
		}
	}
	return h;
}
