#include <MFitting/MPseudoTradeMM.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_fastsim/fastsimObjs.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <numeric>
using namespace std;
using namespace gtlib;

MPseudoTradeMM::MPseudoTradeMM(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
weightedPnl_(false),
fee_(0.),
currPos_(0.),
cumIntra_(0.),
cumCost_(0.),
cum1_(0.),
cumBM_(0.),
cum130_(0.),
cumPred_(0.),
cumMMtm_(0.),
cumMMPred_(0.),
cumClcl_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("weightedPnl") )
		weightedPnl_ = conf.find("weightedPnl")->second == "true";
	if( conf.count("predName") )
		predName_ = conf.find("predName")->second.c_str();
}

MPseudoTradeMM::~MPseudoTradeMM()
{}

void MPseudoTradeMM::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;

	//if( market_ == "AK" || market_ == "AQ" || market_ == "KR" )
	//	fee_ = 17.;
	fee_ = mto::fee_bpt(market_);

	string wtd = "";
	if( weightedPnl_ )
		wtd = "_w";

	vector<int> idates = MEnv::Instance()->idates;
	int d1 = idates[0];
	int d2 = idates[idates.size() - 1];
	char buf[1000];
	sprintf(buf, "ptrade_%s_%d_%d_MM%s.txt", predName_.c_str(), d1, d2, wtd.c_str());
	filename_ = buf;
	ofstream(filename_.c_str());

	if( debug_ )
		ofstream( (filename_ + ".debug").c_str() );
}

void MPseudoTradeMM::beginDay()
{
}

void MPseudoTradeMM::beginTicker(const string& ticker)
{
}

void MPseudoTradeMM::endTicker(const string& ticker)
{
}

void MPseudoTradeMM::endDay()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	const map<string, string>* mTicker2Uid = static_cast<const map<string, string>*>(MEvent::Instance()->get("", "mTicker2Uid"));
	const map<string, vector<double> >* mMid = static_cast<const map<string, vector<double> >*>(MEvent::Instance()->get("", "mMid"));
	const map<string, CloseInfo>* mClose = static_cast<const map<string, CloseInfo>*>(MEvent::Instance()->get("", "mClose"));

// Read JL Prediction.
	string jldir = "";
	string jltar = "";
	if( predName_ == "BM" )
	{
		jldir = "KRatBM";
		jltar = "tar10;1_1.0_tar60;11_0.5";
	}
	else
	{
		jldir = "KRat";
		jltar = "tar130;1";
	}

	char filename2a[200];
	char filename2b[200];
	sprintf(filename2a, "\\\\smrc-ltc-mrct43\\f\\hffit\\%s\\txtSig\\tm\\sigTB%dKtm.txt", jldir.c_str(), idate);
	sprintf(filename2b, "\\\\smrc-ltc-mrct43\\f\\hffit\\%s\\fit\\%s\\preds\\pred%d.txt", jldir.c_str(), jltar.c_str(), idate);
	ifstream ifsa(xpf(filename2a).c_str());
	ifstream ifsb(xpf(filename2b).c_str());

	map<string, map<int, double> > mPred;
	if( ifsa.is_open() && ifsb.is_open() )
	{
		string linea;
		string lineb;
		while( getline(ifsa, linea) && getline(ifsb, lineb) )
		{
			vector<string> sla = split(linea, '\t');
			vector<string> slb = split(lineb, '\t');
			if( sla[0] != "uid" && slb[0] != "target" )
			{
				string uid = sla[0];
				int msecs = ceil(atof(sla[2].c_str()) * 1000. - 0.5) + linMod.openMsecs;
				double pred = atof(slb[2].c_str());
				mPred[uid][msecs] = pred / basis_pts_;
			}
		}
	}

