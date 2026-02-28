#include <MFitting/MPseudoTradeFullSimComp.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <numeric>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MPseudoTradeFullSimComp::MPseudoTradeFullSimComp(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
	debug_(false),
	verbose_(0),
	maxShrChg_(10),
	omTarName_("tar60;0_xbmpred60;0"),
	weightedPnl_(false),
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
	if( conf.count("thres") )
		thres_ = atof(conf.find("thres")->second.c_str());
	if( conf.count("predName") )
		predName_ = conf.find("predName")->second.c_str();
	if( conf.count("omTarName") )
		omTarName_ = conf.find("omTarName")->second;
	if( conf.count("sigModel") )
		sigModel_ = conf.find("sigModel")->second;
	if( conf.count("maxPos") )
		maxPos_ = atoi(conf.find("maxPos")->second.c_str());
	if( conf.count("maxPosTicker") )
		maxPosTicker_ = atoi(conf.find("maxPosTicker")->second.c_str());
	if( conf.count("omDesc") )
		om_desc_ = conf.find("omDesc")->second.c_str();
}

MPseudoTradeFullSimComp::~MPseudoTradeFullSimComp()
{}

void MPseudoTradeFullSimComp::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;
	if(market_[0] == 'U')
		omTarName_ = "tar60;0";

	vector<int> idates = MEnv::Instance()->idates;
	int d1 = idates[0];
	int d2 = idates[idates.size() - 1];
	char buf[1000];
	sprintf(buf, "fullsimComp_%s_%d_%d", predName_.c_str(), d1, d2);
	dir_ = buf;
	mkd(dir_);

	sprintf(buf, "%s\\%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
	filename_ = xpf(buf);
	ofstream(filename_.c_str());

	if( debug_ )
		ofstream( (filename_ + ".debug").c_str() );
}

void MPseudoTradeFullSimComp::beginDay()
{
	int idate = MEnv::Instance()->idate;
	char cmd[1000];
	sprintf(cmd, "select o.symbol from hforders o inner join hforderevents e"
			" on o.orderid = e.orderid and o.idate = %d and e.idate = %d"
			" where o.orderschedtype = 1 and o.exectype = 'L' and e.qty > 0 "
			" group by o.symbol, o.orderid",
			idate);

}

void MPseudoTradeFullSimComp::beginTicker(const string& ticker)
{
}

void MPseudoTradeFullSimComp::endTicker(const string& ticker)
{
}

void MPseudoTradeFullSimComp::endDay()
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

	string txtPath = get_sigTxt_path(baseDir, sigModel_, idate, "tm", om_desc_);
	string predOMPath = get_pred_path(baseDir, model, idate, omTarName_, om_desc_);
	string predPath = get_pred_path(baseDir, model, idate, predName_, "reg");
	string lineTxt;
	string linePredOM;
	string linePred;

	ifstream ifsTxt(txtPath.c_str());
	ifstream ifsPredOM(predOMPath.c_str());
	ifstream ifsPred(predPath.c_str());
	getline(ifsTxt, lineTxt);
	getline(ifsPredOM, linePredOM);
	getline(ifsPred, linePred);

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
	while( getline(ifsTxt, lineTxt) && getline(ifsPredOM, linePredOM) && (predName_.empty() || getline(ifsPred, linePred)) )
	{
		vector<string> slTxt = split(lineTxt, '\t');
		vector<string> slPredOM = split(linePredOM, '\t');
		vector<string> slPred = split(linePred, '\t');

		string uid = slTxt[0];
		string ticker = slTxt[1];
		int sec = atoi(slTxt[2].c_str());
		int msecs = sec * 1000 + linMod.openMsecs;
		double sprd = atof(slPred[3].c_str()) / basis_pts_;
		double price = atof(slPred[4].c_str());

		int bidSize = ceil(atof(slTxt[3].c_str()) - .5);
		float bid = atof(slTxt[4].c_str()) - .5;
		float ask = atof(slTxt[5].c_str()) - .5;
		int askSize = ceil(atof(slTxt[6].c_str()) - .5);

		// pred.
		double pred = 0.;
		if( !predName_.empty() )
			pred += atof(slPred[2].c_str());
		pred /= basis_pts_;

		double fee = mto::fee_bpt(model, ExecFeesPrimex(model, ticker), price) / basis_pts_;
		double cost = sprd / 2. + fee;

		if( prev_uid == "" || prev_uid != uid )
		{
			itMid = mMid->find(uid);
			prev_uid = uid;
		}

		if( itMid != mMidEnd )
		{
			double mid = itMid->second[sec];
			if( mid > 0. )
			{
				if( pred > cost + thres_ / basis_pts_ )
				{
					double price = ask;
					vTrade.push_back(MPseudoTradePrep::Trade(uid, msecs, price, mid, cost, 1, pred, bidSize, askSize));
					++ntrds;
				}
				else if( -pred > cost + thres_ / basis_pts_ )
				{
					double price = bid;
					vTrade.push_back(MPseudoTradePrep::Trade(uid, msecs, price, mid, cost, -1, pred, bidSize, askSize));
					++ntrds;
				}
			}
		}
	}
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
		ofsDebug.open( (filename_ + ".debug").c_str(), ios::app );
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

					if(debug_)
					{
						string ticker = mUid2Ticker->find(uid)->second;
						char buf[1000];
						sprintf(buf, "%d %-8s %8d %3d %7.2f %7.2f %7.2f %.4f %8.4f\n", idate, ticker.c_str(), it->msecs, mult*it->bs, it->mid, it->price, ci.close, it->cost, pnl);
						ofsDebug << buf;
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
		sprintf(buf, "%d intra %6.1f pred %7.1f clop %7.1f clcl %7.1f tot %7.1f "
				"cum %8.1f %8.1f %8.1f %8.1f %8.1f "
				"pos %.1f nt %d dv %.1f gpt %.1f gptTot %.1f tpos %.1f %.1f",
			idate, totIntra, totPred, totClop, totClcl, totIntra + totClcl,
			accIntra_.sum, accPred_.sum, accClop_.sum, accClcl_.sum, accIntra_.sum + accClcl_.sum,
			currPos_, nbuy + nsell, totDV, gpt, gptTot,
			accPos.mean(), accPos.stdev());
		ofstream ofs(filename_.c_str(), ios::app);
		ofs << buf << endl;
	}
}

void MPseudoTradeFullSimComp::endJob()
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
