#include <MSignal/MCompSignal.h>
#include <gtlib_sigread/SignalWholeDay.h>
#include <gtlib_sigread/SignalBufferDecoratorPred.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferText.h>
#include <gtlib_sigread/SignalBufferBin.h>
#include <gtlib_model/pathFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
using namespace std;
using namespace gtlib;

MCompSignal::MCompSignal(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	useSimpleTicker_(false),
	sigType_(MEnv::Instance()->sigType),
	fitDesc_(MEnv::Instance()->fitDesc),
	compByTicker_(false),
	printBmpred_(false),
	printPred_(false),
	printVarIndex_(-1)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("compByTicker") )
		compByTicker_ = conf.find("compByTicker")->second == "true";
	if( conf.count("printBmpred") )
		printBmpred_ = conf.find("printBmpred")->second == "true";
	if( conf.count("printPred") )
		printPred_ = conf.find("printPred")->second == "true";
	if( conf.count("txtPath") )
		txtPath_ = conf.find("txtPath")->second;
	if( conf.count("sigModel") )
		sigModel_ = conf.find("sigModel")->second;
	if( conf.count("predModel") )
		predModel_ = conf.find("predModel")->second;
	if( conf.count("compModel") )
		compModel_ = conf.find("compModel")->second;
	if( conf.count("targetName") )
		targetName_ = conf.find("targetName")->second;
	if( conf.count("printVarIndex") )
		printVarIndex_ = atoi(conf.find("printVarIndex")->second.c_str());

	if( conf.count("input") )
	{
		pair<mmit, mmit> inputs = conf.equal_range("input");
		for( mmit mi = inputs.first; mi != inputs.second; ++mi )
		{
			string name = mi->second;
			inputNames_.push_back(name);
		}
	}
	nInputs_ = inputNames_.size();
}

MCompSignal::~MCompSignal()
{}

void MCompSignal::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MCompSignal::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	string sigModel = (!sigModel_.empty()) ? sigModel_ : model;
	string predModel = (!predModel_.empty()) ? predModel_ : model;

	SignalBuffer* pSigBuf1 = getBinSigBuf(sigModel, predModel, idate);
	SignalWholeDay sig1(pSigBuf1, useSimpleTicker_);

	SignalBuffer* pSigBuf2 = getCompSigBuf(idate);
	SignalWholeDay sig2(pSigBuf2, useSimpleTicker_);

	compareAllData(sig1, sig2);
	if( compByTicker_ )
		compareByTicker(sig1, sig2);

	if( printVarIndex_ >= 0 )
		printVar(sig1, sig2, printVarIndex_);
	if( printBmpred_ )
		printBmpred(sig1, sig2);
	if( printPred_ )
		printPred(sig1, sig2);
}

SignalBuffer* MCompSignal::getCompSigBuf(int idate)
{
	SignalBuffer* pSigBuf = nullptr;
	if( !txtPath_.empty() )
	{
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		pSigBuf = new SignalBufferText(txtPath_, inputNames_, linMod.openMsecs);
	}
	else if( !compModel_.empty() )
		pSigBuf = getBinSigBuf(compModel_, idate);
	return pSigBuf;
}

SignalBuffer* MCompSignal::getBinSigBuf(const string& model, int idate)
{
	return getBinSigBuf(model, model, idate);
}

SignalBuffer* MCompSignal::getBinSigBuf(const string& sigModel, const string& predModel, int idate)
{
	int udate = MEnv::Instance()->udate;
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(MEnv::Instance()->baseDir, sigModel, idate, sigType_, fitDesc_);
	if( targetName_.empty() )
		pSigBuf = new SignalBufferDecoratorHeader(
				new SignalBufferBin(binPath, inputNames_), binPath);
	else
	{
		string predPath = get_pred_path(MEnv::Instance()->baseDir, predModel, idate, targetName_, fitDesc_, udate);
		pSigBuf = new SignalBufferDecoratorPred(
				new SignalBufferDecoratorHeader(
					new SignalBufferBin(binPath, inputNames_), binPath), predPath);
	}
	return pSigBuf;
}

void MCompSignal::endDay()
{
}

void MCompSignal::endJob()
{
}

bool MCompSignal::doCompPred() const
{
	return !targetName_.empty();
}

void MCompSignal::compareAllData(SignalWholeDay& sig1, SignalWholeDay& sig2)
{
	vector<Corr> vCorr = compInput(sig1, sig2);
	printCompInput(vCorr);

	if( false && doCompPred() ) // Turn off bmpred stats.
	{
		Corr corrPred;
		Corr corrTargetPred1;
		Corr corrTargetPred2;
		compBmpred(sig1, sig2, corrPred, corrTargetPred1, corrTargetPred2);
		printCompPred(corrPred, corrTargetPred1, corrTargetPred2, "BM");
	}

	if( doCompPred() )
	{
		Corr corrPred;
		Corr corrTargetPred1;
		Corr corrTargetPred2;
		compPred(sig1, sig2, corrPred, corrTargetPred1, corrTargetPred2);

		Corr corrTarget;
		compTarget(sig1, sig2, corrTarget);

		printCompPred(corrPred, corrTargetPred1, corrTargetPred2, "Tot");
		printCompTarget(corrTarget);
	}
}

