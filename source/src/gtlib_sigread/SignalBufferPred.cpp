#include <gtlib_sigread/SignalBufferPred.h>
#include <gtlib/util.h>
#include <jl_lib/jlutil.h>
#include <algorithm>
using namespace std;

namespace gtlib {

SignalBufferPred::SignalBufferPred(const string& predPath)
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
		askSizeIndex_ = getIndex("intraTar");
	}
}

int SignalBufferPred::getIndex(const string& label)
{
	auto beg = begin(labels_);
	auto endi = end(labels_);
	int indexMax = endi - beg - 1;
	int index = find(beg, endi, label) - beg; 
	if( index > indexMax )
		return -1;
	return index;
}

bool SignalBufferPred::is_open()
{
	return ifs_.is_open();
}

bool SignalBufferPred::next()
{
	rawInput_ = getNextLineSplitFloat(ifs_);
	return ifs_.rdstate() == 0;
}

float SignalBufferPred::getTarget()
{
	return rawInput_[targetIndex_];
}

float SignalBufferPred::getBmpred()
{
	return rawInput_[bmpredIndex_];
}

float SignalBufferPred::getPred()
{
	return rawInput_[predIndex_];
}

float SignalBufferPred::getSprd()
{
	return rawInput_[sprdIndex_];
}

float SignalBufferPred::getPrice()
{
	return rawInput_[priceIndex_];
}

float SignalBufferPred::getIntraTar()
{
	if( intraTarIndex_ < 0 )
		return 0.;
	return rawInput_[intraTarIndex_];
}

int SignalBufferPred::getBidSize()
{
	if( bidSizeIndex_ < 0 )
		return 0.;
	return rawInput_[bidSizeIndex_];
}

int SignalBufferPred::getAskSize()
{
	if( askSizeIndex_ < 0 )
		return 0.;
	return rawInput_[askSizeIndex_];
}

} // namespace gtlib
