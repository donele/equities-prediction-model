#include <HLearnObj/Signal.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <sstream>
#include <algorithm>
using namespace std;

namespace hnn {
	SignalSummary::SignalSummary(vector<string>& signalNames_, vector<string>& tickers_)
	{
		nSignals = signalNames_.size();
		signalNames = signalNames_;
		nTickers = tickers_.size();
		tickers = tickers_;
	}

	void SignalSummary::print()
	{
		cout << nSignals << " variables: ";
		copy(signalNames.begin(), signalNames.end(), ostream_iterator<string>(cout, " "));
		cout << "\n" << nTickers << " tickers: ";
		copy(tickers.begin(), tickers.end(), ostream_iterator<string>(cout, " "));
		cout << "\n";
		return;
	}

	SignalLabel::SignalLabel(string ticker_, int nEntries_)
	{
		ticker = ticker_;
		nEntries = nEntries_;
	}

	void SignalLabel::print()
	{
		cout << ticker << " has " << nEntries << " entries.\n";
		return;
	}

	SignalContent::SignalContent(const SignalContent& ns)
	{
		input = ns.input;
	}

	void SignalContent::print()
	{
		copy(input.begin(), input.end(), ostream_iterator<float>(cout, " "));
		cout << "\n";
		return;
	}

	SignalContent::SignalContent(vector<float>& vi)
	:input(vi)
	{}

	BinSig::BinSig(int ncols_)
		:ncols(ncols_)
	{
		//input.resize(ncols);
	}

	bool BinSigHeader::valid()
	{
		return ncols == labels.size();
	}
}

ostream& operator <<(ostream& os, const hnn::SignalSummary& obj)
{
	os.write( (char*)&obj.nSignals, sizeof(int) );
	for( vector<string>::const_iterator it = obj.signalNames.begin(); it != obj.signalNames.end(); ++it )
	{
		string inputName = *it;
		char cInput[20];
		sprintf(cInput, "%s", inputName.c_str());
		int isize = inputName.size();
		os.write( (char*)&isize, sizeof(int) );
		os.write( (char*)&cInput, isize * sizeof(char) );
	}
	os.write( (char*)&obj.nTickers, sizeof(int) );
	for( vector<string>::const_iterator it = obj.tickers.begin(); it != obj.tickers.end(); ++it )
	{
		string ticker = *it;
		char cticker[20];
		sprintf(cticker, "%s", ticker.c_str());
		int tsize = ticker.size();
		os.write( (char*)&tsize, sizeof(int) );
		os.write( (char*)&cticker, tsize * sizeof(char) );
	}
	return os;
}

ostream& operator <<(ostream& os, const hnn::SignalLabel& obj)
{
	int tsize = obj.ticker.size();
	char cticker[20];
	sprintf(cticker, "%s", obj.ticker.c_str());
	os.write( (char*)&tsize, sizeof(int) );
	os.write( (char*)&cticker, tsize * sizeof(char) );
	os.write( (char*)&obj.nEntries, sizeof(int) );
	return os;
}

ostream& operator <<(ostream& os, const hnn::SignalContent& obj)
{
	int nSignals = obj.input.size();
	os.write( (char*)&nSignals, sizeof(int) );
	for( vector<float>::const_iterator it = obj.input.begin(); it != obj.input.end(); ++it )
	{
		float v = *it;
		os.write( (char*)&v, sizeof(float) );
	}
	return os;
}

istream& operator >>(istream& is, hnn::SignalSummary& obj)
{
	is.read( (char*)&obj.nSignals, sizeof(int) );
	for( int i = 0; i < obj.nSignals; ++i )
	{
		int isize = 0;
		char cInput[100];
		is.read( (char*)&isize, sizeof(int) );
		is.read( (char*)&cInput, isize * sizeof(char) );
		cInput[isize] = '\0';
		obj.signalNames.push_back(cInput);
	}
	is.read( (char*)&obj.nTickers, sizeof(int) );
	for( int i = 0; i < obj.nTickers; ++i )
	{
		int tsize = 0;
		char cticker[20];
		is.read( (char*)&tsize, sizeof(int) );
		is.read( (char*)&cticker, tsize * sizeof(char) );
		cticker[tsize] = '\0';
		obj.tickers.push_back(cticker);
	}
	return is;
}

istream& operator >>(istream& is, hnn::SignalLabel& obj)
{
	int tsize = 0;
	char cticker[20];
	is.read( (char*)&tsize, sizeof(int) );
	is.read( (char*)&cticker, tsize * sizeof(char) );
	cticker[tsize] = '\0';
	obj.ticker = cticker;
	is.read( (char*)&obj.nEntries, sizeof(int) );
	return is;
}

istream& operator >>(istream& is, hnn::SignalContent& obj)
{
	int nSignals = 0;
	is.read( (char*)&nSignals, sizeof(int) );
	obj.input.clear();
	for( int i = 0; i<nSignals; ++i )
	{
		float input = 0;
		is.read( (char*)&input, sizeof(float) );
		obj.input.push_back(input);
	}

	return is;
}

istream& operator >>(istream& is, hnn::BinSigHeader& obj)
{
	is.read( (char*)&obj.nrows, sizeof(int) );
	is.read( (char*)&obj.ncols, sizeof(int) );

	long long int len = 0;
	is.read( (char*)&len, sizeof(len) );

	vector<char> str;
	str.resize(len);
	is.read(&str[0], len);

	obj.labels.clear();
	vector<char>::iterator itFrom = str.begin();
	vector<char>::iterator itEnd = str.end();
	vector<char>::iterator itTo = itFrom;
	while( itTo != itEnd )
	{
		itTo = find(itFrom, itEnd, ',');
		obj.labels.push_back(string(itFrom, itTo));
		if( itEnd != itTo )
			itFrom = itTo + 1;
	}

	return is;
}

istream& operator >>(istream& is, hnn::BinSig& obj)
{
	obj.input.clear();
	obj.input.resize(obj.ncols);
	is.read( (char*)&obj.input[0], obj.ncols * sizeof(float) );

	return is;
}
