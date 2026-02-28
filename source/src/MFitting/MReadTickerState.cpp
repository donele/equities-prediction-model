#include <MFitting/MReadTickerState.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <jl_lib.h>
#include <jl_lib/GFee.h>
#include <map>
#include <string>
#include <numeric>
using namespace std;
using namespace gtlib;

MReadTickerState::MReadTickerState(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
addON_(false),
rollingModelDate_(false),
interval_(0),
fee_(0.),
minMsecsON_(0),
maxMsecsON_(86400000),
//om_pred_name_("restar60;0"),
om_desc_("reg"),
tm_desc_("reg"),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("rollingModelDate") )
		rollingModelDate_ = conf.find("rollingModelDate")->second == "true";
	if( conf.count("omPredName") )
		om_pred_name_ = conf.find("omPredName")->second.c_str();
	if( conf.count("predName") )
		tm_pred_name_ = conf.find("predName")->second.c_str();
	if( conf.count("interval") )
		interval_ = atoi(conf.find("interval")->second.c_str());
	if( conf.count("minMsecsON") )
		minMsecsON_ = atoi(conf.find("minMsecsON")->second.c_str());
	if( conf.count("maxMsecsON") )
		maxMsecsON_ = atoi(conf.find("maxMsecsON")->second.c_str());
	if( conf.count("omModel") )
		om_model_ = conf.find("omModel")->second;
	if( conf.count("tmModel") )
		tm_model_ = conf.find("tmModel")->second;
	if( conf.count("omPredModel") )
		om_pred_model_ = conf.find("omPredModel")->second;
	if( conf.count("tmPredModel") )
		tm_pred_model_ = conf.find("tmPredModel")->second;
	if( conf.count("onModel") )
		on_model_ = conf.find("onModel")->second;
	if( conf.count("omDesc") )
		om_desc_ = conf.find("omDesc")->second;
	if( conf.count("tmDesc") )
		tm_desc_ = conf.find("tmDesc")->second;
}

MReadTickerState::~MReadTickerState()
{}

void MReadTickerState::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	string market = linMod.market;
	string model = MEnv::Instance()->model;

	fee_ = mto::fee_bpt(market);

	if( om_model_.empty() )
		om_model_ = model;
	if( tm_model_.empty() )
		tm_model_ = model;
	if( on_model_.empty() )
		on_model_ = model;

	if( om_pred_model_.empty() )
		om_pred_model_ = om_model_;
	if( tm_pred_model_.empty() )
		tm_pred_model_ = tm_model_;
}

void MReadTickerState::beginDay()
{
}

void MReadTickerState::beginTicker(const string& ticker)
{
}

void MReadTickerState::endTicker(const string& ticker)
{
}

void MReadTickerState::endDay()
{
	if( !om_pred_name_.empty() && !tm_pred_name_.empty() )
		get_ticker_state_match_time();
	else if( !om_pred_name_.empty() || !tm_pred_name_.empty() )
		get_ticker_state();

	MEvent::Instance()->add<map<string, vector<TickerState> > >(tm_pred_name_, "mTickerState", &mTickerState_);
}

void MReadTickerState::endJob()
{
	cout << om_pred_name_ << "\t" << tm_pred_name_ << "\t" << corr_.corr() << endl;
}

