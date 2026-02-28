#include <gtlib_sigread/SignalHeader.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib/util.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
using namespace std;

namespace gtlib {

SignalHeader::SignalHeader(const string& txtPath)
{
	market_ = getMarket(txtPath);
	ifsTxt_.open(txtPath.c_str());
	if( ifsTxt_.is_open() )
	{
		labels_ = getNextLineSplitString(ifsTxt_);
		int index = 0;
		for( string label : labels_ )
			mIndex_[label] = index++;
	}
}

bool SignalHeader::is_open()
{
	return ifsTxt_.is_open();
}

bool SignalHeader::next()
{
	vector<string> sl = getNextLineSplitString(ifsTxt_);
	if( sl.size() > 0 )
	{
		uid_ = sl[mIndex_["uid"]];
		ticker_ = sl[mIndex_["ticker"]];
		time_ = ceil((atof(sl[mIndex_["time"]].c_str())) * 1000 - 0.5);
		if(mIndex_.count("bidEx"))
			bidEx_ = sl[mIndex_["bidEx"]].c_str()[0];
		if(mIndex_.count("askEx"))
			askEx_ = sl[mIndex_["askEx"]].c_str()[0];
	}
	return ifsTxt_.rdstate() == 0;
}

const string& SignalHeader::getTicker()
{
	return ticker_;
}

const string& SignalHeader::getUid()
{
	return uid_;
}

int SignalHeader::getMsecs()
{
	return time_ + mto::msecOpen(market_);
}

char SignalHeader::getBidEx()
{
	return bidEx_;
}

char SignalHeader::getAskEx()
{
	return askEx_;
}

} // namespace gtlib
