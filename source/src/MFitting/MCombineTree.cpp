#include <MFitting/MCombineTree.h>
#include <MFitting.h>
#include <gtlib/util.h>
#include <gtlib_model/pathFtns.h>
#include <jl_lib/jlutil.h>
#include <MFitObj.h>
#include <MFramework.h>
#include <time.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MCombineTree::MCombineTree(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	fitDesc_(MEnv::Instance()->fitDesc)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";

	vNTrees_ = getConfigValuesInt(conf, "nTrees");

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
	if( nTargets_ == 1 && targetWeights_.empty() )
		targetWeights_.push_back(1.);
}

MCombineTree::~MCombineTree()
{}

void MCombineTree::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
	write_model();
}

void MCombineTree::beginDay()
{
}

void MCombineTree::endDay()
{
}

void MCombineTree::endJob()
{
}

void MCombineTree::write_model()
{
	const hff::LinearModel linMod = MEnv::Instance()->linearModel;
	const hff::NonLinearModel& nonLinMod = MEnv::Instance()->nonLinearModel;
	vector<string> markets = MEnv::Instance()->markets;
	int udate = MEnv::Instance()->udate;
	const string& baseDir = MEnv::Instance()->baseDir;

	// ntrees
	for( int i = 0; i < nTargets_; ++i )
	{
		if(vNTrees_[i] == 0)
		{
			string tar = targetNames_[i];
			if(tar.substr(0, 6) == "tar60;")
				vNTrees_[i] = linMod.nTreesOmProd;
			else if(tar.substr(0, 6) == "tar600")
				vNTrees_[i] = nonLinMod.nTreesTmProd;
			else if(tar.substr(0, 9) == "tar71C")
				vNTrees_[i] = nonLinMod.nTrees71ClProd;
			else if(tar.substr(0, 6) == "tarClc")
				vNTrees_[i] = nonLinMod.nTreesClclProd;
			else if(tar.substr(0, 6) == "tarOpc")
				vNTrees_[i] = nonLinMod.nTreesOpclProd;
		}
		if(vNTrees_[i] < 2)
			exit(42);
	}

	// read
	vector<string> outLines;
	int iTreeCum = -1;
	int prevITree = 0;
	for(int i = 0; i < nTargets_; ++i)
	{
		char buf[1000];
		float wgt = targetWeights_[i];
		float ntree = vNTrees_[i];
		fitDir_ = get_fit_dir(baseDir, MEnv::Instance()->model, targetNames_[i], fitDesc_);
		string filename = fitDir_ + "/coef/coef" + itos(udate) + ".txt";

		ifstream ifs(xpf(filename).c_str());
		string line;
		getline(ifs, line); // header
		if(outLines.empty())
			outLines.push_back(line + '\n');

		int targetNTreeRead = 0;
		int targetNRowRead = 0;
		while( getline(ifs, line) )
		{
			ModelRow mr = getRow(line);
			if(mr.iTree < ntree)
			{
				++targetNRowRead;
				if(outLines.size() == 1 || mr.iTree != prevITree)
				{
					if(mr.iTree == prevITree + 1 || mr.iTree == 0)
					{
						++iTreeCum;
						++targetNTreeRead;
					}
					else
						exit(48);
				}

				if(fabs(wgt - 1.) > 0.001)
					mr.mu *= wgt;
				sprintf(buf, "%4d %7.4f %3d %12.6f %10d %10.5f %15.4f\n", iTreeCum, mr.weight, mr.var, mr.cut, mr.indnum, mr.mu, mr.deviance);
				outLines.push_back(buf);

				prevITree = mr.iTree;
			}
		}
		printf("target %s %d trees %d rows\n", targetNames_[i].c_str(), targetNTreeRead, targetNRowRead);
	}

	// write
	char filename[1000];
	//string outModel = MEnv::Instance()->model + "_combinedModels";
	string outModel = MEnv::Instance()->model;
	string combinedTargetName = getCombinedTarget();
	string outDir = get_fit_dir(baseDir, outModel, getCombinedTarget(), fitDesc_) + "/coef";
	mkd(outDir);
	sprintf(filename, "%s/coef%d.txt", outDir.c_str(), udate);
	ofstream ofs(filename);
	for(auto& line : outLines)
		ofs << line;
	printf("combined target %s written to:\n\t%s\n", combinedTargetName.c_str(), filename);
}

MCombineTree::ModelRow MCombineTree::getRow(const string& line)
{
	vector<string> sl = split(line);
	ModelRow mr;
	mr.iTree = atoi(sl[0].c_str());
	mr.weight = atof(sl[1].c_str());
	mr.var = atoi(sl[2].c_str());
	mr.cut = atof(sl[3].c_str());
	mr.indnum = atoi(sl[4].c_str());
	mr.mu = atof(sl[5].c_str());
	mr.deviance = atof(sl[6].c_str());
	return mr;
}

string MCombineTree::getCombinedTarget()
{
	// target name
	string combinedTarget;
	if( nTargets_ == 1 )
		combinedTarget = targetNames_[0];
	else if( nTargets_ > 1 )
	{
		combinedTarget = "";
		for( int i = 0; i < nTargets_; ++i )
		{
			char buf[200];
			string name = targetNames_[i];
			float wgt = targetWeights_[i];
			if(name == "tar600;60_1.0_tar3600;660_0.5" && wgt == 1.)
				sprintf(buf, "%s", name.c_str());
			else
				sprintf(buf, "%s_%.1f", name.c_str(), wgt);
			if( combinedTarget.empty() )
				combinedTarget += buf;
			else
				combinedTarget += (string)"_" + buf;
		}
	}
	return combinedTarget;
}
