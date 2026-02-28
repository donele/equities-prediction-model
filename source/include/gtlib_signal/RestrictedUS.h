#ifndef __gtlib_signal_RestrictedUS__
#define __gtlib_signal_RestrictedUS__
#include <map>
#include <string>
namespace gtlib {

class RestrictedUS {
public:
	RestrictedUS();
	virtual ~RestrictedUS();
	void beginDay(int idate);
	bool isHalted(int idate, const std::string& ticker, int msecs);
private:
	int lastIdate_;
	std::map<int, std::map<std::string, int>> m_;
	std::map<int, std::map<std::string, int>>::iterator itd_;
};

} // namespace gtlib

#endif
