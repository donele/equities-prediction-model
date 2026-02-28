#ifndef __HaltSched__
#define __HaltSched__
#include <vector>

class HaltSched {
public:
	HaltSched():haltLenSec_(0){}
	HaltSched(int haltLenSec):haltLenSec_(haltLenSec){}
	void addHalt(int msecs);
	bool isHalted(int msecs);
	int getNhalts();
private:
	int haltLenSec_;
	std::vector<int> vStopMsecs_;
};

#endif
