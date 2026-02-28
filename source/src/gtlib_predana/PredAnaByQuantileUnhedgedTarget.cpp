#include <gtlib_predana/PredAnaByQuantileUnhedgedTarget.h>
#include <gtlib_predana/GsprObjectSimple.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib/util.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_sigread/SignalBufferDecoratorPredFromSig.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferBin.h>
#include <gtlib_sigread/SignalWholeDay.h>
#include <jl_lib/sort_util.h>
#include <jl_lib/mto.h>
#include <vector>
#include <string>
#include <functional>
#include <boost/filesystem.hpp>
using namespace std;

namespace gtlib {

PredAnaByQuantileUnhedgedTarget::PredAnaByQuantileUnhedgedTarget(const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName,
		int udate, const vector<int>& testdates, const string& fitDesc, float flatFee)
	:flatFee_(flatFee),
	model_(vPredModel[0]),
	inputNames_({"sprd", "logPrice"}),
	testdates_(testdates),
	statDir_(getStatDir(fitDir, udate)),
	controlVar_("sprdAdj")
{
	useFlatFee_ = flatFee_ >= 0.;
	mkd(statDir_);

	for( int idate : testdates )
	{
		SignalWholeDay* pSigWholeDay = nullptr;
		for( int i = 0; i < vTargetName.size(); ++i )
		{
			const string& targetName = vTargetName[i];
			vector<string> inputNames = inputNames_;
			vector<string> targetReplacementNames = getTargetReplacementNames(targetName);
			inputNames.insert(end(inputNames), begin(targetReplacementNames), end(targetReplacementNames));

			if( pSigWholeDay == nullptr )
				pSigWholeDay = new SignalWholeDay(getBinSigBuf(baseDir, vPredModel[i], vSigModel[i],
							vTargetName[i], targetReplacementNames, inputNames, udate, idate, fitDesc));
			else
				pSigWholeDay->merge(new SignalWholeDay(getBinSigBuf(baseDir, vPredModel[i], vSigModel[i],
								vTargetName[i], targetReplacementNames, inputNames, udate, idate, fitDesc)));
		}
		getData(pSigWholeDay);
		delete pSigWholeDay;
	}
	nData_ = vControl_.size();
}

vector<string> PredAnaByQuantileUnhedgedTarget::getTargetReplacementNames(const string& targetName)
{
	vector<string> repNames;
	if( getSigType(targetName) == "tm" )
	{
		if( targetName == "tar600;60_1.0_tar3600;660_0.5" )
			repNames = {"tar600;60uh", "tar3600;660uh"};
	}
	return repNames;
}

PredAnaByQuantileUnhedgedTarget::PredAnaByQuantileUnhedgedTarget(const string& baseDir,
		const string& fitDir, const string& model,
		const string& targetName,
		int udate, const vector<int>& testdates, const string& fitDesc, float flatFee)
	:PredAnaByQuantileUnhedgedTarget(baseDir, fitDir, vector<string>(1, model), vector<string>(1, model),
			vector<string>(1, targetName), udate, testdates, fitDesc, flatFee)
{
}

SignalBuffer* PredAnaByQuantileUnhedgedTarget::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel,
		const string& targetName, const vector<string>& targetReplacementNames,
		const vector<string>& inputNames, int udate, int idate, const string& fitDesc)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), fitDesc);
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, fitDesc, udate);
	pSigBuf = new SignalBufferDecoratorPredFromSig(
			new SignalBufferDecoratorHeader(
				new SignalBufferBin(binPath, inputNames), binPath), predPath, targetReplacementNames);
	return pSigBuf;
}

void PredAnaByQuantileUnhedgedTarget::getData(SignalWholeDay* pSigWholeDay)
{
	auto tickerMsecs = pSigWholeDay->getTickerMsecs();
	for( auto& kv : tickerMsecs )
	{
		const string& ticker = kv.first;
		for( auto& msecs : kv.second )
		{
			float sprd = pSigWholeDay->getInput(ticker,  msecs, "sprd");
			float logPrice = pSigWholeDay->getInput(ticker,  msecs, "logPrice");
			float price = exp10(logPrice);
			float fee_bpt = useFlatFee_ ? flatFee_ : mto::fee_bpt(model_, ExecFeesPrimex(model_, ticker), price);
			float sprdAdj = .5 * sprd / basis_pts_ + fee_bpt / basis_pts_;

			float control = 0.;
			if( controlVar_ == "sprdAdj" )
				control = sprdAdj;

			vControl_.push_back(control);
			vSprd_.push_back(sprd);
			vSprdAdj_.push_back(sprdAdj);
			vPrice_.push_back(price);
			vTarget_.push_back(pSigWholeDay->getTarget(ticker, msecs));
			vBmpred_.push_back(pSigWholeDay->getBmpred(ticker, msecs));
			vPred_.push_back(pSigWholeDay->getPred(ticker, msecs));
		}
	}
}

void PredAnaByQuantileUnhedgedTarget::anaQuantile(int nBins)
{
	if( nData_ > 0 )
	{
		vector<GsprObjectSimple*> vObj;
		vector<int> vSortedIndex(nData_);
		gsl_heapsort_index(vSortedIndex, vControl_);
		for( int iB = 0; iB < nBins; ++iB )
		{
			int iFrom = iB * nData_ / nBins;
			int iTo = (iB + 1) * nData_ / nBins;
			if( iB == nBins - 1 )
				iTo = nData_;

			gtlib::GsprObjectSimple* anaObj = new GsprObjectSimple();
			for( int i = iFrom; i < iTo; ++i )
			{
				int ii = vSortedIndex[i];
				anaObj->addData(vControl_[ii], vSprdAdj_[ii], vPred_[ii], vTarget_[ii]);
			}
			anaObj->calculate();
			vObj.push_back(anaObj);
		}
		ofstream ofs(getOutputPath().c_str());
		for( auto p : vObj )
		{
			p->print(ofs);
			delete p;
		}
	}
}

string PredAnaByQuantileUnhedgedTarget::getOutputPath()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename = "ana" + firstToUpper(controlVar_) + "QuantileUH_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

} // namespace gtlib
