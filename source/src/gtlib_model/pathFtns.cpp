#include <gtlib_model/pathFtns.h>
#include <jl_lib/jlutil.h>
#include <boost/filesystem.hpp>
using namespace std;

namespace gtlib {

// ---------------------------------------------------------------
// Paths.
// ---------------------------------------------------------------

//string get_np_dir(const string& baseDir, const string& model, const string& fitDesc)
//{
//	string desc = get_desc(fitDesc);
//	char dir[400];
//	sprintf( dir, "%s/%s/npSig%s/", baseDir.c_str(), model.c_str(), desc.c_str() );
//	mkd(dir);
//	return xpf(dir);
//}
//
//string get_np_path(const string& baseDir, const string& model, int idate, const string& fitDesc)
//{
//	string dir = get_np_dir(baseDir, model, fitDesc);
//	string filename = "sig" + itos(idate) + ".npz";
//	string path = xpf((string)dir + "/" + filename);
//	return path;
//}

string get_sig_dir(const string& baseDir, const string& model, const string& sigType, const string& fitDesc, const string& sigFormat)
{
	string desc = get_desc(fitDesc);
	char dir[400];
	sprintf( dir, "%s/%s/%sSig%s/%s",
			baseDir.c_str(), model.c_str(), sigFormat.c_str(), desc.c_str(), sigType.c_str() );
	mkd(dir);
	return xpf(dir);
}

string get_txt_path(const string& baseDir, const string& model, int idate, const string& sigType, const string& fitDesc)
{
	string dir = get_sig_dir(baseDir, model, sigType, fitDesc, "txt");
	string filename = "sigTB" + itos(idate) + ".txt";
	string path = xpf((string)dir + "/" + filename);
	return path;
}

string get_sig_path(const string& baseDir, const string& model, int idate, const string& sigType, const string& fitDesc)
{
	string dir = get_sig_dir(baseDir, model, sigType, fitDesc, "bin");
	string ext = (sigType == "np") ? ".npz" : ".bin";
	string filename = "sig" + itos(idate) + ext;
	string path = xpf((string)dir + "/" + filename);
	return path;
}

string get_sigTxt_path(const string& baseDir, const string& model, int idate, const string& sigType, const std::string& fitDesc)
{
	string dir = get_sig_dir(baseDir, model, sigType, fitDesc, "bin");
	string filename = "sig" + itos(idate) + ".txt";
	string path = xpf((string)dir + "/" + filename);
	return path;
}

string get_fit_dir(const string& baseDir, const string& model, const string& targetName, const string& fitDesc)
{
	string desc = get_desc(fitDesc);
	char outdir[800];
	sprintf( outdir, "%s/%s/fit%s/%s", baseDir.c_str(), model.c_str(), desc.c_str(), targetName.c_str() );
	return xpf(outdir);
}

string getTargetName(const string& sigType, const string& fitDesc)
{
	string targetName;
	if( fitDesc == "reg" )
	{
		if( sigType == "om" )
			targetName = "tar60;0_xbmpred60;0";
		else if( sigType == "tm" )
			targetName = "tar600;60_1.0_tar3600;660_0.5";
	}
	else if( fitDesc == "tevt" )
	{
		if( sigType == "om" )
			targetName = "tar60;0_xbmpred60;0";
	}
	return targetName;
}

string get_pred_dir(const string& baseDir, const string& model, int idate, const string& targetName, const string& fitDesc, int udate)
{
	string desc = get_desc(fitDesc);
	char outdir[800];
	if( udate == 0 )
		udate = get_modelDate( get_fit_dir(baseDir, model, targetName, fitDesc) + xpf("/coef"), idate );

	sprintf( outdir, "%s/%s/fit%s/%s/stat_%d/preds", baseDir.c_str(), model.c_str(), desc.c_str(), targetName.c_str(), udate );
	return xpf(outdir);
}

string get_pred_path(const string& baseDir, const string& model, int idate, const string& targetName, const string& fitDesc, int udate, bool is_oos)
{
	string outdir = get_pred_dir(baseDir, model, idate, targetName, fitDesc, udate);
	mkd(outdir);

	string filename = "pred" + itos(idate) + ".txt";
	string path = xpf((string)outdir + "/" + filename);
	return path;
}

string get_predvar_path(const string& baseDir, const string& model, int idate, const string& targetName)
{
	char outdir[400];
	sprintf( outdir, "%s/%s/statVar/%s/preds", baseDir.c_str(), model.c_str(), targetName.c_str() );
	mkd(outdir);

	string filename = "pred" + itos(idate) + ".txt";
	string path = xpf((string)outdir + "/" + filename);
	return path;
}

string get_cov_dir(const string& baseDir, const string& model)
{
	char dir[400];
	sprintf(dir, "%s/%s/cov", baseDir.c_str(), model.c_str());
	return dir;
}

string get_tmCov_dir(const string& baseDir, const string& model)
{
	char dir[400];
	sprintf(dir, "%s/%s/tmCov", baseDir.c_str(), model.c_str());
	return dir;
}

string get_desc(const string& fitDesc)
{
	string desc = (fitDesc == "reg") ? "" : fitDesc;
	if( !desc.empty() )
		desc[0] = toupper(desc[0]);
	return desc;
}

string get_weight_dir(const string& baseDir, const string& model)
{
	char outdir[400];
	sprintf( outdir, "%s/%s/weight", baseDir.c_str(), model.c_str() );
	return xpf(outdir);
}

string get_tmWeight_dir(const string& baseDir, const string& model)
{
	char outdir[400];
	sprintf( outdir, "%s/%s/tmWeight", baseDir.c_str(), model.c_str() );
	return xpf(outdir);
}

vector<int> get_modelDates(const string& coefDir)
{
	namespace fs = boost::filesystem;
	fs::path dir(coefDir);

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

	sort(modelDates.begin(), modelDates.end());
	return modelDates;
}

int get_modelDate(const string& coefDir, int testDate)
{
	vector<int> modelDates = get_modelDates(coefDir);
	int modelDate = 0;
	for( auto it = begin(modelDates); it != end(modelDates); ++it )
	{
		if( *it <= testDate )
			modelDate = *it;
	}
	return modelDate;
}

vector<int> getDiskIdates(const string& path)
{
	namespace fs = boost::filesystem;
	fs::path dir(path);

	if( !fs::exists( dir ) )
		cout << dir << " does not exist.\n";

	vector<int> diskIdates;
	if( fs::is_directory( dir ) )
	{
		fs::directory_iterator end_iter;
		for( fs::directory_iterator itd(dir); itd != end_iter; ++itd )
		{
			string filename = itd->path().filename().string();
			int N = filename.size();
			int pos0 = filename.find("20");
			if( pos0 != string::npos && N >= pos0 + 8 )
			{
				int idate = atoi(filename.substr(pos0, 8).c_str());
				if( idate > 20000000 && idate < 21000000 )
					diskIdates.push_back(idate);
			}
		}
	}
	sort(begin(diskIdates), end(diskIdates));
	return diskIdates;
}

int getIdateToBeginWith(const string& sigDir, const string& market)
{
	vector<int> diskIdates = getDiskIdates(sigDir);
	int ret = 0;
	if( !diskIdates.empty() )
	{
		int maxIdate = *max_element(begin(diskIdates), end(diskIdates));
		ret = nextOpen(market, maxIdate);
	}
	return ret;
}

int getIdateToBeginWith(const string& sigDir, const string& market,
		const vector<int>& idateRange)
{
	vector<int> diskIdates = getDiskIdates(sigDir);

	int minMissingIdate = 99999999;
	for( int idate : idateRange )
	{
		if( find(begin(diskIdates), end(diskIdates), idate) == diskIdates.end() )
		{
			minMissingIdate = idate;
			break;
		}
	}
	return minMissingIdate;
}

int getIdateToBeginWithPred(const string& predDir, const string& market)
{
	int udate = 0;
	int pos = predDir.find("20");
	if( pos != string::npos && predDir.size() > pos + 8 )
		udate = atoi(predDir.substr(pos, 8).c_str());
	else
	{
		cerr << "Invalid pred dir \t" << predDir << endl;
		exit(23);
	}

	vector<int> diskIdates = getDiskIdates(predDir);

	int ret = udate;
	if( !diskIdates.empty() )
	{
		int maxIdate = *max_element(begin(diskIdates), end(diskIdates));
		ret = nextOpen(market, maxIdate);
	}

	return ret;
}

} // namespace gtlib