//void MReadTickerState::get_ticker_state()
//{
//	if( !tm_pred_name_.empty() )
//	{
//		mTickerState_.clear();
//
//		// uid, msecs, sec, bidSize, bid, ask, askSize, sprd, cost, pred
//		int idate = MEnv::Instance()->idate;
//		string model = MEnv::Instance()->model;
//		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
//
//		string txtPath = get_tmBinTxt_path(model, idate);
//		string predOMPath = get_pred_path(model, idate, om_pred_name_);
//		string predPath = get_pred_path(model, idate, tm_pred_name_);
//		string predONPath = get_pred_path(model, idate, "tarON");
//
//		string lineTxt; // uid, sec, bidsize, bid, ask, asksize.
//		string linePredOM; // pred.
//		string linePred; // pred, sprd.
//		string linePredON; // predON.
//
//		ifstream ifsTxt(txtPath.c_str());
//		ifstream ifsPredOM(predOMPath.c_str());
//		ifstream ifsPred(predPath.c_str());
//		ifstream ifsPredON(predONPath.c_str());
//		getline(ifsTxt, lineTxt);
//		getline(ifsPredOM, linePredOM);
//		getline(ifsPred, linePred);
//		getline(ifsPredON, linePredON);
//
//		string prev_uid;
//		vector<TickerState> vTickerState;
//
//		while( getline(ifsTxt, lineTxt) && getline(ifsPredOM, linePredOM)&& getline(ifsPred, linePred) && (!addON_ || getline(ifsPredON, linePredON)) )
//		{
//			vector<string> slTxt = split(lineTxt, '\t');
//			vector<string> slPredOM = split(linePredOM, '\t');
//			vector<string> slPred = split(linePred, '\t');
//			vector<string> slPredON = split(linePredON, '\t');
//
//			string uid = slTxt[0].c_str();
//			if( prev_uid != "" && prev_uid != uid )
//			{
//				mTickerState_[prev_uid] = vTickerState;
//				vTickerState.clear();
//			}
//			prev_uid = uid;
//
//			int sec = atoi(slTxt[2].c_str());
//			int min = sec / 60;
//			int msecs = sec * 1000 + linMod.openMsecs;
//			int bidSize = ceil(atof(slTxt[3].c_str()) * 100 - 0.5);
//			float bid = atof(slTxt[4].c_str());
//			float ask = atof(slTxt[5].c_str());
//			int askSize = ceil(atof(slTxt[6].c_str()) * 100 - 0.5);
//			float sprd = atof(slPred[3].c_str()) / basis_pts_;
//			float cost = sprd / 2. + fee_ / basis_pts_;
//			float target = 0.;
//			float pred = atof(slPred[2].c_str()) + atof(slPredOM[2].c_str());
//			double predON = atof(slPredON[2].c_str());
//			if( addON_ )
//			{
//				if( minMsecsON_ <= msecs && msecs <= maxMsecsON_ )
//					pred += predON;
//			}
//			pred /= basis_pts_;
//
//			vTickerState.push_back(TickerState(msecs, bidSize, bid, ask, askSize, target, pred, predON, sprd, cost));
//		}
//		if( !vTickerState.empty() )
//		{
//			mTickerState_[prev_uid] = vTickerState;
//		}
//	}
//}

