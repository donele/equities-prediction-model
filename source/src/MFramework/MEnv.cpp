#include <MFramework/MEnv.h>
#include <jl_lib/jlutil.h>
#include <string>
#include <vector>
#include <chrono>
using namespace std;

MEnv* MEnv::instance_ = nullptr;

MEnv* MEnv::Instance()
{
	static MEnv::Cleaner cleaner;
	if( instance_ == nullptr )
		instance_ = new MEnv();
	return instance_;
}

MEnv::Cleaner::~Cleaner() {
	delete MEnv::instance_;
	MEnv::instance_ = nullptr;
}

MEnv::MEnv()
{
	initialize();
}

MEnv::~MEnv()
{
}

void MEnv::initialize()
{
	multiThreadModule = false;
	multiThreadTicker = false;
	nMaxThreadTicker = 4;
	udate = 0;
	debugFlag;
	loopingOrder = "mdt";
	sourceFlag = "";
	fitDesc = "reg";
	sigType = "";
	fullTargetName = "";
	markets.clear();
	baseDir.clear();
	idates.clear();
	allIdates.clear();
	midates.clear();
	tickersDebug.clear();
	tickers.clear();
	market = "";
	model = "";
	nTickerMax = max_int_;
	idate = 0;
	d1 = 0;
	d2 = 0;
	linearModel = hff::LinearModel();
	nonLinearModel = hff::NonLinearModel();
	futSigPar = FutSigPar();
}

bool MEnv::multiThread() const
{
	return multiThreadTicker || multiThreadModule;
}
