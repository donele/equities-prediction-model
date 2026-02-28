#ifndef __NewsCatRP__
#define __NewsCatRP__
#include <string>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC NewsCatRP {
public:
	NewsCatRP();
	~NewsCatRP();

	int cat2i(std::string cat);
	std::string i2cat(int i);
	int i2iGroup(int i);

private:
	std::map<std::string, int> m_;

	void initMap();
};

#endif