void MReadTickerState::get_ticker_state_match_time()
{
	if( !tm_pred_name_.empty() )
	{
		mTickerState_.clear();

		// uid, msecs, sec, bidSize, bid, ask, askSize, sprd, cost, pred
		int idate = MEnv::Instance()->idate;
		int udate = MEnv::Instance()->udate;
		if( rollingModelDate_ )
			udate = 0;
		string model = MEnv::Instance()->model;
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		const string& baseDir = MEnv::Instance()->baseDir;

		string omBinTxtPath = get_sigTxt_path(baseDir, om_model_, idate, "om", om_desc_);
		string tmBinTxtPath = get_sigTxt_path(baseDir, tm_model_, idate, "tm", tm_desc_);
		string predOMPath = get_pred_path(baseDir, om_pred_model_, idate, om_pred_name_, om_desc_, udate);
		string predPath = get_pred_path(baseDir, tm_pred_model_, idate, tm_pred_name_, tm_desc_, udate);
		string predONPath = get_pred_path(baseDir, on_model_, idate, "tarON", "reg", udate);

		string lineOmBinTxt; // uid, sec.
		string lineTmBinTxt; // uid, sec, bidsize, bid, ask, asksize.
		string linePredOM; // pred.
		string linePred; // pred, sprd.
		string linePredON; // predON.

		ifstream ifsOmBinTxt(omBinTxtPath.c_str());
		ifstream ifsTmBinTxt(tmBinTxtPath.c_str());
		ifstream ifsPredOM(predOMPath.c_str());
		ifstream ifsPred(predPath.c_str());
		ifstream ifsPredON(predONPath.c_str());

		getline(ifsOmBinTxt, lineOmBinTxt);
		getline(ifsTmBinTxt, lineTmBinTxt);
		getline(ifsPredOM, linePredOM);
		getline(ifsPred, linePred);
		getline(ifsPredON, linePredON);

		// Read.
		string om_prev_uid = "";
		map<string, map<int, double> > mPredOM;
		map<int, double> mpred;
		map<string, map<int, double> > mTargetOM;
		map<int, double> mtarget;
		while( getline(ifsPredOM, linePredOM) && getline(ifsOmBinTxt, lineOmBinTxt) )
		{
			vector<string> slOmBinTxt = split(lineOmBinTxt, '\t');
			vector<string> slPredOM = split(linePredOM, '\t');

			int msecs = ceil(atof(slOmBinTxt[2].c_str()) * 1000. - 0.5) + linMod.openMsecs;
			int sec = (msecs - linMod.openMsecs) / 1000;
			if( interval_ == 0 || sec % interval_ == 0 )
			{
				string uid = slOmBinTxt[0].c_str();

				if( om_prev_uid != "" && om_prev_uid != uid )
				{
					mPredOM[om_prev_uid] = mpred;
					mpred.clear();
					mTargetOM[om_prev_uid] = mtarget;
					mtarget.clear();
				}
				om_prev_uid = uid;

				int min = sec / 60;
				//int msecs = sec * 1000 + linMod.openMsecs;

				float target = atof(slPredOM[0].c_str()) / basis_pts_;
				float pred = atof(slPredOM[2].c_str()) / basis_pts_;

				mpred[msecs] = pred;
				mtarget[msecs] = target;
			}
		}
		if( !mpred.empty() )
		{
			mPredOM[om_prev_uid] = mpred;
			mTargetOM[om_prev_uid] = mtarget;
		}

		// Read and match.
		map<string, map<int, double> >::iterator tsEnd = mPredOM.end();
		string prev_uid;
		vector<TickerState> vTickerState;
		while( getline(ifsTmBinTxt, lineTmBinTxt) && getline(ifsPred, linePred) && (!addON_ || getline(ifsPredON, linePredON)) )
		{
			vector<string> slTmTxt = split(lineTmBinTxt, '\t');
			vector<string> slPred = split(linePred, '\t');
			vector<string> slPredON = split(linePredON, '\t');

			string uid = slTmTxt[0].c_str();
			string ticker = slTmTxt[1].c_str();
			if( prev_uid != "" && prev_uid != uid )
			{
				mTickerState_[prev_uid] = vTickerState;
				vTickerState.clear();
			}
			prev_uid = uid;

			int msecs = ceil(atof(slTmTxt[2].c_str()) * 1000. - 0.5) + linMod.openMsecs;
			int sec = (msecs - linMod.openMsecs) / 1000;
			//int sec = atoi(slTmTxt[2].c_str());
			int min = sec / 60;
			//int msecs = sec * 1000 + linMod.openMsecs;

			map<string, map<int, double> >::iterator it1 = mPredOM.find(uid);
			if( it1 != tsEnd )
			{
				map<int, double>::iterator it2 = it1->second.find(msecs);
				if( it2 != it1->second.end() )
				{
					int bidSize = ceil(atof(slTmTxt[3].c_str()) * 100 - 0.5);
					float bid = atof(slTmTxt[4].c_str());
					float ask = atof(slTmTxt[5].c_str());
					int askSize = ceil(atof(slTmTxt[6].c_str()) * 100 - 0.5);
					if( bidSize <= 0 || bidSize > 1000000 )
						bidSize = 1;
					if( askSize <= 0 || askSize > 1000000 )
						askSize = 1;

					float sprd = atof(slPred[3].c_str()) / basis_pts_;
					float cost = sprd / 2. + GFee::Instance().feeBpt(model, ExecFeesPrimex(model, ticker), (bid+ask)/2.) / basis_pts_;

					float target = mTargetOM[uid][msecs] + atof(slPred[0].c_str()) / basis_pts_;
					float pred = it2->second + atof(slPred[2].c_str()) / basis_pts_;
					double predON = 0.;
					if( addON_ )
					{
						predON = atof(slPredON[2].c_str()) / basis_pts_;
						if( minMsecsON_ <= msecs && msecs <= maxMsecsON_ )
							pred += predON;
					}

					corr_.add(target, pred);
					vTickerState.push_back(TickerState(msecs, bidSize, bid, ask, askSize, target, pred, predON, sprd, cost));
				}
			}
		}
		if( !vTickerState.empty() )
		{
			mTickerState_[prev_uid] = vTickerState;
		}
	}
}

