#include <HFitting/HBiLinModTest.h>
#include <HFitting.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include "TFile.h"
#include <boost/thread.hpp>
using namespace std;
using namespace hff;
using namespace sig;

HBiLinModTest::HBiLinModTest(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false),
verbose_(0),
debug_signal_(false),
debug_wgt_(false),
//txt_corr_(true),
cntDay_(0),
nThreads_(1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debug_signal") )
		debug_signal_ = conf.find("debug_signal")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("debug_wgt") )
		debug_wgt_ = conf.find("debug_wgt")->second == "true";
}

HBiLinModTest::~HBiLinModTest()
{}

void HBiLinModTest::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	pid_ = get_pid();

	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();
	const hff::NonLinearModel& nonLinMod = HEnv::Instance()->nonLinearModel();

	double ridgeReg = 100 * 20. / linMod.gridInterval;
	biLinMod_ = BiLinearModel(linMod.nHorizon, linMod.num_time_slices, om_num_sig_, /*om_fit_days_, */om_num_err_sig_, ridgeReg, ridgeReg, uids_, pid_, /*txt_corr_, */debug_wgt_, verbose_);
}

void HBiLinModTest::beginDay()
{
	int idate = HEnv::Instance()->idate();
	vector<string> markets = HEnv::Instance()->markets();
	const hff::LinearModel linMod = HEnv::Instance()->linearModel();
	string model = HEnv::Instance()->model();

	bool doAfterHours = false;
	string market = markets[0];
	longTicker_ = mto::longTicker(market);

	// Read hedge.
	string hPath = get_hedge_path(model, idate);
	hInfo_ = HedgeInfo(hPath, linMod.gpts);
	hValid_ = hInfo_.valid();

	// idatesFirst.
	int idateFirst = 0;
	vector<int> idates = HEnv::Instance()->idates();
	if( cntDay_ >= linMod.om_univ_fit_days() )
		idateFirst = idates[cntDay_ - linMod.om_univ_fit_days()];
	else
		idateFirst = idates[0];

	biLinMod_.beginDay(idate/*, idateFirst, idateFirst*/);

	corrPr_.clear();

	read_signal(idate);
}

void HBiLinModTest::beginMarket()
{
	string market = HEnv::Instance()->market();
	vector<string> tickers;
	idate_ = HEnv::Instance()->idate();
	idate_p_ = prevClose(market, idate_);
	idate_pp_ = prevClose(market, idate_p_);
	idate_n_ = nextClose(market, idate_);
	longTicker_ = mto::longTicker(market);

	if( hValid_ )
	{
		tickers = comp_ticker( marketTickers_[market], market );
		sort(tickers.begin(), tickers.end());

		get_uid_map(market, idate_p_);
	}
	HEnv::Instance()->tickers(tickers);
}

void HBiLinModTest::beginTicker(const string& ticker)
{
}

void HBiLinModTest::endTicker(const string& ticker)
{
}

void HBiLinModTest::endMarket()
{
}

void HBiLinModTest::endDay()
{
	++cntDay_;

	cout << " corr " << corrPr_.corr() << endl;
}

void HBiLinModTest::endJob()
{
}

void HBiLinModTest::get_uid_map(string market, int idate_p)
{
	mTickerUid_.clear();

	char cmd[1000];
	sprintf(cmd, "select %s, uniqueID from stockcharacteristics "
		" where %s and uniqueID is not null ",
		mto::compTicker(market).c_str(),
		mto::selChara(market, idate_p).c_str());
	vector<vector<string> > vv;
	GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);

	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
	{
		string ticker = trim((*it)[0]);
		string uid = trim((*it)[1]);
		if( uids_.count(uid) && !ticker.empty() )
			mTickerUid_[ticker] = uid;
	}
}

void HBiLinModTest::free_sig_object(int iSig)
{
	vSigInUse_[iSig] = 0;
}

void HBiLinModTest::read_signal(int idate)
{
	const hff::LinearModel& linMod = HEnv::Instance()->linearModel();

	// Bin file.
	char filename[200];
	sprintf(filename, "\\\\smrc-ltc-mrct23\\e\\hfs\\txtSigs\\om\\lseEuEcn\\sigTB%dEom.txt", idate);
	ifstream ifs(filename);

	string line;
	getline(ifs, line);

	// Read.
	int cnt = 0;
	vector<float> v(16);
	vector<float> debugSignal(64);
	while( getline(ifs, line) )
	{
		++cnt;
		vector<string> sl = split(line, '\t');
		string uid = trim(sl[0]);
		int time = atoi(sl[2].c_str());
		int k = time / linMod.gridInterval;
		int timeFac = ceil( float(linMod.gpts - 1) / linMod.num_time_slices );
		int timeIdx = k / timeFac;

		v[0] = atof(sl[6].c_str());		// quoteImb.
		v[1] = atof(sl[31].c_str());	// quoteImb since last trade.
		v[2] = atof(sl[7].c_str());		// sI.mret60.
		v[3] = atof(sl[22].c_str());	// sI.relativeHiLo.
		v[4] = atof(sl[32].c_str());	// gsig.
		//v[5] = atof(sl[33].c_str());	// gsig.
		//v[6] = atof(sl[34].c_str());	// gsig.
		//v[7] = atof(sl[35].c_str());	// gsig.
		v[8] = atof(sl[11].c_str());	// tob.
		v[9] = atof(sl[12].c_str());	// tob.
		v[10] = atof(sl[8].c_str());	// tob.
		v[11] = atof(sl[13].c_str());	// tob.
		//v[12] = atof(sl[36].c_str());	// gsig.
		//v[13] = atof(sl[37].c_str());	// gsig.
		v[14] = atof(sl[9].c_str());	// mret300.
		v[15] = atof(sl[10].c_str());	// mret300L.

		double logVolu = atof(sl[4].c_str());
		double logPrice = atof(sl[5].c_str());
		double absSprd = fabs( atof(sl[3].c_str()) );
		if( absSprd > 50 )
			absSprd = 50.;
		double target = atof(sl[18].c_str());

		for( int i = 0; i < hff::om_num_basic_sig_; ++i )
		{
			debugSignal[i] = v[i];
			debugSignal[i + hff::om_num_basic_sig_] = logVolu * v[i];
			debugSignal[i + 2 * hff::om_num_basic_sig_] = logPrice * v[i];
			debugSignal[i + 3 * hff::om_num_basic_sig_] = absSprd * v[i];
		}

		double pr = biLinMod_.pred(0, timeIdx, debugSignal);

		if( k >= skip_first_secs_ / linMod.gridInterval )
		{
			boost::mutex::scoped_lock lock(biLinMod_mutex_);
			int timeIdx = k / timeFac;
			biLinMod_.update(0, timeIdx, debugSignal, target);

			corrPr_.add(pr, target);
			if( cnt < 10 )
				cout << pr << "\t" << target << endl;
		}
	}
}
