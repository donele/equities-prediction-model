#ifndef __TickerSector__
#define __TickerSector__
#include <string>
#include <set>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC TickerSector {
public:
	TickerSector(const std::string& market, int idate);
	std::string get_sector(std::string ticker);
	std::set<std::string> get_sectors();
	int get_idate();

private:
	int idate_;
	std::map<std::string, std::string> tickerSector_;
};

#endif
