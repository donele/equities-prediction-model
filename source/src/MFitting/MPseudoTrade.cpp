#include <MFitting/MPseudoTrade.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <jl_lib.h>
#include <jl_lib/GFee.h>
#include <map>
#include <string>
#include <numeric>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MPseudoTrade::MPseudoTrade(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
	debug_(false),
	resetPos_(true),
	verbose_(0),
	maxShrChg_(1),
	omTarName_("tar60;0_xbmpred60;0"),
	weightedPnl_(false),
	addON_(false),
	ONmult_(1.),
	minMsecsON_(0),
	maxMsecsON_(86400000),
	thres_(0.),
	maxPos_(0.),
	maxPosTicker_(0.),
	currPos_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("resetPos") )
		resetPos_ = conf.find("resetPos")->second == "true";
	if( conf.count("verbose") )
		verbose_ = atoi(conf.find("verbose")->second.c_str());
	if( conf.count("weightedPnl") )
		weightedPnl_ = conf.find("weightedPnl")->second == "true";
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());
	if( conf.count("addON") )
		addON_ = conf.find("addON")->second == "true";
	if( conf.count("predName") )
		predName_ = conf.find("predName")->second.c_str();
	if( conf.count("omTarName") )
		omTarName_ = conf.find("omTarName")->second;
	if( conf.count("sigModel") )
		sigModel_ = conf.find("sigModel")->second;
	if( conf.count("maxShrChg") )
		maxShrChg_ = atoi(conf.find("maxShrChg")->second.c_str());
	if( conf.count("maxPos") )
		maxPos_ = atoi(conf.find("maxPos")->second.c_str());
	if( conf.count("maxPosTicker") )
		maxPosTicker_ = atoi(conf.find("maxPosTicker")->second.c_str());
	if( conf.count("ONmult") )
		ONmult_ = atof(conf.find("ONmult")->second.c_str());
	if( conf.count("minMsecsON") )
		minMsecsON_ = atoi(conf.find("minMsecsON")->second.c_str());
	if( conf.count("maxMsecsON") )
		maxMsecsON_ = atoi(conf.find("maxMsecsON")->second.c_str());
	if( conf.count("omDesc") )
		om_desc_ = conf.find("omDesc")->second.c_str();
	if( conf.count("dest") )
	{
		pair<mmit, mmit> dests = conf.equal_range("dest");
		for( mmit mi = dests.first; mi != dests.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			for(string x : vs)
				dest_.insert(x[0]);
			//dest_ = set<string>(begin(vs), end(vs));
		}
	}
}

MPseudoTrade::~MPseudoTrade()
{}

void MPseudoTrade::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;
	if(market_[0] == 'U')
		omTarName_ = "tar60;0";

	string onDesc = "";
	if( addON_ )
	{
		char buf[10];
		sprintf(buf, "_ON%.2f", ONmult_);
		onDesc = buf;
		//onDesc = (string)"_" + "ON" + itos(ONmult_);
	}

	vector<int> idates = MEnv::Instance()->idates;
	int d1 = idates[0];
	int d2 = idates[idates.size() - 1];
	char buf[1000];
	sprintf(buf, "ptrade_%s_%d_%d%s", predName_.c_str(), d1, d2, onDesc.c_str());
	dir_ = buf;
	mkd(dir_);

	sprintf(buf, "%s\\%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
	filename_ = xpf(buf);
	ofstream(filename_.c_str());

	//if( debug_ )
		//ofstream( (filename_ + ".debug").c_str() );
}

void MPseudoTrade::beginDay()
{
	if(resetPos_)
		mPos_.clear();
}

void MPseudoTrade::beginTicker(const string& ticker)
{
}

void MPseudoTrade::endTicker(const string& ticker)
{
}

