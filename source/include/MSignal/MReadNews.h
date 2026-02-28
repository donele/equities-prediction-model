#ifndef __MReadNews__
#define __MReadNews__
#include <MFramework/MModuleBase.h>
#include <map>
#include <string>
#include <vector>
#include <optionlibs/TickData.h>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MReadNews: public MModuleBase {
public:
	MReadNews(const std::string& moduleName, const std::multimap<std::string, std::string>& conf);
	virtual ~MReadNews();

	virtual void beginJob();
	virtual void beginMarketDay();
	virtual void beginTicker(const std::string& ticker);
	virtual void endTicker(const std::string& ticker);
	virtual void endMarketDay();
	virtual void endJob();

private:
	bool debug_;
	int openMsecs_;
	std::map<std::string, std::vector<QuoteInfo> > mNews_;

	void read_news(const std::string& market, int idate);
};

#endif
