#ifndef __OneSecRet__
#define __OneSecRet__
#include <optionlibs/TickData.h>
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

class CLASS_DECLSPEC OneSecRet {
public:
	OneSecRet();
	OneSecRet(const std::string& market, int idate);
	virtual ~OneSecRet();

	void add_quote(std::string symbol, const QuoteInfo& quote);

	void calculate_each_return();
	void calculate_each_return_paneu();
	void calculate_average_return(const std::set<std::string>& symbols = std::set<std::string>());

	void insert_each_return( TickStorage<ReturnData>& tsR );
	void insert_average_return( TickStorage<ReturnData>& tsR, const std::string& symbol );
	void insert_average_return( TickSeries<ReturnData>& ts );
	void insert_average_return_sig( TickSeries<ReturnData>& ts );
	void insert_return( const std::string& symbol, TickSeries<ReturnData>& ts );

protected:
	std::string market_;
	int idate_;
	int msecOpen_;
	int msecClose_;
	std::map<std::string, std::vector<double> > mvMsec_;
	std::map<std::string, std::vector<double> > mvBid_;
	std::map<std::string, std::vector<double> > mvAsk_;
	std::map<std::string, std::vector<double> > mvRet_;
	std::vector<double> vAvgSecRet_;
	std::vector<double> vAvgSecRetSig_;
};

#endif
