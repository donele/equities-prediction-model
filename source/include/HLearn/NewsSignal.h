#ifndef __NewsSignal__
#define __NewsSignal__

#include <map>
#include <string>
#include <vector>

class NewsSignal {
public:
	NewsSignal();
	std::vector<std::string> get_names();
	std::vector<float> get_signals(std::string ticker, int msecs);
	void read_news(std::string market, int idate, std::string newsDir);
	float get_volume(std::string ticker, int nday, int msecs);
	float get_sent(std::string ticker, int nday, int msecs);
	float get_lagged_sent(std::string ticker, int nday, int msecs);
	float get_cosent(std::string ticker, int nday, std::string tickerRef, int ndayRef, int msecs);
	float get_lagged_cosent(std::string ticker, int nday, std::string tickerRef, int ndayRef, int msecs);
	float get_cosent_3way(std::string ticker, int nday, std::string sector, int ndayRef, int msecs);

private:
	std::string market_;
	int idate_;
	std::vector<int> vDays_;
	std::vector<std::string> vViews_;

	std::string get_sector_ticker(std::string sector, std::string ticker);
	std::string get_market_ticker(std::string ticker);

	std::map<int, std::map<std::string, std::map<int, int> > > mDayTickerTimeSent_;
	std::map<int, std::map<std::string, std::map<int, int> > > mDayTickerTimeSentLag_;
	std::map<int, std::map<std::string, std::map<int, int> > > mDayTickerTimeVol_;
};

#endif