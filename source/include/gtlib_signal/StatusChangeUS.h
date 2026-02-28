#ifndef __gtlib_signal_StatusChangeUS__
#define __gtlib_signal_StatusChangeUS__
#include <map>
#include <vector>
#include <string>
namespace gtlib {

class StatusChangeUS {
public:
	StatusChangeUS();
	virtual ~StatusChangeUS();
	void beginDay(int idate);
	bool isHalted(int idate, const std::string& ticker, int msecs);
private:
	std::vector<std::string> statusDirs_;
	std::map<std::string, int> firstStatusOnMap_;
};

} // namespace gtlib

#endif
