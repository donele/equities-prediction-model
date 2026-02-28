#include <MFitting/MPseudoTrade_10m.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <jl_lib.h>
#include <gtlib/util.h>
#include <map>
#include <string>
#include <numeric>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MPseudoTrade_10m::MPseudoTrade_10m(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
	debug_(false),
	iPolicy_(0),
	verbose_(0),
	maxShrChg_(10),
	weightedPnl_(false),
	wgtSecondPred_(1.),
	capLevel_(1.),
	cutoffTime_(max_int_),
	thres_(0.),
	maxPos_(0),
	maxPosTicker_(0),
	currPos_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("weightedPnl") )
		weightedPnl_ = conf.find("weightedPnl")->second == "true";
	if( conf.count("wgtSecondPred") )
		wgtSecondPred_ = atof(conf.find("wgtSecondPred")->second.c_str());
	if( conf.count("capLevel") )
		capLevel_ = atof(conf.find("capLevel")->second.c_str());
	if( conf.count("cutoffTime") )
		cutoffTime_ = atoi(conf.find("cutoffTime")->second.c_str());
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());
	if( conf.count("sigModel") )
		sigModel_ = conf.find("sigModel")->second;
	if( conf.count("iPolicy") )
		iPolicy_ = atoi(conf.find("iPolicy")->second.c_str());
	if( conf.count("maxPos") )
		maxPos_ = atoi(conf.find("maxPos")->second.c_str());
	if( conf.count("maxPosTicker") )
		maxPosTicker_ = atoi(conf.find("maxPosTicker")->second.c_str());

	targetNames_ = getConfigValuesString(conf, "targetName");
	for(string& targetName : targetNames_)
	{
		if(!fullTargetName_.empty())
			fullTargetName_ += "_";
		fullTargetName_ += targetName;
	}

	ntrees_ = getConfigValuesInt(conf, "ntrees");
	if(ntrees_.empty())
		ntrees_ = vector<int>(targetNames_.size(), 0);
}

MPseudoTrade_10m::~MPseudoTrade_10m()
{}

