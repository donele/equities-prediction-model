#include <MSignal/MTopBottomAna.h>
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

MTopBottomAna::MTopBottomAna(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	sigType_(MEnv::Instance()->sigType)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("targetName") )
		targetName_ = conf.find("targetName")->second;

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
	ofs_.open("output.txt");
	printHeader();
}

MTopBottomAna::~MTopBottomAna()
{}

void MTopBottomAna::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MTopBottomAna::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;
	SignalBuffer* pSigBuf = getBinSigBuf(model, idate);
	if( pSigBuf->is_open() )
	{
		while( pSigBuf->next() )
		{
			//double target = pSigBuf->getTarget();
			//double bmpred = pSigBuf->getBmpred();
			double pred = pSigBuf->getPred();
			if( fabs(pred) > 2. ) // roughly top/bottom 1% in US.
			{
				printRow(pSigBuf);
			}
		}
	}
}

SignalBuffer* MTopBottomAna::getBinSigBuf(const string& model, int idate)
{
	int udate = MEnv::Instance()->udate;
	SignalBuffer* pSigBuf = nullptr;
	string binPath = get_sig_path(MEnv::Instance()->baseDir, model, idate, sigType_, "reg");
	string predPath = get_pred_path(MEnv::Instance()->baseDir, model, idate, targetName_, "reg", udate);
	pSigBuf = new SignalBufferDecoratorPred(
			new SignalBufferDecoratorHeader(
				new SignalBufferBin(binPath, inputNames_), binPath), predPath);
	return pSigBuf;
}

void MTopBottomAna::endDay()
{
}

void MTopBottomAna::endJob()
{
}

void MTopBottomAna::printHeader()
{
	ofs_ << "target pred bmpred";
	for( auto input : inputNames_ )
		ofs_ << ' ' << input;
	ofs_ << endl;
}

void MTopBottomAna::printRow(SignalBuffer* pSigBuf)
{
	char buf[1000];
	sprintf(buf, "%.4f %.4f %.4f", pSigBuf->getTarget(), pSigBuf->getPred(), pSigBuf->getBmpred());
	ofs_ << buf;

	vector<float> inputs = pSigBuf->getInputs();
	for( auto input : inputs )
	{
		sprintf(buf, " %.4f", input);
		ofs_ << buf;
	}
	ofs_ << endl;
}
