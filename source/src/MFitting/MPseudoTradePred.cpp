#include <MFitting/MPseudoTradePred.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/GFee.h>
#include <jl_lib/jlutil_tickdata.h>
#include <map>
#include <string>
#include <numeric>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MPseudoTradePred::MPseudoTradePred(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
verbose_(0),
maxShrChg_(10),
weightedPnl_(false),
rollingModelDate_(false),
predName_("restar60;0"),
thres_(0.),
fee_(0.),
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
	if( conf.count("rollingModelDate") )
		rollingModelDate_ = conf.find("rollingModelDate")->second == "true";
	if( conf.count("predName") )
		predName_ = conf.find("predName")->second.c_str();
	if( conf.count("maxPos") )
		maxPos_ = atoi(conf.find("maxPos")->second.c_str());
	if( conf.count("maxPosTicker") )
		maxPosTicker_ = atoi(conf.find("maxPosTicker")->second.c_str());
	if( conf.count("omDesc") )
		exit(7);
	if( conf.count("desc") )
		desc_ = conf.find("desc")->second.c_str();
}

MPseudoTradePred::~MPseudoTradePred()
{}

void MPseudoTradePred::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;
	fee_ = mto::fee_bpt(market_);
	string onDesc = "";

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

	if( debug_ )
		ofstream( (filename_ + ".debug").c_str() );
}

void MPseudoTradePred::beginDay()
{
}

void MPseudoTradePred::beginTicker(const string& ticker)
{
}

void MPseudoTradePred::endTicker(const string& ticker)
{
}

