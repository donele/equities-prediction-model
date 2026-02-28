#include <MFitting/MPseudoTradeAdapt.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <numeric>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MPseudoTradeAdapt::MPseudoTradeAdapt(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
verbose_(0),
maxShrChg_(10),
restoreMax_(0.),
hold_(0),
addON_(false),
ONmult_(1),
minMsecs_(0),
maxMsecs_(86400000),
minMsecsON_(0),
maxMsecsON_(86400000),
min_medvol_(0.),
max_medvol_(max_double_),
max_thres_(8.),
thres_seed_(0.),
thres_step_(0.1),
learning_rate_seed_(1.),
ntrdfail_(0),
fee_(0.),
maxPos_(0),
maxPosTicker_(0),
previous_position_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("thres") )
		thres_seed_ = atof(conf.find("thres")->second.c_str());
	if( conf.count("thresStep") )
		thres_step_ = atof(conf.find("thresStep")->second.c_str());
	if( conf.count("learningRate") )
		learning_rate_seed_ = atof(conf.find("learningRate")->second.c_str());
	if( conf.count("minMedvol") )
		min_medvol_ = atof(conf.find("minMedvol")->second.c_str());
	if( conf.count("maxMedvol") )
		max_medvol_ = atof(conf.find("maxMedvol")->second.c_str());
	if( conf.count("restoreMax") )
		restoreMax_ = atof(conf.find("restoreMax")->second.c_str());
	if( conf.count("addON") )
		addON_ = conf.find("addON")->second == "true";
	if( conf.count("predName") )
		predName_ = conf.find("predName")->second.c_str();
	if( conf.count("maxPos") )
		maxPos_ = atoi(conf.find("maxPos")->second.c_str());
	if( conf.count("hold") )
		hold_ = atoi(conf.find("hold")->second.c_str());
	if( conf.count("maxPosTicker") )
		maxPosTicker_ = atoi(conf.find("maxPosTicker")->second.c_str());
	if( conf.count("ONmult") )
		ONmult_ = atoi(conf.find("ONmult")->second.c_str());
	if( conf.count("minMsecs") )
		minMsecs_ = atoi(conf.find("minMsecs")->second.c_str());
	if( conf.count("maxMsecs") )
		maxMsecs_ = atoi(conf.find("maxMsecs")->second.c_str());
	if( conf.count("minMsecsON") )
		minMsecsON_ = atoi(conf.find("minMsecsON")->second.c_str());
	if( conf.count("maxMsecsON") )
		maxMsecsON_ = atoi(conf.find("maxMsecsON")->second.c_str());

	thres_adapt_ = thres_seed_;
	learning_rate_ = learning_rate_seed_;
}

MPseudoTradeAdapt::~MPseudoTradeAdapt()
{}

void MPseudoTradeAdapt::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;

	fee_ = mto::fee_bpt(market_);

	string onDesc = "";
	if( addON_ )
		onDesc = (string)"_" + "ON" + itos(ONmult_);

	vector<int> idates = MEnv::Instance()->idates;
	int d1 = idates[0];
	int d2 = idates[idates.size() - 1];
	char buf[1000];
	sprintf(buf, "%s_r%.0f_h%d_%d_%d%s", predName_.c_str(), restoreMax_, hold_, d1, d2, onDesc.c_str());
	dir_ = buf;
	mkd(dir_);

	sprintf(buf, "%s\\%d_%d_%.1f_%.2f_%.2f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_seed_, thres_step_, learning_rate_seed_);
	filename_ = buf;
	ofstream ofs(filename_.c_str());
	ofs << "idate     intra    clop     clcl     total    cumintra cumclop  cumclcl  cumtotal nt  dv      pos     apos    tpos   tposd  ntmaxpos ntrdfail\n";

	if( debug_ )
		ofstream( (filename_ + ".debug").c_str() );
}

