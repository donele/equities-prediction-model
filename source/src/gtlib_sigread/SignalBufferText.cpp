#include <gtlib_sigread/SignalBufferText.h>
#include <gtlib/util.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

namespace gtlib {

SignalBufferText::SignalBufferText(const string& path, const vector<string>& inputNames, int openMsecs)
	:inputNames_(inputNames),
	openMsecs_(openMsecs)
{
	ifs_.open(path.c_str());
	if( ifs_.is_open() )
	{
		readLabels();
		setDesiredInputs();
	}
}

void SignalBufferText::readLabels()
{
	labels_ = getNextLineSplitString(ifs_);

	for( auto& label : labels_ )
	{
		string replaceLabel;
		if( label == "spxPspxSprd" )
			replaceLabel = "indxPred1Sprd";
		else if( label == "lcPlcSprd" )
			replaceLabel = "indxPred2Sprd";
		else if( label == "mret300lag" )
			replaceLabel = "mret300L";
		else if( label == "trim" )
			replaceLabel = "fillImb";
		else if( label == "fillImb" )
			replaceLabel = "fillImbNonlin";
		else if( label == "tb1m" )
			replaceLabel = "pred";
		else if( label == "linPred" )
			replaceLabel = "bmpred60;0";
		//else if( label == "spxPspxPara" )
			//replaceLabel = "spxPspx";
		//else if( label == "spfPspxPara" )
			//replaceLabel = "spfPspx";
		//else if( label == "lcPlcPara" )
			//replaceLabel = "lcPlc";
		//else if( label == "spfPlcPara" )
			//replaceLabel = "spfPlc";
		//else if( label == "rusfPspxPara" )
			//replaceLabel = "rusfPspx";
		//else if( label == "rusfPlcPara" )
			//replaceLabel = "rusfPlc";

		if( !replaceLabel.empty() )
			label = replaceLabel;
	}

	targetIndex_ = findIndex("tgtLT");
	bmpredIndex_ = findIndex("predLT_HR");
	predIndex_ = findIndex("predLT_HR");
	tickerIndex_ = findIndex("symbol");
	uidIndex_ = -1;
	msecsIndex_ = findIndex("msecs");
	if( msecsIndex_ >= 0 )
		openMsecs_ = 0;
	if( msecsIndex_ < 0 )
		msecsIndex_ = findIndex("time");
	if( msecsIndex_ < 0 )
		msecsIndex_ = findIndex("Secs");
}

int SignalBufferText::findIndex(const string& label)
{
	auto labelsBeg = begin(labels_);
	auto labelsEnd = end(labels_);
	auto it = find(labelsBeg, labelsEnd, label);
	int index = -1;
	if( it != labelsEnd )
		index = it - labelsBeg;
	return index;
}

void SignalBufferText::setDesiredInputs()
{
	auto beg = begin(labels_);
	auto theEnd = end(labels_);
	for( auto name : inputNames_ )
	{
		auto it = find(beg, theEnd, name);
		int index = -1;
		if( it != theEnd )
			index = it - beg;
		indexMap_.push_back(index);
		nameMap_[name] = index;
	}
}

bool SignalBufferText::is_open()
{
	return ifs_.is_open();
}

bool SignalBufferText::next()
{
	rawInputString_ = getNextLineSplitString(ifs_);
	return ifs_.rdstate() == 0;
}

const vector<string>& SignalBufferText::getLabels()
{
	return labels_;
}

const vector<string>& SignalBufferText::getInputNames()
{
	return inputNames_;
}

int SignalBufferText::getNInputs()
{
	return indexMap_.size();
}

float SignalBufferText::getInput(int i)
{
	float ret = 0.;
	int index = indexMap_[i];
	if( index > 0 )
		ret = atof(rawInputString_[index].c_str());
	return ret;
}

float SignalBufferText::getInput(const string& name)
{
	auto it = nameMap_.find(name);
	if( it != end(nameMap_) )
		exit(14);
	return atof(rawInputString_[it->second].c_str());
}

vector<float> SignalBufferText::getInputs()
{
	vector<float> inputs;
	for( int i : indexMap_ )
	{
		if( i >= 0 )
			inputs.push_back(atof(rawInputString_[i].c_str()));
		else
			inputs.push_back(0.);
	}
	return inputs;
}

float SignalBufferText::getTarget()
{
	float target = 0.;
	if( targetIndex_ >= 0 )
		target = atof(rawInputString_[targetIndex_].c_str());
	return target;
}

float SignalBufferText::getBmpred()
{
	float bmpred = 0.;
	if( bmpredIndex_ >= 0 )
		bmpred = atof(rawInputString_[bmpredIndex_].c_str());
	return bmpred;
}

float SignalBufferText::getPred()
{
	float pred = 0.;
	if( predIndex_ >= 0 )
		pred = atof(rawInputString_[predIndex_].c_str());
	return pred;
}

const string& SignalBufferText::getTicker()
{
	return rawInputString_[tickerIndex_];
}

const string& SignalBufferText::getUid()
{
	return rawInputString_[uidIndex_];
}

int SignalBufferText::getMsecs()
{
	int msecs = atoi(rawInputString_[msecsIndex_].c_str());
	if( openMsecs_ > 0 )
		msecs = msecs * 1000 + openMsecs_;
	return msecs;
}

} // namespace gtlib
