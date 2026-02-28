#include <gtlib_fitting/DailyDataCount.h>
using namespace std;

namespace gtlib {

int DailyDataCount::getNSamplePoints() const
{
	auto itRBeg = mIdateOffsetNdata_.rbegin();
	return itRBeg->second.first + itRBeg->second.second;
}

void DailyDataCount::add(int idate, int ndata)
{
	int offset = 0;
	if( !mIdateOffsetNdata_.empty() )
		offset = getNSamplePoints();
	mIdateOffsetNdata_[idate] = make_pair(offset, ndata);
}

vector<int> DailyDataCount::getIdates() const
{
	vector<int> idates;
	for( auto& kv : mIdateOffsetNdata_ )
		idates.push_back(kv.first);
	return idates;
}

int DailyDataCount::getOffset(int idate)
{
	return mIdateOffsetNdata_[idate].first;
}

int DailyDataCount::getNdata(int idate)
{
	return mIdateOffsetNdata_[idate].second;
}

} // namespace gtlib
