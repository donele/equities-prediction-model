#ifndef __QTH1F__
#define __QTH1F__
#include <map>
#include <TH1.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC QTH1F {
public:
	QTH1F();
	QTH1F(std::string name, std::string title);
	void Fill(float f);
	TH1F getObject(int nbins, int nbinsGraph = 100, std::string mode = "linear");

private:
	std::string name_;
	std::string title_;
	std::vector<float> v_;
};

#endif
