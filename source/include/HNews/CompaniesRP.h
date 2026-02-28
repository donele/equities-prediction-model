#ifndef __CompaniesRP__
#define __CompaniesRP__
#include<string>
#include<map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC CompaniesRP {
public:
	CompaniesRP();
	CompaniesRP(std::string market);

	std::string ticker(std::string company_id);
	void beginDay(int idate);

private:
	std::string market_;
	std::string country_;

	std::map<std::string, std::string> mIdIsin_;
	std::map<std::string, std::string> mIsinTicker_;
	std::map<std::string, std::string> mIdTicker_;
};

#endif
