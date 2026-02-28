#include <HLib/HEnv.h>
#include <gtlib_model/hff.h>
#include <string>
#include <vector>
using namespace std;

HEnv* HEnv::instance_ = 0;

HEnv* HEnv::Instance()
{
	static HEnv::Cleaner cleaner;
	if( instance_ == 0 )
		instance_ = new HEnv();
	return instance_;
}

HEnv::Cleaner::~Cleaner() {
	delete HEnv::instance_;
	HEnv::instance_ = 0;
}

HEnv::HEnv()
:f_(0)
{
	initialize();
}

void HEnv::initialize()
{
	multiThreadModule_ = false;
	multiThreadTicker_ = false;
	nMaxThreadTicker_ = 4;
	nDates_ = 0;
	iMarketRep_ = 0;
	marketRep_ = 1;
	loopingOrder_ = "mdt";
	markets_.clear();
	linearModel_ = hff::LinearModel();
	nonLinearModel_ = hff::NonLinearModel();
	baseDir_.clear();
	idates_.clear();
	midates_.clear();
	tickers_.clear();
	market_ = "";
	model_ = "";
	idate_ = 0;
	d1_ = 0;
	d2_ = 0;
}

HEnv::~HEnv()
{
	if( f_ != 0 )
	{
		f_->Close();
		delete f_;
	}
}

void HEnv::clear()
{
	initialize();
}

bool HEnv::multiThread() const
{
	return multiThreadTicker_ || multiThreadModule_;
}

bool HEnv::multiThreadTicker() const
{
	return multiThreadTicker_;
}

bool HEnv::multiThreadModule() const
{
	return multiThreadModule_;
}

int HEnv::nMaxThreadTicker() const
{
	return nMaxThreadTicker_;
}

int HEnv::nDates() const
{
	return nDates_;
}

int HEnv::iMarketRep() const
{
	return iMarketRep_;
}

int HEnv::marketRep() const
{
	return marketRep_;
}

string HEnv::loopingOrder() const
{
	return loopingOrder_;
}

const vector<string>& HEnv::markets() const
{
	return markets_;
}

const hff::IndexFilters& HEnv::indexFilters() const
{
	return indexFilters_;
}

const hff::LinearModel& HEnv::linearModel() const
{
	return linearModel_;
}

const hff::NonLinearModel& HEnv::nonLinearModel() const
{
	return nonLinearModel_;
}

const string HEnv::baseDir() const
{
	return baseDir_;
}

const vector<int>& HEnv::idates() const
{
	return idates_;
}

const vector<int>& HEnv::idates(string market) const
{
	map<string, vector<int> >::const_iterator it = midates_.find(market);
	if( it != midates_.end() )
		return it->second;
	return vector<int>();
}

const vector<string>& HEnv::tickers() const
{
	return tickers_;
}

string HEnv::market() const
{
	return market_;
}

string HEnv::model() const
{
	return model_;
}

int HEnv::idate() const
{
	return idate_;
}

int HEnv::d1() const
{
	return d1_;
}

int HEnv::d2() const
{
	return d2_;
}

//const vector<boost::thread::id>& HEnv::threadIds() const
//{
//	return vThreadId_;
//}

/*
*	Modifiers
*/

void HEnv::multiThreadTicker(bool tf)
{
	multiThreadTicker_ = tf;
}

void HEnv::multiThreadModule(bool tf)
{
	multiThreadModule_ = tf;
}

void HEnv::nMaxThreadTicker(int n)
{
	nMaxThreadTicker_ = n;
}

void HEnv::nDates(int n)
{
	nDates_ = n;
}

void HEnv::iMarketRep(int i)
{
	iMarketRep_ = i;
}

void HEnv::marketRep(int n)
{
	marketRep_ = n;
}

void HEnv::loopingOrder(string order)
{
	loopingOrder_ = order;
}

void HEnv::markets(const vector<string>& markets)
{
	markets_ = markets;
}

void HEnv::indexFilters(const hff::IndexFilters& indexFilters)
{
	indexFilters_ = indexFilters;
}

void HEnv::linearModel(const hff::LinearModel& linearModel)
{
	linearModel_ = linearModel;
}

void HEnv::nonLinearModel(const hff::NonLinearModel& nonLinearModel)
{
	nonLinearModel_ = nonLinearModel;
}

void HEnv::baseDir(string dir)
{
	baseDir_ = dir;
}

void HEnv::idates(const vector<int>& idates)
{
	idates_ = idates;
}

void HEnv::idates(string market, const vector<int>& idates)
{
	midates_[market] = idates;
}

void HEnv::tickers(const vector<string>& tickers)
{
	tickers_ = tickers;
}

void HEnv::market(string market)
{
	market_ = market;
}

void HEnv::model(string model)
{
	model_ = model;
}

void HEnv::idate(int idate)
{
	idate_ = idate;
}

void HEnv::d1(int d1)
{
	d1_ = d1;
}

void HEnv::d2(int d2)
{
	d2_ = d2;
}

//void HEnv::addThreadId(boost::thread::id id)
//{
//	boost::mutex::scoped_lock lock(id_mutex_);
//	vThreadId_.push_back(id);
//}

void HEnv::outfile(string outfile)
{
	if( f_ != 0 )
		delete f_;
	f_ = new TFile(outfile.c_str(), "recreate");
}

TFile* HEnv::outfile()
{
	return f_;
}

boost::mutex& HEnv::io_mutex()
{
	return io_mutex_;
}

boost::mutex& HEnv::root_mutex()
{
	return root_mutex_;
}

boost::mutex& HEnv::event_mutex()
{
	return event_mutex_;
}
