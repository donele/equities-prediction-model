#include <MFitting/MPseudoTradeProfile.h>
#include <MFitting.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <map>
#include <string>
#include <numeric>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MPseudoTradeProfile::MPseudoTradeProfile(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
verbose_(0),
maxShrChg_(10),
restoreMax_(0.),
hold_(0),
nP_(10),
addON_(false),
ONmult_(1),
minMsecs_(0),
maxMsecs_(86400000),
minMsecsON_(0),
maxMsecsON_(86400000),
thres_(0.),
ntrdfail_(0),
maxPos_(0),
maxPosTicker_(0),
previous_position_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());
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
	if( conf.count("condVar") )
		condVar_ = conf.find("condVar")->second;
}

MPseudoTradeProfile::~MPseudoTradeProfile()
{}

void MPseudoTradeProfile::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;
	perf_.resize(nP_);

	string onDesc = "";
	if( addON_ )
		onDesc = (string)"_" + "ON" + itos(ONmult_);

	vector<int> idates = MEnv::Instance()->idates;
	int d1 = idates[0];
	int d2 = idates[idates.size() - 1];
	char buf[1000];
	sprintf(buf, "%s_%s_r%.0f_h%d_%d_%d%s", condVar_.c_str(), predName_.c_str(), restoreMax_, hold_, d1, d2, onDesc.c_str());
	dir_ = buf;
	mkd(dir_);

	//sprintf(buf, "%s\\%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
	//filename_ = buf;
	//ofstream ofs(filename_.c_str());
	//ofs << "idate     intra    clop     clcl     total    cumintra cumclop  cumclcl  cumtotal nt  dv      pos     apos    tpos   tposd  ntmaxpos ntrdfail\n";

	//if( debug_ )
	//	ofstream( (filename_ + ".debug").c_str() );
}

void MPseudoTradeProfile::beginDay()
{
	int idate = MEnv::Instance()->idate;
	nMin_ = (mto::msecClose(market_, idate) - mto::msecOpen(market_, idate)) / 1000 / 60;
	vPositionMin_ = vector<double>(nMin_, previous_position_);
	GCurr::Instance()->set_idate(idate);
	ntrdfail_ = 0;

	//read_chara(idate);
	charaInfo_.read(market_, idate, condVar_, nP_, "uid");
}

//void MPseudoTradeProfile::read_chara(int idate)
//{
//	charaInfo_.clear();
//
//	char cmd[2000];
//	if( market_ == "US" || market_ == "CJ" )
//	{
//		sprintf(cmd, "select uniqueID, %s "
//			" from stockcharacteristics "
//			" where %s and uniqueID is not null and inuniverse = 1 "
//			" and sectype != 'P' and sectype != 'F' and sectype != 'X' ",
//			condVar_.c_str(),
//			mto::selChara(market_, idate).c_str());
//	}
//	else
//	{
//		sprintf(cmd, "select uniqueID, %s "
//			" from stockcharacteristics "
//			" where %s and uniqueID is not null and inuniverse = 1 ",
//			condVar_.c_str(),
//			mto::selChara(market_, idate).c_str());
//	}
//	vector<vector<string> > vv;
//	GODBC::Instance()->get(mto::hf(market_))->ReadTable(cmd, &vv);
//
//	int indx = 0;
//	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
//	{
//		string uid = trim((*it)[0]);
//		if( !uid.empty() )
//		{
//			float var = atof((*it)[1].c_str());
//			charaInfo_.mUidIndx[uid] = indx++;
//			charaInfo_.v.push_back(var);
//		}
//	}
//	charaInfo_.endDay(nP_);
//}

