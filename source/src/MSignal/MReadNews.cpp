#include <MSignal/MReadNews.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <map>
#include <string>
using namespace std;
using namespace hff;

MReadNews::MReadNews(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

MReadNews::~MReadNews()
{}

void MReadNews::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MReadNews::beginMarketDay()
{
	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	read_news(market, idate);
	openMsecs_ = mto::msecOpen(market, idate);
}

void MReadNews::beginTicker(const string& ticker)
{
	map<int, pair<int, int> > newsSentVol;
	newsSentVol[-1] = make_pair(-99, -99);

	if( mNews_.count(ticker) )
	{
		vector<QuoteInfo>& vNews = mNews_[ticker];
		for( vector<QuoteInfo>::iterator it = vNews.begin(); it != vNews.end(); ++it )
		{
			QuoteInfo& quote = *it;
			int rel = quote.bidEx;
			int ens = quote.askEx;
			int aes = quote.bid;
			int aev = quote.ask;

			int msso = quote.msecs - openMsecs_;
			newsSentVol[msso] = make_pair(aes, aev);
		}
	}

	MEvent::Instance()->add<map<int, pair<int, int> > >(ticker, "newsSentVol", newsSentVol);
}

void MReadNews::endTicker(const string& ticker)
{
	MEvent::Instance()->remove<map<int, pair<int, int> > >(ticker, "newsSentVol");
}

void MReadNews::endMarketDay()
{
}

void MReadNews::endJob()
{
}

void MReadNews::read_news(const string& market, int idate)
{
	string dir;
	TickAccess<QuoteInfo> ta(dir, mto::longTicker(market));
	TickSeries<QuoteInfo> ts(20000, 1.2);

	mNews_.clear();

	vector<string> names;
	ta.GetNames(idate, &names);
	for( vector<string>::iterator it = names.begin(); it != names.end(); ++it )
	{
		string ticker = *it;
		vector<QuoteInfo> vNews;

		ta.GetTickSeries(ticker, idate, &ts);
		int n = ts.NTicks();
		ts.StartRead();
		QuoteInfo quote;
		for( int i = 0; i < n; ++i )
		{
			ts.Read(&quote);
			vNews.push_back(quote);
		}

		mNews_[ticker] = vNews;
	}
}
