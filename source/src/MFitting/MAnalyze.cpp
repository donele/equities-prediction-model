#include <MFitting/MAnalyze.h>
#include <MFramework.h>
#include <jl_lib/jlutil.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;

MAnalyze::MAnalyze(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
desc_(""),
minThres_(0.),
maxThres_(0.)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("desc") )
		desc_ = conf.find("desc")->second;
	if( conf.count("targetFilter") )
		targetFilter_ = conf.find("targetFilter")->second;
	if( conf.count("minThres") )
		minThres_ = atof(conf.find("minThres")->second.c_str());
	if( conf.count("maxThres") )
		maxThres_ = atof(conf.find("maxThres")->second.c_str());

	// Target Names.
	if( conf.count("targetName") )
	{
		pair<mmit, mmit> targetNames = conf.equal_range("targetName");
		for( mmit mi = targetNames.first; mi != targetNames.second; ++mi )
		{
			string targetName = mi->second;
			targetNames_.push_back(targetName);
		}
	}
	nTargets_ = targetNames_.size();

	// Pred Names.
	if( conf.count("predName") )
	{
		pair<mmit, mmit> predNames = conf.equal_range("predName");
		for( mmit mi = predNames.first; mi != predNames.second; ++mi )
		{
			string predName = mi->second;
			predNames_.push_back(predName);
		}
	}

	// Target weights.
	if( conf.count("targetWeight") )
	{
		pair<mmit, mmit> weights = conf.equal_range("targetWeight");
		for( mmit mi = weights.first; mi != weights.second; ++mi )
		{
			double weight = atof(mi->second.c_str());
			targetWeights_.push_back(weight);
		}
	}

	// Pred weights.
	if( conf.count("predWeight") )
	{
		pair<mmit, mmit> weights = conf.equal_range("predWeight");
		for( mmit mi = weights.first; mi != weights.second; ++mi )
		{
			double weight = atof(mi->second.c_str());
			predWeights_.push_back(weight);
		}
	}

	spectatorNames_.push_back("target");
	spectatorNames_.push_back("bmpred");
	spectatorNames_.push_back("pred");
	spectatorNames_.push_back("sprd");
	nSpectator_ = spectatorNames_.size();
}

MAnalyze::~MAnalyze()
{}

void MAnalyze::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	market_ = linMod.market;
}

void MAnalyze::endJob()
{
	if( !targetWeights_.empty() && !predWeights_.empty() )
	{
		analyze_oos();
		analyze_ins();
	}
}

void MAnalyze::analyze_oos()
{
	bool is_oos = true;

	// Prepare data.
	const vector<vector<vector<float> > >* pvvvSpectator = &MEvent::Instance()->get<vector<vector<vector<float> > > >("", (string)("vvvSpectatorOOS") + desc_);
	const map<int, pair<int, int> >* pOffset = &MEvent::Instance()->get<map<int, pair<int, int> > >("", "oosOffset");
	if( pvvvSpectator != 0 && pOffset != 0 )
	{
		anaDir_ = get_ana_dir(is_oos);
		mkd(anaDir_);
		vector<vector<float> > vvSpectator;
		prepare_data(vvSpectator, pvvvSpectator);
		analyze(vvSpectator, *pOffset, is_oos);
	}
}

void MAnalyze::analyze_ins()
{
	bool is_oos = false;

	// Prepare data.
	const vector<vector<vector<float> > >* pvvvSpectator = &MEvent::Instance()->get<vector<vector<vector<float> > > >("", (string)("vvvSpectator") + desc_);
	const map<int, pair<int, int> >* pOffset = &MEvent::Instance()->get<map<int, pair<int, int> > >("", "fitOffset");
	if( pvvvSpectator != 0 && pOffset != 0 )
	{
		anaDir_ = get_ana_dir(is_oos);
		mkd(anaDir_);
		vector<vector<float> > vvSpectator;
		prepare_data(vvSpectator, pvvvSpectator);
		analyze(vvSpectator, *pOffset, is_oos);
	}
}

string MAnalyze::get_ana_dir(bool is_oos)
{
	// Set directories.
	string model = MEnv::Instance()->model;

	string targetDesc = "ana";
	if( !is_oos )
		targetDesc += "ins";
	for( vector<string>::iterator it = targetNames_.begin(); it != targetNames_.end(); ++it )
		targetDesc += "_" + *it;
	if( !predNames_.empty() )
	{
		targetDesc += "_p";
		for( vector<string>::iterator it = predNames_.begin(); it != predNames_.end(); ++it )
			targetDesc += "_" + *it;
	}

	string wgtDesc = "t";
	for( vector<double>::iterator it = targetWeights_.begin(); it != targetWeights_.end(); ++it )
	{
		char desc[10];
		sprintf(desc, "%.1f", *it);
		wgtDesc += (string)"_" + desc;
	}

	if( !targetFilter_.empty() )
		targetDesc += "_" + targetFilter_;
	if( !desc_.empty() )
		targetDesc += "_" + desc_;

	wgtDesc += "_p";
	for( vector<double>::iterator it = predWeights_.begin(); it != predWeights_.end(); ++it )
	{
		char desc[10];
		sprintf(desc, "%.1f", *it);
		wgtDesc += (string)"_" + desc;
	}

	string ret = xpf(MEnv::Instance()->baseDir + "\\" + model + "\\ana\\" + targetDesc + "\\" + wgtDesc + "\\");
	return ret;
}

