#include <gtlib_fastsim/FastSim.h>
#include <gtlib_model/mdl.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferPredMulti.h>
#include <gtlib_predana/PredWholeDay.h>
#include <jl_lib/mto.h>
#include <boost/filesystem.hpp>
using namespace std;

// endDay
//   getData
//     getSample
//   getStat

namespace gtlib {

int FastSim::cntPrint_ = 0;

FastSim::FastSim(const string& name, const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName, const vector<double>& vWgt,
		double restore, double thres, int maxShrTrade, double maxDollarPosNet, double maxDollarPosTicker)
	:debug_(false),
	verbose_(1),
	name_(name),
	baseDir_(baseDir),
	vPredModel_(vPredModel),
	vSigModel_(vSigModel),
	vTargetName_(vTargetName),
	nTarget_(vTargetName.size()),
	maxShrTrade_(maxShrTrade),
	vWgt_(vWgt),
	restore_(restore),
	thres_(thres),
	maxDollarPosNet_(maxDollarPosNet),
	maxDollarPosTicker_(maxDollarPosTicker),
	netDollarPos_(0.),
	model_(vPredModel[0]),
	market_(mdl::markets(model_)[0])
{
	openMsecs_ = mto::msecOpen(market_);
	closeMsecs_ = mto::msecClose(market_);
	viPred_ = vector<int>(nTarget_, 0);
}

void FastSim::endDay(vector<PredWholeDay*>& vPredDay, int udate, int idate, const map<string, CloseInfo>* mClose,
		const map<string, string>* mTicker2Uid, const map<string, vector<double>>* mMid)
{
	getData(vPredDay, mTicker2Uid, mMid);
	getStat(idate, mClose, mTicker2Uid);
}


void FastSim::endDay(int udate, int idate, const map<string, CloseInfo>* mClose,
		const map<string, string>* mTicker2Uid, const map<string, vector<double>>* mMid)
{
	vector<PredWholeDay*> vPredDay;
	int nPred = vPredModel_.size();
	for( int i = 0; i < nPred; ++i )
		vPredDay.push_back(new PredWholeDay(getBinSigBuf(baseDir_, vPredModel_[i], vSigModel_[i], vTargetName_[i], udate, idate)));
	getData(vPredDay, mTicker2Uid, mMid);
	for( auto p : vPredDay )
		delete p;
	getStat(idate, mClose, mTicker2Uid);
}

void FastSim::setDebug(const bool& x)
{
	debug_ = x;
}

void FastSim::setIPred(const vector<int>& v)
{
	viPred_ = v;
}

SignalBuffer* FastSim::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), "reg");
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, "reg", udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPredMulti(predPath), binPath);
	return pSigBuf;
}

SampleData FastSim::getSample(vector<PredWholeDay*>& vPredDay, const string& ticker, int msecs,
		const vector<double>& vMid)
{
	float sprd = vPredDay[0]->getSprd(ticker, msecs) / basis_pts_;
	int sec = (msecs - openMsecs_) / 1000;
	float price = vMid[sec];
	float sprdAdj = .5 * sprd + mto::fee_bpt(model_, ExecFeesPrimex(model_, ticker), price) / basis_pts_;
	int bidSize = vPredDay[1]->getBidSize(ticker, msecs);
	int askSize = vPredDay[1]->getAskSize(ticker, msecs);

	if( sprdAdj < 0. )
		sprdAdj = 0.;

	double pred = 0.;
	for( int i = 0; i < nTarget_; ++i )
	{
		double predPiece = 0.;
		if( viPred_[i] > 0 )
			predPiece = vPredDay[i]->getPred(ticker, msecs, viPred_[i]) * vWgt_[i];
		else
			predPiece = vPredDay[i]->getPred(ticker, msecs) * vWgt_[i];
		pred += predPiece;
	}

	return SampleData(sprd, sprdAdj, pred, price, bidSize, askSize, ticker, msecs);
}

DailySimStat FastSim::getStat(int idate,
		const map<string, CloseInfo>* mClose,
		const map<string, string>* mTicker2Uid)
{
	DailySimStat dss;
	getOvernightInfo(dss, mClose, mTicker2Uid);
	getIntradayInfo(dss, mClose, mTicker2Uid);
	getEnddayInfo(dss, mClose, mTicker2Uid);

	//dss.print(name_);
	if( verbose_ > 0 )
		printSummDay(idate, dss);
	mIdateStat_[idate] = dss;
}

void FastSim::getOvernightInfo(DailySimStat& dss,
		const map<string, CloseInfo>* mClose,
		const map<string, string>* mTicker2Uid)
{
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	map<string, string>::const_iterator ittEnd = mTicker2Uid->end();

	// Calculate pnl.
	for( auto& kv : mTickerData_ )
	{
		auto& ticker = kv.first;
		int tickerPos = kv.second.pos;

		auto itt = mTicker2Uid->find(ticker);
		if( itt != ittEnd )
		{
			string uid = itt->second;
			auto itc = mClose->find(uid);
			if( itc != itcEnd )
			{
				const CloseInfo& ci = itc->second;
				double retClop = tickerPos * ci.retclop;
				double retClcl = tickerPos * ci.retclcl;
				dss.retClop += retClop;
				dss.retClcl += retClcl;

				double price = ci.close;
				dss.clop += retClop * price;
				dss.clcl += retClcl * price;

				kv.second.close = ci.close;
			}
		}
	}
}

