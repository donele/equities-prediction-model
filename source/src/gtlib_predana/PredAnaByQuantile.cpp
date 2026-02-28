#include <gtlib_predana/PredAnaByQuantile.h>
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

namespace gtlib {

PredAnaByQuantile::PredAnaByQuantile(const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName, const vector<float>& vWgt,
		int udate, const vector<int>& testdates, const string& fitDesc, float flatFee,
		bool volDepThres, bool tradedTickersOnly, bool debug)
	:debug_(debug),
	flatFee_(flatFee),
	singleThread_(false),
	volDepThres_(volDepThres),
	tradedTickersOnly_(tradedTickersOnly),
	model_(vPredModel[0]),
	testdates_(testdates),
	statDir_(getStatDir(fitDir, udate))
{
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

PredAnaByQuantile::PredAnaByQuantile(const string& baseDir,
		const string& fitDir, const string& model, const string& targetName, float wgt,
		int udate, const vector<int>& testdates, const string& fitDesc, float flatFee)
	:PredAnaByQuantile(baseDir, fitDir, vector<string>(1, model), vector<string>(1, model),
			vector<string>(1, targetName), vector<float>(1, wgt), udate, testdates, fitDesc, flatFee)
{
}

SignalBuffer* PredAnaByQuantile::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate, const string& fitDesc)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), fitDesc);
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, fitDesc, udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPred(predPath), binPath);
	return pSigBuf;
}

void PredAnaByQuantile::getData(PredWholeDay* pPredWholeDay, int idate)
{
	ofstream ofs;
	char buf[1000];
	//if(debug_)
	//{
	//	sprintf(buf, "anaQuantileDebug_%d.txt", idate);
	//	ofs.open(buf);
	//	sprintf(buf, "%-6s %9s %8s %8s %8s %8s %8s %7s\n", "ticker", "msecs",
	//			"target", "pred", "sprdAdj", "price", "intraTar", "volatNorm");
	//	ofs << buf;
	//}
	unordered_map<string, float> mTickerLogVol = getLogVol(model_, idate);
	unordered_map<string, float> mTickerClose = getClose(model_, idate);
	unordered_map<string, float> mTickerVolatNorm = getVolatNorm(model_, idate);
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
			float bmpred = pPredWholeDay->getBmpred(ticker, msecs) / basis_pts_;
			float intraTar = pPredWholeDay->getIntraTar(ticker, msecs) / basis_pts_;

			if(debug_)
			{
				if((pred > 0. && askSize < 100) || (pred < 0. && bidSize < 100))
				{
					sprd = 1e6;
					sprdAdj = 1e6;
				}
			}
			//if( sprdAdj > 0. )
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
				vMsecs_.push_back(msecs);
				vTrdPrc_.push_back(trdPrc);
				vClosePrc_.push_back(closePrc);
				vFee_.push_back(fee_bpt);
			}
			//if(debug_)
			//{
			//	int N = vTarget_.size();
			//	sprintf(buf, "%-6s %9d %8.2f %8.2f %8.2f %8.2f %8.2f %7.4f\n",
			//			ticker.c_str(), msecs,
			//			target*basis_pts_, pred*basis_pts_, sprdAdj*basis_pts_, trdPrc, intraTar*basis_pts_, volatNorm);
			//	ofs << buf;
			//}
		}
	}
}

// ---------------------------------------------------------------------
// anaQuantileFeatures
// ---------------------------------------------------------------------

void PredAnaByQuantile::anaQuantileFeatures(const string& var, int nBins, float anaThres, float anaBreak, float anaMaxPos)
{
	//vector<float> vThres = getThresSeries();
	vector<int> vSortedIndex = getSortedIndex(var);
	vector<future<PAnaSimple>> vFut;
	for( int iB = 0; iB < nBins; ++iB )
	{
		const vector<float>* vControl = getControl(var);
		if( vControl != nullptr )
		{
			if( singleThread_ )
				vFut.push_back(async(launch::deferred, &PredAnaByQuantile::getAnaFeatures,
							this, vControl, iB, nBins, vSortedIndex, anaThres, anaBreak, anaMaxPos));
			else
				vFut.push_back(async(launch::async, &PredAnaByQuantile::getAnaFeatures,
							this, vControl, iB, nBins, vSortedIndex, anaThres, anaBreak, anaMaxPos));
		}
	}
	char desc[100];
	sprintf(desc, "");
	ofstream ofs((getOutputPath(var, desc)).c_str());
	for( auto& f : vFut )
	{
		//vector<PAnaSimple> vObj = f.get();
		//for( auto& o : vObj )
			//o.print(ofs);
		PAnaSimple a = f.get();
		a.print(ofs);
	}
}