void MReadTickerState::get_ticker_state()
{
	bool isom = tm_pred_name_.empty();
	string pred_name = isom ? om_pred_name_ : tm_pred_name_;
	//if( !pred_name.empty() )
	{
		mTickerState_.clear();

		// uid, msecs, sec, bidSize, bid, ask, askSize, sprd, cost, pred
		int idate = MEnv::Instance()->idate;
		string model = MEnv::Instance()->model;
		const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
		const string& baseDir = MEnv::Instance()->baseDir;

		//string omBinTxtPath = get_omBinTxt_path(om_model_, idate);
		string tmBinTxtPath = get_sigTxt_path(baseDir, tm_model_, idate, "tm", "reg");
		//string predOMPath = get_pred_path(om_model_, idate, om_pred_name_);
		//string predPath = get_pred_path(tm_model_, idate, tm_pred_name_);
		string predONPath = get_pred_path(baseDir, on_model_, idate, "tarON", "reg");
		//string bintxtPath = isom ? get_omBinTxt_path(om_model_, idate);

		//string lineOmBinTxt; // uid, sec.
		string lineTmBinTxt; // uid, sec, bidsize, bid, ask, asksize.
		//string linePredOM; // pred.
		//string linePred; // pred, sprd.
		string linePredON; // predON.

		//ifstream ifsOmBinTxt(omBinTxtPath.c_str());
		ifstream ifsTmBinTxt(tmBinTxtPath.c_str());
		//ifstream ifsPredOM(predOMPath.c_str());
		//ifstream ifsPred(predPath.c_str());
		ifstream ifsPredON(predONPath.c_str());

		//getline(ifsOmBinTxt, lineOmBinTxt);
		getline(ifsTmBinTxt, lineTmBinTxt);
		//getline(ifsPredOM, linePredOM);
		//getline(ifsPred, linePred);
		getline(ifsPredON, linePredON);

		// Read.
		//string om_prev_uid = "";
		//map<string, map<int, double> > mPredOM;
		//map<int, double> mpred;
		//map<string, map<int, double> > mTargetOM;
		//map<int, double> mtarget;
		//while( getline(ifsPredOM, linePredOM) && getline(ifsOmBinTxt, lineOmBinTxt) )
		//{
		//	vector<string> slOmBinTxt = split(lineOmBinTxt, '\t');
		//	vector<string> slPredOM = split(linePredOM, '\t');

		//	int sec = atoi(slOmBinTxt[2].c_str());
		//	if( interval_ == 0 || sec % interval_ == 0 )
		//	{
		//		string uid = slOmBinTxt[0].c_str();

		//		if( om_prev_uid != "" && om_prev_uid != uid )
		//		{
		//			mPredOM[om_prev_uid] = mpred;
		//			mpred.clear();
		//			mTargetOM[om_prev_uid] = mtarget;
		//			mtarget.clear();
		//		}
		//		om_prev_uid = uid;

		//		int min = sec / 60;
		//		int msecs = sec * 1000 + linMod.openMsecs;

		//		float target = atof(slPredOM[0].c_str()) / basis_pts_;
		//		float pred = atof(slPredOM[2].c_str()) / basis_pts_;

		//		mpred[msecs] = pred;
		//		mtarget[msecs] = target;
		//	}
		//}
		//if( !mpred.empty() )
		//{
		//	mPredOM[om_prev_uid] = mpred;
		//	mTargetOM[om_prev_uid] = mtarget;
		//}

		// Read and match.
		//map<string, map<int, double> >::iterator tsEnd = mPredOM.end();
		string prev_uid;
		vector<TickerState> vTickerState;
		//while( getline(ifsTmBinTxt, lineTmBinTxt) && getline(ifsPred, linePred) && (!addON_ || getline(ifsPredON, linePredON)) )
		while( getline(ifsTmBinTxt, lineTmBinTxt) && getline(ifsPredON, linePredON) )
		{
			vector<string> slTmTxt = split(lineTmBinTxt, '\t');
			//vector<string> slPred = split(linePred, '\t');
			vector<string> slPredON = split(linePredON, '\t');

			string uid = slTmTxt[0].c_str();
			string ticker = slTmTxt[1].c_str();
			if( prev_uid != "" && prev_uid != uid )
			{
				mTickerState_[prev_uid] = vTickerState;
				vTickerState.clear();
			}
			prev_uid = uid;

			int msecs = ceil(atof(slTmTxt[2].c_str()) * 1000. - 0.5) + linMod.openMsecs;
			int sec = (msecs - linMod.openMsecs) / 1000;
			//int sec = atoi(slTmTxt[2].c_str());
			int min = sec / 60;
			//int msecs = sec * 1000 + linMod.openMsecs;

			//map<string, map<int, double> >::iterator it1 = mPredOM.find(uid);
			//if( it1 != tsEnd )
			{
				//map<int, double>::iterator it2 = it1->second.find(msecs);
				//if( it2 != it1->second.end() )
				{
					int bidSize = ceil(atof(slTmTxt[3].c_str()) * 100 - 0.5);
					float bid = atof(slTmTxt[4].c_str());
					float ask = atof(slTmTxt[5].c_str());
					int askSize = ceil(atof(slTmTxt[6].c_str()) * 100 - 0.5);

					float sprd = atof(slPredON[3].c_str()) / basis_pts_;
					float cost = sprd / 2. + GFee::Instance().feeBpt(model, ExecFeesPrimex(model, ticker), (bid+ask)/2) / basis_pts_;

					//float target = /*mTargetOM[uid][msecs] + */atof(slPred[0].c_str()) / basis_pts_;
					//float pred = /*it2->second + */atof(slPred[2].c_str()) / basis_pts_;
					//double predON = atof(slPredON[2].c_str()) / basis_pts_;
					//if( addON_ )
					//{
					//	if( minMsecsON_ <= msecs && msecs <= maxMsecsON_ )
					//		pred += predON;
					//}

					float target = 0.;
					float pred = 0.;
					float predON = atof(slPredON[2].c_str()) / basis_pts_;

					corr_.add(target, pred);
					vTickerState.push_back(TickerState(msecs, bidSize, bid, ask, askSize, target, pred, predON, sprd, cost));
				}
			}
		}
		if( !vTickerState.empty() )
		{
			mTickerState_[prev_uid] = vTickerState;
		}
	}
}
