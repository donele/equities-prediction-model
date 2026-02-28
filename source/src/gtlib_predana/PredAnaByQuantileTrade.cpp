#include <gtlib_predana/PredAnaByQuantileTrade.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_predana/PAnaSimple.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib/util.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_sigread/SignalBufferDecoratorPred.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_predana/PredWholeDay.h>
#include <jl_lib/GFee.h>
#include <jl_lib/sort_util.h>
#include <jl_lib/mto.h>
#include <vector>
#include <string>
#include <future>
#include <boost/filesystem.hpp>
using namespace std;

// Single (or combined) target. Full trees.
// Sprd Quantiles.
// Threshold is specified.

namespace gtlib {

PredAnaByQuantileTrade::PredAnaByQuantileTrade(const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName, const vector<float>& vWgt,
		int udate, const vector<int>& testdates, const string& fitDesc, float flatFee, float thres,
		bool volDepThres, bool tradedTickersOnly, bool debug)
	:debug_(debug),
	flatFee_(flatFee),
	thres_(thres),
	singleThread_(false),
	volDepThres_(volDepThres),
	tradedTickersOnly_(tradedTickersOnly),
	model_(vPredModel[0]),
	testdates_(testdates),
	statDir_(getStatDir(fitDir, udate))
{
	if(volDepThres_)
		thres_ = 0.;
	useFlatFee_ = flatFee_ >= 0.;
	mkd(statDir_);
	for( int idate : testdates )
	{
		PredWholeDay* pPredWholeDay = nullptr;
		for( int i = 0; i < vTargetName.size(); ++i )
		{
			const string& targetName = vTargetName[i];
			if( pPredWholeDay == nullptr )
				pPredWholeDay = new PredWholeDay(getBinSigBuf(baseDir, vPredModel[i], vSigModel[i], vTargetName[i], udate, idate, fitDesc), vWgt[i]);
			else
				pPredWholeDay->merge(new PredWholeDay(getBinSigBuf(baseDir, vPredModel[i], vSigModel[i], vTargetName[i], udate, idate, fitDesc), vWgt[i]));
		}
		getData(pPredWholeDay, idate);
		delete pPredWholeDay;
	}
}

PredAnaByQuantileTrade::PredAnaByQuantileTrade(const string& baseDir,
		const string& fitDir, const string& model, const string& targetName, float wgt,
		int udate, const vector<int>& testdates, const string& fitDesc, float flatFee, float thres)
	:PredAnaByQuantileTrade(baseDir, fitDir, vector<string>(1, model), vector<string>(1, model),
			vector<string>(1, targetName), vector<float>(1, wgt), udate, testdates, fitDesc, flatFee, thres)
{
}

SignalBuffer* PredAnaByQuantileTrade::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate, const string& fitDesc)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), fitDesc);
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, fitDesc, udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPred(predPath), binPath);
	//pSigBuf = new SignalBufferDecoratorHeader(
			//new SignalBufferDecoratorPred(
				//new SignalBufferBin(binPath, inputNames), predPath), binPath);
	return pSigBuf;
}

void PredAnaByQuantileTrade::getData(PredWholeDay* pPredWholeDay, int idate)
{
	ofstream ofs;
	char buf[1000];
	unordered_map<string, float> mTickerLogVol = getLogVol(model_, idate);
	unordered_map<string, float> mTickerVolatNorm = getVolatNorm(model_, idate);
	unordered_map<string, float> mTickerClose = getClose(model_, idate);
	unordered_map<string, float> mTickerVolDepThres;
	unordered_set<string> tradedTickers;
	if(volDepThres_)
		mTickerVolDepThres = getVolDepThres(model_, idate);
	if(tradedTickersOnly_)
		tradedTickers = getTradedTickers(model_, idate);
	auto tickerMsecs = pPredWholeDay->getTickerMsecs();
	float debugSum = 0.;
	for( auto& kv : tickerMsecs )
	{
		const string& ticker = kv.first;
		if(tradedTickersOnly_ && !tradedTickers.count(ticker))
			continue;

		float logVol = mTickerLogVol[ticker];
		float volatNorm = mTickerVolatNorm[ticker];
		float volDepThres = mTickerVolDepThres[ticker];
		float closePrc = mTickerClose[ticker];
		for( auto& msecs : kv.second )
		{
			float bmpred = pPredWholeDay->getBmpred(ticker, msecs) / basis_pts_;
			float pred = pPredWholeDay->getPred(ticker, msecs) / basis_pts_;
			float bid = pPredWholeDay->getBidPrice(ticker, msecs);
			float ask = pPredWholeDay->getAskPrice(ticker, msecs);
			float trdPrc = pred > 0. ? ask : bid;

			float sprd = pPredWholeDay->getSprd(ticker, msecs);
			//float mid = pPredWholeDay->getPrice(ticker, msecs);
			int bidSize = pPredWholeDay->getBidSize(ticker, msecs);
			int askSize = pPredWholeDay->getAskSize(ticker, msecs);
			char bidEx = pPredWholeDay->getBidEx(ticker, msecs);
			char askEx = pPredWholeDay->getAskEx(ticker, msecs);
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
			float fee_bpt = pred > 0. ? fees[1] : fees[0];
			float sprdAdj = (.5 * sprd + fee_bpt) / basis_pts_ + volDepThres;
			float target = pPredWholeDay->getTarget(ticker, msecs) / basis_pts_;
			float intraTar = pPredWholeDay->getIntraTar(ticker, msecs) / basis_pts_;
			bool tradable = pred > sprdAdj + thres_ / basis_pts_ || -pred > sprdAdj + thres_ / basis_pts_;
			//if( sprdAdj > 0. && tradable )
			if(debug_ && sprd > 0. && tradable)
			{
				debugSum += (pred>0)?(intraTar-sprdAdj):(-intraTar-sprdAdj);
				//printf("@@@ %.4f %.4f %.4f\n", pred*basis_pts_, intraTar*basis_pts_, sprdAdj*basis_pts_);
			}
			if( sprd > 0. && tradable )
			{
				vLogVol_.push_back(logVol);
				vVolatNorm_.push_back(volatNorm);
				vSprdAdj_.push_back(sprdAdj);
				vTarget_.push_back(target);
				vIntraTar_.push_back(intraTar);
				vPred_.push_back(pred);
				//vPrice_.push_back(trdPrc);
				vBid_.push_back(bid);
				vAsk_.push_back(ask);
				vClosePrc_.push_back(closePrc);
				vMsecs_.push_back(msecs);
				vFee_.push_back(fee_bpt);
			}
		}
	}
	if(debug_)
		printf("TTT %.4f\n", debugSum);
}