PAnaSimple PredAnaByQuantile::getAnaFeatures(const vector<float>* vControl, int iB, int nBins,
		const vector<int>& vSortedIndex, float anaThres, float anaBreak, float anaMaxPos)
{
	vector<int> vIndx = getIndx(vSortedIndex, iB, nBins);
	PAnaSimple obj(vControl, &vMsecs_, &vSprdAdj_, &vPred_, &vTarget_, &vIntraTar_, &vFee_, &vBid_, &vAsk_, &vClosePrc_, vIndx);
	//vector<PAnaSimple> vObj;
	obj.calculate(anaThres, anaBreak, anaMaxPos);
	//vObj.push_back(obj);
	return obj;
}


// ---------------------------------------------------------------------
// anaQuantileThres(var, nBins)
// ---------------------------------------------------------------------

void PredAnaByQuantile::anaQuantileThres(const string& var, int nBins)
{
	vector<float> vThres = getThresSeries();
	vector<int> vSortedIndex = getSortedIndex(var);
	vector<future<vector<PAnaSimple>>> vvFut;
	for( int iB = 0; iB < nBins; ++iB )
	{
		const vector<float>* vControl = getControl(var);
		if( vControl != nullptr )
		{
			if( singleThread_ )
				vvFut.push_back(async(launch::deferred, &PredAnaByQuantile::getAnaThres,
							this, vControl, iB, nBins, vSortedIndex, vThres));
			else
				vvFut.push_back(async(launch::async, &PredAnaByQuantile::getAnaThres,
							this, vControl, iB, nBins, vSortedIndex, vThres));
		}
	}
	ofstream ofs((getOutputPath(var, "Thres")).c_str());
	for( auto& f : vvFut )
	{
		vector<PAnaSimple> vObj = f.get();
		for( auto& o : vObj )
			o.printThres(ofs);
	}
}

vector<PAnaSimple> PredAnaByQuantile::getAnaThres(const vector<float>* vControl, int iB, int nBins,
		const vector<int>& vSortedIndex, const vector<float>& vThres)
{
	vector<int> vIndx = getIndx(vSortedIndex, iB, nBins);
	PAnaSimple obj(vControl, &vMsecs_, &vSprdAdj_, &vPred_, &vTarget_, &vIntraTar_, &vFee_, &vBid_, &vAsk_, &vClosePrc_, vIndx);
	vector<PAnaSimple> vObj;
	for( double thres : vThres )
	{
		obj.calculateThres(thres);
		vObj.push_back(obj);
	}
	return vObj;
}

// ---------------------------------------------------------------------
// anaQuantileBreak(var, nBins)
// ---------------------------------------------------------------------

void PredAnaByQuantile::anaQuantileBreak(const string& var, int nBins)
{
	vector<float> vBreak = getBreakSeries();
	vector<int> vSortedIndex = getSortedIndex(var);
	vector<future<vector<PAnaSimple>>> vvFut;
	for( int iB = 0; iB < nBins; ++iB )
	{
		const vector<float>* vControl = getControl(var);
		if( vControl != nullptr )
		{
			if( singleThread_ )
				vvFut.push_back(async(launch::deferred, &PredAnaByQuantile::getAnaBreak,
							this, vControl, iB, nBins, vSortedIndex, vBreak));
			else
				vvFut.push_back(async(launch::async, &PredAnaByQuantile::getAnaBreak,
							this, vControl, iB, nBins, vSortedIndex, vBreak));
		}
	}
	ofstream ofs((getOutputPath(var, "Break")).c_str());
	for( auto& f : vvFut )
	{
		vector<PAnaSimple> vObj = f.get();
		for( auto& o : vObj )
			o.printBreak(ofs);
	}
}

vector<PAnaSimple> PredAnaByQuantile::getAnaBreak(const vector<float>* vControl, int iB, int nBins,
		const vector<int>& vSortedIndex, const vector<float>& vBreak)
{
	vector<int> vIndx = getIndx(vSortedIndex, iB, nBins);
	PAnaSimple obj(vControl, &vMsecs_, &vSprdAdj_, &vPred_, &vTarget_, &vIntraTar_, &vFee_, &vBid_, &vAsk_, &vClosePrc_, vIndx);
	vector<PAnaSimple> vObj;
	for( auto brk : vBreak )
	{
		obj.calculateBreak(brk);
		vObj.push_back(obj);
	}
	return vObj;
}

