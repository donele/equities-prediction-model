#ifndef __MSessions__
#define __MSessions__
#include <string>
#include <vector>
#include <jl_lib/Sessions.h>

class MSessions : public Sessions {
public:
	virtual ~MSessions(){}

	bool isAfterOpenBeforeClose(int msecs) const;
	bool inSessionStrict(int msecs) const;
	bool inSessionStrictWait(int msecs, int waitSec) const;
private:
	MSessions();
	MSessions(const std::string& market, int idate);
};

#endif
