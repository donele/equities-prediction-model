#include <MSignal/MDumpSignalDL.h>
#include <gtlib_sigread/SignalWholeDay.h>
#include <gtlib_sigread/SignalBufferDecoratorPred.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <gtlib_sigread/SignalBufferBin.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_model/mFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
using namespace std;
using namespace gtlib;

MDumpSignalDL::MDumpSignalDL(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	useSimpleTicker_(false),
	outDir_("."),
	nDatesTickerSel_(40),
	retain_(1.),
	sigType_(MEnv::Instance()->sigType),
	fitDesc_(MEnv::Instance()->fitDesc)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("targetName") )
		targetName_ = conf.find("targetName")->second;
	if( conf.count("retain") )
		retain_ = atof(conf.find("retain")->second.c_str());

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

MDumpSignalDL::~MDumpSignalDL()
{}

void MDumpSignalDL::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	char buf[100];
	sprintf(buf, "%s/%s/concSig/om/u%d", MEnv::Instance()->baseDir.c_str(), MEnv::Instance()->model.c_str(), MEnv::Instance()->udate);
	outDir_ = buf;
	mkd(outDir_);

	getAllTickers();
	if( retain_ < 1. )
		setHighVolTickers();
	printTickerList();
}

void MDumpSignalDL::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;

	SignalBuffer* pSigBuf = getBinSigBuf(model, model, idate);
	SignalWholeDay sig(pSigBuf, useSimpleTicker_);

	dump(sig, model, idate);
}

void MDumpSignalDL::getAllTickers()
{
	const vector<int>& idates = MEnv::Instance()->idates;
	int nDates = idates.size();
	string model2 = MEnv::Instance()->model.substr(0, 2);
	const vector<string>& markets = MEnv::Instance()->markets;

	int idateFrom = idates[nDates-nDatesTickerSel_];
	int idateTo = idates[nDates-1];

	allTickers_.clear();
	// Get all the tickers, ordered by volume.
	map<string, vector<string>> marketTickers = getMarketTickersHighVol(model2, markets, idateFrom, idateTo, 1.);
	for( auto& kv : marketTickers )
	{
		auto& v = kv.second;
		allTickers_.insert(end(allTickers_), begin(v), end(v));
	}
}

void MDumpSignalDL::setHighVolTickers()
{
	const vector<int>& idates = MEnv::Instance()->idates;
	int nDates = idates.size();
	string model2 = MEnv::Instance()->model.substr(0, 2);
	const vector<string>& markets = MEnv::Instance()->markets;

	int idateFrom = idates[nDates-nDatesTickerSel_];
	int idateTo = idates[nDates-1];

	vector<string> vHighVolTickers;
	map<string, vector<string>> marketTickers = getMarketTickersHighVol(model2, markets, idateFrom, idateTo, retain_);
	for( auto& kv : marketTickers )
	{
		auto& v = kv.second;
		vHighVolTickers.insert(end(vHighVolTickers), begin(v), end(v));
	}
	highVolTickers_ = set<string>(begin(vHighVolTickers), end(vHighVolTickers));
}

void MDumpSignalDL::printTickerList()
{
	const vector<int>& idates = MEnv::Instance()->idates;
	int nDates = idates.size();
	int idateFrom = idates[0];
	int idateTo = idates[nDates-1];
	char filename[100];
	sprintf(filename, "%s/tickerList_%d_%d.txt", outDir_.c_str(), idates[0], idates[nDates-1]);
	ofstream ofs(filename);
	char buf[100];
	for( auto ticker : allTickers_ )
	{
		int valid = validTicker(ticker) ? 1 : 0;
		sprintf(buf, "%12s %d\n", ticker.c_str(), valid);
		ofs << buf;
	}
}

bool MDumpSignalDL::validTicker(const string& ticker)
{
	return highVolTickers_.empty() || highVolTickers_.count(ticker);
}

SignalBuffer* MDumpSignalDL::getBinSigBuf(const string& sigModel, const string& predModel, int idate)
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

void MDumpSignalDL::endDay()
{
}

void MDumpSignalDL::endJob()
{
}

