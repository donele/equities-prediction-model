#include <gtlib_predana/PredAnaByNTreeMultTarV2.h>
#include <gtlib_predana/PObjMultTar.h>
#include <gtlib_model/mFtns.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib/util.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferPredMulti.h>
#include <gtlib_predana/PredWholeDay.h>
#include <jl_lib/sort_util.h>
#include <jl_lib/mto.h>
#include <vector>
#include <future>
#include <string>
#include <functional>
#include <boost/filesystem.hpp>
using namespace std;

namespace gtlib {

PredAnaByNTreeMultTarV2::PredAnaByNTreeMultTarV2(const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName, int udate, const vector<int>& testdates,
		const string& fitDesc, float flatFee, bool debug)
	:debug_(debug),
	debugSum_(0.),
	flatFee_(flatFee),
	model_(vPredModel[0]),
	testdates_(testdates),
	statDir_(getStatDir(fitDir, udate)),
	vThres_(getThresSeries())
{
	useFlatFee_ = flatFee_ >= 0.;
	mkd(statDir_);
	int nTarget = vTargetName.size();
	for( int idate : testdates )
	{
		int offset = vPrice_.size();
		vector<PredWholeDay*> vPred;
		for(int i = 0; i < nTarget; ++i)
			vPred.push_back(new PredWholeDay(getBinSigBuf(baseDir, vPredModel[i], vSigModel[i], vTargetName[i], udate, idate, fitDesc)));
		getData(vPred);
		mDayOffset_[idate] = make_pair(offset, vPrice_.size());
		for(auto& p : vPred)
			delete p;
	}
}

PredAnaByNTreeMultTarV2::PredAnaByNTreeMultTarV2(const string& baseDir,
		const string& fitDir, const string& model,
		const string& targetName, int udate, const vector<int>& testdates,
		const string& fitDesc, float flatFee)
	:PredAnaByNTreeMultTarV2(baseDir, fitDir, vector<string>(1, model), vector<string>(1, model),
			vector<string>(1, targetName), udate, testdates, fitDesc, flatFee)
{
}

SignalBuffer* PredAnaByNTreeMultTarV2::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate, const string& fitDesc)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), fitDesc);
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, fitDesc, udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPredMulti(predPath), binPath);
	return pSigBuf;
}

void PredAnaByNTreeMultTarV2::getData(vector<PredWholeDay*>& vPred);
{
	init(vPred);
	map<string, vector<int>> tickerMsecs = (*vPred.rbegin())->getTickerMsecs();
	for( auto& kv : tickerMsecs )
	{
		const string& ticker = kv.first;
		for( int& msecs : kv.second )
		{
			bool exist = true;
			for(int i = 0; i < n - 1; ++i)
			{
				if(!vPred[i]->exist(ticker, msecs))
				{
					exist = false;
					break;
				}
			}
			if(exist)
				fetchData(vPred, ticker, msecs);
		}
	}

	//if( vvPredOmT_.size() != vPredSubOm_.size() )
	//{
	//	cerr << "ERROR: om pred size is " << vvPredOmT_.size() << ", expected " << vPredSubOm_.size() << '\n';
	//	exit(21);
	//}
	//if( vvPredTmT_.size() != vPredSubTm_.size() )
	//{
	//	cerr << "ERROR: tm pred size is " << vvPredTmT_.size() << ", expected " << vPredSubTm_.size() << '\n';
	//	exit(21);
	//}
}

