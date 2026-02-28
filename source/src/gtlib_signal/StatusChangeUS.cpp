#include <gtlib_signal/StatusChangeUS.h>
#include <jl_lib/jlutil.h>
#include <fstream>
using namespace std;

namespace gtlib {

StatusChangeUS::StatusChangeUS()
{
	statusDirs_.push_back("/mnt/gf1/tickC/us/cqs/nbbo");
	statusDirs_.push_back("/mnt/gf1/tickC/us/uqdf/nbbo");
}

StatusChangeUS::~StatusChangeUS()
{
}

void StatusChangeUS::beginDay(int idate)
{
	firstStatusOnMap_.clear();

	std::map<std::string, std::vector<StatusMessage>> statusChangeMap;
	LoadStatusData(idate, statusDirs_, &statusChangeMap);
	for( auto& kv : statusChangeMap )
	{
		int firstOn = 0;
		for( ; firstOn < kv.second.size(); ++firstOn )
		{
			if( kv.second[firstOn].flagsOn != 0 )
				break;
		}
		if( firstOn < kv.second.size() )
			firstStatusOnMap_[kv.first] = kv.second[firstOn].msecs;
	}
}

bool StatusChangeUS::isHalted(int idate, const string& ticker, int msecs)
{
	bool ret = false;
	auto it = firstStatusOnMap_.find(ticker);
	if( it != firstStatusOnMap_.end() )
	{
		int changeMsecs = it->second;
		if( msecs >= changeMsecs )
			ret = true;
	}
	return ret;
}

} // namespace gtlib
