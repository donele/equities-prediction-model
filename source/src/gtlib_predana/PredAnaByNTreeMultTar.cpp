#include <gtlib_predana/PredAnaByNTreeMultTar.h>
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

PredAnaByNTreeMultTar::PredAnaByNTreeMultTar(const string& baseDir,
		const string& fitDir, const vector<string>& vPredModel, const vector<string>& vSigModel,
		const vector<string>& vTargetName, int udate, const vector<int>& testdates,
		float omWgt, float tmWgt, const string& fitDesc, float flatFee, char exch, bool debug, bool volDepThres)
	:debug_(debug),
	volDepThres_(volDepThres),
	debugSum_(0.),
	exch_(exch),
	flatFee_(flatFee),
	nOm_(0),
	nTm_(0),
	omWgt_(omWgt),
	tmWgt_(tmWgt),
	model_(vPredModel[0]),
	testdates_(testdates),
	statDir_(getStatDir(fitDir, udate)),
	vConstThres_(getThresSeries())
{
	useFlatFee_ = flatFee_ >= 0.;
	mkd(statDir_);
	for( int idate : testdates )
	{
		int offset = vPrice_.size();
		PredWholeDay* pOm = new PredWholeDay(getBinSigBuf(baseDir, vPredModel[0], vSigModel[0], vTargetName[0], udate, idate, fitDesc));
		PredWholeDay* pTm = new PredWholeDay(getBinSigBuf(baseDir, vPredModel[1], vSigModel[1], vTargetName[1], udate, idate, fitDesc));
		getData(pOm, pTm, idate);
		mDayOffset_[idate] = make_pair(offset, vPrice_.size());
		delete pOm;
		delete pTm;
	}
}

PredAnaByNTreeMultTar::PredAnaByNTreeMultTar(const string& baseDir,
		const string& fitDir, const string& model,
		const string& targetName, int udate, const vector<int>& testdates,
		float omWgt, float tmWgt, const string& fitDesc, float flatFee)
	:PredAnaByNTreeMultTar(baseDir, fitDir, vector<string>(1, model), vector<string>(1, model),
			vector<string>(1, targetName), udate, testdates, omWgt, tmWgt, fitDesc, flatFee)
{
}

SignalBuffer* PredAnaByNTreeMultTar::getBinSigBuf(const string& baseDir,
		const string& predModel, const string& sigModel, const string& targetName, int udate, int idate, const string& fitDesc)
{
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(baseDir, sigModel, idate, getSigType(targetName), fitDesc);
	string predPath = get_pred_path(baseDir, predModel, idate, targetName, fitDesc, udate);
	pSigBuf = new SignalBufferDecoratorHeader(
			new SignalBufferPredMulti(predPath), binPath);
	return pSigBuf;
}

void PredAnaByNTreeMultTar::getData(PredWholeDay* pOm, PredWholeDay* pTm, int idate)
{
	if(volDepThres_)
		mTickerVolDepThres_ = getVolDepThres(model_, idate);
	init(pOm, pTm);
	map<string, vector<int>> tickerMsecs = pTm->getTickerMsecs();
	for( auto& kv : tickerMsecs )
	{
		const string& ticker = kv.first;
		if( pOm->exist(ticker) )
		{
			for( int& msecs : kv.second )
			{
				if( pOm->exist(ticker, msecs) )
					fetchData(pOm, pTm, ticker, msecs);
			}
		}
	}
	//if(debug_)
		//printf("DDD %.4f\n", debugSum_);

	if( vvPredOmT_.size() != vPredSubOm_.size() )
	{
		cerr << "ERROR: om pred size is " << vvPredOmT_.size() << ", expected " << vPredSubOm_.size() << '\n';
		exit(21);
	}
	if( vvPredTmT_.size() != vPredSubTm_.size() )
	{
		cerr << "ERROR: tm pred size is " << vvPredTmT_.size() << ", expected " << vPredSubTm_.size() << '\n';
		exit(21);
	}
}

