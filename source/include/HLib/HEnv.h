#ifndef __HEnv__
#define __HEnv__
#include <gtlib_model/hff.h>
#include <string>
#include <vector>
#include <TFile.h>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HEnv {
public:
	static HEnv* Instance();
	~HEnv();
	void initialize();
	void clear();

	bool multiThread() const;
	bool multiThreadModule() const;
	bool multiThreadTicker() const;
	int nMaxThreadTicker() const;
	int nDates() const;
	int iMarketRep() const;
	int marketRep() const;
	std::string loopingOrder() const;
	const std::vector<std::string>& markets() const;
	//const std::vector<hff::IndexFilter>& indexFilters() const;
	const hff::IndexFilters& indexFilters() const;
	const hff::LinearModel& linearModel() const;
	const hff::NonLinearModel& nonLinearModel() const;
	const std::string baseDir() const;
	const std::vector<int>& idates() const;
	const std::vector<int>& idates(std::string market) const;
	const std::vector<std::string>& tickers() const;
	std::string market() const;
	std::string model() const;
	int idate() const;
	int d1() const;
	int d2() const;

	void multiThreadModule(bool tf);
	void multiThreadTicker(bool tf);
	void nMaxThreadTicker(int n);
	void nDates(int n);
	void iMarketRep(int i);
	void marketRep(int n);
	void loopingOrder(std::string);
	void markets(const std::vector<std::string>& markets);
	//void indexFilters(std::vector<hff::IndexFilter>& indexFilters);
	void indexFilters(const hff::IndexFilters& indexFilters);
	void linearModel(const hff::LinearModel& linearModel);
	void nonLinearModel(const hff::NonLinearModel& nonLinearModel);
	void baseDir(std::string dir);
	void idates(const std::vector<int>& idates);
	void idates(std::string market, const std::vector<int>& idates);
	void tickers(const std::vector<std::string>& tickers);
	void market(std::string market);
	void model(std::string model);
	void idate(int idate);
	void d1(int d1);
	void d2(int d2);

	void outfile(std::string outfile);
	TFile* outfile();

	boost::mutex& io_mutex();
	boost::mutex& root_mutex();
	boost::mutex& event_mutex();

private:
	static HEnv* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	HEnv();
	HEnv( const HEnv& );
	const HEnv& operator=( const HEnv& );

	bool multiThreadModule_;
	bool multiThreadTicker_;
	int nMaxThreadTicker_;
	int nDates_;
	int iMarketRep_;
	int marketRep_;
	std::string loopingOrder_;
	std::vector<std::string> markets_;
	hff::IndexFilters indexFilters_;
	hff::LinearModel linearModel_;
	hff::NonLinearModel nonLinearModel_;
	std::string baseDir_;
	std::vector<int> idates_;
	std::map<std::string, std::vector<int> > midates_;
	std::vector<std::string> tickers_;
	std::string market_;
	std::string model_;
	int idate_;
	int d1_;
	int d2_;

	boost::mutex io_mutex_;
	boost::mutex root_mutex_;
	boost::mutex event_mutex_;

	TFile* f_;
};

#endif