void MPseudoTradeAdapt::beginDay()
{
	int idate = MEnv::Instance()->idate;
	nMin_ = (mto::msecClose(market_, idate) - mto::msecOpen(market_, idate)) / 1000 / 60;
	vPositionMin_ = vector<double>(nMin_, previous_position_);
	GCurr::Instance()->set_idate(idate);
	ntrdfail_ = 0;
	trades_.clear();

	uids_.clear();
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	vector<string> markets = linMod.markets;
	for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
	{
		string market = *it;
		char cmd[1000];
		sprintf(cmd, "select uniqueID, medvolume from stockcharacteristics "
			" where idate = %d and market = '%s' and inuniverse = 1 ",
			idate, mto::code(market).c_str());

		vector<vector<string> > vv;
		//GODBC::Instance()->get(mto::hf(market))->ReadTable(cmd, &vv);
		GODBC::Instance()->read(mto::hf(market), cmd, vv);
		for( vector<vector<string> >::iterator itvv = vv.begin(); itvv != vv.end(); ++itvv )
		{
			string uid = trim((*itvv)[0]);
			double medvol = atof((*itvv)[1].c_str());
			if( min_medvol_ < medvol && medvol < max_medvol_ )
				uids_.insert(uid);
		}
	}
}

void MPseudoTradeAdapt::market_close(DaySum& daysum, ofstream& ofsDebug, int idate)
{
	// Clcl.
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	Acc accPos;
	double eodPos = 0.;
	double eodAbsPos = 0.;
	int ntmaxpos = 0;
	for( map<string, TickerPosition>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
	{
		map<string, CloseInfo>::const_iterator itc = mClose->find(it->first);
		if( itc != itcEnd )
		{
			string uid = it->first;
			string market = uid.substr(0, 2);
			double exchrat = GCurr::Instance()->exchrat("US", market);

			double price_close = itc->second.close * exchrat; // forex
			double clop = price_close * it->second.position * itc->second.retclop;
			double clcl = price_close * it->second.position * itc->second.retclcl;
			daysum.clop += clop;
			daysum.clcl += clcl;

			if( it->second.position != 0 )
			{
				double tickerpos = it->second.position * price_close;
				double abspos = abs(tickerpos);
				accPos.add(abspos);
				eodPos += tickerpos;
				eodAbsPos += abspos;

				if( abs(maxPosTicker_ - abspos) <= (1.1 * price_close) )
					++ntmaxpos;
			}
		}
	}
	previous_position_ = eodPos;
	
	accIntra_.add(daysum.intra);
	accClcl_.add(daysum.clcl);
	accClop_.add(daysum.clop);
	accNtrd_.add(daysum.nbuy + daysum.nsell);
	accDV_.add(daysum.dv);
	accTotal_.add(daysum.intra + daysum.clcl);

	// Adapt.

	// print.
	double gpt = 0.;
	if( daysum.dv > 0. )
		gpt = daysum.intra / daysum.dv * 10000.;

	if( verbose_ > 0 )
	{
		char buf[1000];
		sprintf(buf, "%d %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f %4d %.1f %.1f %.1f %.1f %.1f %d %d",//       pos %.1f nt %d dv %.1f gpt %.1f tpos %.1f %.1f",
			idate,
			daysum.intra, daysum.clop, daysum.clcl, daysum.intra + daysum.clcl,
			accIntra_.sum, accClop_.sum, accClcl_.sum, accIntra_.sum + accClcl_.sum,
			daysum.nbuy + daysum.nsell, daysum.dv,
			previous_position_, eodAbsPos, accPos.mean(), accPos.stdev(), ntmaxpos,
			ntrdfail_);
		ofstream ofs(filename_.c_str(), ios::app);
		ofs << buf << endl;
	}
}

void MPseudoTradeAdapt::thres_ana()
{
	int N = 10;
	//vector<double> vPnl(N);
	double pnl0 = 0.;
	OLS ols(1, 0, 2);

	for( double i = 0; i < N; ++i )
	{
		double extra_thres = i * thres_step_;
		double total_thres = extra_thres + thres_adapt_;
		double pnl = 0.;
		for( vector<TradeMade>::iterator it = trades_.begin(); it != trades_.end(); ++it )
		{
			if( it->maxThres >= total_thres )
				pnl += it->pnl;
		}
		ols.add(total_thres, pnl);
		if( i == 0 )
			pnl0 = pnl;
	}

	vector<float> coeffs = ols.getCoeffs();

	double diff = learning_rate_ * coeffs[1];

	if( pnl0 == 0. )
		diff = -0.05;
	else if( diff > 0.5 )
	{
		diff = 0.5;
		learning_rate_ *= 0.5;
	}
	else if( diff < -0.5 )
	{
		diff = -0.5;
		learning_rate_ *= 0.5;
	}
	else if( - 0.1 < diff && diff < 0.1 )
	{
		learning_rate_ *= 1.2;
	}

	if( thres_adapt_ + diff > max_thres_ )
	{
		diff = 0.;
		learning_rate_ *= 0.5;
	}
	else if( thres_adapt_ + diff < - max_thres_ )
	{
		diff = 0.;
		learning_rate_ *= 0.5;
	}

	thres_adapt_ += diff;

	printf("%s new threshold %.2f learnig rate %.3f\n", moduleName_.c_str(), thres_adapt_, learning_rate_);
}

void MPseudoTradeAdapt::endDay()
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	int idate = MEnv::Instance()->idate;

	for( map<string, TickerPosition>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
	{
		it->second.beginDay();
	}

	ofstream ofsDebug;
	if( debug_ )
	{
		ofsDebug.open( (filename_ + ".debug").c_str(), ios::app );
	}

	DaySum daysum;
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));
	const map<string, vector<MReadTickerState::TickerState> >* mTickerState = static_cast<const map<string, vector<MReadTickerState::TickerState> >*>(MEvent::Instance()->get(predName_, "mTickerState"));
	map<string, vector<MReadTickerState::TickerState> >::const_iterator mTickerStateEnd = mTickerState->end();
	for( map<string, vector<MReadTickerState::TickerState> >::const_iterator itt = mTickerState->begin(); itt != mTickerStateEnd; ++itt )
	{
		string uid = itt->first;
		if( !uids_.count(uid) )
			continue;

		map<string, CloseInfo>::const_iterator itc = mClose->find(uid);
		if( itc == mClose->end() )
			continue;

		TickerPosition& uidPos = mPos_[uid];
		string market = uid.substr(0, 2);
		double exchrat = GCurr::Instance()->exchrat("US", market);

		vector<MReadTickerState::TickerState>::const_iterator ittSecondEnd = itt->second.end();
		for( vector<MReadTickerState::TickerState>::const_iterator it = itt->second.begin(); it != ittSecondEnd; ++it )
		{
			const MReadTickerState::TickerState& ts = *it;
			if( ts.msecs < minMsecs_ || ts.msecs > maxMsecs_ )
				continue;

			int minmin = (ts.msecs - linMod.openMsecs) / 1000 / 60;

			double mid = 0.;
			if( ts.bid > 0. && ts.ask > 0. )
				mid = (ts.bid + ts.ask) / 2.0 * exchrat; // forex

			double marketPos = vPositionMin_[minmin];
			double restoreForce = 0.;
			if( maxPosTicker_ > 0. )
				restoreForce = - (uidPos.adjpos(ts.msecs, hold_) * mid) / maxPosTicker_ * (restoreMax_ / basis_pts_);

			double price = 0.;
			int bs = 0;
			int nshare = 0;
			if( ts.pred + restoreForce > ts.cost + thres_adapt_ / basis_pts_ ) // buy.
			{
				price = ts.ask * exchrat; // forex
				bs = 1;
				nshare = min(maxShrChg_, ts.askSize);
			}
			else if( - (ts.pred + restoreForce) > ts.cost + thres_adapt_ / basis_pts_ ) // sell.
			{
				price = ts.bid * exchrat; // forex
				bs = -1;
				nshare = min(maxShrChg_, ts.bidSize);
			}

			if( maxPosTicker_ != 0. )
			{
				int headroom = (maxPosTicker_ - bs * uidPos.position * mid) / mid;
				if( headroom < 0 )
					headroom = 0;
				nshare = min(nshare, headroom);
			}
			if( maxPos_ != 0. )
			{
				int headroom = (maxPos_ - bs * marketPos) / mid;
				if( headroom < 0 )
					headroom = 0;
				nshare = min(nshare, headroom);
			}

			if( bs != 0 && nshare == 0 )
				++ntrdfail_;

			// trade.
			if( bs != 0 && mid > 0. && nshare != 0 )
			{
				const CloseInfo& ci = itc->second;
				double posChg = bs * nshare * mid;
				double price_close = itc->second.close * exchrat; // forex

				// pnl to close.
				double pnl = posChg * (price_close / mid - 1.) - abs(posChg * ts.cost);

				// pnl to pred.
				double pnlPred = posChg * ts.pred - abs(posChg * ts.cost);

				// position.
				uidPos.add(ts.msecs, nshare * bs);
				for( int m = minmin; m < nMin_; ++m )
					vPositionMin_[m] += nshare * bs * mid;

				(bs > 0) ? ++daysum.nbuy : ++daysum.nsell;
				daysum.dv += fabs(posChg);
				daysum.intra += pnl;
				daysum.pred += pnlPred;

				float maxThres = 0.;
				if( bs > 0 ) // buy
					maxThres = (ts.pred + restoreForce - ts.cost) * basis_pts_;
				else if( bs < 0 ) // sell
					maxThres = (- (ts.pred + restoreForce) - ts.cost) * basis_pts_;

				trades_.push_back( TradeMade(maxThres, pnl) );

				if( debug_ )
				{
					char buf[1000];
					sprintf(buf, "%d %-8s %8d %3d %7.2f %7.2f %8.4f\n", idate, uid.c_str(), ts.msecs, nshare * bs, price, ci.close, pnl);
					ofsDebug << buf;
				}
			}
		}
	}

	market_close(daysum, ofsDebug, idate);
	thres_ana();
}