void FastSim::getEnddayInfo(DailySimStat& dss,
		const map<string, CloseInfo>* mClose,
		const map<string, string>* mTicker2Uid)
{
	map<string, CloseInfo>::const_iterator itcEnd = mClose->end();
	map<string, string>::const_iterator ittEnd = mTicker2Uid->end();
	double sumAbsDolPos = 0.;
	double sumSgnDolPos = 0.;
	double sumDvTraded = 0.;
	double sumDolPosTraded = 0.;
	int n = 0;
	for( auto& kv : mTickerData_ )
	{
		++n;
		auto& tdata = kv.second;
		int tickerPos = tdata.pos;
		double price = tdata.close;
		double dolPos = abs(tickerPos) * price;
		sumAbsDolPos += dolPos;
		sumSgnDolPos += tickerPos * price;
		sumDvTraded += tdata.dvDay;
		sumDolPosTraded += dolPos;
		if( verbose_ > 1 )
			printf("%10s %8.1f %8.1f\n", kv.first.c_str(), dolPos, tdata.dvDay);
		tdata.dvDay = 0.;
	}
	dss.absDolPos = sumAbsDolPos;
	dss.sgnDolPos = sumSgnDolPos;
	dss.turnover = sumDvTraded / sumDolPosTraded;

	// Adjust position.
	auto mte = mTickerData_.end();
	for( auto itc = mClose->begin(); itc != mClose->end(); ++itc )
	{
		double adjust = itc->second.adjust;
		if( adjust < 0.7 || adjust > 1.3 )
		{
			auto& uid = itc->first;
			auto itt = mTickerData_.find(uid);
			if( itt != mte )
				itt->second.pos *= adjust;
		}
	}

	netDollarPos_ = sumSgnDolPos;
}

void FastSim::print()
{
	if( cntPrint_++ < 1 )
	{
		printf("%20s %8s %8s %7s %5s %5s %5s | %7s %5s %5s %5s %5s %5s\n",
				"(per day):", "ntrd", "dv", "buyR", "intra", "clop", "clcl",
				"grsPos", "netPos", "gptI", "shrpI", "srptC", "turnover");
	}
	Acc accRetIntra;
	Acc accRetClop;
	Acc accRetClcl;
	Acc accRetTot;
	Acc accIntra;
	Acc accClop;
	Acc accClcl;
	Acc accTot;
	Acc accN;
	Acc accDV;
	Acc accTO;
	Acc accBuyDV;
	Acc accGrossPos;
	Acc accNetPos;
	for( auto& kv : mIdateStat_ )
	{
		auto& x = kv.second;
		int idate = kv.first;

		accRetIntra.add(x.retIntra);
		accRetClop.add(x.retClop);
		accRetClcl.add(x.retClcl);
		accRetTot.add(x.retIntra + x.retClcl);
		accIntra.add(x.intra);
		accClop.add(x.clop);
		accClcl.add(x.clcl);
		accTot.add(x.intra + x.clcl);
		accN.add(x.n);
		accDV.add(x.dv);
		accTO.add(x.turnover);
		accBuyDV.add(x.dvBuy);
		accGrossPos.add(x.absDolPos);
		accNetPos.add(x.sgnDolPos);
	}
	double stdevI = accIntra.stdev();
	double stdevC = accClcl.stdev();
	int N = accDV.n;
	double shrpI = accIntra.mean() / stdevI;
	double shrpC = accClcl.mean() / stdevC;
	double gptI = accIntra.sum / accDV.sum * basis_pts_;
	double buyR = accBuyDV.sum / accDV.sum * 100.;
	printf("%20s %8.0f %8.1f (%.1f%%) %5.1f %5.1f %5.1f | %7.1f %5.1f %5.1f %5.2f %5.2f %5.2f\n",
			name_.c_str(), accN.sum/N, accDV.sum/N, buyR,
			accIntra.sum/N, accClop.sum/N, accClcl.sum/N,
			accGrossPos.mean(), accNetPos.mean(),
			gptI, shrpI, shrpC, accTO.mean());
}

void FastSim::printSummDay(int idate, const DailySimStat& x)
{
	double gptRetIntra = x.retIntra / x.n * basis_pts_;
	double gptRetTot = (x.retIntra + x.retClcl) / x.n * basis_pts_;
	double gptIntra = x.intra / x.dv * basis_pts_;
	double gptTot = (x.intra + x.clcl) / x.dv * basis_pts_;
	double buyR = x.dvBuy / x.dv * 100.;
	printf("%20s %8d %8.1f (%.1f%%) %5.1f %5.1f %5.1f P(%.1f %5.1f) %5.2f\n",
			name_.c_str(), x.n, x.dv, buyR,
			x.intra, x.clop, x.clcl, x.absDolPos, x.sgnDolPos, x.turnover);
}

} // namespace gtlib

