#ifndef __Sessions__
#define __Sessions__
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC Sessions {
public:
	Sessions();
	Sessions(const std::string& market, int idate = 99999999, int delayOpen = 0, int delayClose = 0);
	virtual ~Sessions(){}

	void add_session(const std::pair<int, int>& v);
	bool isAfterOpenBeforeClose(int msecs) const;
	bool inSession(int msecs) const ;
	bool inSession(int msecs1, int msecs2) const ;
	bool inSessionStrict(int msecs) const;
	bool inSessionStrictWait(int msecs, int waitSec) const;
	int length();

protected:
	std::vector<std::pair<int, int> > v_;
};

#endif
