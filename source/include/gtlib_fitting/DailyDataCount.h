#ifndef __gtlib_fitting_DailyDataCount__
#define __gtlib_fitting_DailyDataCount__
#include <vector>
#include <map>

namespace gtlib {

class DailyDataCount {
public:
	DailyDataCount(){}
	int getNSamplePoints() const;
	void add(int idate, int ndata);
	std::vector<int> getIdates() const;
	int getOffset(int idate);
	int getNdata(int idate);
private:
	std::map<int, std::pair<int, int>> mIdateOffsetNdata_;
};

} // namespace gtlib

#endif