// Read MM Trade.
	vector<MPseudoTradePrep::Trade> vTrade;
	map<string, string>::const_iterator mTicker2UidEnd = mTicker2Uid->end();
	map<string, map<int, double> >::iterator mPredEnd = mPred.end();

	char filename[1000];
	sprintf(filename, "\\\\smrc-ltc-mrct43\\f\\jelee\\work\\mx\\hffit\\KR\\simPred\\hford_KR%d%s_KQ2_3.txt", idate, predName_.c_str());
	ifstream ifs(xpf(filename).c_str());
	string line;
	getline(ifs, line);
	while( getline(ifs, line) )
	{
		vector<string> sl = split(line, '\t');
		map<string, string>::const_iterator itu = mTicker2Uid->find(trim(sl[1]));
		if( itu != mTicker2UidEnd )
		{
			string uid = itu->second;
			int msecs = ceil(atof(sl[3].c_str()) * 1000. - 0.5) + linMod.openMsecs;

			map<string, map<int, double> >::iterator itPred1 = mPred.find(uid);
			if( itPred1 != mPredEnd )
			{
				map<int, double>::iterator itPred2 = itPred1->second.find(msecs);
				if( itPred2 != itPred1->second.end() )
				{
					double ask = atof(sl[11].c_str());
					double bid = atof(sl[10].c_str());
					double sprd = 2. * (ask - bid) / (ask + bid);
					double mid = (ask + bid) / 2.;
					double cost = sprd / 2. + fee_ / basis_pts_;
					double price = atof(sl[5].c_str());
					int qty = atoi(sl[4].c_str());
					double mmom = atof(sl[48].c_str());
					double mmtm = atof(sl[50].c_str());
					vTrade.push_back(MPseudoTradePrep::Trade(uid, msecs, price, mid, cost, qty, itPred2->second, mmom, mmtm));
				}
			}
		}
	}

