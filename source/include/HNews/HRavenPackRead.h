#ifndef __HRavenPackRead__
#define __HRavenPackRead__
#include <HLib/HModule.h>
//#include <jl_lib/NewsCatRP.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
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

class CLASS_DECLSPEC HRavenPackRead: public HModule {
public:
	HRavenPackRead(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~HRavenPackRead();

	virtual void beginJob();
	virtual void beginMarket();
	virtual void beginDay();
	virtual void endDay();
	virtual void endMarket();
	virtual void endJob();

private:
	int verbose_;
	std::string input_dir_;
	std::string input_file_;
	std::ifstream ifs_;

	std::string temp_line_;
	std::vector<std::string> lines_;

	int get_idate(std::string& line);
	void read_text(int idate);
};

#endif