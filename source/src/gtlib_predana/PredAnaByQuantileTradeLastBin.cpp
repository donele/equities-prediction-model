#include <gtlib_predana/PredAnaByQuantileTradeLastBin.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_predana/PAnaSimple.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib/util.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_sigread/SignalBufferDecoratorPred.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_predana/PredWholeDay.h>
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

PredAnaByQuantileTradeLastBin::PredAnaByQuantileTradeLastBin(const string& baseDir,
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

PredAnaByQuantileTradeLastBin::PredAnaByQuantileTradeLastBin(const string& baseDir,
		const string& fitDir, const string& model, const string& targetName, float wgt,
		int udate, const vector<int>& testdates, const string& fitDesc, float flatFee, float thres)
	:PredAnaByQuantileTradeLastBin(baseDir, fitDir, vector<string>(1, model), vector<string>(1, model),
			vector<string>(1, targetName), vector<float>(1, wgt), udate, testdates, fitDesc, flatFee, thres)
{
}

SignalBuffer* PredAnaByQuantileTradeLastBin::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate, const string& fitDesc)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), fitDesc);
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, fitDesc, udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPred(predPath), binPath);
	return pSigBuf;
}

void PredAnaByQuantileTradeLastBin::getData(PredWholeDay* pPredWholeDay, int idate)
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
			int bidSize = pPredWholeDay->getBidSize(ticker, msecs);
			int askSize = pPredWholeDay->getAskSize(ticker, msecs);
			float fee_bpt = useFlatFee_ ? flatFee_ : mto::fee_bpt(model_, ExecFeesPrimex(model_, ticker), trdPrc);
			float sprdAdj = (.5 * sprd + fee_bpt) / basis_pts_ + volDepThres;
			float target = pPredWholeDay->getTarget(ticker, msecs) / basis_pts_;
			float intraTar = pPredWholeDay->getIntraTar(ticker, msecs) / basis_pts_;
			bool tradable = pred > sprdAdj + thres_ / basis_pts_ || -pred > sprdAdj + thres_ / basis_pts_;
			if( sprd > 0. && tradable )
			{
				vTicker_.push_back(ticker);
				vDate_.push_back(idate);
				vLogVol_.push_back(logVol);
				vVolatNorm_.push_back(volatNorm);
				vSprdAdj_.push_back(sprdAdj);
				vTarget_.push_back(target);
				vIntraTar_.push_back(intraTar);
				vPred_.push_back(pred);
				vBid_.push_back(bid);
				vAsk_.push_back(ask);
				vClosePrc_.push_back(closePrc);
				vMsecs_.push_back(msecs);
				vFee_.push_back(fee_bpt);
			}
		}
	}
}

// ---------------------------------------------------------------------
// anaQuantile(var, nBins)
// ---------------------------------------------------------------------
void PredAnaByQuantileTradeLastBin::anaQuantile(const string& var, int nBins)
{
	ofstream ofs((getOutputPath(var)).c_str());
	vector<int> vSortedIndex = getSortedIndex(var);
	const vector<float>* vControl = getControl(var);
	if( vControl != nullptr )
	{
		int iB = nBins - 1;
		getAna(vControl, iB, nBins, vSortedIndex);
	}
}

void PredAnaByQuantileTradeLastBin::getAna(const vector<float>* vControl, int iB, int nBins, const vector<int>& vSortedIndex)
{
	vector<int> vIndx = getIndx(vSortedIndex, iB, nBins);
	//set<pair<string, int>> sTickerDate;
	for(int indx : vIndx)
	{
		//sTickerDate.insert(make_pair(vTicker_[indx], vDate_[indx]));
		string ticker = vTicker_[indx];
		int idate = vDate_[indx];
		int msecs = vMsecs_[indx];
		float pred = vPred_[indx];
		float price = (pred > 0.) ? vAsk_[indx] : vBid_[indx];
		float closePrc = vClosePrc_[indx];
		int sign = (pred > 0.) ? 1 : -1;
		//printf("%d %6s %d %2d %.2f %.2f\n", idate, ticker.c_str(), msecs, sign, price, closePrc);
		mDateTickers_[idate].insert(ticker);
		mmTrades_[idate][ticker].push_back(LastBinTrade(msecs, price, sign));
	}
}

// ---------------------------------------------------------------------

const vector<float>* PredAnaByQuantileTradeLastBin::getControl(const string& var)
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

vector<int> PredAnaByQuantileTradeLastBin::getSortedIndex(const string& var)
{
	const vector<float>* vControl = getControl(var);
	int nData = vControl->size();
	vector<int> vSortedIndex(nData);
	gsl_heapsort_index(vSortedIndex, *vControl);
	return vSortedIndex;
}

vector<int> PredAnaByQuantileTradeLastBin::getIndx(const vector<int>& vSortedIndex, int iB, int nBins)
{
	int nData = vSortedIndex.size();
	int iFrom = iB * nData / nBins;
	int iTo = (iB + 1) * nData / nBins;
	if( iB == nBins - 1 )
		iTo = nData;
	vector<int> vIndx(vSortedIndex.begin() + iFrom, vSortedIndex.begin() + iTo);
	return vIndx;
}

string PredAnaByQuantileTradeLastBin::getOutputPath(const string& var, string desc)
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	if(volDepThres_)
		desc = (string)"VDT" + desc;
	if(tradedTickersOnly_)
		desc = (string)"Traded" + desc;
	desc += "_";
	desc += itos(thres_);
	string filename = "ana" + firstToUpper(var) + "QuantileTradeLastBin" + desc + '_' + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

set<string> PredAnaByQuantileTradeLastBin::getTickers(int idate)
{
	return mDateTickers_[idate];
}

vector<PredAnaByQuantileTradeLastBin::LastBinTrade> PredAnaByQuantileTradeLastBin::getTrades(int idate, const string& ticker)
{
	return mmTrades_[idate][ticker];
}

} // namespace gtlib