// Pnl.
	double totIntra = 0.;
	double totCost = 0.;
	double tot1 = 0.;
	double totBM = 0.;
	double tot130 = 0.;
	double totPred = 0.;
	double totMMtm = 0.;
	double totMMPred = 0.;
	double totClcl = 0.;
	int nbuy = 0;
	int nsell = 0;

	ofstream ofsDebug;
	if( debug_ )
		ofsDebug.open( (filename_ + ".debug").c_str(), ios::app );

	// Loop trades.
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	map<string, vector<double> >::const_iterator itmEnd = mMid->end();
	for( vector<MPseudoTradePrep::Trade>::iterator it = vTrade.begin(); it != vTrade.end(); ++it )
	{
		string uid = it->uid;
		double weightFactor = weightedPnl_ ? it->mid * abs(it->bs) : 1.;
		int sec = (it->msecs - linMod.openMsecs) / 1000;

		map<string, CloseInfo>::const_iterator itc = mClose->find(uid);
		map<string, vector<double> >::const_iterator itm = mMid->find(uid);
		if( itc != itcEnd && itm != itmEnd )
		{
			int& uidPos = mPos_[uid];
			const CloseInfo& ci = itc->second;
			const vector<double>& vMid = itm->second;
			int nMid = vMid.size();
			double posChg = weightFactor * sign(it->bs);
			//double pred = mPred[uid][it->msecs];

			int sec1 = sec + 1 * 60;
			if( sec1 >= nMid )
				sec1 = nMid - 1;
			int sec11 = sec + 11 * 60;
			if( sec11 >= nMid )
				sec11 = nMid - 1;
			int sec61 = sec + 61 * 60;
			if( sec61 >= nMid )
				sec61 = nMid - 1;
			int sec131 = sec + 131 * 60;
			if( sec131 >= nMid )
				sec131 = nMid - 1;

			// pnl to close. (intra)
			double pnl = posChg * (ci.close / it->mid - 1.) - abs(posChg * it->cost);
			totIntra += pnl;

			// Cost.
			double cost = - abs(posChg * it->cost);
			totCost += cost;

			// pnl 1 min.
			double pnl1 = posChg * (vMid[sec1] / it->mid - 1.);
			tot1 += pnl1;

			// pnl to 10 + .5 * 60 min.
			double pnlBM = 0.;
			if( vMid[sec1] > 0. && vMid[sec11] > 0. )
				pnlBM = posChg * ((vMid[sec11] / vMid[sec1] - 1.) + 0.5 * (vMid[sec61] / vMid[sec11] - 1.));
			totBM += pnlBM;

			// pnl 130 min
			double pnl130 = 0.;
			if( vMid[sec1] > 0. )
				pnl130 = posChg * (vMid[sec131] / vMid[sec1] - 1.);
			tot130 += pnl130;

			// pnl to pred.
			double pnlPred = posChg * it->pred;
			totPred += pnlPred;

			// pnl to MM pred.
			double pnlMMtm = posChg * it->mmtm;
			totMMtm += pnlMMtm;

			// pnl to MM pred.
			double pnlMMPred = posChg * (it->mmom + it->mmtm);
			totMMPred += pnlMMPred;

			// position.
			currPos_ += posChg;
			uidPos += (weightedPnl_ ? abs(it->bs) : 1) * sign(it->bs);
			(it->bs > 0) ? ++nbuy : ++nsell;

			if( debug_ )
			{
				char buf[1000];
				sprintf(buf, "%-8s %8d %7.2f %3.0f %8.4f %10.4f\n", uid.c_str(), it->msecs, it->price, posChg, pnl, totIntra);
				ofsDebug << buf;
			}
		}
	}

	// Clcl.
	for( map<string, int>::iterator it = mPos_.begin(); it != mPos_.end(); ++it )
	{
		map<string, CloseInfo>::const_iterator itc = mClose->find(it->first);
		if( itc != itcEnd )
		{
			double weightFactor = weightedPnl_ ? itc->second.close : 1.;
			double clop = weightFactor * it->second * itc->second.retclop;
			//totClop += clop;
			double clcl = weightFactor * it->second * itc->second.retclcl;
			totClcl += clcl;
		}
	}

	cumIntra_ += totIntra;
	cumCost_ += totCost;
	cum1_ += tot1;
	cumBM_ += totBM;
	cum130_ += tot130;
	cumPred_ += totPred;
	cumMMtm_ += totMMtm;
	cumMMPred_ += totMMPred;
	cumClcl_ += totClcl;

	accIntra_.add(totIntra);
	accCost_.add(totCost);
	acc1_.add(tot1);
	accBM_.add(totBM);
	acc130_.add(tot130);
	accPred_.add(totPred);
	accMMtm_.add(totMMtm);
	accMMPred_.add(totMMPred);
	accClcl_.add(totClcl);

	accTotal_.add(totIntra + totClcl);

	char buf[1000];
	sprintf(buf, "%d cl %5.1f cost %5.1f 1m %5.1f BM %5.1f 130M %5.1f jltm %5.1f mmtm %5.1f mmomtm %5.1f clcl %6.1f tot %6.1f "
		"\n%20s cum %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f pos %.1f nt %d",
		idate, totIntra, totCost, tot1, totBM, tot130, totPred, totMMtm, totMMPred, totClcl, totIntra + totClcl,
		" ", cumIntra_, cumCost_, cum1_, cumBM_, cum130_, cumPred_, cumMMtm_, cumMMPred_, cumClcl_, cumIntra_ + cumClcl_, currPos_, nbuy + nsell);
	ofstream ofs(filename_.c_str(), ios::app);
	ofs << buf << endl;
}

void MPseudoTradeMM::endJob()
{
	ofstream ofs(filename_.c_str(), ios::app);
	char buf[1000];
	sprintf(buf, "%7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f\n",
		accIntra_.mean(), accCost_.mean(), acc1_.mean(), accBM_.mean(), acc130_.mean(), accPred_.mean(), accMMtm_.mean(), accMMPred_.mean(), accClcl_.mean(), accTotal_.mean());
	ofs << buf;
	sprintf(buf, "%7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f\n",
		accIntra_.stdev() / sqrt(accIntra_.n),
		accCost_.stdev() / sqrt(accCost_.n),
		acc1_.stdev() / sqrt(acc1_.n),
		accBM_.stdev() / sqrt(accBM_.n),
		acc130_.stdev() / sqrt(acc130_.n),
		accPred_.stdev() / sqrt(accPred_.n),
		accMMtm_.stdev() / sqrt(accMMtm_.n),
		accMMPred_.stdev() / sqrt(accMMPred_.n),
		accClcl_.stdev() / sqrt(accClcl_.n),
		accTotal_.stdev() / sqrt(accTotal_.n));
	ofs << buf;
}