void MPseudoTrade::endDay()
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

	//string txtPath = (om_desc_ == "tevt") ? get_omBinTxt_tevt_path(model, idate) : get_omBinTxt_path(model, idate);
	string txtPath = get_sigTxt_path(baseDir, sigModel_, idate, "tm", om_desc_);
	string predOMPath = get_pred_path(baseDir, model, idate, omTarName_, om_desc_, udate);
	string predPath = get_pred_path(baseDir, model, idate, predName_, "reg", udate);
	string predONPath = get_pred_path(baseDir, model, idate, "tarON", "reg", udate);
	string lineTxt;
	string linePredOM;
	string linePred;
	string linePredON;

	ifstream ifsTxt(txtPath.c_str());
	ifstream ifsPredOM(predOMPath.c_str());
	ifstream ifsPred(predPath.c_str());
	ifstream ifsPredON(predONPath.c_str());
	getline(ifsTxt, lineTxt);
	getline(ifsPredOM, linePredOM);
	getline(ifsPred, linePred);
	getline(ifsPredON, linePredON);

	int err = 0;
	int ntrds = 0;
	int nuntrds = 0;
	string prev_uid;
	vector<MPseudoTradePrep::Trade> vTrade;
	vector<double>* pMid = 0;
	const map<string, vector<double> >* mMid = static_cast<const map<string, vector<double> >*>(MEvent::Instance()->get("", "mMid"));
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));
	map<string, vector<double> >::const_iterator mMidEnd = mMid->end();
	map<string, vector<double> >::const_iterator itMid = mMid->end();
	while( getline(ifsTxt, lineTxt) && getline(ifsPredOM, linePredOM) && (predName_.empty() || getline(ifsPred, linePred)) && (!addON_ || getline(ifsPredON, linePredON)) )
	{
		vector<string> slTxt = split(lineTxt, '\t');
		vector<string> slPredOM = split(linePredOM, '\t');
		vector<string> slPred = split(linePred, '\t');
		vector<string> slPredON = split(linePredON, '\t');

		string uid = slTxt[0];
		string ticker = slTxt[1];
		int sec = atoi(slTxt[2].c_str());
		int msecs = sec * 1000 + linMod.openMsecs;
		double sprd = atof(slPred[3].c_str()) / basis_pts_;
		double price = atof(slPred[4].c_str());
		//int bidSize = 1;
		//double bid = price - sprd * price / 2.;
		//double ask = sprd * price / 2. + price;
		//int askSize = 1;

		int bidSize = ceil(atof(slTxt[3].c_str()) - .5);
		float bid = atof(slTxt[4].c_str());
		float ask = atof(slTxt[5].c_str());
		int askSize = ceil(atof(slTxt[6].c_str()) - .5);
		char bidEx = slTxt[7].c_str()[0];
		char askEx = slTxt[8].c_str()[0];
		float mid = (bid + ask) / 2.;

		// pred.
		//double pred = 0.;
		double pred = atof(slPredOM[2].c_str());
		if( !predName_.empty() )
			pred += atof(slPred[2].c_str());
		if( addON_ )
		{
			double predON = atof(slPredON[2].c_str());
			if( minMsecsON_ <= msecs && msecs <= maxMsecsON_ )
				pred += predON * ONmult_;
		}
		pred /= basis_pts_;

		if(!dest_.empty())
		{
			char ex = (pred > 0.) ? askEx : bidEx;
			if(!dest_.count(ex))
				continue;
		}

		double fee = mto::fee_bpt(model, ExecFeesPrimex(model, ticker), price) / basis_pts_;
		double cost = sprd / 2. + fee;

		if( prev_uid == "" || prev_uid != uid )
		{
			itMid = mMid->find(uid);
			prev_uid = uid;
		}

		//if( itMid != mMidEnd )
		{
			//double mid = itMid->second[sec];
			if( mid > 0. )
			{
				if( pred > 0. && pred > cost + thres_ / basis_pts_ )
				{
					double price = ask;
					vTrade.push_back(MPseudoTradePrep::Trade(uid, msecs, price, mid, cost, 1, pred, bidSize, askSize));
					++ntrds;
				}
				else if( pred < 0. && -pred > cost + thres_ / basis_pts_ )
				{
					double price = bid;
					vTrade.push_back(MPseudoTradePrep::Trade(uid, msecs, price, mid, cost, -1, pred, bidSize, askSize));
					++ntrds;
				}
			}
		}
	}
	if(maxPos_ > 0.)
		sort(vTrade.begin(), vTrade.end(), MPseudoTradePrep::TradeLess());