void MAnalyze::prepare_data(vector<vector<float> >& vvSpectator, const vector<vector<vector<float> > >* pvvvSpectator)
{
	int cntData = (*pvvvSpectator)[0][0].size();
	vvSpectator = vector<vector<float> >(nSpectator_, vector<float>(cntData));
	int iTarget = 0;
	for( vector<string>::iterator it = targetNames_.begin(); it != targetNames_.end(); ++it, ++iTarget )
	{
		// bmpred is alread added to the target and pred.
		for( int j = 0; j < cntData; ++j )
		{
			vvSpectator[0][j] += targetWeights_[iTarget] * (*pvvvSpectator)[iTarget][0][j]; // target.
			vvSpectator[1][j] += predWeights_[iTarget] * (*pvvvSpectator)[iTarget][1][j]; // bmpred.
			vvSpectator[2][j] += predWeights_[iTarget] * (*pvvvSpectator)[iTarget][2][j]; // pred.
		}

		if( iTarget == 0 )
			for( int j = 0; j < cntData; ++j )
				vvSpectator[3][j] = (*pvvvSpectator)[iTarget][3][j]; // sprd.
	}
}

void MAnalyze::analyze(vector<vector<float> >& vvSpectator, const map<int, pair<int, int> >& mOffset, bool is_oos)
{
	// Get the list of dates.
	int udate = MEnv::Instance()->udate;
	vector<int> idates = MEnv::Instance()->idates;
	vector<int> testDates;
	if( is_oos )
		testDates = vector<int>( lower_bound(idates.begin(), idates.end(), udate), idates.end() );
	else
		testDates = vector<int>( idates.begin(), lower_bound(idates.begin(), idates.end(), udate) );

	// Threshold values for trading simulation.
	vector<double> vthres;
	for( double thres = minThres_; thres <= maxThres_; thres += 0.2 )
	{
		double theThres = thres;
		//if( fabs(theThres) < 1e-6 )
			//theThres += 1e-10;
		vthres.push_back(theThres);
	}

	//// Tight spread.
	//AnaObject anaObjT("t", 0, 20);
	////anaObjT.setMarket(market_);
	//anaObjT.setThres(vthres);

	//// Wide spread.
	//AnaObject anaObjW("w", 20, 100);
	////anaObjW.setMarket(market_);
	//anaObjW.setThres(vthres);

	//// Loop over the dates.
	//for( vector<int>::iterator it = testDates.begin(); it != testDates.end(); ++it )
	//{
	//	// Get the range fot the date.
	//	int idate = *it;
	//	if( !mOffset.count(idate) )
	//		continue;
	//	int offset1 = mOffset.find(idate)->second.first;
	//	int offset2 = mOffset.find(idate)->second.second;

	//	// Add bm to target if necessary.

	//	// Process the data.
	//	anaObjT.beginDay(idate, vvSpectator, offset1, offset2);
	//	anaObjW.beginDay(idate, vvSpectator, offset1, offset2);
	//}

	//// Old format for each threshold.
	//for( vector<double>::iterator it = vthres.begin(); it != vthres.end(); ++it )
	//{
	//	double thres = *it;
	//	char fout1[400];
	//	string insoos = is_oos ? "oos" : "ins";
	//	sprintf(fout1, "%s%s%d_%06.2f.txt", anaDir_.c_str(), insoos.c_str(), udate, thres);
	//	ofstream ofs1(fout1);
	//	anaObjT.write_header(ofs1);
	//	for( vector<int>::iterator it = testDates.begin(); it != testDates.end(); ++it )
	//	{
	//		int idate = *it;
	//		anaObjT.write_by_date(ofs1, idate, thres, 0);
	//		anaObjW.write_by_date(ofs1, idate, thres, 0);
	//	}
	//}

	//// Write in another format.
	//char fout2[400];
	//string ins = is_oos ? "" : "ins";
	//sprintf(fout2, "%sana%s%d.txt", anaDir_.c_str(), ins.c_str(), udate);
	//ofstream ofs2(fout2);
	//anaObjT.write_by_tree_header(ofs2);
	//anaObjT.write_by_tree(ofs2, 0);
	//anaObjW.write_by_tree(ofs2, 0);

	return;
}
