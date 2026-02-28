#ifndef __QProfile__
#define __QProfile__
#include <map>
#include <TProfile.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC QProfile {
public:
	QProfile();
	QProfile(std::string name, std::string title, double min1 = 1, double max1 = -1, double min2 = 1, double max2 = -1);
	void Fill(float f1, float f2);
	TProfile getObject(int nbins, int nbinsGraph = 100, std::string mode = "linear");

private:
	double min1_;
	double max1_;
	double min2_;
	double max2_;
	std::string name_;
	std::string title_;
	std::vector<std::pair<float, float> > v_;
};

#endif