// Pnl.
	double totIntra130 = 0.;
	double totIntra = 0.;
	double totClcl = 0.;
	double totClop = 0.;
	double totPred = 0.;
	double totDV = 0.;
	int nbuy = 0;
	int nsell = 0;

	ofstream ofsDebug;
	if( debug_ )
	{
		//ofsDebug.open( (filename_ + ".debug").c_str(), ios::app );
	}

	// Loop trades.
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	map<string, vector<double> >::const_iterator itmEnd = mMid->end();
	for( vector<MPseudoTradePrep::Trade>::iterator it = vTrade.begin(); it != vTrade.end(); ++it )
	{
		string uid = it->uid;
		double weightFactor = weightedPnl_ ? it->mid : 1.;
		int sec = (it->msecs - linMod.openMsecs) / 1000;

		map<string, CloseInfo>::const_iterator itc = mClose->find(uid);
		map<string, vector<double> >::const_iterator itm = mMid->find(uid);
		if( itc != itcEnd && itm != itmEnd )
		{
			int& uidPos = mPos_[uid];
			const CloseInfo& ci = itc->second;
			const vector<double>& vMid = itm->second;
			int nMid = vMid.size();

			// Check position.
			int maxMult = min(maxShrChg_, (it->bs > 0) ? it->askSize : it->bidSize);
			int mult = maxMult;
			bool posOK = false;
			bool posTickerOK = false;
			double posChg = 0.;
			for( ; mult > 0; --mult )
			{
				posChg = mult * weightFactor * it->bs;
				posOK = maxPos_ <= 0. || abs(currPos_ + posChg) <= maxPos_ || abs(currPos_ + posChg) < abs(currPos_);
				posTickerOK = maxPosTicker_ <= 0. || abs(uidPos * weightFactor + posChg) <= maxPosTicker_ || abs(uidPos * weightFactor + posChg) < abs(uidPos * weightFactor);
				if( posOK && posTickerOK )
				{
					// pnl 130 min
					int secTo = sec + 130 * 60;
					if( secTo >= nMid )
						secTo = nMid - 1;
					double pnl130 = posChg * (vMid[secTo] / it->mid - 1.) - abs(posChg * it->cost);
					totIntra130 += pnl130;

					// pnl to close.
					double pnl = posChg * (ci.close / it->mid - 1.) - abs(posChg * it->cost);
					totIntra += pnl;

					// pnl to pred.
					double pnlPred = posChg * it->pred - abs(posChg * it->cost);
					totPred += pnlPred;

					// position.
					currPos_ += mult * weightFactor * it->bs;
					uidPos += mult * it->bs;
					(it->bs > 0) ? ++nbuy : ++nsell;

					// dv
					totDV += fabs(posChg);

					//if(debug_)
					//{
					//	char buf[1000];
					//	sprintf(buf, "%d %-8s %8d %3d %7.2f %7.2f %7.2f %.4f %8.4f\n", idate, ticker.c_str(), it->msecs, mult*it->bs, it->mid, it->price, ci.close, it->cost, pnl);
					//	ofsDebug << buf;
					//}
					if(debug_)
					{
						string ticker = mUid2Ticker->find(uid)->second;
						float intraTar = ci.close / it->mid - 1.;
						printf("### %5s %5d %2.0f %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f\n",
								ticker.c_str(), sec, posChg, it->pred*basis_pts_,
								it->mid, ci.close, intraTar*basis_pts_, it->cost*basis_pts_, pnl*basis_pts_);
					}

					break;
				}
			}
		}
	}

	//if( debug_ )
	//{
	//	const map<string, string>* mUid2Ticker = static_cast<const map<string, string>*>(MEvent::Instance()->get("", "mUid2Ticker"));
	//	for( map<string, int>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
	//	{
	//		string uid = it->first;
	//		string ticker = mUid2Ticker->find(uid)->second;
	//		map<string, CloseInfo>::const_iterator itc = mClose->find(uid);
	//		map<string, vector<double> >::const_iterator itm = mMid->find(uid);

	//		const CloseInfo& ci = itc->second;
	//		int maxPosShares = maxPosTicker_ / ci.close;
	//		double notional = it->second * ci.close;
	//		char buf[1000];
	//		sprintf(buf, "%s\t%d\t%.2f\t%d\t%.1f\n", ticker.c_str(), maxPosShares, ci.close, it->second, notional);
	//		ofsDebug << buf;
	//	}
	//}

	// Clcl.
	Acc accPos;
	for( map<string, int>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
	{
		map<string, CloseInfo>::const_iterator itc = mClose->find(it->first);
		if( itc != itcEnd )
		{
			double weightFactor = weightedPnl_ ? itc->second.close : 1.;
			double clop = weightFactor * it->second * itc->second.retclop;
			totClop += clop;
			double clcl = weightFactor * it->second * itc->second.retclcl;
			totClcl += clcl;

			if( it->second != 0 )
			{
				accPos.add(abs(it->second * itc->second.close));

				//if( debug_ )
				//{
				//	char buf[1000];
				//	sprintf(buf, "%-8s %8d %7.2f\n", it->first.c_str(), it->second, itc->second.close);
				//	ofsDebug << buf;
				//}
			}
		}
	}

	accIntra130_.add(totIntra130);
	accIntra_.add(totIntra);
	accClcl_.add(totClcl);
	accClop_.add(totClop);
	accPred_.add(totPred);
	accNtrd_.add(nbuy + nsell);
	accDV_.add(totDV);

	accTotal_.add(totIntra + totClcl);

	double gpt = 0.;
	double gptTot = 0.;
	if( totDV > 0. )
	{
		gpt = totIntra / totDV * 10000.;
		gptTot = (totIntra + totClcl) / totDV * 10000.;
	}

	if( verbose_ > 0 )
	{
		char buf[1000];
		sprintf(buf, "%d intra %6.1f clop %7.1f clcl %7.1f tot %7.1f "
				"cum %8.1f %8.1f %8.1f %8.1f "
				"pos %.1f nt %d dv %.1f gpt %.1f gptTot %.1f tpos %.1f %.1f",
			idate, totIntra, totClop, totClcl, totIntra + totClcl,
			accIntra_.sum, accClop_.sum, accClcl_.sum, accIntra_.sum + accClcl_.sum,
			currPos_, nbuy + nsell, totDV, gpt, gptTot,
			accPos.mean(), accPos.stdev());
		ofstream ofs(filename_.c_str(), ios::app);
		ofs << buf << endl;
	}
}