void MPseudoTradePred::endDay()
{
// Trade.
	int idate = MEnv::Instance()->idate;
	int udate = MEnv::Instance()->udate;
	if( rollingModelDate_ )
		udate = 0;
	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	// om tm
	bool isom = predName_.size() > 3 && predName_.substr(0, 3) == "res";
	string sigType = isom ? "om" : "tm";

	string txtPath = get_sigTxt_path(MEnv::Instance()->baseDir, model, idate, sigType, desc_);
	//if( isom )
	//	txtPath = (desc_ == "tevt") ? get_omBinTxt_tevt_path(model, idate) : get_omBinTxt_path(model, idate);
	//else
	//	txtPath = get_tmBinTxt_path(model, idate);
	string predPath = get_pred_path(MEnv::Instance()->baseDir, model, idate, predName_, desc_, udate);
	string lineTxt;
	string linePred;

	ifstream ifsTxt(txtPath.c_str());
	ifstream ifsPred(predPath.c_str());
	getline(ifsTxt, lineTxt);
	getline(ifsPred, linePred);

	int err = 0;
	int ntrds = 0;
	int nuntrds = 0;
	string prev_uid;
	vector<MPseudoTradePrep::Trade> vTrade;
	vector<double>* pMid = 0;
	const map<string, map<int, QuoteInfo> >* mMsecQuote = static_cast<const map<string, map<int, QuoteInfo> >*>(MEvent::Instance()->get("", "mMsecQuote"));
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));
	map<string, map<int, QuoteInfo> >::const_iterator mMsecQuoteEnd = mMsecQuote->end();
	map<string, map<int, QuoteInfo> >::const_iterator itMsecQuote = mMsecQuote->end();
	int sec_last_trade = 0;
	while( getline(ifsTxt, lineTxt) && getline(ifsPred, linePred) )
	{
		vector<string> slTxt = split(lineTxt, '\t');
		vector<string> slPred = split(linePred, '\t');

		string uid = slTxt[0].c_str();
		string ticker = slTxt[1].c_str();
		int msecs = ceil(atof(slTxt[2].c_str()) * 1000. - 0.5) + linMod.openMsecs;
		int sec = (msecs - linMod.openMsecs) / 1000;
		//int sec = atoi(slTxt[2].c_str());
		//int msecs = sec * 1000 + linMod.openMsecs;

		// Initialize.
		if( sec < sec_last_trade )
			sec_last_trade = 0;

		// pred.
		double pred = atof(slPred[2].c_str());
		pred /= basis_pts_;

		double sprd = atof(slPred[3].c_str()) / basis_pts_;

		if( prev_uid == "" || prev_uid != uid )
		{
			itMsecQuote = mMsecQuote->find(uid);
			prev_uid = uid;
		}

		if( itMsecQuote != mMsecQuoteEnd )
		{
			map<int, QuoteInfo>::const_iterator itq = itMsecQuote->second.lower_bound(msecs);
			if( itq != itMsecQuote->second.begin() )
			{
				advance(itq, -1);
				QuoteInfo quote = itq->second;
				int bidSize = quote.bidSize;
				double bid = quote.bid;
				double ask = quote.ask;
				int askSize = quote.askSize;
				if( bidSize <= 0 || bidSize > 1000000 )
					bidSize = 1;
				if( askSize <= 0 || askSize > 1000000 )
					askSize = 1;

				double mid = get_mid(quote);
				if( mid > 0. )
				{
					double cost = sprd / 2. + GFee::Instance().feeBpt(model, ExecFeesPrimex(model, ticker), (bid+ask)/2) / basis_pts_;
					if( pred > cost + thres_ / basis_pts_ && (sec_last_trade == 0 || sec > sec_last_trade + 15) )
					{
						double price = ask;
						vTrade.push_back(MPseudoTradePrep::Trade(uid, msecs, price, mid, cost, 1, pred, bidSize, askSize));
						++ntrds;
						sec_last_trade = sec;
					}
					else if( -pred > cost + thres_ / basis_pts_ && (sec_last_trade == 0 || sec > sec_last_trade + 15) )
					{
						double price = bid;
						vTrade.push_back(MPseudoTradePrep::Trade(uid, msecs, price, mid, cost, -1, pred, bidSize, askSize));
						++ntrds;
						sec_last_trade = sec;
					}
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
	for( vector<MPseudoTradePrep::Trade>::iterator it = vTrade.begin(); it != vTrade.end(); ++it )
	{
		string uid = it->uid;
		double weightFactor = weightedPnl_ ? it->mid : 1.;
		int sec = (it->msecs - linMod.openMsecs) / 1000;

		map<string, CloseInfo>::const_iterator itc = mClose->find(uid);
		if( itc != itcEnd )
		{
			int& uidPos = mPos_[uid];
			const CloseInfo& ci = itc->second;

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

					if( debug_ )
					{
						char buf[1000];
						sprintf(buf, "%d %-8s %8d %3d %7.2f %7.2f %8.4f\n", idate, uid.c_str(), it->msecs, mult*it->bs, it->price, ci.close, pnl);
						ofsDebug << buf;
					}

					break;
				}
			}
		}
	}

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
				accPos.add(abs(it->second * itc->second.close));
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
	if( totDV > 0. )
		gpt = totIntra / totDV * 10000.;

	if( verbose_ > 0 )
	{
		char buf[1000];
		sprintf(buf, "%d intra %6.1f pred %7.1f clop %7.1f clcl %7.1f tot %7.1f cum %8.1f %8.1f %8.1f %8.1f %8.1f pos %.1f nt %d dv %.1f gpt %.1f tpos %.1f %.1f",
			idate, totIntra, totPred, totClop, totClcl, totIntra + totClcl,
			accIntra_.sum, accPred_.sum, accClop_.sum, accClcl_.sum, accIntra_.sum + accClcl_.sum, currPos_, nbuy + nsell, totDV, gpt,
			accPos.mean(), accPos.stdev());
		ofstream ofs(filename_.c_str(), ios::app);
		ofs << buf << endl;
	}
}

void MPseudoTradePred::endJob()
{
	char buf[1000];
	sprintf(buf, "%s\\summ_%d_%d_%.1f.txt", dir_.c_str(), (int)maxPosTicker_, (int)maxPos_, thres_);
	ofstream ofs(xpf(buf).c_str());
	sprintf(buf, "maxPosTicker %.1f maxPos %.1f\n", maxPosTicker_, maxPos_);
	ofs << buf;
	sprintf(buf, "intra pred clop clcl total ntrd dv gpt\n");
	ofs << buf;

	double gpt = 0.;
	if( accDV_.mean() > 0. )
		gpt = accIntra_.mean() / accDV_.mean() * basis_pts_;
	sprintf(buf, "mean %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f\n",
		accIntra_.mean(), accPred_.mean(), accClop_.mean(), accClcl_.mean(), accTotal_.mean(), accNtrd_.mean(), accDV_.mean(),
		gpt);
	ofs << buf;
	sprintf(buf, "stdev %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f\n",
		accIntra_.stdev() / sqrt(accIntra_.n), accPred_.stdev() / sqrt(accPred_.n),
		accClop_.stdev() / sqrt(accClop_.n), accClcl_.stdev() / sqrt(accClcl_.n), accTotal_.stdev() / sqrt(accTotal_.n),
		accNtrd_.stdev() / sqrt(accNtrd_.n), accDV_.stdev() / sqrt(accDV_.n));
	ofs << buf;
}
