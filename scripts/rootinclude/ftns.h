#ifndef __ftns__
#define __ftns__
#include <vector>
#include <string>
#include <limits>

vector<string> split(string& s, char d = 9)
{
  vector<string> ret;
  int i = 0;

  while( i != s.size() )
  {
    while( i != s.size() && (isspace(s[i]) || s[i] == d) )
      ++i;

    int j = i;
    while( j != s.size() && s[j] != d && !isspace(s[j]) )
      ++j;

    if( i != j )
    {
      ret.push_back(s.substr(i, j-i));
      i=j;
    }
  }

  return ret;
}


void get_ma(vector<double>& vMA, vector<double>& v, int nma)
{
  int N = v.size();
  if( N > nma )
  {
    for( int i = nma - 1; i < N; ++i )
    {
      double sum = 0.;
      for( int j = i - nma + 1; j <= i; ++j )
        sum += v[j];
      vMA.push_back(sum / nma);
    }
  }
}

double getHistMin(const vector<TH1F*>& vh)
{
	double histMin = numeric_limits<double>::max();
	for( auto h : vh )
	{
		int N = h->GetNbinsX();
		for( int i = 0; i < N; ++i )
		{
			double binCont = h->GetBinContent(i + 1);
			if( binCont < histMin )
				histMin = binCont;
		}
	}
	return histMin;
}

double getHistMax(const vector<TH1F*>& vh)
{
	double histMax = numeric_limits<double>::min();
	for( auto h : vh )
	{
		int N = h->GetNbinsX();
		for( int i = 0; i < N; ++i )
		{
			double binCont = h->GetBinContent(i + 1);
			if( binCont > histMax )
				histMax = binCont;
		}
	}
	return histMax;
}

#endif

