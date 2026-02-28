#include <gtlib_fitting/DTFtns.h>
#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_fitting/fittingObjs.h>
#include <gtlib_fitting/DecisionTree.h>
#include <gtlib_fitting/DecisionTreeRegression.h>
#include <gtlib_fitting/FitData.h>
#include <gtlib_model/mdl.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/jlutil.h>
using namespace std;

namespace gtlib {

shared_ptr<DTNode> make_node(vector<string>::const_iterator& itFrom, vector<string>::const_iterator& itTo)
{
	if( itFrom == itTo )
	{
		cerr << "ERROR: Corrupt model.\n";
		exit(12);
	}
	vector<string> thisLine = split(*itFrom);
	++itFrom;

	int iInput = atof(thisLine[2].c_str());
	double cut = atof(thisLine[3].c_str());
	int indnum = atoi(thisLine[4].c_str());
	double mu = atof(thisLine[5].c_str());
	double deviance = atof(thisLine[6].c_str());

	shared_ptr<DTNode> pnode = make_shared<DTNode>(mu, deviance, indnum, iInput, cut);
	if( pnode->iInput > -1 ) // not terminal.
	{
		pnode->left = make_node(itFrom, itTo);
		pnode->right = make_node(itFrom, itTo);
	}

	return pnode;
}

void DTreadModel(const string& path, vector<DecisionTree*>& vpT, vector<double>& wts)
{
	for( auto pTree : vpT)
		delete pTree;

	ifstream ifs(path.c_str());
	vector<string> lines;
	string line;
	while( getline(ifs, line) )
		lines.push_back(line);

	for( int iTree = 0; ; ++iTree )
	{
		auto itFrom = getPositionFrom(lines, iTree);
		auto itTo = getPositionTo(lines, iTree);
		if( itFrom == itTo )
			break;
		vector<string> sl = split(*itFrom);
		float wgt = atof(sl[1].c_str());
		wts.push_back(wgt);
		vpT.push_back(new DecisionTreeRegression(itFrom, itTo));
	}
}

void DTreadModel(const string& dbname, const string& dbtable, const string& mkt, int udate, vector<DecisionTree*>& vpT, vector<double>& wts)
{
	for( auto pTree : vpT)
		delete pTree;

	char cmd[1000];
	sprintf(cmd, "select treenumber, treewt, cutvariable, cutvalue, npts, mu, deviance "
			" from %s where idate = %d and market = '%s' order by treenumber, idx",
			dbtable.c_str(), udate, mkt.c_str());

	vector<vector<string>> vv;
	GODBC::Instance()->read(dbname, cmd, vv);

	vector<string> lines;
	for(vector<vector<string>>::iterator it = vv.begin(); it != vv.end(); ++it)
	{
		string line;
		for(string& s : (*it))
			line += (s + " ");
		lines.push_back(line);
	}

	for( int iTree = 0; ; ++iTree )
	{
		auto itFrom = getPositionFrom(lines, iTree);
		auto itTo = getPositionTo(lines, iTree);
		if( itFrom == itTo )
			break;
		vector<string> sl = split(*itFrom);
		float wgt = atof(sl[1].c_str());
		wts.push_back(wgt);
		vpT.push_back(new DecisionTreeRegression(itFrom, itTo));
	}
}

int getPredStep(int ntrees)
{
	int step = 0;
	if( ntrees < 10 )
		step = 1;
	else if( ntrees < 20 )
		step = 2;
	else if( ntrees < 100 )
		step = 5;
	else
		step = 10;
	return step;
}

void DTupdateTarget(FitData* pFitData, int iTree, vector<DecisionTree*>& vpT, vector<double>& wts)
{
	// Subtract prediction of vpTrees_[iTree] from the target.
	for( int ic = 0; ic < pFitData->nSamplePoints; ++ic )
	{
		double pred = vpT[iTree]->pred(ic);
		double wgt = wts[iTree];
		pFitData->target(ic) -= pred * wgt;
	}
}

void DTwriteModel(const string& path, vector<DecisionTree*>& vpT, vector<double>& wts)
{
	ofstream ofs(path.c_str());
	char buf[200];
	sprintf(buf, "%4s %7s %3s %12s %10s %10s %15s\n", "num", "weight", "var", "varCut<", "nPts", "mu", "deviance");
	ofs << buf;

	int ntrees = vpT.size();
	for( int iTree = 0; iTree < ntrees; ++iTree )
		writeNode(ofs, vpT[iTree]->pRoot_, iTree, wts[iTree]);
}

void writeNode(ofstream& ofs, shared_ptr<DTNode> z, int iTree, double weight)
{
	int iInput = -1;
	double cut = 0.;
	if( z != 0 )
	{
		if( z->left != 0 ) // not a terminal node.
		{
			iInput = z->iInput;
			cut = z->cut;
		}
		char buf[1000];
		sprintf(buf, "%4d %7.4f %3d %12.6f %10d %10.5f %15.4f\n", iTree, weight, iInput, cut, z->indnum, z->mu, z->deviance);
		ofs << buf;
		writeNode(ofs, z->left, iTree, weight);
		writeNode(ofs, z->right, iTree, weight);
	}
}

void DTstat(FitData* pFitData, vector<DecisionTree*>& vpT, const string& fitDir, int udate)
{
	int ntrees = vpT.size();
	int nInputFields = pFitData->nInputFields;
	auto& inputNames = pFitData->inputNames;
	vector<vector<double> > stats(ntrees + 1, vector<double>(nInputFields));

	// Calculate.
	for( int it = 0; it < ntrees; ++it )
		vpT[it]->deviance_improvement(stats[it]);

	// Normalize.
	for( int it = 0; it < ntrees; ++it )
		for( int ic = 0; ic < nInputFields; ++ic )
			stats[ntrees][ic] += stats[it][ic] / ntrees;

	for( int it = 0; it < ntrees + 1; ++it )
		for( int ic = 0; ic < nInputFields; ++ic )
			stats[it][ic] = sqrt(stats[it][ic]);

	for( int it = 0; it < ntrees + 1; ++it )
	{
		double maxElem = *max_element(stats[it].begin(), stats[it].end());
		for( int ic = 0; ic < nInputFields; ++ic )
			stats[it][ic] = 100. * stats[it][ic] / maxElem;
	}

	string filename = getStatDir(fitDir, udate) + "/stat.txt";
	ofstream ofs(filename.c_str());

	// Write first row.
	ofs << "    ";
	for( string name : inputNames )
		ofs << " " << name;
	ofs << "\n";

	// Write other rows.
	for( int it = 0; it < ntrees + 1; ++it )
	{
		char rowLabel[10];
		if( it == ntrees )
			sprintf(rowLabel, " mu");
		else
			sprintf(rowLabel, "%3d", it);
		ofs << rowLabel;

		for( int ic = 0; ic < nInputFields; ++ic )
		{
			char buf[10];
			sprintf(buf, " %6.2f", stats[it][ic]);
			ofs << buf;
		}
		ofs << "\n";
	}
}

float DTgetPred(const vector<float>& input,
		vector<DecisionTree*>& vpT, vector<double>& wts)
{
	int ntrees = vpT.size();
	double pred = 0.;
	for( int iT = 0; iT < ntrees; ++iT )
	{
		pred += vpT[iT]->pred(input) * wts[iT];
	}
	return pred;
}

vector<float> DTgetPredSeries(int iSample, FitData* p,
		vector<DecisionTree*>& vpT, vector<double>& wts)
{
	int ntrees = vpT.size();
	int predStep = getPredStep(ntrees);
	vector<float> input = p->getSample(iSample);
	vector<float> predSeries;
	double pred = 0.;
	for( int iT = 0; iT < ntrees; ++iT )
	{
		pred += vpT[iT]->pred(input) * wts[iT];
		if( iT == ntrees - 1 || (iT + 1) > 1 && (iT + 1) % predStep == 0 )
			predSeries.push_back(pred);
	}
	return predSeries;
}

vector<string> DTgetPredSeriesNames(int ntrees)
{
	int predStep = getPredStep(ntrees);
	vector<string> predSeriesNames;
	for( int iT = 0; iT < ntrees; ++iT )
	{
		if( iT == ntrees - 1 || (iT + 1) > 1 && (iT + 1) % predStep == 0 )
		{
			string name = "pred" + itos(iT + 1);
			predSeriesNames.push_back(name);
		}
	}
	return predSeriesNames;
}

vector<string>::const_iterator getPositionFrom(const vector<string>& lines, int iTree)
{
	auto linesEnd = end(lines);
	for( auto it = begin(lines); it != linesEnd; ++it )
	{
		auto sl = split(*it);
		if( isdigit(sl[0][0]) )
		{
			if( atoi(sl[0].c_str()) == iTree )
				return it;
		}
	}
	return linesEnd;
}

vector<string>::const_iterator getPositionTo(const vector<string>& lines, int iTree)
{
	auto linesEnd = end(lines);
	for( auto it = begin(lines); it != linesEnd; ++it )
	{
		auto sl = split(*it);
		if( isdigit(sl[0][0]) )
		{
			if( atoi(sl[0].c_str()) > iTree )
				return it;
		}
	}
	return linesEnd;
}

} // namespace gtlib