void MPseudoTradeAdapt::endJob()
{
	char buf[1000];
	sprintf(buf, "%s\\summ_%d_%d_%.1f_%.2f_%.2f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_seed_, thres_step_, learning_rate_seed_);
	ofstream ofs(xpf(buf).c_str());
	sprintf(buf, "maxPosTicker %.1f maxPos %.1f\n", maxPosTicker_, maxPos_);
	ofs << buf;
	sprintf(buf, "      intra     clop     clcl     total    ntrd    dv          gpt\n");
	ofs << buf;

	double gpt = 0.;
	if( accDV_.mean() > 0. )
		gpt = accIntra_.mean() / accDV_.mean() * basis_pts_;
	sprintf(buf, "mean  %7.1f %8.1f %8.1f %8.1f %7.1f %7.1f %7.1f\n",
		accIntra_.mean(), accClop_.mean(), accClcl_.mean(), accTotal_.mean(), accNtrd_.mean(), accDV_.mean(),
		gpt);
	ofs << buf;
	sprintf(buf, "stdev %7.1f %8.1f %8.1f %8.1f %7.1f %7.1f\n",
		accIntra_.stdev() / sqrt(accIntra_.n),
		accClop_.stdev() / sqrt(accClop_.n), accClcl_.stdev() / sqrt(accClcl_.n), accTotal_.stdev() / sqrt(accTotal_.n),
		accNtrd_.stdev() / sqrt(accNtrd_.n), accDV_.stdev() / sqrt(accDV_.n));
	ofs << buf;
}

//void MPseudoTradeAdapt::TickerPosition::add(int msecs, int shares)
//{
//	if( (shares > 0 && position >= 0) || (shares < 0 && position <= 0) ) // accumulate the position.
//	{
//		position += shares;
//		mMsecsPos[msecs] += shares;
//	}
//	else if( (shares > 0 && position < 0) || (shares < 0 && position > 0) ) //
//	{
//		position += shares;
//		int remain = shares;
//		for( map<int, int>::iterator it = mMsecsPos.begin(); it != mMsecsPos.end() && remain != 0; )
//		{
//			if( (it->second > 0 && remain < 0) || (it->second < 0 && remain > 0) ) // opposite sign.
//			{
//				int abs_remain = abs(remain);
//				int abs_position = abs(it->second);
//				if( abs_remain >= abs_position )
//				{
//					remain += it->second;
//					mMsecsPos.erase(it++);
//				}
//				else
//				{
//					it->second += remain;
//					remain = 0;
//					++it;
//				}
//			}
//			else
//				exit(7);
//		}
//
//		if( remain != 0 )
//			mMsecsPos[msecs] = remain;
//	}
//}
//
//int MPseudoTradeAdapt::TickerPosition::adjpos(int msecs, int hold)
//{
//	int pos = 0;
//	int maxMsecs = msecs - hold * 60 * 1000;
//	for( map<int, int>::iterator it = mMsecsPos.begin(); it != mMsecsPos.end(); ++it )
//	{
//		if( it->first <= maxMsecs )
//			pos += it->second;
//		else
//			break;
//	}
//	return pos;
//}
//
//void MPseudoTradeAdapt::TickerPosition::beginDay()
//{
//	int lastPos = position;
//	mMsecsPos.clear();
//	if( lastPos != 0 )
//		mMsecsPos[0] = lastPos;
//}