void MPseudoTrade::endJob()
{
	char buf[1000];
	sprintf(buf, "%s\\summ_%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
	ofstream ofs(xpf(buf).c_str());
	sprintf(buf, "maxPosTicker %.1f maxPos %.1f\n", maxPosTicker_, maxPos_);
	ofs << buf;
	sprintf(buf, "     intra pred clop clcl total ntrd dv gpt gptTot\n");
	ofs << buf;

	double gpt = 0.;
	double gptTot = 0.;
	if( accDV_.mean() > 0. )
	{
		gpt = accIntra_.mean() / accDV_.mean() * basis_pts_;
		gptTot = accTotal_.mean() / accDV_.mean() * basis_pts_;
	}
	sprintf(buf, "mean %7.2f %7.2f %7.2f %7.2f %7.2f %7.0f %7.1f %7.1f %7.1f\n",
		accIntra_.mean(),
		accPred_.mean(),
		accClop_.mean(),
		accClcl_.mean(),
		accTotal_.mean(),
		accNtrd_.mean(),
		accDV_.mean(),
		gpt,
		gptTot);
	ofs << buf;
	sprintf(buf, "stdev %7.2f %7.2f %7.2f %7.2f %7.2f %7.1f %7.1f\n",
		accIntra_.stdev() / sqrt(accIntra_.n),
		accPred_.stdev() / sqrt(accPred_.n),
		accClop_.stdev() / sqrt(accClop_.n),
		accClcl_.stdev() / sqrt(accClcl_.n),
		accTotal_.stdev() / sqrt(accTotal_.n),
		accNtrd_.stdev() / sqrt(accNtrd_.n),
		accDV_.stdev() / sqrt(accDV_.n));
	ofs << buf;
}