// ---------------------------------------------------------------------
// anaQuantileMaxPos(var, nBins)
// ---------------------------------------------------------------------

void PredAnaByQuantile::anaQuantileMaxPos(const string& var, int nBins)
{
	vector<float> vMaxPos = getMaxPosSeries();
	vector<int> vSortedIndex = getSortedIndex(var);
	vector<future<vector<PAnaSimple>>> vvFut;
	for( int iB = 0; iB < nBins; ++iB )
	{
		const vector<float>* vControl = getControl(var);
		if( vControl != nullptr )
		{
			if( singleThread_ )
				vvFut.push_back(async(launch::deferred, &PredAnaByQuantile::getAnaMaxPos,
							this, vControl, iB, nBins, vSortedIndex, vMaxPos));
			else
				vvFut.push_back(async(launch::async, &PredAnaByQuantile::getAnaMaxPos,
							this, vControl, iB, nBins, vSortedIndex, vMaxPos));
		}
	}
	ofstream ofs((getOutputPath(var, "MaxPos")).c_str());
	for( auto& f : vvFut )
	{
		vector<PAnaSimple> vObj = f.get();
		for( auto& o : vObj )
			o.printMaxPos(ofs);
	}
}

vector<PAnaSimple> PredAnaByQuantile::getAnaMaxPos(const vector<float>* vControl, int iB, int nBins,
		const vector<int>& vSortedIndex, const vector<float>& vMaxPos)
{
	vector<int> vIndx = getIndx(vSortedIndex, iB, nBins);
	PAnaSimple obj(vControl, &vMsecs_, &vSprdAdj_, &vPred_, &vTarget_, &vIntraTar_, &vFee_, &vBid_, &vAsk_, &vClosePrc_, vIndx);
	vector<PAnaSimple> vObj;
	for( auto maxPos : vMaxPos )
	{
		obj.calculateMaxPos(maxPos);
		vObj.push_back(obj);
	}
	return vObj;
}

// ---------------------------------------------------------------------
// anaQuantileNTree(var, nBins)
// ---------------------------------------------------------------------
void PredAnaByQuantile::anaQuantileNTree(const string& var, int nBins)
{
}

// ---------------------------------------------------------------------
// anaQuantile(var, nBins)
// ---------------------------------------------------------------------
void PredAnaByQuantile::anaQuantile(const string& var, int nBins)
{
	vector<int> vSortedIndex = getSortedIndex(var);
	vector<future<PAnaSimple>> vFut;
	for( int iB = 0; iB < nBins; ++iB )
	{
		const vector<float>* vControl = getControl(var);
		if( vControl != nullptr )
		{
			if( singleThread_ )
				vFut.push_back(async(launch::deferred, &PredAnaByQuantile::getAna,
							this, vControl, iB, nBins, vSortedIndex));
			else
				vFut.push_back(async(launch::async, &PredAnaByQuantile::getAna,
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

PAnaSimple PredAnaByQuantile::getAna(const vector<float>* vControl, int iB, int nBins, const vector<int>& vSortedIndex)
{
	vector<int> vIndx = getIndx(vSortedIndex, iB, nBins);
	PAnaSimple p(vControl, &vMsecs_, &vSprdAdj_, &vPred_, &vTarget_, &vIntraTar_, &vFee_, &vBid_, &vAsk_, &vClosePrc_, vIndx);
	p.calculate();
	return p;
}

// ---------------------------------------------------------------------

const vector<float>* PredAnaByQuantile::getControl(const string& var)
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

vector<int> PredAnaByQuantile::getSortedIndex(const string& var)
{
	const vector<float>* vControl = getControl(var);
	int nData = vControl->size();
	vector<int> vSortedIndex(nData);
	gsl_heapsort_index(vSortedIndex, *vControl);
	return vSortedIndex;
}

vector<int> PredAnaByQuantile::getIndx(const vector<int>& vSortedIndex, int iB, int nBins)
{
	int nData = vSortedIndex.size();
	int iFrom = iB * nData / nBins;
	int iTo = (iB + 1) * nData / nBins;
	if( iB == nBins - 1 )
		iTo = nData;
	vector<int> vIndx(vSortedIndex.begin() + iFrom, vSortedIndex.begin() + iTo);
	return vIndx;
}

string PredAnaByQuantile::getOutputPath(const string& var, string desc)
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	if(volDepThres_)
		desc = (string)"VDT" + desc;
	if(tradedTickersOnly_)
		desc = (string)"Traded" + desc;
	string filename = "ana" + firstToUpper(var) + "Quantile" + desc + '_' + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

} // namespace gtlib