// ---------------------------------------------------------------------
// anaQuantile(var, nBins)
// ---------------------------------------------------------------------
void PredAnaByQuantileTrade::anaQuantile(const string& var, int nBins)
{
	vector<int> vSortedIndex = getSortedIndex(var);
	vector<future<PAnaSimple>> vFut;
	for( int iB = 0; iB < nBins; ++iB )
	{
		const vector<float>* vControl = getControl(var);
		if( vControl != nullptr )
		{
			if( singleThread_ )
				vFut.push_back(async(launch::deferred, &PredAnaByQuantileTrade::getAna,
							this, vControl, iB, nBins, vSortedIndex));
			else
				vFut.push_back(async(launch::async, &PredAnaByQuantileTrade::getAna,
							this, vControl, iB, nBins, vSortedIndex));
		}
	}
	ofstream ofs((getOutputPath(var)).c_str());
	for( auto& f : vFut )
	{
		PAnaSimple a = f.get();
		a.print(ofs);
	}
}

PAnaSimple PredAnaByQuantileTrade::getAna(const vector<float>* vControl, int iB, int nBins, const vector<int>& vSortedIndex)
{
	vector<int> vIndx = getIndx(vSortedIndex, iB, nBins);
	PAnaSimple p(vControl, &vMsecs_, &vSprdAdj_, &vPred_, &vTarget_, &vIntraTar_, &vFee_, &vBid_, &vAsk_, &vClosePrc_, vIndx, debug_);
	p.calculateThres(thres_);
	return p;
}

// ---------------------------------------------------------------------

const vector<float>* PredAnaByQuantileTrade::getControl(const string& var)
{
	const vector<float>* vControl = nullptr;
	if( var == "sprdAdj" )
		vControl = &vSprdAdj_;
	else if( var == "logVol" )
		vControl = &vLogVol_;
	else if( var == "price" )
		vControl = &vBid_;
	else if( var == "relVolat" )
		vControl = &vVolatNorm_;
	return vControl;
}

vector<int> PredAnaByQuantileTrade::getSortedIndex(const string& var)
{
	const vector<float>* vControl = getControl(var);
	int nData = vControl->size();
	vector<int> vSortedIndex(nData);
	gsl_heapsort_index(vSortedIndex, *vControl);
	return vSortedIndex;
}

vector<int> PredAnaByQuantileTrade::getIndx(const vector<int>& vSortedIndex, int iB, int nBins)
{
	int nData = vSortedIndex.size();
	int iFrom = iB * nData / nBins;
	int iTo = (iB + 1) * nData / nBins;
	if( iB == nBins - 1 )
		iTo = nData;
	vector<int> vIndx(vSortedIndex.begin() + iFrom, vSortedIndex.begin() + iTo);
	return vIndx;
}

string PredAnaByQuantileTrade::getOutputPath(const string& var, string desc)
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	if(volDepThres_)
		desc = (string)"VDT" + desc;
	if(tradedTickersOnly_)
		desc = (string)"Traded" + desc;
	desc += "_";
	desc += itos(thres_);
	string filename = "ana" + firstToUpper(var) + "QuantileTrade" + desc + '_' + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

} // namespace gtlib
