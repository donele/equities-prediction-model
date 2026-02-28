#include <gtlib_fitana/PredLine.h>
#include <jl_lib/jlutil.h>
#include <algorithm>
using namespace std;

namespace gtlib {

PredLine::PredLine(const string& header)
{
	vector<string> tokens = split(header);
	initializeStorage(tokens);
	findIndexes(tokens);
	findPredLabelsIndexes(tokens);
}

void PredLine::initializeStorage(const vector<string>& tokens)
{
	v_.resize(tokens.size());
	std::fill(begin(v_), end(v_), 0.);
}

void PredLine::findIndexes(const vector<string>& tokens)
{
	sprdIndex_ = find(begin(tokens), end(tokens), "sprd") - begin(tokens);
	targetIndex_ = find(begin(tokens), end(tokens), "target") - begin(tokens);
	bmpredIndex_ = find(begin(tokens), end(tokens), "bmpred") - begin(tokens);
}

void PredLine::findPredLabelsIndexes(const vector<string>& tokens)
{
	int NT = tokens.size();
	for( int i = 0; i < NT; ++i )
	{
		const string& token = tokens[i];
		if( token.size() > 4 && token.find("pred") == 0 )
		{
			vPredLabel_.push_back(token);
			vPredIndex_.push_back(i);
		}
	}
}

float PredLine::getSprd()
{
	return v_[sprdIndex_];
}

float PredLine::getTarget()
{
	return v_[targetIndex_];
}

float PredLine::getBmpred()
{
	return v_[bmpredIndex_];
}

vector<float> PredLine::getPredSeries()
{
	vector<float> v;
	for( int i : vPredIndex_ )
		v.push_back(v_[i]);
	return v;
}

vector<string> PredLine::getPredLabels()
{
	return vPredLabel_;
}

istream& operator >>(istream& is, PredLine& rhs)
{
	rhs.v_.clear();
	string line;
	getline(is, line);
	auto tokens = split(line);
	for(auto& token : tokens)
		rhs.v_.push_back(atof(token.c_str()));
	return is;
}

} // namespace gtlib