void MPseudoTradeProfile::endDay()
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	int idate = MEnv::Instance()->idate;

	for( map<string, TickerPosition>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
		it->second.beginDay();

	ofstream ofsDebug;
	if( debug_ )
		ofsDebug.open( (filename_ + ".debug").c_str(), ios::app );

	for( int i = 0; i < nP_; ++i )
		perf_[i].beginDay();

	// Intra.
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));
	const map<string, vector<MReadTickerState::TickerState> >* mTickerState = static_cast<const map<string, vector<MReadTickerState::TickerState> >*>(MEvent::Instance()->get(predName_, "mTickerState"));
	map<string, vector<MReadTickerState::TickerState> >::const_iterator mTickerStateEnd = mTickerState->end();
	for( map<string, vector<MReadTickerState::TickerState> >::const_iterator itt = mTickerState->begin(); itt != mTickerStateEnd; ++itt )
	{
		string uid = itt->first;
		int iBin = charaInfo_.get_indx(uid);

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
			if( ts.pred + restoreForce > ts.cost + thres_ / basis_pts_ ) // buy.
			{
				price = ts.ask * exchrat; // forex
				bs = 1;
				nshare = min(maxShrChg_, ts.askSize);
			}
			else if( - (ts.pred + restoreForce) > ts.cost + thres_ / basis_pts_ ) // sell.
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

				(bs > 0) ? ++perf_[iBin].nbuy : ++perf_[iBin].nsell;
				perf_[iBin].dv += fabs(posChg);
				perf_[iBin].intra += pnl;
				perf_[iBin].pred += pnlPred;
			}
		}
	}

	// Clcl.
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
			int iBin = charaInfo_.get_indx(uid);
			string market = uid.substr(0, 2);
			double exchrat = GCurr::Instance()->exchrat("US", market);

			double price_close = itc->second.close * exchrat; // forex
			double clop = price_close * it->second.position * itc->second.retclop;
			double clcl = price_close * it->second.position * itc->second.retclcl;
			perf_[iBin].clop += clop;
			perf_[iBin].clcl += clcl;

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

	for( int i = 0; i < nP_; ++i )
		perf_[i].endDay();
}

void MPseudoTradeProfile::endJob()
{
	char buf[1000];
	sprintf(buf, "%s\\summ_%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
	ofstream ofs(xpf(buf).c_str());
	sprintf(buf, "%7s %7s %8s %8s %8s %7s %7s %7s %8s %8s %8s %7s %7s\n",
		"gpt", "intra", "clop", "clcl", "total", "ntrd", "dv", "intra", "clop", "clcl", "total", "ntrd", "dv");
	ofs << buf;

	for( int i = 0; i < nP_; ++i )
	{
		double gpt = 0.;
		if( perf_[i].accDV.mean() > 0. )
			gpt = perf_[i].accIntra.mean() / perf_[i].accDV.mean() * basis_pts_;

		sprintf(buf, "%7.1f %7.1f %8.1f %8.1f %8.1f %7.1f %7.1f ",
			gpt,
			perf_[i].accIntra.mean(), perf_[i].accClop.mean(), perf_[i].accClcl.mean(),
			perf_[i].accTotal.mean(), perf_[i].accNtrd.mean(), perf_[i].accDV.mean());
		ofs << buf;

		sprintf(buf, "%7.1f %8.1f %8.1f %8.1f %7.1f %7.1f\n",
			perf_[i].accIntra.stdev() / sqrt(perf_[i].accIntra.n),
			perf_[i].accClop.stdev() / sqrt(perf_[i].accClop.n),
			perf_[i].accClcl.stdev() / sqrt(perf_[i].accClcl.n),
			perf_[i].accTotal.stdev() / sqrt(perf_[i].accTotal.n),
			perf_[i].accNtrd.stdev() / sqrt(perf_[i].accNtrd.n),
			perf_[i].accDV.stdev() / sqrt(perf_[i].accDV.n));
		ofs << buf;
	}
}

//void MPseudoTradeProfile::TickerPosition::add(int msecs, int shares)
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
//int MPseudoTradeProfile::TickerPosition::adjpos(int msecs, int hold)
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
//void MPseudoTradeProfile::TickerPosition::beginDay()
//{
//	int lastPos = position;
//	mMsecsPos.clear();
//	if( lastPos != 0 )
//		mMsecsPos[0] = lastPos;
//}

void MPseudoTradeProfile::Performance::beginDay()
{
	intra = 0.;
	pred = 0.;
	clcl = 0.;
	clop = 0.;
	dv = 0.;
	nbuy = 0;
	nsell = 0;
}

void MPseudoTradeProfile::Performance::endDay()
{
	accIntra.add(intra);
	accClcl.add(clcl);
	accClop.add(clop);
	accNtrd.add(nbuy + nsell);
	accDV.add(dv);
	accTotal.add(intra + clcl);
}
