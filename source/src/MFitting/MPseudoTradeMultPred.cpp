#include <MFitting/MPseudoTradeMultPred.h>
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

// Spun out of MPsedoTrade_10m.
// Trade decision made from not only pred, but also position and direction.

MPseudoTradeMultPred::MPseudoTradeMultPred(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
	debug_(false),
	setSecondModelNTreesTmProd_(false),
	lastPredOptional_(false),
	verbose_(0),
	maxShrChg_(10),
	weightedPnl_(false),
	cutoffTime_(max_int_),
	thres_(0.),
	maxPos_(0),
	maxPosTicker_(0),
	currPos_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("setSecondModelNTreesTmProd") )
		setSecondModelNTreesTmProd_ = conf.find("setSecondModelNTreesTmProd")->second == "true";
	if( conf.count("lastPredOptional") )
		lastPredOptional_ = conf.find("lastPredOptional")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("weightedPnl") )
		weightedPnl_ = conf.find("weightedPnl")->second == "true";
	if( conf.count("cutoffTime") )
		cutoffTime_ = atoi(conf.find("cutoffTime")->second.c_str());
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());
	if( conf.count("sigModel") )
		sigModel_ = conf.find("sigModel")->second;
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
	predModels_ = getConfigValuesString(conf, "predModel");

	ntrees_ = getConfigValuesInt(conf, "ntrees");
	if(ntrees_.empty())
		ntrees_ = vector<int>(targetNames_.size(), 0);

	wgt_ = getConfigValuesFloat(conf, "wgt");
	if(wgt_.empty())
		wgt_ = vector<float>(targetNames_.size(), 1.);
}

MPseudoTradeMultPred::~MPseudoTradeMultPred()
{}

void MPseudoTradeMultPred::beginJob()
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

void MPseudoTradeMultPred::beginDay()
{
}

void MPseudoTradeMultPred::beginTicker(const string& ticker)
{
}

void MPseudoTradeMultPred::endTicker(const string& ticker)
{
}

void MPseudoTradeMultPred::endDay()
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

	if(setSecondModelNTreesTmProd_)
	{
		const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
		ntrees_[1] = nonLinMod.nTreesTmProd;
	}

	// Pnl
	if(predModels_.empty())
		predModels_ = vector<string>(targetNames_.size(), model);
	TradeSim dp(targetNames_, baseDir, predModels_, sigModel_, MEnv::Instance()->fitDesc, udate, idate, maxPosTicker_, ntrees_,
			wgt_, cutoffTime_, capLevel_, thres_, lastPredOptional_, debug_);
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

void MPseudoTradeMultPred::endJob()
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

MPseudoTradeMultPred::PredictionSet::PredictionSet()
	:time(0),
	bs(0),
	totPred(0.),
	mid(0.),
	sprd(0.),
	cost(0.)
{
}

MPseudoTradeMultPred::Prediction::Prediction(const string& u, int t, float p, float m, float s)
	:uid(u),
	time(t),
	pred(p),
	mid(m),
	sprd(s)
{
}

MPseudoTradeMultPred::PredictionSet::PredictionSet(const string& model, vector<Prediction>& v, float thres, bool lastTradeOptional)
	:uid(v[0].uid),
	time(v.rbegin()->time),
	bs(0),
	totPred(0.),
	mid(v.rbegin()->mid),
	sprd(v.rbegin()->sprd),
	cost(0.)
{
	for(int i = 0; i < v.size(); ++i)
		vPred.push_back(v[i].pred);

	for(auto x : vPred)
		totPred += x;
	float bid = mid * (1. - sprd/basis_pts_ / 2.);
	float ask = mid * (1. + sprd/basis_pts_ / 2.);
	float price = (totPred > 0.) ? ask : bid;
	double fee = mto::fee_bpt(model, price);
	cost = sprd / 2. + fee;

	if(lastTradeOptional)
	{
		int n = vPred.size();
		float pred = 0.;
		for(int i = 0; i < n-1; ++i)
			pred += vPred[i];

		//if(pred * totPred > 0.)
		{
			if(pred > 0. && pred > cost + thres)
				bs = 1;
			else if(pred < 0. && -pred > cost + thres)
				bs = -1;
		}
	}

	if(bs == 0)
	{
		if(totPred > 0. && totPred > cost + thres)
			bs = 1;
		else if(totPred < 0. && -totPred > cost + thres)
			bs = -1;
	}
}

MPseudoTradeMultPred::TradeSim::~TradeSim()
{
	for(auto& ifs : vIfsTxt_)
		delete ifs;
	vIfsTxt_.clear();
	for(auto& ifs : vIfsPred_)
		delete ifs;
	vIfsPred_.clear();
}

