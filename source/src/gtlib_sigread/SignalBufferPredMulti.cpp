#include <gtlib_sigread/SignalBufferPredMulti.h>
#include <gtlib/util.h>
#include <jl_lib/jlutil.h>
#include <algorithm>
using namespace std;

namespace gtlib {

SignalBufferPredMulti::SignalBufferPredMulti(const string& predPath)
{
	ifs_.open(predPath.c_str());
	if( ifs_.is_open() )
	{
		labels_ = getNextLineSplitString(ifs_);
		targetIndex_ = getIndex("target");
		bmpredIndex_ = getIndex("bmpred");
		predIndex_ = getIndex("tbpred");
		if( predIndex_ < 0 )
			predIndex_ = getIndex("pred");
		sprdIndex_ = getIndex("sprd");
		priceIndex_ = getIndex("price");
		intraTarIndex_ = getIndex("intraTar");
		bidSizeIndex_ = getIndex("bidSize");
		askSizeIndex_ = getIndex("askSize");

		getPredIndex();
	}
}

int SignalBufferPredMulti::getIndex(const string& label)
{
	auto beg = begin(labels_);
	auto endi = end(labels_);
	int indexMax = endi - beg - 1;
	int index = find(beg, endi, label) - beg; 
	if( index > indexMax )
		return -1;
	return index;
}

void SignalBufferPredMulti::getPredIndex()
{
	int cnt = 0;
	for( auto& label : labels_ )
	{
		int N = label.size();
		if( N > 4 && label.substr(0, 4) == "pred" )
		{
			int predSub = atoi(label.substr(4, N - 4).c_str());
			if( predSub > 0 )
				mPredIndex_[predSub] = getIndex(label);
		}
	}
}

bool SignalBufferPredMulti::is_open()
{
	return ifs_.is_open();
}

bool SignalBufferPredMulti::next()
{
	rawInput_ = getNextLineSplitFloat(ifs_);
	return ifs_.rdstate() == 0;
}

float SignalBufferPredMulti::getTarget()
{
	return rawInput_[targetIndex_];
}

float SignalBufferPredMulti::getBmpred()
{
	return rawInput_[bmpredIndex_];
}

float SignalBufferPredMulti::getPred()
{
	return rawInput_[predIndex_];
}

float SignalBufferPredMulti::getPred(int predSub)
{
	return rawInput_[mPredIndex_[predSub]];
}

vector<int> SignalBufferPredMulti::getPredSubs()
{
	vector<int> predSubs;
	for( auto& kv : mPredIndex_ )
		predSubs.push_back(kv.first);
	return predSubs;
}

vector<float> SignalBufferPredMulti::getPreds()
{
	vector<float> preds;
	for( auto& kv : mPredIndex_ )
		preds.push_back(rawInput_[kv.second]);
	return preds;
}

float SignalBufferPredMulti::getSprd()
{
	return rawInput_[sprdIndex_];
}

float SignalBufferPredMulti::getPrice()
{
	return rawInput_[priceIndex_];
}

float SignalBufferPredMulti::getIntraTar()
{
	if( intraTarIndex_ < 0 )
		return 0.;
	return rawInput_[intraTarIndex_];
}

int SignalBufferPredMulti::getBidSize()
{
	if( bidSizeIndex_ < 0 )
		return 0.;
	return rawInput_[bidSizeIndex_];
}

int SignalBufferPredMulti::getAskSize()
{
	if( askSizeIndex_ < 0 )
		return 0.;
	return rawInput_[askSizeIndex_];
}

} // namespace gtlib
