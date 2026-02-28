#ifndef __CrossStat__
#define __CrossStat__
#include "optionlibs/TickData.h"
#include <vector>
#include <string>
#include <utility>
#include <iostream>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC CrossStat
{
public:
	CrossStat();
	CrossStat(const std::string& market, int idate);
	void reset(const std::string& market, int idate, bool there_was_a_valid_quote = false);

	void beginDay();	// restart a day (init)
	void beginTicker(bool there_was_a_valid_quote = false);	// start a ticker (clear)
	void endTicker(bool there_was_a_valid_quote = false);	// end of ticker (update)
	void endDay();		// end of day (finish)
	void add_quote(const QuoteInfo& quote);
	int get_max_cross();
	int get_sum_cross();
	int get_n_cross();
	int get_n_crossed_tickers();
	void get_eff_maxCross( int& n, std::vector<double>& x, std::vector<double>& y );
	void get_eff_sumCross( int& n, std::vector<double>& x, std::vector<double>& y );
	void print_cross(std::ostream& os);

private:
	bool crossed_;
	int lenCross_;
	int lastMsecs_;
	int crossMsecs_;
	int msecOpen_;
	int msecClose_;
	int afterOpen_skip_;
	int afterBreak_skip_;
	double max_cross_;
	double sum_cross_;
	int n_cross_;
	bool there_was_a_valid_quote_;

	std::vector<std::pair<int, int> > vBreaks_;
	std::vector<int> vCrossMsecs_;
	std::vector<int> vLenCross_;
	std::vector<int> vMaxCross_;
	std::vector<int> vSumCross_;
	std::vector<int> vNCross_;

	void get_eff( int& n, std::vector<double>& x, std::vector<double>& y, std::vector<int>& v );
};

#endif
