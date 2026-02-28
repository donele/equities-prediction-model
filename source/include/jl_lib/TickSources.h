#ifndef __TickSources__
#define __TickSources__
#include <string>
#include <vector>
#include <map>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC TickSources {
public:
	TickSources();
	TickSources(std::string region);
	void read(std::string region, const std::string& flag = "");
	std::vector<std::string> getDirs(const std::string& type, const std::string& market, int idate);
	std::vector<std::string> stocksdirectory(const std::string& market, int idate);
	std::vector<std::string> bookdirectory(const std::string& market, int idate);
	std::vector<std::string> nbbodirectory(std::string market, int idate);
	std::vector<std::string> sipdirectory(std::string market, int idate);

	struct Directories {
		Directories(){}
		void addstocksdir(const std::string& dir);
		void addbookdir(const std::string& dir);
		void addnbbodir(const std::string& dir);
		void addsipdir(const std::string& dir);
		std::vector<std::string> stocksdirectory;
		std::vector<std::string> bookdirectory;
		std::vector<std::string> nbbodirectory;
		std::vector<std::string> sipdirectory;
	};

private:
	std::map<std::string, std::map<std::string, std::map<int, Directories> > > mDir_;
};

#endif
