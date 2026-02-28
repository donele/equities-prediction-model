#include <HLearnObj/Sample.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <sstream>
using namespace std;

namespace hnn {
	SampleSummary::SampleSummary(vector<string>& inputNames_, vector<string>& tickers_)
	{
		nInput = inputNames_.size();
		inputNames = inputNames_;
		nTickers = tickers_.size();
		tickers = tickers_;
	}

	SampleHeader::SampleHeader(string ticker_, int nEntries_)
	{
		ticker = ticker_;
		nEntries = nEntries_;
	}

	Sample::Sample(const Sample& ns)
	{
		input = ns.input;
		target = ns.target;
		spectator = ns.spectator;
		quote = ns.quote;
	}

	Sample::Sample(vector<float>& vi, float& t)
	:input(vi), target(t)
	{}

	Sample::Sample(vector<float>& vi, float& t, QuoteInfo& qt)
	:input(vi), target(t), quote(qt)
	{}

	Sample::Sample(vector<float>& vi, float& t, vector<float>& vs)
	:input(vi), target(t), spectator(vs)
	{}
}

ostream& operator <<(ostream& os, const hnn::SampleSummary& obj)
{
	os.write( (char*)&obj.nInput, sizeof(int) );
	for( vector<string>::const_iterator it = obj.inputNames.begin(); it != obj.inputNames.end(); ++it )
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

ostream& operator <<(ostream& os, const hnn::SampleHeader& obj)
{
	int tsize = obj.ticker.size();
	char cticker[20];
	sprintf(cticker, "%s", obj.ticker.c_str());
	os.write( (char*)&tsize, sizeof(int) );
	os.write( (char*)&cticker, tsize * sizeof(char) );
	os.write( (char*)&obj.nEntries, sizeof(int) );
	return os;
}

ostream& operator <<(ostream& os, const hnn::Sample& obj)
{
	int nInput = obj.input.size();
	os.write( (char*)&nInput, sizeof(int) );
	for( vector<float>::const_iterator it = obj.input.begin(); it != obj.input.end(); ++it )
	{
		float v = *it;
		os.write( (char*)&v, sizeof(float) );
	}
	os.write( (char*)&obj.target, sizeof(float) );
	os.write( (char*)&obj.quote, sizeof(QuoteInfo) );
	return os;
}

istream& operator >>(istream& is, hnn::SampleSummary& obj)
{
	is.read( (char*)&obj.nInput, sizeof(int) );
	for( int i = 0; i < obj.nInput; ++i )
	{
		int isize = 0;
		char cInput[100];
		is.read( (char*)&isize, sizeof(int) );
		is.read( (char*)&cInput, isize * sizeof(char) );
		cInput[isize] = '\0';
		obj.inputNames.push_back(cInput);
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

istream& operator >>(istream& is, hnn::SampleHeader& obj)
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

istream& operator >>(istream& is, hnn::Sample& obj)
{
	int nInput = 0;
	is.read( (char*)&nInput, sizeof(int) );
	obj.input.clear();
	for( int i = 0; i < nInput; ++i )
	{
		float input = 0;
		is.read( (char*)&input, sizeof(float) );
		obj.input.push_back(input);
	}
	is.read( (char*)&obj.target, sizeof(float) );
	is.read( (char*)&obj.quote, sizeof(QuoteInfo) );

	return is;
}