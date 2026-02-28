#ifndef __CorrMov__
#define __CorrMov__
#include <jl_lib/jlutil.h>
//#include <jl_lib.h>
#include <map>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC CorrMov {
public:
	CorrMov();
	CorrMov(int fitDays);
	void beginDay(int idate, int idateFirst);
	void add(float v1, float v2);
	float getCov();

private:
	int fitDays_;
	Corr corr_;
	Corr corrSum_;
	std::map<int, Corr> mCorr_;
};

#endif
