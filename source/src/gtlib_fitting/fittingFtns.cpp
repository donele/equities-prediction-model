#include <gtlib_fitting/fittingFtns.h>
#include <gtlib_model/mdl.h>
#include <jl_lib/jlutil.h>
#include <boost/filesystem.hpp>
using namespace std;

namespace gtlib {

string getStatDir(const string& fitDir, int udate)
{
	string statDir = fitDir + "/stat_" + itos(udate);
	return statDir;
}

string getCoefDir(const string& fitDir)
{
	string statDir = fitDir + "/coef";
	return statDir;
}

string getPredDir(const string& fitDir, int udate)
{
	string predDir = getStatDir(fitDir, udate) + "/preds";
	return predDir;
}

string getPredPath(const string& fitDir, int udate, int idate)
{
	return getPredPath(fitDir, fitDir, udate, idate);
}

string getPredPath(const string& fitDir, const string& coefFitDir, int udate, int idate)
{
	string predDir = (udate > 0) ? getPredDir(fitDir, udate) : getPredDir(fitDir, getRecentModelDate(coefFitDir, idate));
	string predPath = predDir + "/pred" + itos(idate) + ".txt";
	return predPath;
}

string getPredPath(const string& fitDir, int idate)
{
	string coefFitDir = fitDir;
	string predDir = getPredDir(fitDir, getRecentModelDate(coefFitDir, idate));
	string predPath = predDir + "/pred" + itos(idate) + ".txt";
	return predPath;
}

string getTxtPath(const string& binPath)
{
	string txtPath = binPath;
	int extPos = txtPath.find(".bin");
	txtPath.replace(extPos, 4, ".txt");
	return txtPath;
}

vector<int> get_fitdates(const vector<int>& idates, int udate)
{
	auto itU = lower_bound(begin(idates), end(idates), udate);
	vector<int> fitdates;
	int N = idates.size();
	if( N > 0 )
	{
		if( idates[0] < udate && idates[N - 1] < udate )
			fitdates = vector<int>(idates.begin(), itU);
		else if( idates[0] >= udate )
			fitdates = vector<int>(itU, idates.end());
	}
	return fitdates;
}

vector<int> getModelDates(const string& fitDir)
{
	namespace fs = boost::filesystem;
	fs::path dir(getCoefDir(fitDir));

	if( !fs::exists( dir ) )
		cout << dir << " does not exist.\n";

	vector<int> modelDates;
	if( fs::is_directory( dir ) )
	{
		fs::directory_iterator end_iter;
		for( fs::directory_iterator itd(dir); itd != end_iter; ++itd )
		{
			string filename = itd->path().filename().string();
			if( filename.size() == 16 )
			{
				int idate = atoi(filename.substr(4, 8).c_str());
				modelDates.push_back(idate);
			}
		}
	}

	sort(begin(modelDates), end(modelDates));
	return modelDates;
}

int getRecentModelDate(const string& coefFitDir, int idate)
{
	auto modelDates = getModelDates(coefFitDir);
	int modelDate = 0;
	for( int tempModelDate : modelDates )
	{
		if( tempModelDate <= idate )
			modelDate = tempModelDate;
	}
	return modelDate;
}

string getModelPath(const string& fitDir, int modelDate)
{
	string modelPath = getCoefDir(fitDir) + "/coef" + itos(modelDate) + ".txt";
	return modelPath;
}

string getMarket(const string& path)
{
	string market;
	namespace fs = boost::filesystem;
	const fs::path absolute_path = fs::absolute(fs::path(path));
	string dir = absolute_path.parent_path().string();
	int pos_hffit = dir.find("hffit");
	if( pos_hffit != string::npos )
	{
		int pos_model = dir.find("/", pos_hffit) + 1;
		if( pos_model != string::npos )
		{
			string model = dir.substr(pos_model, 2);
			market = mdl::markets(model)[0];
		}
	}
	return market;
}

string getSigType(const string& targetName)
{
	string sigType;
	if( targetName == "tarON" || targetName == "tarClcl" )
		sigType = "tm";
	else
	{
		int posBeg = 0;
		while( !isdigit(targetName[posBeg]) )
			++posBeg;
		int posEnd = posBeg + 1;
		while( isdigit(targetName[posEnd]) )
			++posEnd;
		int horizon = atoi(targetName.substr(posBeg, posEnd).c_str());
		if( horizon <= 60 )
			sigType = "om";
		else if( horizon > 60 )
			sigType = "tm";
	}
	return sigType;
}

} // namespace gtlib