void MDumpSignalDL::dump(SignalWholeDay& sig, const string& model, int idate)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;

	ofstream ofsPredSig(getPredSigFilename(model, idate).c_str());
	ofstream ofsSig(getSigFilename(model, idate).c_str());
	ofstream ofsTar(getTarFilename(model, idate).c_str());
	ofstream ofsRestar(getRestarFilename(model, idate).c_str());
	ofstream ofsMeta(getMetaFilename(model, idate).c_str());
	ofstream ofsWgt(getWgtFilename(model, idate).c_str());

	map<string, vector<int>> tm = sig.getTickerMsecs();
	vector<int> vMsecs = getMsecs(tm);
	char buf[100];
	int nTickers = allTickers_.size();
	for( int msecs : vMsecs )
	{
		float sec = (msecs - linMod.openMsecs) / 1000.;
		sprintf(buf, "%.3f\n", sec);
		ofsMeta << buf;

		bool firstInput = true;
		for( int it = 0; it < nTickers; ++it )
		{
			string& ticker = allTickers_[it];

			vector<float> inputs(nInputs_, 0.);
			float predsig = 0.; // target as signal.
			float target = 0.;
			float restar = 0.;
			float weight = 0.;
			if( sig.exist(ticker, msecs) )
			{
				// pred
				float pred = 0.;
				pred = sig.getPred(ticker, msecs); // this is total pred.

				// input
				if( validTicker(ticker) ) // high vol ticker.
				{
					inputs = sig.getInputs(ticker, msecs);
					predsig = pred;
				}

				// target
				target = sig.getTarget(ticker, msecs);
				restar = target - pred;

				weight = 1.;
			}

			if( validTicker(ticker) )
			{
				if( firstInput )
				{
					firstInput = false;
					// input
					for( int ii = 0; ii < nInputs_; ++ii )
					{
						if( ii == 0 )
							sprintf(buf, "%.4f", inputs[ii]);
						else
							sprintf(buf, ",%.4f", inputs[ii]);
						ofsSig << buf;
					}
					sprintf(buf, "%.4f", predsig);
					ofsPredSig << buf;
				}
				else
				{
					// input
					for( int ii = 0; ii < nInputs_; ++ii )
					{
						sprintf(buf, ",%.4f", inputs[ii]);
						ofsSig << buf;
					}
					sprintf(buf, ",%.4f", predsig);
					ofsPredSig << buf;
				}
			}

			if( it == 0 ) // first ticker.
			{
				// target
				sprintf(buf, "%.4f", target);
				ofsTar << buf;
				sprintf(buf, "%.4f", restar);
				ofsRestar << buf;
				sprintf(buf, "%.4f", weight);
				ofsWgt << buf;
			}
			else
			{
				// target
				sprintf(buf, ",%.4f", target);
				ofsTar << buf;
				sprintf(buf, ",%.4f", restar);
				ofsRestar << buf;
				sprintf(buf, ",%.4f", weight);
				ofsWgt << buf;
			}
		}
		ofsPredSig << '\n';
		ofsSig << '\n';
		ofsTar << '\n';
		ofsRestar << '\n';
		ofsWgt << '\n';
	}
}

string MDumpSignalDL::getPredSigFilename(const string& model, int idate)
{
	char buf[400];
	sprintf(buf, "%s/concurrent_predsig_%d.csv", outDir_.c_str(), idate);
	return buf;
}

string MDumpSignalDL::getSigFilename(const string& model, int idate)
{
	char buf[400];
	sprintf(buf, "%s/concurrent_%dinput_%d.csv", outDir_.c_str(), nInputs_, idate);
	return buf;
}

string MDumpSignalDL::getTarFilename(const string& model, int idate)
{
	char buf[400];
	sprintf(buf, "%s/concurrent_target_%d.csv", outDir_.c_str(), idate);
	return buf;
}

string MDumpSignalDL::getRestarFilename(const string& model, int idate)
{
	char buf[400];
	sprintf(buf, "%s/concurrent_restar_%d.csv", outDir_.c_str(), idate);
	return buf;
}

string MDumpSignalDL::getMetaFilename(const string& model, int idate)
{
	char buf[400];
	sprintf(buf, "%s/concurrent_meta_%d.csv", outDir_.c_str(), idate);
	return buf;
}

string MDumpSignalDL::getWgtFilename(const string& model, int idate)
{
	char buf[400];
	sprintf(buf, "%s/concurrent_wgt_%d.csv", outDir_.c_str(), idate);
	return buf;
}

vector<int> MDumpSignalDL::getMsecs(const map<string, vector<int>>& tm)
{
	set<int> sMsecs;
	for( auto& kv : tm )
	{
		for( int msecs : kv.second )
			sMsecs.insert(msecs);
	}
	vector<int> vMsecs(begin(sMsecs), end(sMsecs));
	return vMsecs;
}