void PredAnaByNTreeMultTar::fetchData(PredWholeDay* pOm, PredWholeDay* pTm, const string& ticker, int msecs)
{
	float sprd = pTm->getSprd(ticker, msecs);
	float volDepThres = mTickerVolDepThres_[ticker] * basis_pts_;

	if(sprd > 0. && (exch_ == '\0' || exch_ == ticker[0]))
	{
		float price = pTm->getPrice(ticker, msecs);
		float bid = pTm->getBidPrice(ticker, msecs);
		float ask = pTm->getAskPrice(ticker, msecs);
		float targetOm = pOm->getTarget(ticker, msecs) / basis_pts_;
		float predOm = pOm->getPred(ticker, msecs) / basis_pts_;
		float targetTm = pTm->getTarget(ticker, msecs) / basis_pts_;
		float predTm = pTm->getPred(ticker, msecs) / basis_pts_;
		float intraTar = pTm->getIntraTar(ticker, msecs) / basis_pts_;
		float pred = predOm + predTm;
		float trdPrc = pred > 0. ? ask : bid;
		float fee_bpt = useFlatFee_ ? flatFee_ : mto::fee_bpt(model_, ExecFeesPrimex(model_, ticker), trdPrc);
		float sprdAdj = .5 * sprd / basis_pts_ + fee_bpt / basis_pts_;

		vSprdAdj_.push_back(sprdAdj);
		vPrice_.push_back(trdPrc);

		vPredTot_.push_back(pred);
		vTargetTot_.push_back(targetOm + targetTm);
		vTargetOm_.push_back(targetOm);
		vTargetTm_.push_back(targetTm);
		vIntraTar_.push_back(intraTar);
		if(volDepThres_)
			vThres_.push_back(volDepThres);
		else
			vThres_.push_back(0.);

		vector<float> vPredOm = pOm->getPreds(ticker, msecs);
		vector<float> vPredTm = pTm->getPreds(ticker, msecs);
		for( auto& p : vPredOm )
			p = p * omWgt_ / basis_pts_;
		for( auto& p : vPredTm )
			p = p * tmWgt_ / basis_pts_;

		for( int i = 0; i < nOm_; ++i )
			vvPredOmT_[i].push_back(vPredOm[i]);
		for( int i = 0; i < nTm_; ++i )
			vvPredTmT_[i].push_back(vPredTm[i]);

		//if(debug_)
		//{
		//	float pred = predOm + predTm;
		//	float thres = 0.;
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

void PredAnaByNTreeMultTar::init(PredWholeDay* pOm, PredWholeDay* pTm)
{
	if( vPredSubOm_.empty() )
	{
		vPredSubOm_ = pOm->getPredSubs();
		nOm_ = vPredSubOm_.size();
		vvPredOmT_ = vector<vector<float>>(nOm_, vector<float>(0));
	}
	if( vPredSubTm_.empty() )
	{
		vPredSubTm_ = pTm->getPredSubs();
		nTm_ = vPredSubTm_.size();
		vvPredTmT_ = vector<vector<float>>(nTm_, vector<float>(0));
	}
}

vector<float> PredAnaByNTreeMultTar::getPredTot(const vector<float>& vPredOm, const vector<float>& vPredTm)
{
	vector<float> vPredTot(vPredOm.size());
	int N = vPredOm.size();
	for( int i = 0; i < N; ++i )
		vPredTot[i] = vPredOm[i] + vPredTm[i];
	return vPredTot;
}

// --------------- AnaNTree()

void PredAnaByNTreeMultTar::anaNTree()
{
	int nData = vTargetOm_.size();
	if( nData > 0 && !vPredSubOm_.empty() && !vPredSubTm_.empty() )
	{
		int nNTreeOm = vPredSubOm_.size();
		int nNTreeTm = vPredSubTm_.size();
		vector<future<PObjMultTar>> vFut;
		if( debug_ )
		{
			int iTO = nNTreeOm - 1;
			int iTT = nNTreeTm - 1;
			vFut.push_back(async(launch::async, &PredAnaByNTreeMultTar::getAnaNTree, this, iTO, iTT));
		}
		else
		{
			for( int iTO = 0; iTO < nNTreeOm; ++iTO )
			{
				for( int iTT = 0; iTT < nNTreeTm; ++iTT )
					vFut.push_back(async(launch::async, &PredAnaByNTreeMultTar::getAnaNTree, this, iTO, iTT));
			}
		}
		ofstream ofs(getOutputPath().c_str());
		for( auto& f : vFut )
		{
			PObjMultTar o = f.get();
			o.print(ofs);
		}
	}
}

PObjMultTar PredAnaByNTreeMultTar::getAnaNTree(int iTO, int iTT)
{
	vector<float> vPredTot = getPredTot(vvPredOmT_[iTO], vvPredTmT_[iTT]);
	int nTreeNTree = vPredSubOm_[iTO] * 1000 + vPredSubTm_[iTT];
	PObjMultTar anaObj(nTreeNTree, &vSprdAdj_, &vPredTot,
			&vvPredOmT_[iTO], &vvPredTmT_[iTT], &vTargetTot_, &vTargetOm_, &vTargetTm_, &vIntraTar_, &vPrice_);
	anaObj.calculateThres(vThres_);
	return anaObj;
}

string PredAnaByNTreeMultTar::getOutputPath()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename;
	if(volDepThres_)
		filename = "anaNTreeMultTarVDT_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	else
		filename = "anaNTreeMultTar_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

// --------------- AnaDay()

void PredAnaByNTreeMultTar::anaDay()
{
	int nData = vTargetOm_.size();
	if( nData > 0 && !vPredSubOm_.empty() && !vPredSubTm_.empty() )
	{
		vector<future<PObjMultTar>> vFut;
		for(auto& kv : mDayOffset_)
		{
			int idate = kv.first;
			int iFrom = kv.second.first;
			int iTo = kv.second.second;
			int control = idate % 20000000;
			vFut.push_back(async(launch::async, &PredAnaByNTreeMultTar::getAnaDay, this, control, iFrom, iTo));
		}
		ofstream ofs(getOutputPathDay().c_str());
		for( auto& f : vFut )
		{
			PObjMultTar obj = f.get();
			obj.print(ofs);
		}
	}
}

PObjMultTar PredAnaByNTreeMultTar::getAnaDay(int idate, int iFrom, int iTo)
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
	//float thres = 0.;
	anaObj.calculateThres(vThres_);
	return anaObj;
}

string PredAnaByNTreeMultTar::getOutputPathDay()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename;
	if(volDepThres_)
		filename = "anaDayVDT_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	else
		filename = "anaDay_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

// --------------- AnaThres()

void PredAnaByNTreeMultTar::anaThres(const vector<float>& vConstThres)
{
	int nData = vTargetOm_.size();
	if( nData > 0 && !vPredSubOm_.empty() && !vPredSubTm_.empty() )
	{
		int nNTreeOm = vPredSubOm_.size();
		int nNTreeTm = vPredSubTm_.size();
		vector<future<vector<PObjMultTar>>> vvFut;
		for( int iTO = 0; iTO < nNTreeOm; ++iTO )
		{
			for( int iTT = 0; iTT < nNTreeTm; ++iTT )
				vvFut.push_back(async(launch::async, &PredAnaByNTreeMultTar::getAnaThres,
							this, iTO, iTT, vConstThres));
		}
		ofstream ofs(getOutputPathThres().c_str());
		for( auto& f : vvFut )
		{
			vector<PObjMultTar> vObj = f.get();
			for( auto& o : vObj )
				o.printThres(ofs);
		}
	}
}

vector<PObjMultTar> PredAnaByNTreeMultTar::getAnaThres(int iTO, int iTT, const vector<float> vConstThres)
{
	if(!vConstThres.empty())
		vConstThres_ = vConstThres;
	vector<float> vPredTot = getPredTot(vvPredOmT_[iTO], vvPredTmT_[iTT]);
	int nTreeNTree = vPredSubOm_[iTO] * 1000 + vPredSubTm_[iTT];
	PObjMultTar anaObj(nTreeNTree, &vSprdAdj_, &vPredTot,
			&vvPredOmT_[iTO], &vvPredTmT_[iTT], &vTargetTot_, &vTargetOm_, &vTargetTm_, &vIntraTar_, &vPrice_);
	vector<PObjMultTar> vObj;
	for( double thres : vConstThres )
	{
		anaObj.calculateThres(thres);
		vObj.push_back(anaObj);
	}
	return vObj;
}

string PredAnaByNTreeMultTar::getOutputPathThres()
{
	int idateFrom = *testdates_.begin();
	int idateTo = *testdates_.rbegin();
	string filename = "anaThres_" + itos(idateFrom) + '_' + itos(idateTo) + ".txt";
	string path = statDir_ + '/' + filename;
	return path;
}

} // namespace gtlib