void MCompSignal::printCompInput(const vector<Corr>& vCorr)
{
	char buf[100];
	sprintf(buf, "%2s %15s %6s %8s %8s %6s\n", "i", "name","corr", "stdev1", "stdev2", "stdRat");
	cout << buf;
	for( int i = 0; i < nInputs_; ++i )
	{
		double stdX = vCorr[i].accX.stdev();
		double stdY = vCorr[i].accY.stdev();
		double stdRat = (stdY != 0.) ? stdY / stdX : 0.;
		sprintf(buf, "%2d %15s %6.2f %8.4f %8.4f %6.2f\n",
				i, inputNames_[i].c_str(), vCorr[i].corr() * 100., vCorr[i].accX.stdev(), vCorr[i].accY.stdev(), stdRat);
		cout << buf;
	}
}

void MCompSignal::printCompPred(const Corr& corrPred,
		const Corr& corrTargetPred1, const Corr& corrTargetPred2, const string& predDesc)
{
	char buf[100];
	cout << predDesc << '\n';
	sprintf(buf, "%13s %13s %13s\n", "pred-pred", "target-pred1", "target-pred2");
	cout <<buf;
	sprintf(buf, "%13.4f %13.4f %13.4f\n", corrPred.corr(), corrTargetPred1.corr(), corrTargetPred2.corr());
	cout << buf;
}

void MCompSignal::printCompTarget(const Corr& corrTarget)
{
	char buf[100];
	sprintf(buf, "%13s\n", "target-target");
	cout <<buf;
	sprintf(buf, "%13.4f\n", corrTarget.corr());
	cout << buf;
}

void MCompSignal::compareByTicker(SignalWholeDay& sig1, SignalWholeDay& sig2)
{
	map<string, vector<Corr>> mvCorr = compInputByTicker(sig1, sig2);
	printCompInputByTicker(mvCorr);

	if( false && doCompPred() )
	{
		map<string, Corr> mCorrPred;
		map<string, Corr> mCorrTargetPred1;
		map<string, Corr> mCorrTargetPred2;
		compBmpredByTicker(sig1, sig2, mCorrPred, mCorrTargetPred1, mCorrTargetPred2);
		printCompPredByTicker(mCorrPred, mCorrTargetPred1, mCorrTargetPred2);
	}

	if( doCompPred() )
	{
		map<string, Corr> mCorrPred;
		map<string, Corr> mCorrTargetPred1;
		map<string, Corr> mCorrTargetPred2;
		compPredByTicker(sig1, sig2, mCorrPred, mCorrTargetPred1, mCorrTargetPred2);
		printCompPredByTicker(mCorrPred, mCorrTargetPred1, mCorrTargetPred2);
	}
}

void MCompSignal::printCompInputByTicker(const map<string, vector<Corr>>& mvCorr)
{
	char buf[100];
	sprintf(buf, "%6s %3s", "ticker", "N");
	cout << buf;
	for( int i = 0; i < nInputs_; ++i )
	{
		sprintf(buf, " %3d", i);
		cout << buf;
	}
	cout << '\n';

	for( const auto& kv : mvCorr )
	{
		const string& ticker = kv.first;
		const vector<Corr>& vCorr = kv.second;
		sprintf(buf, "%6s %3d", ticker.c_str(), vCorr[0].accX.n);
		cout << buf;

		for( int i = 0; i < nInputs_; ++i )
		{
			sprintf(buf, " %3.0f", vCorr[i].corr() * 100);
			cout << buf;
		}
		cout << '\n';
	}
}

void MCompSignal::printCompPredByTicker(const map<string, Corr>& mCorr,
		const map<string, Corr>& mCorrTargetPred1, const map<string, Corr>& mCorrTargetPred2)
{
	char buf[100];
	sprintf(buf, "%13s %13s %13s %13s\n", "ticker", "pred-pred", "pred1-target", "pred2-target");
	cout << buf;
	for( auto& kv : mCorr )
	{
		const string& ticker = kv.first;
		const Corr& c = kv.second;
		const Corr& ctp1 = mCorrTargetPred1.find(ticker)->second;
		const Corr& ctp2 = mCorrTargetPred2.find(ticker)->second;
		sprintf(buf, "%13s %13.2f %13.2f %13.2f\n", ticker.c_str(),
				c.corr() * 100., ctp1.corr() * 100., ctp2.corr() * 100.);
		cout << buf;
	}
}
