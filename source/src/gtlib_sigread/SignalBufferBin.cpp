#include <gtlib_sigread/SignalBufferBin.h>
#include <jl_lib/jlutil.h>
#include <algorithm>
using namespace std;

namespace gtlib {

SignalBufferBin::SignalBufferBin(const string& binPath, const vector<string>& inputNames)
	:inputNames_(inputNames)
{
	ifs_.open(binPath.c_str(), ios::binary);
	if( ifs_.is_open() )
	{
		readHeader();
		setDesiredInputs();
	}
}

void SignalBufferBin::readHeader()
{
	int nSamplePoints;
	ifs_.read( (char*)&nSamplePoints, sizeof(int) );
	ifs_.read( (char*)&nInputFields_, sizeof(int) );

	long long int len = 0;
	ifs_.read( (char*)&len, sizeof(len) );

	vector<char> str;
	str.resize(len);
	ifs_.read(&str[0], len);

	auto itFrom = str.begin();
	auto itEnd = str.end();
	auto itTo = itFrom;
	while( itTo != itEnd )
	{
		itTo = find(itFrom, itEnd, ',');
		string label = string(itFrom, itTo).c_str(); // Drop '\0' if exists.
		labels_.push_back(label);
		if( itEnd != itTo )
			itFrom = itTo + 1;
	}
}

void SignalBufferBin::setDesiredInputs()
{
	auto beg = begin(labels_);
	auto theEnd = end(labels_);
	for( auto name : inputNames_ )
	{
		int index = find(beg, theEnd, name) - beg;
		indexMap_.push_back(index);
		nameMap_[name] = index;
	}
}

bool SignalBufferBin::is_open()
{
	return ifs_.is_open();
}

bool SignalBufferBin::next()
{
	rawInput_.clear();
	rawInput_.resize(nInputFields_);
	ifs_.read( (char*)&rawInput_[0], nInputFields_ * sizeof(float) );
	return ifs_.rdstate() == 0;
}

const vector<string>& SignalBufferBin::getLabels()
{
	return labels_;
}

const vector<string>& SignalBufferBin::getInputNames()
{
	return inputNames_;
}

int SignalBufferBin::getNInputs()
{
	return indexMap_.size();
}

float SignalBufferBin::getInput(int i)
{
	return rawInput_[indexMap_[i]];
}

float SignalBufferBin::getInput(const string& name)
{
	auto it = nameMap_.find(name);
	if( it == end(nameMap_) )
		exit(14);
	return rawInput_[it->second];
}

vector<float> SignalBufferBin::getInputs()
{
	vector<float> inputs;
	for( int i : indexMap_ )
		inputs.push_back(rawInput_[i]);
	return inputs;
}

} // namespace gtlib
