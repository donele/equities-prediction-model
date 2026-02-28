#include <gtlib_predana/PredAnaByNTree.h>
#include <gtlib_predana/GsprObjectSimple.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib/util.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_sigread/SignalBufferDecoratorPred.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferPredMulti.h>
#include <gtlib_predana/PredWholeDay.h>
#include <jl_lib/sort_util.h>
#include <jl_lib/mto.h>
#include <vector>
#include <string>
#include <functional>
#include <boost/filesystem.hpp>
using namespace std;

namespace gtlib {

PredAnaByNTree::PredAnaByNTree(const string& baseDir,
		const string& fitDir, const string& predModel, const string& sigModel,
		const string& targetName, int udate, const vector<int>& testdates, const string& fitDesc, float flatFee)
	:flatFee_(flatFee),
	model_(predModel),
	testdates_(testdates),
	statDir_(getStatDir(fitDir, udate))
{
	useFlatFee_ = flatFee >= 0.;
	mkd(statDir_);
	for( int idate : testdates )
	{
		PredWholeDay* pPredWholeDay = new PredWholeDay(getBinSigBuf(baseDir, predModel, sigModel, targetName, udate, idate, fitDesc));
		getData(pPredWholeDay);
		delete pPredWholeDay;
	}
}

SignalBuffer* PredAnaByNTree::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate, const string& fitDesc)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), fitDesc);
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, fitDesc, udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPredMulti(predPath), binPath);
	return pSigBuf;
}

void PredAnaByNTree::getData(PredWholeDay* pPredWholeDay)
{
	if( vPredSub_.empty() )
		vPredSub_ = pPredWholeDay->getPredSubs();

	auto tickerMsecs = pPredWholeDay->getTickerMsecs();
	for( auto& kv : tickerMsecs )
	{
		const string& ticker = kv.first;
		for( auto& msecs : kv.second )
		{
			float sprd = pPredWholeDay->getSprd(ticker, msecs);
			float price = pPredWholeDay->getPrice(ticker, msecs);
			float fee_bpt = useFlatFee_ ? flatFee_ : mto::fee_bpt(model_, ExecFeesPrimex(model_, ticker), price);
			float sprdAdj = .5 * sprd / basis_pts_ + fee_bpt / basis_pts_;
			float control = sprdAdj;

			vSprdAdj_.push_back(sprdAdj);

			float target = pPredWholeDay->getTarget(ticker, msecs) / basis_pts_;
			vector<float> vPred = pPredWholeDay->getPreds(ticker, msecs);
			for( auto& p : vPred )
				p /= basis_pts_;

			vTarget_.push_back(target);
			vvPred_.push_back(vPred);
		}
	}
}

void PredAnaByNTree::anaNTree()
{
	int nData = vTarget_.size();
	if( nData > 0 )
	{
		vector<GsprObjectSimple*> vObj;
		int nNTree = vPredSub_.size();
		for( int iT = 0; iT < nNTree; ++iT )
		{
			gtlib::GsprObjectSimple* anaObj = new GsprObjectSimple();
			for( int i = 0; i < nData; ++i )
				anaObj->addData(vPredSub_[iT], vSprdAdj_[i], vvPred_[i][iT], vTarget_[i]);
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

string PredAnaByNTree::getOutputPath()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename = "anaNTree_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

void PredAnaByNTree::anaDay()
{
	int nData = vTarget_.size();
	if( nData > 0 )
	{
		vector<GsprObjectSimple*> vObj;
		int nNTree = vPredSub_.size();
		//for( int iT = 0; iT < nNTree; ++iT )
		int iT = nNTree- 1;
		for(auto& kv : mDayOffset_)
		{
			int idate = kv.first;
			int iFrom = kv.second.first;
			int iTo = kv.second.second;

			gtlib::GsprObjectSimple* anaObj = new GsprObjectSimple();
			for( int i = iFrom; i < iTo; ++i )
				anaObj->addData(idate, vSprdAdj_[i], vvPred_[i][iT], vTarget_[i]);
			anaObj->calculate();
			vObj.push_back(anaObj);
		}
		ofstream ofs(getOutputPathDay().c_str());
		for( auto p : vObj )
		{
			p->print(ofs);
			delete p;
		}
	}
}

string PredAnaByNTree::getOutputPathDay()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename = "anaDay_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}


} // namespace gtlib
