#include <gtlib_predana/PredAnaByNTreeMultTarPrintTrades.h>
#include <gtlib_predana/PObjMultTar.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib/util.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferPredMulti.h>
#include <gtlib_predana/PredWholeDay.h>
#include <jl_lib/GFee.h>
#include <jl_lib/sort_util.h>
#include <jl_lib/mto.h>
#include <vector>
#include <future>
#include <string>
#include <functional>
#include <boost/filesystem.hpp>
using namespace std;

namespace gtlib {

PredAnaByNTreeMultTarPrintTrades::PredAnaByNTreeMultTarPrintTrades(const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName, int udate, const vector<int>& testdates,
		float omWgt, float tmWgt, const string& fitDesc, float flatFee, float thres, bool debug)
	:debug_(debug),
	flatFee_(flatFee),
	thres_(thres),
	nOm_(0),
	nTm_(0),
	omWgt_(omWgt),
	tmWgt_(tmWgt),
	model_(vPredModel[0]),
	testdates_(testdates),
	statDir_(getStatDir(fitDir, udate))
{
	useFlatFee_ = flatFee_ >= 0.;
	ofs_.open(getOutputPath().c_str());
	ofs_ << "idate msecs ticker mid nShr bidSize bid ask askSize ompred tmpred sprd fee edge volatSurp bpFromPrc bpFromRet sign tarIntra tar\n";

	mkd(statDir_);
	for( int idate : testdates )
	{
		PredWholeDay* pOm = new PredWholeDay(getBinSigBuf(baseDir, vPredModel[0], vSigModel[0], vTargetName[0], udate, idate, fitDesc));
		PredWholeDay* pTm = new PredWholeDay(getBinSigBuf(baseDir, vPredModel[1], vSigModel[1], vTargetName[1], udate, idate, fitDesc));
		getData(pOm, pTm, idate);
		delete pOm;
		delete pTm;
	}
}

PredAnaByNTreeMultTarPrintTrades::PredAnaByNTreeMultTarPrintTrades(const string& baseDir,
		const string& fitDir, const string& model,
		const string& targetName, int udate, const vector<int>& testdates,
		float omWgt, float tmWgt, const string& fitDesc, float flatFee, float thres)
	:PredAnaByNTreeMultTarPrintTrades(baseDir, fitDir, vector<string>(1, model), vector<string>(1, model),
			vector<string>(1, targetName), udate, testdates, omWgt, tmWgt, fitDesc, flatFee, thres)
{
}

SignalBuffer* PredAnaByNTreeMultTarPrintTrades::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate, const string& fitDesc)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), fitDesc);
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, fitDesc, udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPredMulti(predPath), binPath);
	return pSigBuf;
}

void PredAnaByNTreeMultTarPrintTrades::getData(PredWholeDay* pOm, PredWholeDay* pTm, int idate)
{
	mTickerVolatNorm_ = getVolatNorm(model_, idate);
	mTickerClose_ = getClose(model_, idate);
	map<string, vector<int>> tickerMsecs = pTm->getTickerMsecs();
	for( auto& kv : tickerMsecs )
	{
		const string& ticker = kv.first;
		if( pOm->exist(ticker) )
		{
			for( int& msecs : kv.second )
			{
				if( pOm->exist(ticker, msecs) )
					fetchData(pOm, pTm, idate, ticker, msecs);
			}
		}
	}
}

void PredAnaByNTreeMultTarPrintTrades::fetchData(PredWholeDay* pOm, PredWholeDay* pTm, int idate, const string& ticker, int msecs)
{
	float volatNorm = mTickerVolatNorm_[ticker];
	float closePrc = mTickerClose_[ticker];
	float sprd = pTm->getSprd(ticker, msecs);
	float mid = pTm->getPrice(ticker, msecs);
	//float fee_bpt = useFlatFee_ ? flatFee_ : mto::fee_bpt(model_, mid);
	//float sprdAdj = .5 * sprd / basis_pts_ + fee_bpt / basis_pts_;
	char buf[1000];

	//if( sprdAdj > 0. )
	//if( sprd > 0. )
	{
		float targetOm = pOm->getTarget(ticker, msecs) / basis_pts_;
		float predOm = pOm->getPred(ticker, msecs) / basis_pts_;
		float targetTm = pTm->getTarget(ticker, msecs) / basis_pts_;
		float predTm = pTm->getPred(ticker, msecs) / basis_pts_;
		float intraTar = pTm->getIntraTar(ticker, msecs) / basis_pts_;
		float predTot = predOm + predTm;
		float bid = pTm->getBidPrice(ticker, msecs);
		float ask = pTm->getAskPrice(ticker, msecs);
		int bidSize = pTm->getBidSize(ticker, msecs);
		int askSize = pTm->getAskSize(ticker, msecs);
		char bidEx = pTm->getBidEx(ticker, msecs);
		char askEx = pTm->getAskEx(ticker, msecs);
		int nShr = (predTot > 0.) ? askSize : -bidSize;

		float sign = 0.;
		float trdPrc = 0.;
		if(predTot > 0.)
		{
			sign = 1.;
			trdPrc = ask;
		}
		else
		{
			sign = -1.;
			trdPrc = bid;
		}
		//float fee_bpt = useFlatFee_ ? flatFee_ : mto::fee_bpt(model_, ExecFeesPrimex(model_, ticker), trdPrc);
		QuoteInfo nbbo;
		char primex = ExecFeesPrimex(model_, ticker);
		nbbo.bidEx = bidEx;
		nbbo.bidSize = bidSize;
		nbbo.bid = bid;
		nbbo.ask = ask;
		nbbo.askSize = askSize;
		nbbo.askEx = askEx;
		auto fees = GFee::Instance().feeBptBidAsk(model_, ExecFeesPrimex(model_, ticker), nbbo);
		float fee_bpt = predTot > 0. ? fees[1] : fees[0];
		float sprdAdj = .5 * sprd / basis_pts_ + fee_bpt / basis_pts_;

		float bpFromPrc = sign * (closePrc - trdPrc) / trdPrc * basis_pts_ - fee_bpt;
		float bpFromRet = (sign * intraTar - sprdAdj) * basis_pts_;
		bool tradable = predTot > sprdAdj + thres_ / basis_pts_ || -predTot > sprdAdj + thres_ / basis_pts_;
		if(tradable)
		{
			float edge = (abs(predTot) - sprdAdj) * basis_pts_;
			sprintf(buf, "%d %d %s %.2f %d %d %.2f %.2f %d %.2f %.2f %.2f %.2f %.1f %.3f %.2f %.2f %.0f %.2f %.2f\n",
					idate, msecs, ticker.c_str(), mid, nShr, bidSize, bid, ask, askSize,
					predOm*basis_pts_, predTm*basis_pts_, sprd, fee_bpt, edge, volatNorm, bpFromPrc, bpFromRet,
					sign, intraTar * basis_pts_, (targetOm + targetTm)*basis_pts_);
			ofs_ << buf;
		}
	}
}

string PredAnaByNTreeMultTarPrintTrades::getOutputPath()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename = "trades_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

} // namespace gtlib