void PredAnaByNTreeMultTarV2::fetchData(vector<PredWholeDay*> vPred, const string& ticker, int msecs)
{
	auto itLast = vPred.rbegin();
	float sprd = itLast->getSprd(ticker, msecs);

	if( sprd > 0. )
	{
		float price = itLast->getPrice(ticker, msecs);
		float bid = itLast->getBidPrice(ticker, msecs);
		float ask = itLast->getAskPrice(ticker, msecs);

		float target = 0.;
		float pred = 0.;
		for(auto& p : vPred)
		{
			target += p->getTarget(ticker, msecs) / basis_pts_;
			pred += p->getPred(ticker, msecs) / basis_pts_;
		}

		//float targetOm = pOm->getTarget(ticker, msecs) / basis_pts_;
		//float predOm = pOm->getPred(ticker, msecs) / basis_pts_;
		//float targetTm = pTm->getTarget(ticker, msecs) / basis_pts_;
		//float predTm = pTm->getPred(ticker, msecs) / basis_pts_;
		float intraTar = itLast->getIntraTar(ticker, msecs) / basis_pts_;
		//float pred = predOm + predTm;
		float trdPrc = pred > 0. ? ask : bid;
		float fee_bpt = useFlatFee_ ? flatFee_ : mto::fee_bpt(model_, ExecFeesPrimex(model_, ticker), trdPrc);
		float sprdAdj = .5 * sprd / basis_pts_ + fee_bpt / basis_pts_;

		vSprdAdj_.push_back(sprdAdj);
		vPrice_.push_back(trdPrc);

		vPredTot_.push_back(pred);
		//vTargetTot_.push_back(targetOm + targetTm);
		vTargetTot_.push_back(target);
		//vTargetOm_.push_back(targetOm);
		//vTargetTm_.push_back(targetTm);
		vIntraTar_.push_back(intraTar);

		//vector<float> vPredOm = pOm->getPreds(ticker, msecs);
		//vector<float> vPredTm = pTm->getPreds(ticker, msecs);
		//for( auto& p : vPredOm )
		//	p = p * omWgt_ / basis_pts_;
		//for( auto& p : vPredTm )
		//	p = p * tmWgt_ / basis_pts_;

		//for( int i = 0; i < nOm_; ++i )
		//	vvPredOmT_[i].push_back(vPredOm[i]);
		//for( int i = 0; i < nTm_; ++i )
		//	vvPredTmT_[i].push_back(vPredTm[i]);

		//if(debug_)
		//{
		//	float pred = predOm + predTm;
		//	float thres = 1.;
		//	bool tradable = pred > sprdAdj + thres / basis_pts_ || -pred > sprdAdj + thres / basis_pts_;
		//	if(tradable)
		//	{
		//		int sec = (msecs - 34200000)/1000;
		//		int posChg = (pred > 0.) ? 1 : -1;
		//		double pnl = posChg * intraTar - abs(posChg * sprdAdj);
		//		printf("### %5s %5d %8.2f %8.2f %8.2f %8.2f\n", ticker.c_str(), sec, pred*basis_pts_, intraTar*basis_pts_,
		//				sprdAdj*basis_pts_, pnl*basis_pts_);
		//		debugSum_ += (pred>0)?(intraTar-sprdAdj):(-intraTar-sprdAdj);
		//	}
		//}
	}
}

void PredAnaByNTreeMultTarV2::init(vector<PredWholeDay*>& vPred);
{
	int n = vPred.size();
	for(int i = 0; i < n; ++i)
	{
		if(vvPredSub_[i].empty())
		{
			vvPredSub_[i] = vPred[i]->getPredSubs();
			vNdata_ = vvPredSub[i].size();
			vvvPredT_ = vector<vector<float>>(nData_[i], vector<float>(0));
		}
	}
}

vector<float> PredAnaByNTreeMultTarV2::getPredTot(const vector<float>& vPredOm, const vector<float>& vPredTm)
{
	vector<float> vPredTot(vPredOm.size());
	int N = vPredOm.size();
	for( int i = 0; i < N; ++i )
		vPredTot[i] = vPredOm[i] + vPredTm[i];
	return vPredTot;
}

// --------------- AnaNTree()

void PredAnaByNTreeMultTarV2::anaNTree()
{
	//int nData = vTargetOm_.size();
	//if( nData > 0 && !vPredSubOm_.empty() && !vPredSubTm_.empty() )
	//{
	//	int nNTreeOm = vPredSubOm_.size();
	//	int nNTreeTm = vPredSubTm_.size();
	//	vector<future<PObjMultTar>> vFut;
	//	if( debug_ )
	//	{
	//		int iTO = nNTreeOm - 1;
	//		int iTT = nNTreeTm - 1;
	//		vFut.push_back(async(launch::async, &PredAnaByNTreeMultTarV2::getAnaNTree, this, iTO, iTT));
	//	}
	//	else
	//	{
	//		for( int iTO = 0; iTO < nNTreeOm; ++iTO )
	//		{
	//			for( int iTT = 0; iTT < nNTreeTm; ++iTT )
	//				vFut.push_back(async(launch::async, &PredAnaByNTreeMultTarV2::getAnaNTree, this, iTO, iTT));
	//		}
	//	}
	//	ofstream ofs(getOutputPath().c_str());
	//	for( auto& f : vFut )
	//	{
	//		PObjMultTar o = f.get();
	//		o.print(ofs);
	//	}
	//}
}

PObjMultTar PredAnaByNTreeMultTarV2::getAnaNTree(int iTO, int iTT)
{
	vector<float> vPredTot = getPredTot(vvPredOmT_[iTO], vvPredTmT_[iTT]);
	int nTreeNTree = vPredSubOm_[iTO] * 1000 + vPredSubTm_[iTT];
	PObjMultTar anaObj(nTreeNTree, &vSprdAdj_, &vPredTot,
			&vvPredOmT_[iTO], &vvPredTmT_[iTT], &vTargetTot_, &vTargetOm_, &vTargetTm_, &vIntraTar_, &vPrice_);
	anaObj.calculate();
	return anaObj;
}

