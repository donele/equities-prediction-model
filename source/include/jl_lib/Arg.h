#ifndef __Arg__
#define __Arg__
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC Arg {
public:
	Arg(){};
	Arg(int argc, char* argv[]);
	~Arg();
	typedef std::multimap<std::string, std::string>::const_iterator mmit;

	bool isGiven(const std::string& opt) const;
	std::string getVal(const std::string& opt) const;
	std::vector<std::string> getVals(const std::string& opt) const;

private:
	std::multimap<std::string, std::string> amm_;
};

#endif