void MPseudoTrade_10m::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;

	vector<int> idates = MEnv::Instance()->idates;
	int d1 = idates[0];
	int d2 = idates[idates.size() - 1];
	char buf[1000];
	sprintf(buf, "ptrade_%s_%d_%d", fullTargetName_.c_str(), d1, d2);
	dir_ = buf;
	mkd(dir_);

	sprintf(buf, "%s/summ_%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
	summPath_ = buf;

	if(verbose_ > 0)
	{
		sprintf(buf, "%s/%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
		detailPath_ = buf;
		ofstream(detailPath_.c_str());
	}
}

void MPseudoTrade_10m::beginDay()
{
}

void MPseudoTrade_10m::beginTicker(const string& ticker)
{
}

void MPseudoTrade_10m::endTicker(const string& ticker)
{
}

void MPseudoTrade_10m::endDay()
{
	const map<string, string>* mUid2Ticker = static_cast<const map<string, string>*>(MEvent::Instance()->get("", "mUid2Ticker"));

	// Trade.
	int idate = MEnv::Instance()->idate;
	int udate = MEnv::Instance()->udate;
	string model = MEnv::Instance()->model;
	if( sigModel_.empty() )
		sigModel_ = model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const string& baseDir = MEnv::Instance()->baseDir;

	// Pnl
	TradeSim dp(targetNames_, baseDir, model, sigModel_, udate, idate, maxPosTicker_, ntrees_, thres_,
			iPolicy_, wgtSecondPred_, cutoffTime_, capLevel_, debug_);
	DayPnl dayPnl = dp.calculate_pnl();

	accIntra_.add(dayPnl.totIntra);
	accClcl_.add(dayPnl.totClcl);
	accClop_.add(dayPnl.totClop);
	accNtrd_.add(dayPnl.nbuy + dayPnl.nsell);
	accDV_.add(dayPnl.totDV);
	accTotal_.add(dayPnl.totIntra + dayPnl.totClcl);

	if( verbose_ > 0 )
	{
		double gpt = 0.;
		double gptTot = 0.;
		if( dayPnl.totDV > 0. )
		{
			gpt = dayPnl.totIntra / dayPnl.totDV * 10000.;
			gptTot = (dayPnl.totIntra + dayPnl.totClcl) / dayPnl.totDV * 10000.;
		}

		char buf[1000];
		sprintf(buf, "%d intra %6.1f clop %7.1f clcl %7.1f tot %7.1f "
				"cum %8.1f %8.1f %8.1f %8.1f "
				"pos %.1f nt %d dv %.1f gpt %.1f gptTot %.1f",
				idate, dayPnl.totIntra, dayPnl.totClop, dayPnl.totClcl, dayPnl.totIntra + dayPnl.totClcl,
				accIntra_.sum, accClop_.sum, accClcl_.sum, accIntra_.sum + accClcl_.sum,
				currPos_, dayPnl.nbuy + dayPnl.nsell, dayPnl.totDV, gpt, gptTot);
				
		ofstream ofs(detailPath_.c_str(), ios::app);
		ofs << buf << endl;
	}
}

void MPseudoTrade_10m::endJob()
{
	ofstream ofs(summPath_.c_str());
	char buf[1000];
	sprintf(buf, "maxPosTicker %.1f maxPos %.1f\n", maxPosTicker_, maxPos_);
	ofs << buf;
	sprintf(buf, "     intra clop clcl total ntrd dv gpt gptTot\n");
	ofs << buf;

	double gpt = 0.;
	double gptTot = 0.;
	if( accDV_.mean() > 0. )
	{
		gpt = accIntra_.mean() / accDV_.mean() * basis_pts_;
		gptTot = accTotal_.mean() / accDV_.mean() * basis_pts_;
	}
	sprintf(buf, "mean %7.2f %7.2f %7.2f %7.2f %7.0f %7.1f %7.1f %7.1f\n",
		accIntra_.mean(),
		accClop_.mean(),
		accClcl_.mean(),
		accTotal_.mean(),
		accNtrd_.mean(),
		accDV_.mean(),
		gpt,
		gptTot);
	ofs << buf;
	sprintf(buf, "stdev %7.2f %7.2f %7.2f %7.2f %7.1f %7.1f\n",
		accIntra_.stdev() / sqrt(accIntra_.n),
		accClop_.stdev() / sqrt(accClop_.n),
		accClcl_.stdev() / sqrt(accClcl_.n),
		accTotal_.stdev() / sqrt(accTotal_.n),
		accNtrd_.stdev() / sqrt(accNtrd_.n),
		accDV_.stdev() / sqrt(accDV_.n));
	ofs << buf;
}

MPseudoTrade_10m::PredictionSet::PredictionSet()
	:time(0),
	bs(0),
	pred1m(0.),
	pred10m(0.),
	pred40m(0.),
	mid(0.),
	cost(0.)
{
}

MPseudoTrade_10m::Prediction::Prediction(const string& u, int t, float p, float m, float s)
	:uid(u),
	time(t),
	pred(p),
	mid(m),
	sprd(s)
{
}

MPseudoTrade_10m::PredictionSet::PredictionSet(const string& model, Prediction& p1, Prediction& p10, Prediction& p40)
	:uid(p1.uid),
	bs(0),
	time(p40.time),
	totPred(p1.pred + p40.pred),
	pred1m(p1.pred),
	pred10m(p10.pred),
	pred40m(p40.pred),
	mid(p40.mid),
	sprd(p40.sprd)
{
	float bid = mid * (1. - sprd/basis_pts_ / 2.);
	float ask = mid * (1. + sprd/basis_pts_ / 2.);
	float price = (totPred > 0.) ? ask : bid;
	double fee = mto::fee_bpt(model, price);
	cost = sprd / 2. + fee;
}

MPseudoTrade_10m::TradeSim::TradeSim(vector<string>& targetNames, const string& baseDir,
		string& model, string& sigModel, int udate, int idate, double maxPosTicker,
		const vector<int>& ntrees, float thres, int iPolicy, float wgtSecondPred,
		int cutoffTime, float capLevel, bool debug)
	:model_(model),
	debug_(debug),
	cutoffTime_(cutoffTime),
	capLevel_(capLevel),
	thres_(thres),
	iPolicy_(iPolicy),
	wgtSecondPred_(wgtSecondPred),
	maxPosTicker_(maxPosTicker)
{
	txt1mPath_ = get_sigTxt_path(baseDir, sigModel, idate, "om", "reg");
	txt10mPath_ = get_sigTxt_path(baseDir, sigModel, idate, "tm", "reg");
	txt40mPath_ = get_sigTxt_path(baseDir, sigModel, idate, "tm", "reg");

	pred1mPath_ = get_pred_path(baseDir, model, idate, targetNames[0], "reg", udate);
	pred10mPath_ = get_pred_path(baseDir, model, idate, targetNames[1], "reg", udate);
	pred40mPath_ = get_pred_path(baseDir, model, idate, targetNames[2], "reg", udate);

	ifsTxt1m_.open(txt1mPath_.c_str());
	ifsTxt10m_.open(txt10mPath_.c_str());
	ifsTxt40m_.open(txt40mPath_.c_str());

	ifsPred1m_.open(pred1mPath_.c_str());
	ifsPred10m_.open(pred10mPath_.c_str());
	ifsPred40m_.open(pred40mPath_.c_str());

	// Read headers
	string lineTxt1m;
	string lineTxt10m;
	string lineTxt40m;
	string linePred1m;
	string linePred10m;
	string linePred40m;
	getline(ifsTxt1m_, lineTxt1m);
	getline(ifsTxt10m_, lineTxt10m);
	getline(ifsTxt40m_, lineTxt40m);
	getline(ifsPred1m_, linePred1m);
	getline(ifsPred10m_, linePred10m);
	getline(ifsPred40m_, linePred40m);

	vIndxPred_.push_back(getIndxPred(linePred1m, ntrees[0]));
	vIndxPred_.push_back(getIndxPred(linePred10m, ntrees[1]));
	vIndxPred_.push_back(getIndxPred(linePred40m, ntrees[2]));

	select_trades();
}

int MPseudoTrade_10m::TradeSim::getIndxPred(const string& line, int ntrees)
{
	vector<string> slPred = split(line, '\t');
	string predColName = "pred";
	if(ntrees > 0)
		predColName += itos(ntrees);
	auto it = find(begin(slPred), end(slPred), predColName);
	int dist = distance(slPred.begin(), it);
	if(dist < 0 || dist >= slPred.size())
	{
		cerr << "column " <<  predColName << " not found\n";
		exit(41);
	}
	return dist;
}

MPseudoTrade_10m::DayPnl::DayPnl(float ti, float tcc, float tco, int nb, int ns, float tdv)
	:totIntra(ti),
	totClcl(tcc),
	totClop(tco),
	nbuy(nb),
	nsell(ns),
	totDV(tdv)
{
}

void MPseudoTrade_10m::TradeSim::select_trades()
{
	clear_buffer();
	PredictionSet p;
	while(getNextTrade(p))
	{
		if(isNewTicker(p))
			flush_buffer();
		addData(p);
	}
	flush_buffer();
}

void MPseudoTrade_10m::TradeSim::clear_buffer()
{
	buffer_.clear();
}

bool MPseudoTrade_10m::TradeSim::getNextTrade(PredictionSet& p)
{
	string lineTxt1m;
	string lineTxt10m;
	string lineTxt40m;
	string linePred1m;
	string linePred10m;
	string linePred40m;
	while( getline(ifsTxt1m_, lineTxt1m) && getline(ifsTxt10m_, lineTxt10m) && getline(ifsTxt40m_, lineTxt40m) &&
		getline(ifsPred1m_, linePred1m) && getline(ifsPred10m_, linePred10m) && getline(ifsPred40m_, linePred40m) )
	{
		Prediction p1 = getPred(vIndxPred_[0], lineTxt1m, linePred1m);
		Prediction p10 = getPred(vIndxPred_[1], lineTxt10m, linePred10m);
		Prediction p40 = getPred(vIndxPred_[2], lineTxt40m, linePred40m);
		if(p1.uid == p10.uid && p10.uid == p40.uid && p1.time == p10.time && p10.time == p40.time)
		{
			PredictionSet p0(model_, p1, p10, p40);
			if(isTradable(p0))
			{
				p = p0;
				return true;
			}
		}
	}
	return false;
}

bool MPseudoTrade_10m::TradeSim::isTradable(PredictionSet& p)
{
	p.bs = 0;
	if(iPolicy_ == 0)
		p.bs = get_bs(p.totPred, p.cost);
	else if(iPolicy_ == 1) // not trade if p11->71 is large enough to trade
	{
		float p_11_71 = (p.pred40m - p.pred10m)*wgtSecondPred_;
		int bs_second = get_bs(p_11_71, p.cost);
		if(bs_second == 0)
			p.bs = get_bs(p.totPred, p.cost);
		else
			p.bs = 0;
	}
	else if(iPolicy_ == 11) // trade only if p_11 is larger than p_11_71 and they are of same sign.
		// Missing the case when e1 > e2 and e1*e2<0.
	{
		float p_11 = p.pred1m + p.pred10m;
		float p_11_71 = (p.pred40m - p.pred10m)*wgtSecondPred_;
		if(p_11 * p_11_71 > 0. && fabs(p_11) > fabs(p_11_71))
			p.bs = get_bs(p.totPred, p.cost);
		else
			p.bs = 0;
	}
	else if(iPolicy_ == 111) // trade only if p_1 is larger than p_40 and they are of same sign.
		// Missing the case when e1 > e2 and e1*e2<0.
	{
		if(p.pred1m * p.pred40m > 0. && fabs(p.pred1m) > fabs(p.pred40m))
			p.bs = get_bs(p.totPred, p.cost);
		else
			p.bs = 0;
	}
	else if(iPolicy_ == 12) // trade only if p_11 is larger than p_11_71. (Improved 11).
	{
		float p_11 = p.pred1m + p.pred10m;
		float p_11_71 = (p.pred40m - p.pred10m)*wgtSecondPred_;
		int bs_second = get_bs(p_11_71, p.cost);
		if(bs_second != 0 || fabs(p_11) > fabs(p_11_71))
			p.bs = get_bs(p.totPred, p.cost);
		else
			p.bs = 0;
	}
	else if(iPolicy_ == 13) // trade only if p_11 is larger than p_11_71, and they are not tradable in opposite. (Improved 12).
	{
		float p_11 = p.pred1m + p.pred10m;
		float p_11_71 = (p.pred40m - p.pred10m)*wgtSecondPred_;
		int bs_first = get_bs(p_11, p.cost);
		int bs_second = get_bs(p_11_71, p.cost);
		if(bs_second != 0 || (fabs(p_11) > fabs(p_11_71) && bs_first * bs_second != -1))
			p.bs = get_bs(p.totPred, p.cost);
		else
			p.bs = 0;
	}
	else if(iPolicy_ == 14) // Use p11->71 only if enough time left.
	{
		float usePred = p.pred1m + p.pred40m;
		if(p.time > 22394) // more than 6 min left
			usePred = p.pred1m;
		else if(p.time > 23359) // more than 41 min left
			usePred = p.pred1m + p.pred10m;
		p.bs = get_bs(usePred, p.cost);
	}
	else if(iPolicy_ == 2)
	{
		float p_11 = p.pred1m + p.pred10m;
		int bs_first = get_bs(p_11, p.cost);
		float p_11_71 = (p.pred40m - p.pred10m)*wgtSecondPred_;
		int bs_second = get_bs(p_11_71, p.cost);
		int bs = get_bs(p.totPred, p.cost);
		if(bs_second == 0)
			p.bs = bs;
		else if(bs_first * bs_second == -1)
			p.bs = bs_first;
	}
	else if(iPolicy_ == 3)
	{
		if(p.pred1m * p.pred40m >= 0.)
			p.bs = get_bs(p.totPred, p.cost);
		else
			p.bs = 0;
	}
	else if(iPolicy_ == 31)
	{
		float p_11 = p.pred1m + p.pred10m;
		float p_11_71 = (p.pred40m - p.pred10m)*wgtSecondPred_;
		if(p_11 * p_11_71 >= 0.)
			p.bs = get_bs(p.totPred, p.cost);
		else
			p.bs = 0;
	}
	else if(iPolicy_ == 4) // totPred is a sell, but p11 is strong buy, then buy instead of sell.
	{
		float p_11 = p.pred1m + p.pred10m;
		float p_11_71 = (p.pred40m - p.pred10m)*wgtSecondPred_;
		int bs_first = get_bs(p_11, p.cost);
		int bs = get_bs(p.totPred, p.cost);
		if(bs_first * bs < 0)
			p.bs = bs_first;
		else
			p.bs = bs;
	}
	else if(iPolicy_ == 5) // 1m, 40m, 71Close
	{
		float tot = p.pred1m + p.pred10m;
		if(p.time < cutoffTime_)
			tot += p.pred40m * wgtSecondPred_;
		p.bs = get_bs(tot, p.cost);
	}
	else if(iPolicy_ == 51) // 1m+40m or 1m+40m+71Close
	{
		float tot1 = p.pred1m + p.pred10m;
		float tot2 = tot1 + p.pred40m;
		p.bs = get_bs(tot1, p.cost);
		if(p.bs == 0)
			p.bs = get_bs(tot2, p.cost);
	}
	else if(iPolicy_ == 52) // 1m, 40m. Cap 71Close.
	{
		float pred40m = p.pred40m;
		if(fabs(pred40m) > fabs(p.pred10m) * capLevel_)
			pred40m = pred40m > 0. ? fabs(p.pred10m) * capLevel_ : -fabs(p.pred10m) * capLevel_;
		float tot = p.pred1m + p.pred10m;
		if(p.time < cutoffTime_)
			tot += pred40m * wgtSecondPred_;
		p.bs = get_bs(tot, p.cost);
	}
	else if(iPolicy_ == 6) // 1m, 10m, 11m61m, 71Close
	{
		//float tot = p.pred1m + p.pred10m
	}

	return p.bs != 0;
}

int MPseudoTrade_10m::TradeSim::get_bs(float pred, float cost)
{
	int bs = 0;
	if(pred > 0. && pred > cost + thres_)
		bs = 1;
	else if(pred < 0. && -pred > cost + thres_)
		bs = -1;
	return bs;
}

MPseudoTrade_10m::Prediction MPseudoTrade_10m::TradeSim::getPred(int indxPred, const std::string& lineTxt, const std::string& linePred)
{
	vector<string> slTxt = split(lineTxt, '\t');
	vector<string> slPred = split(linePred, '\t');

	string uid = slTxt[0];
	int time = atoi(slTxt[2].c_str());
	float pred = atof(slPred[indxPred].c_str());
	double sprd = atof(slPred[3].c_str());
	double mid = atof(slPred[4].c_str());

	return MPseudoTrade_10m::Prediction(uid, time, pred, mid, sprd);
}

bool MPseudoTrade_10m::TradeSim::isNewTicker(const PredictionSet& p)
{
	return !buffer_.empty() && buffer_.rbegin()->uid != p.uid;
}

void MPseudoTrade_10m::TradeSim::flush_buffer()
{
	if(!buffer_.empty())
	{
		tradesBySym_[buffer_[0].uid] = buffer_;
		clear_buffer();
	}
}

void MPseudoTrade_10m::TradeSim::addData(PredictionSet& p)
{
	buffer_.push_back(p);
}

MPseudoTrade_10m::DayPnl MPseudoTrade_10m::TradeSim::calculate_pnl()
{
	const map<string, vector<double> >* mMid = static_cast<const map<string, vector<double> >*>(MEvent::Instance()->get("", "mMid"));
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));

	// pnlIntra
	double totDV = 0.;
	double totIntra = 0.;
	int nbuy = 0;
	int nsell = 0;
	unordered_map<string, int> mPos;
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	map<string, vector<double> >::const_iterator itmEnd = mMid->end();
	for(auto& kv : tradesBySym_)
	{
		int uidPos = 0;
		const string& uid = kv.first;
		double totIntraTicker = 0.;
		map<string, CloseInfo>::const_iterator itc = mClose->find(uid);
		map<string, vector<double> >::const_iterator itm = mMid->find(uid);
		if( itc != itcEnd && itm != itmEnd )
		{
			const CloseInfo& ci = itc->second;
			const vector<double>& vMid = itm->second;

			for(auto& trade : kv.second)
			{
				int posChg = 1 * trade.bs;
				bool posTickerOK = maxPosTicker_ <= 0. || abs(uidPos + posChg) <= maxPosTicker_ || abs(uidPos + posChg) < abs(uidPos);
				if(posTickerOK)
				{
					(posChg > 0) ? ++nbuy : ++nsell;
					double pnl = posChg * (ci.close / trade.mid - 1.) - abs(posChg * trade.cost / basis_pts_);
					totIntra += pnl;
					totIntraTicker += pnl;
					totDV += fabs(posChg);
					uidPos += posChg;
					//if(debug_)
						//printf("uid %s %d %d %.2f %.2f %.2f %.2f %.2f %.4f %.4f\n",
								//uid.c_str(), trade.time, posChg, trade.totPred,
								//trade.pred1m, trade.pred40m, trade.mid, ci.close, trade.cost/basis_pts_, pnl);
				}
			}
			mPos[uid] = uidPos;
		}
		if(debug_)
			printf("debug uid %s pnlIntra %.2f\n", uid.c_str(), totIntraTicker);
	}

	// Clcl
	double totClop = 0.;
	double totClcl = 0.;
	for(auto& kv : mPos)
	{
		map<string, CloseInfo>::const_iterator itc = mClose->find(kv.first);
		if( itc != itcEnd )
		{
			double clop = kv.second * itc->second.retclop;
			totClop += clop;
			double clcl = kv.second * itc->second.retclcl;
			totClcl += clcl;
		}
	}

	return DayPnl(totIntra, totClcl, totClop, nbuy, nsell, totDV);
}