string PredAnaByNTreeMultTarV2::getOutputPath()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename = "anaNTreeMultTar_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

// --------------- AnaDay()

void PredAnaByNTreeMultTarV2::anaDay()
{
	//int nData = vTargetOm_.size();
	//if( nData > 0 && !vPredSubOm_.empty() && !vPredSubTm_.empty() )
	//{
	//	vector<future<PObjMultTar>> vFut;
	//	for(auto& kv : mDayOffset_)
	//	{
	//		int idate = kv.first;
	//		int iFrom = kv.second.first;
	//		int iTo = kv.second.second;
	//		int control = idate % 20000000;
	//		vFut.push_back(async(launch::async, &PredAnaByNTreeMultTarV2::getAnaDay, this, control, iFrom, iTo));
	//	}
	//	ofstream ofs(getOutputPathDay().c_str());
	//	for( auto& f : vFut )
	//	{
	//		PObjMultTar obj = f.get();
	//		obj.print(ofs);
	//	}
	//}
}

PObjMultTar PredAnaByNTreeMultTarV2::getAnaDay(int idate, int iFrom, int iTo)
{
	PObjMultTar anaObj(idate,
			vSprdAdj_.begin() + iFrom, vSprdAdj_.begin() + iTo,
			vPredTot_.begin() + iFrom, vPredTot_.begin() + iTo,
			vvPredOmT_.rbegin()->begin() + iFrom, vvPredOmT_.rbegin()->begin() + iTo,
			vvPredTmT_.rbegin()->begin() + iFrom, vvPredTmT_.rbegin()->begin() + iTo,
			vTargetTot_.begin() + iFrom, vTargetTot_.begin() + iTo,
			vTargetOm_.begin() + iFrom, vTargetOm_.begin() + iTo,
			vTargetTm_.begin() + iFrom, vTargetTm_.begin() + iTo,
			vIntraTar_.begin() + iFrom, vIntraTar_.begin() + iTo,
			vPrice_.begin() + iFrom, vPrice_.begin() + iTo);
	float thres = 1.;
	anaObj.calculateThres(thres, debug_);
	return anaObj;
}

string PredAnaByNTreeMultTarV2::getOutputPathDay()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename = "anaDay_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

// --------------- AnaThres()

void PredAnaByNTreeMultTarV2::anaThres(const vector<float>& vThres)
{
	//if(!vThres.empty())
	//	vThres_ = vThres;
	//int nData = vTargetOm_.size();
	//if( nData > 0 && !vPredSubOm_.empty() && !vPredSubTm_.empty() )
	//{
	//	int nNTreeOm = vPredSubOm_.size();
	//	int nNTreeTm = vPredSubTm_.size();
	//	vector<future<vector<PObjMultTar>>> vvFut;
	//	for( int iTO = 0; iTO < nNTreeOm; ++iTO )
	//	{
	//		for( int iTT = 0; iTT < nNTreeTm; ++iTT )
	//			vvFut.push_back(async(launch::async, &PredAnaByNTreeMultTarV2::getAnaThres, this, iTO, iTT));
	//	}
	//	ofstream ofs(getOutputPathThres().c_str());
	//	for( auto& f : vvFut )
	//	{
	//		vector<PObjMultTar> vObj = f.get();
	//		for( auto& o : vObj )
	//			o.printThres(ofs);
	//	}
	//}
}

vector<PObjMultTar> PredAnaByNTreeMultTarV2::getAnaThres(int iTO, int iTT)
{
	vector<float> vPredTot = getPredTot(vvPredOmT_[iTO], vvPredTmT_[iTT]);
	int nTreeNTree = vPredSubOm_[iTO] * 1000 + vPredSubTm_[iTT];
	PObjMultTar anaObj(nTreeNTree, &vSprdAdj_, &vPredTot,
			&vvPredOmT_[iTO], &vvPredTmT_[iTT], &vTargetTot_, &vTargetOm_, &vTargetTm_, &vIntraTar_, &vPrice_);
	vector<PObjMultTar> vObj;
	for( double thres : vThres_ )
	{
		anaObj.calculateThres(thres);
		vObj.push_back(anaObj);
	}
	return vObj;
}

string PredAnaByNTreeMultTarV2::getOutputPathThres()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename = "anaThres_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

} // namespace gtlib