MPseudoTradeMultPred::TradeSim::TradeSim(vector<string>& targetNames, const string& baseDir,
		vector<string>& predModels, string& sigModel, const string& fitDesc, int udate, int idate, double maxPosTicker,
		const vector<int>& ntrees, vector<float> wgt, int cutoffTime, float capLevel, float thres, bool lastPredOptional, bool debug)
	:model_(predModels[0]),
	lastPredOptional_(lastPredOptional),
	debug_(debug),
	cutoffTime_(cutoffTime),
	thres_(thres),
	maxPosTicker_(maxPosTicker),
	wgt_(wgt)
{
	int NT = targetNames.size();
	vector<string> txtPath;
	txtPath.push_back(get_sigTxt_path(baseDir, sigModel, idate, "om", fitDesc));
	for(int i = 1; i < NT;++i)
		txtPath.push_back(get_sigTxt_path(baseDir, sigModel, idate, "tm", fitDesc));

	vector<string> predPath;
	for(int i = 0; i < NT; ++i)
		predPath.push_back(get_pred_path(baseDir, predModels[i], idate, targetNames[i], fitDesc, udate));

	for(int i = 0; i < NT; ++i)
	{
		vIfsTxt_.push_back(new ifstream);
		vIfsTxt_[i]->open(txtPath[i].c_str());
	}

	for(int i = 0; i < NT; ++i)
	{
		vIfsPred_.push_back(new ifstream);
		vIfsPred_[i]->open(predPath[i].c_str());
	}

	// Read headers
	string header;
	for(auto& ifs : vIfsTxt_)
	{
		if(!getline(*ifs, header))
			exit(59);
	}
	for(int i = 0; i < NT; ++i)
	{
		if(!getline(*vIfsPred_[i], header))
			exit(60);
		vIndxPred_.push_back(getIndxPred(header, ntrees[i]));
	}

	select_trades();
}

int MPseudoTradeMultPred::TradeSim::getIndxPred(const string& line, int ntrees)
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

MPseudoTradeMultPred::DayPnl::DayPnl(float ti, float tcc, float tco, int nb, int ns, float tdv)
	:totIntra(ti),
	totClcl(tcc),
	totClop(tco),
	nbuy(nb),
	nsell(ns),
	totDV(tdv)
{
}

void MPseudoTradeMultPred::TradeSim::select_trades()
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

void MPseudoTradeMultPred::TradeSim::clear_buffer()
{
	buffer_.clear();
}

bool MPseudoTradeMultPred::TradeSim::getNextTrade(PredictionSet& p)
{
	vector<string> lineTxt(vIfsTxt_.size());
	vector<string> linePred(vIfsPred_.size());
	while(1)
	{
		for(int i = 0; i < vIfsTxt_.size(); ++i)
		{
			if(!getline(*vIfsTxt_[i], lineTxt[i]))
				return false;
		}
		for(int i = 0; i < vIfsPred_.size(); ++i)
		{
			if(!getline(*vIfsPred_[i], linePred[i]))
				return false;
		}
		vector<Prediction> vPrediction;
		for(int i = 0; i < lineTxt.size(); ++i)
		{
			Prediction p = getPred(vIndxPred_[i], lineTxt[i], linePred[i], wgt_[i]);
			vPrediction.push_back(p);
		}
		for(int i = 1; i < lineTxt.size(); ++i)
		{
			if(vPrediction[i].uid != vPrediction[0].uid || vPrediction[i].time != vPrediction[0].time)
				return false;
		}
		PredictionSet p0(model_, vPrediction, thres_, lastPredOptional_);
		if(isTradable(p0))
		{
			p = p0;
			return true;
		}
	}
	return false;
}

bool MPseudoTradeMultPred::TradeSim::isTradable(PredictionSet& p)
{
	return p.bs != 0;
}

MPseudoTradeMultPred::Prediction MPseudoTradeMultPred::TradeSim::getPred(int indxPred, const std::string& lineTxt, const std::string& linePred, float wgt)
{
	vector<string> slTxt = split(lineTxt, '\t');
	vector<string> slPred = split(linePred, '\t');

	string uid = slTxt[0];
	int time = atoi(slTxt[2].c_str());
	float pred = atof(slPred[indxPred].c_str()) * wgt;
	double sprd = atof(slPred[3].c_str());
	double mid = atof(slPred[4].c_str());

	return MPseudoTradeMultPred::Prediction(uid, time, pred, mid, sprd);
}

bool MPseudoTradeMultPred::TradeSim::isNewTicker(const PredictionSet& p)
{
	return !buffer_.empty() && buffer_.rbegin()->uid != p.uid;
}

void MPseudoTradeMultPred::TradeSim::flush_buffer()
{
	if(!buffer_.empty())
	{
		tradesBySym_[buffer_[0].uid] = buffer_;
		clear_buffer();
	}
}

void MPseudoTradeMultPred::TradeSim::addData(PredictionSet& p)
{
	buffer_.push_back(p);
}

MPseudoTradeMultPred::DayPnl MPseudoTradeMultPred::TradeSim::calculate_pnl()
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
