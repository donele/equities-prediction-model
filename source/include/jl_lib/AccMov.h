#ifndef __AccMov__
#define __AccMov__
#include "jl_lib/jlutil.h"
#include <map>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC AccMov {
public:
	AccMov();
	AccMov(int fitDays);
	void beginDay(int idate, int idateFirst);
	void add(float val);
	float getRMS();
	float getMean();

private:
	int fitDays_;
	Acc* pAcc_;
	Acc accSum_;
	std::map<int, Acc> mAcc_;
};

#endif
