#ifndef __MWriteFilters__
#define __MWriteFilters__
#include <MFramework/MModuleBase.h>
#include <MSignal/sig.h>
#include <gtlib_linmod/ARMulti.h>
#include <MFramework.h>
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

class CLASS_DECLSPEC MWriteFilters: public MModuleBase {
public:
	MWriteFilters(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MWriteFilters();

	virtual void beginJob();
	virtual void beginDay();
	virtual void endDay();
	virtual void endJob();

private:
	bool write_filters_debug_;
	int write_filters_horizon_;
	int write_filters_lag_;
	int db_offset_;
	std::string dbname_;

	std::vector<int> vlen_;
	std::vector<gtlib::ARMulti> ar_;

	std::string get_colname(const std::string& model02);
	void print_coeff(const std::string& colname, const std::vector<std::vector<float> >& coeff);
	bool horizon_match(const std::string& title, int horizon);
};

#endif
