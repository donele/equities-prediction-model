#include <map>
#include <string>
#include <vector>
#include <iterator>
#include <jl_lib.h>
using namespace std;

// Compare two signal files.

string get_hffit_dir(string model, string machine, string disk);
string get_hfs_dir(string model, string machine, string disk);
string bin_path(string dir, int idate);
string txt_path(string dir, int idate);
void comp(vector<Corr>& vCorr, vector<Corr>& vCorrTar, map<string, Corr>& mCorrUniv, map<string, Corr>& mCorrErr, map<string, Corr>& mCorrTot,
		  int idate, string dir1, string dir2, vector<int>& vVars1, vector<int>& vVars2, vector<string>& varNameSorted);
void cnt_data(map<string, int>& mUidCnt, string dir, int idate);
void comp_cnt(map<string, int>& mUidCnt1, map<string, int>& mUidCnt2);
void get_uids(set<string>& uids, map<string, int>& mUidCnt1, map<string, int>& mUidCnt2, int n);
void get_uids(set<string>& uids, string dir, int idate, int nSymbols);
void get_data(set<string>& tickres, map<string, map<int, vector<float> > >& mUidTimeSig, int& NV, vector<string>& varName, string dir, int idate);
void get_data_bin(set<string>& tickres, map<string, map<int, vector<float> > >& mUidTimeSig, int& NV, vector<string>& varName, string dir, int idate);
void get_data_txt(set<string>& tickres, map<string, map<int, vector<float> > >& mUidTimeSig, int& NV, vector<string>& varName, string dir, int idate);
void check_diff(string uid, int time, vector<float>& v1, vector<float>& v2, vector<string>& varNameSorted);
bool diff_much(int i, float v1, float v2, vector<string>& varNameSorted);
void comp_name(vector<string>& varName1, vector<string>& varName2);
void get_var_list(vector<int>& vVars1, vector<int>& vVars2, string framework1, string framework2);
void fill_stat(vector<Corr>& vCorr, vector<Corr>& vCorrTar, Corr& corrUniv, Corr& corrErr, Corr& corrTot, vector<float>& v1, vector<float>& v2);

bool excluded(string symbol);

const string uid = "";
int nSymbols = 0;

const int max_debug_cnt = 0;
int n_debug_cnt = 0;

int max_debug_diff = -1;
int n_debug_diff = 0;
int max_debug_diff_tarpred = -1;
int n_debug_diff_tarpred = 0;
int max_debug_diff_mret = -1;
int n_debug_diff_mret = 0;

const bool debug_bmpred = false;
bool do_count = true;
bool ticker_corr = false;
const bool debug_varname = false;
const bool exclude_some_uids = false;
int min_msecs = 0;
const bool debug_rz_extra_bmpred = false;
bool debug_asx = false;

// Global variables.
string type = "tm"; // om or tm.
string input = "bin"; // bin or txt.
string sample = "reg"; // reg or booktrade.
string mCode = "H";
int iPredUniv = 0;
int iPredErr = 0;

int main(int argc, char* argv[])
{
	string framework1 = "hffit";
	string framework2 = "hfs";
	string model1 = "AH";
	string model2 = "hk";
	string machine1 = "nas03";
	string machine2 = "mrct45";
	string drive1 = "gf1";
	string drive2 = "fdrive";
	vector<int> idates;

	if( argc > 1 && argv[1][0] == '-' )
	{
		Arg arg(argc, argv);
		if( arg.isGiven("dx") )
			debug_asx = true;

		if( arg.isGiven("s1") )
		{
			vector<string> sig = arg.getVals("s1");
			framework1 = sig[0];
			model1 = sig[1];
			machine1 = sig[2];
			drive1 = sig[3];
		}

		if( arg.isGiven("s2") )
		{
			vector<string> sig = arg.getVals("s2");
			framework2 = sig[0];
			model2 = sig[1];
			machine2 = sig[2];
			drive2 = sig[3];
		}

		if( arg.isGiven("c") )
			do_count = true;

		if( arg.isGiven("m") )
			mCode = arg.getVal("m");

		if( arg.isGiven("t1") )
			min_msecs = atoi(arg.getVal("t1").c_str());

		if( arg.isGiven("n") )
			nSymbols = atoi(arg.getVal("n").c_str());

		if( arg.isGiven("t") )
			type = arg.getVal("t");

		if( arg.isGiven("i") )
			input = arg.getVal("i");

		if( arg.isGiven("s") )
			sample = arg.getVal("s");

		if( arg.isGiven("nd") )
			max_debug_diff = atoi(arg.getVal("nd").c_str());

		if( arg.isGiven("np") )
			max_debug_diff_tarpred = atoi(arg.getVal("np").c_str());

		if( arg.isGiven("nr") )
			max_debug_diff_mret = atoi(arg.getVal("nr").c_str());

		if( arg.isGiven("d") )
		{
			vector<string> dates = arg.getVals("d");
			for( vector<string>::iterator it = dates.begin(); it != dates.end(); ++it )
				idates.push_back(atoi((*it).c_str()));
		}
	}
	else if( argc >= 13 )
	{
		framework1 = argv[1];
		model1 = argv[2];
		machine1 = argv[3];
		drive1 = argv[4];

		framework2 = argv[5];
		model2 = argv[6];
		machine2 = argv[7];
		drive2 = argv[8];

		mCode = argv[9];
		type = argv[10];
		input = argv[11];
		for( int i = 12; i < argc; ++i )
		{
			if( (string)argv[i] == ">" )
				break;
			idates.push_back(atoi(argv[i]));
		}
	}
	else
	{
		cout << "> compSig.exe hffit AH mrct40 l hfs hk mrct45 fdrive H tm bin 20120505 20120506 20120507\n";
		cout << "> compSig.exe -s1 hffit AH mrct40 l -s2 hfs hk mrct45 fdrive -m H -t tm -i bin -d 20120505 20120506 20120507\n";
		exit(1);
	}

	string dir1 = (framework1 == "hffit") ? get_hffit_dir(model1, machine1, drive1) : get_hfs_dir(model1, machine1, drive1);
	string dir2 = (framework2 == "hffit") ? get_hffit_dir(model2, machine2, drive2) : get_hfs_dir(model2, machine2, drive2);

	vector<int> vVars1;
	vector<int> vVars2;
	get_var_list(vVars1, vVars2, framework1, framework2);

	int NVV1 = vVars1.size();
	int NVV2 = vVars2.size();

	vector<Corr> vCorr(NVV1);
	vector<Corr> vCorrTar(4); // 1-1 1-2 2-1 2-2
	map<string, Corr> mCorrUniv;
	map<string, Corr> mCorrErr;
	map<string, Corr> mCorrTot;

	char buf[200];
	vector<string> varNameSorted;
	for( vector<int>::iterator it = idates.begin(); it != idates.end(); ++it )
	{
		int idate = *it;
		cout << idate << endl;
		comp(vCorr, vCorrTar, mCorrUniv, mCorrErr, mCorrTot, /*vhist1, vhist2, vprof,*/ idate, dir1, dir2, vVars1, vVars2, varNameSorted);

		if( type == "om" )
		{
			sprintf(buf, "#data %.0f %.4f %.4f", vCorr[0].accX.n, vCorrTar[0].corr(), vCorrTar[2].corr());
			cout << buf << endl;
		}
	}

	int nCorr = vCorr.size();
	for( int i = 0; i < nCorr; ++i )
	{
		double stdevRat = 0.;
		if( vCorr[i].accY.stdev() > 0. )
			stdevRat = vCorr[i].accX.stdev() / vCorr[i].accY.stdev();

		sprintf(buf, "%2d %16s %6.4f %9.4f %9.4f %9.4f %9.4f %.4f\n", i, varNameSorted[i].c_str(), vCorr[i].corr(),
			vCorr[i].accX.mean(), vCorr[i].accX.stdev(), vCorr[i].accY.mean(), vCorr[i].accY.stdev(), stdevRat);
		cout << buf;
	}

	if( type == "om" )
	{
		for( int i = 0; i < 4; ++i )
		{
			sprintf(buf, "corr(bmpred, tar1) %d:%d %.4f\n", i / 2 + 1, i % 2 + 1, vCorrTar[i].corr());
			cout << buf;
		}

		if( ticker_corr && input == "bin" )
		{
			for( map<string, Corr>::iterator it = mCorrUniv.begin(); it != mCorrUniv.end(); ++it )
			{
				sprintf(buf, "%s %9.4f %9.4f %9.4f\n", it->first.c_str(), it->second.corr(), mCorrErr[it->first].corr(), mCorrTot[it->first].corr());
				cout << buf;
			}
		}
	}
	cout << endl;
}

string get_hffit_dir(string model, string machine, string disk)
{
	string subdir;
	if( sample == "reg" )
		subdir = ("bin" == input) ? "binSig" : "txtSig";
	else if( sample == "booktrade" )
		subdir = ("bin" == input) ? "binSigTevt" : "txtSigTevt";

	char dir[200];
	if( machine == "mnt" )
		sprintf(dir, "/home/jelee/%s/%s/%s/%s", disk.c_str(), model.c_str(), subdir.c_str(), type.c_str());
	else if( machine == "mnt2" )
		sprintf(dir, "/home/jelee/%s/%s/%s/%s", disk.c_str(), model.c_str(), subdir.c_str(), type.c_str());
	else
		sprintf(dir, "\\\\smrc-%s\\%s\\hffit\\%s\\%s\\%s", machine.c_str(), disk.c_str(), model.c_str(), subdir.c_str(), type.c_str());
	return dir;
}

string get_hfs_dir(string model, string machine, string disk)
{
	if( model == "eu" )
		model = "lseEuEcn";

	string subdir;
	if( sample == "reg" )
		subdir = ("bin" == input) ? "resbinsigs" : "txtsigs";
	else if( sample == "booktrade" )
	{
		subdir = ("bin" == input) ? "eventBinSigsNew" : "";
		model = "XE\\trdbk_events";
	}

	char dir[200];
	sprintf(dir, "\\\\smrc-%s\\%s\\hfs\\%s\\%s\\%s", machine.c_str(), disk.c_str(), subdir.c_str(), type.c_str(), model.c_str());
	return dir;
}

void comp(vector<Corr>& vCorr, vector<Corr>& vCorrTar, map<string, Corr>& mCorrUniv, map<string, Corr>& mCorrErr, map<string, Corr>& mCorrTot,
		  int idate, string dir1, string dir2, vector<int>& vVars1, vector<int>& vVars2, vector<string>& varNameSorted)
{
	int NVV1 = vVars1.size();
	int NVV2 = vVars2.size();

	// Count the uids and the number of rows.
	if( do_count )
	{
		map<string, int> mUidCnt1;
		map<string, int> mUidCnt2;
		cnt_data(mUidCnt1, dir1, idate);
		cnt_data(mUidCnt2, dir2, idate);
		comp_cnt(mUidCnt1, mUidCnt2);
	}

	// Compare content of the selected uids.
	{
		set<string> uids;
		get_uids(uids, dir1, idate, nSymbols);

		int NV1 = 0;
		int NV2 = 0;
		vector<string> varName1;
		vector<string> varName2;
		map<string, map<int, vector<float> > > mUidTimeSig1;
		map<string, map<int, vector<float> > > mUidTimeSig2;
		get_data(uids, mUidTimeSig1, NV1, varName1, dir1, idate);
		get_data(uids, mUidTimeSig2, NV2, varName2, dir2, idate);

		if( varNameSorted.empty() )
		{
			int n = vVars1.size();
			for( int i = 0; i < n; ++i )
				varNameSorted.push_back(varName1[vVars1[i]]);
		}

		if( debug_varname )
			comp_name(varName1, varName2);

		int time_prev = 0;
		for( map<string, map<int, vector<float> > >::iterator it1 = mUidTimeSig1.begin(); it1 != mUidTimeSig1.end(); ++it1 )
		{
			string uid = it1->first;

			if( exclude_some_uids && excluded(uid) )
				continue;

			Corr corrUniv;
			Corr corrErr;
			Corr corrTot;
			for( map<int, vector<float> >::iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2 )
			{
				int time = it2->first;
				if( time != time_prev )
				{
					map<string, map<int, vector<float> > >::iterator it3 = mUidTimeSig2.find(uid);
					if( it3 != mUidTimeSig2.end() )
					{
						map<int, vector<float> >::iterator it4 = it3->second.find(time);
						if( it4 != it3->second.end() )
						{
							vector<float>& temp_v1 = it2->second;
							vector<float>& temp_v2 = it4->second;

							vector<float> v1;
							for( int i = 0; i < NVV1; ++i )
								v1.push_back(temp_v1[vVars1[i]]);

							vector<float> v2;
							for( int i = 0; i < NVV2; ++i )
								v2.push_back(temp_v2[vVars2[i]]);

							check_diff(uid, time, v1, v2, varNameSorted);

							if( debug_rz_extra_bmpred )
							{
								printf("db %.3f %.3f %.3f %.3f %.3f %.3f\n", v1[33], v1[34], v1[35], v2[33], v2[34], v2[35]);
							}
							if( debug_asx )
								printf("dx %.3f %.3f %.3f %.3f\n", v1[3], v2[3], v1[35], v2[35]);

							fill_stat(vCorr, vCorrTar, corrUniv, corrErr, corrTot, v1, v2);

							vector<float>().swap(v1);
							vector<float>().swap(v2);
							it3->second.erase(it4);
						}
					}
				}
				time_prev = time;
			}
			mCorrUniv[uid] = corrUniv;
			mCorrErr[uid] = corrErr;
			mCorrTot[uid] = corrTot;
		}
	}
}

void fill_stat(vector<Corr>& vCorr, vector<Corr>& vCorrTar, Corr& corrUniv, Corr& corrErr, Corr& corrTot, vector<float>& v1, vector<float>& v2)
{
	int n = vCorr.size();
	for( int i = 0; i < n; ++i )
		vCorr[i].add(v1[i], v2[i]);

	if( type == "om" )
	{
		int N = v1.size();
		if( sample == "reg" )
		{
			vCorrTar[0].add(v1[N - 1], v1[N - 2] + v1[N - 1]);
			vCorrTar[1].add(v1[N - 1], v2[N - 2] + v2[N - 1]);
			vCorrTar[2].add(v2[N - 1], v1[N - 2] + v1[N - 1]);
			vCorrTar[3].add(v2[N - 1], v2[N - 2] + v2[N - 1]);
		}
		else if( sample == "booktrade" )
		{
			vCorrTar[0].add(v1[N - 2], v1[N - 3]);
			vCorrTar[1].add(v1[N - 2], v2[N - 3]);
			vCorrTar[2].add(v2[N - 2], v1[N - 3]);
			vCorrTar[3].add(v2[N - 2], v2[N - 3]);
		}

		if( ticker_corr && input == "bin" )
		{
			if( sample == "reg" )
			{
				corrUniv.add(v1[0], v2[0]);
				corrErr.add(v1[1], v2[1]);
				corrTot.add(v1[0] + v1[1], v2[0] + v2[1]);
			}
			else if( sample == "booktrade" )
			{
				corrUniv.add(v1[N - 2], v1[N - 3]);
				corrErr.add(v1[N - 2], v2[N - 3]);
			}
		}
	}
}

void get_var_list(vector<int>& vVars1, vector<int>& vVars2, string framework1, string framework2)
{
	// Columns to read and compare.
	// tm txt 24 target 25 uhtarget
	// tm bin 22 tar
	// om txt 18 target1 19 bmpred 59 ssECpred
	// om bin 9 restar 10 bmpred 58 target1UH

	int tmtxtinput[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80};
	int tmbininput[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 24, 25, 26, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 47, 50, 51, 52, 53, 54, 55, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87};
	int omtxtinput[] = {2, 3, 4, 5, 6, 7, 8, 9, 40, 20, 21, 22, 42, 16, 17, 23, 24, 25, 26, 41, 28, 29, 30, 43, 44, 47, 48, 50, 51, 32, 33, 34, 35, 36, 37};
	int ombininput[] = {11, 12, 0, 1, 2, 3, 4, 5, 6, 7, 37, 18, 19, 20, 39, 13, 14, 15, 17, 21, 22, 38, 30, 31, 32, 40, 41, 44, 45, 47, 48, 33, 29, 81};
	if( type == "tm" )
	{
		if( input == "txt" )
		{
			vVars1 = vector<int>(tmtxtinput, tmtxtinput + 70);
			vVars1.push_back(24);
			vVars1.push_back(25);
			vVars2 = vector<int>(tmtxtinput, tmtxtinput + 70);
			vVars2.push_back(24);
			vVars2.push_back(25);
		}
		else
		{
			vVars1 = vector<int>(tmbininput, tmbininput + 70);
			vVars1.push_back(22);
			vVars2 = vector<int>(tmbininput, tmbininput + 70);
			vVars2.push_back(22);
		}
	}
	else if( type == "om" )
	{
		if( input == "txt" )
		{
			vVars1 = vector<int>(omtxtinput, omtxtinput + 35);
			vVars1.push_back(18);
			vVars1.push_back(19);
			vVars1.push_back(59);
			vVars2 = vector<int>(omtxtinput, omtxtinput + 35);
			vVars2.push_back(18);
			vVars2.push_back(61);
			vVars2.push_back(59);
		}
		else
		{
			vVars1 = vector<int>(ombininput, ombininput + 34);
			if( framework1 == "hfs" )
			{
				vVars1.push_back(9);
				vVars1.push_back(10);
			}
			else
			{
				vVars1.push_back(69);
				vVars1.push_back(70);
			}
			vVars2 = vector<int>(ombininput, ombininput + 34);
			if( framework2 == "hfs" )
			{
				vVars2.push_back(9);
				vVars2.push_back(10);
			}
			else
			{
				vVars2.push_back(69);
				vVars2.push_back(70);
			}
		}
	}
}

void check_diff(string uid, int time, vector<float>& v1, vector<float>& v2, vector<string>& varNameSorted)
{
	int NV = v1.size();
	if( v2.size() < NV )
		NV = v2.size();

	if( max_debug_diff == 0 || max_debug_diff_tarpred == 0 || max_debug_diff_mret == 0
		|| n_debug_diff < max_debug_diff || n_debug_diff_tarpred < max_debug_diff_tarpred || n_debug_diff_mret < max_debug_diff_mret)
	{
		for( int i = 0; i < NV; ++i )
		{
			if( diff_much(i, v1[i], v2[i], varNameSorted) )
			{
				bool do_print = false;
				if( (varNameSorted[i].find("tar") != string::npos || varNameSorted[i].find("pred") != string::npos) )
				{
					if(max_debug_diff_tarpred == 0 || n_debug_diff_tarpred++ < max_debug_diff_tarpred)
						do_print = true;
				}
				else if( varNameSorted[i].find("mret") != string::npos )
				{
					if(max_debug_diff_mret == 0 || n_debug_diff_mret++ < max_debug_diff_mret)
						do_print = true;
				}
				else
				{
					if(max_debug_diff == 0 || n_debug_diff++ < max_debug_diff)
						do_print = true;
				}

				if( do_print )
					printf("%10s %5d %15s %.5f %.5f\n", uid.c_str(), time, varNameSorted[i].c_str(), v1[i], v2[i]);

				if(max_debug_diff_tarpred > 0 && n_debug_diff_tarpred > max_debug_diff_tarpred
					&& max_debug_diff_mret > 0 && n_debug_diff_mret > max_debug_diff_mret
					&& max_debug_diff > 0 && n_debug_diff > max_debug_diff)
					break;
			}
		}
	}

	if( debug_bmpred )
	{
		int N = v1.size();
		if( abs(v1[N - 1] - v2[N - 1]) > 4. )
		{
			printf("bmpred %10s %.4f %.4f %.4f %.4f\n", uid.c_str(), v1[N - 1], v2[N - 1], v1[N - 2] + v1[N - 1], v2[N - 2] + v2[N - 1]);
		}
	}
}

bool diff_much(int i, float v1, float v2, vector<string>& varNameSorted)
{
	double thres = 1e-4;
	if( varNameSorted[i].find("ret") != string::npos || (varNameSorted[i][0] == 'm' && varNameSorted[i].find("00") != string::npos) )
		thres = 0.005;
	else if( varNameSorted[i].find("tar") != string::npos )
		thres = 0.005;
	else if( varNameSorted[i].find("TOB") != string::npos || varNameSorted[i].find("BsRat") == 0 || varNameSorted[i].find("Boff") == 0 )
		thres = 0.001;
	bool eitherNonZero = fabs(v1) > thres || fabs(v2) > thres;
	if( eitherNonZero )
	{
		bool diffMuch = false;
		if( fabs(v1) > fabs(v2) )
			diffMuch = fabs( (v1 - v2) / v1 ) > 0.1;
		else
			diffMuch = fabs( (v1 - v2) / v2 ) > 0.1;
		return diffMuch;
	}

	return false;
}

string bin_path(string dir, int idate)
{
	char path[200];
	sprintf(path, "%s\\sig%d%s%s.bin", dir.c_str(), idate, mCode.c_str(), type.c_str());
	return xpf(path);
}

string txt_path(string dir, int idate)
{
	char path[200];
	if( "bin" == input )
		sprintf(path, "%s\\sig%d%s%s.txt", dir.c_str(), idate, mCode.c_str(), type.c_str());
	else if( "txt" == input )
		sprintf(path, "%s\\sigTB%d%s%s.txt", dir.c_str(), idate, mCode.c_str(), type.c_str());
	return xpf(path);
}

void cnt_data(map<string, int>& mUidCnt, string dir, int idate)
{
	string tpath = txt_path(dir, idate);
	ifstream ifsTxt(tpath.c_str());

	string line;
	getline(ifsTxt, line); // header

	int indx = ("txt" == input) ? 1 : 0;
	string last_uid;
	int cnt = 0;
	while( getline(ifsTxt, line) )
	{
		vector<string> sl = split(line, '\t');
		string uid = trim(sl[indx]);
		if( uid != last_uid && cnt > 0 )
		{
			mUidCnt[last_uid] = cnt;
			cnt = 0;
		}
		last_uid = uid;
		++cnt;
	}
	if( cnt > 0 )
		mUidCnt[last_uid] = cnt;
}

void comp_cnt(map<string, int>& mUidCnt1, map<string, int>& mUidCnt2)
{
	printf("1 %d tickers\n", mUidCnt1.size());
	printf("2 %d tickers\n", mUidCnt2.size());

	set<string> uid1only;
	set<string> uid2only;
	for( map<string, int>::iterator it = mUidCnt1.begin(); it != mUidCnt1.end(); ++it )
	{
		if( !mUidCnt2.count(it->first) )
			uid1only.insert(it->first);
	}
	for( map<string, int>::iterator it = mUidCnt2.begin(); it != mUidCnt2.end(); ++it )
	{
		if( !mUidCnt1.count(it->first) )
			uid2only.insert(it->first);
	}

	if( !uid1only.empty() )
		for( set<string>::iterator it = uid1only.begin(); it != uid1only.end(); ++it )
			printf("1 only %s\n", (*it).c_str());
	if( !uid2only.empty() )
		for( set<string>::iterator it = uid2only.begin(); it != uid2only.end(); ++it )
			printf("2 only %s\n", (*it).c_str());

	if( max_debug_cnt == 0 || n_debug_cnt < max_debug_cnt )
	{
		for( map<string, int>::iterator it = mUidCnt1.begin(); it != mUidCnt1.end(); ++it )
		{
			map<string, int>::iterator it2 = mUidCnt2.find(it->first);
			if( it2 != mUidCnt2.end() )
				if( it->second != it2->second )
				{
					if( max_debug_cnt == 0 || n_debug_cnt < max_debug_cnt )
					{
						printf("%s %d %d\n", it->first.c_str(), it->second, it2->second);
						++n_debug_cnt;
					}
				}
		}
	}
	cout.flush();
}

void get_uids(set<string>& uids, map<string, int>& mUidCnt1, map<string, int>& mUidCnt2, int n)
{
	if( !uid.empty() )
	{
		uids.insert(uid);
		return;
	}

	int cnt = 0;
	for( map<string, int>::iterator it = mUidCnt1.begin(); it != mUidCnt1.end() && cnt < n; ++it )
	{
		if( mUidCnt2.count(it->first) )
		{
			uids.insert(it->first);
			++cnt;
		}
	}
}

void get_uids(set<string>& uids, string dir, int idate, int nSymbols)
{
	if( !uid.empty() )
	{
		uids.insert(uid);
		return;
	}

	string tpath = txt_path(dir, idate);
	ifstream ifsTxt(tpath.c_str());

	string line;
	getline(ifsTxt, line); // header

	int indx = ("txt" == input) ? 1 : 0;
	string last_uid;
	int cnt = 0;
	while( getline(ifsTxt, line) )
	{
		vector<string> sl = split(line, '\t');
		string uid = trim(sl[indx]);
		if( uid != last_uid )
		{
			uids.insert(uid);
			if( nSymbols > 0 && ++cnt >= nSymbols )
				break;
		}
		last_uid = uid;
	}
}

void get_data(set<string>& uids, map<string, map<int, vector<float> > >& mUidTimeSig, int& NV, vector<string>& varName, string dir, int idate)
{
	if( "bin" == input )
		get_data_bin(uids, mUidTimeSig, NV, varName, dir, idate);
	else if( "txt" == input )
		get_data_txt(uids, mUidTimeSig, NV, varName, dir, idate);
}

void get_data_bin(set<string>& uids, map<string, map<int, vector<float> > >& mUidTimeSig, int& NV, vector<string>& varName, string dir, int idate)
{
	// Header.
	string bpath = bin_path(dir, idate);
	ifstream ifs(bpath.c_str(), ios::binary);

	string tpath = txt_path(dir, idate);
	ifstream ifsTxt(tpath.c_str());

	if( ifs.is_open() )
	{
		int ncols, nrows;
		ifs.read( (char*)&nrows, sizeof(int) );
		ifs.read( (char*)&ncols, sizeof(int) );
		cout << bpath << " has " << nrows << " rows.\n";

		long long int len;
		ifs.read( (char*)&len, sizeof(len) );

		if( len > 0 )
		{
			char labels[2000];
			ifs.read(labels, len);

			NV = ncols;
			varName = split(labels, ',');

			// txt
			string line;
			getline(ifsTxt, line);
		}
		else
			exit(3);
	}
	else
	{
		cerr << "Not open: " << bpath << "\t" << tpath << endl;
		exit(3);
	}

	// Content.
	vector<float> v(NV);
	string line;
	map<int, vector<float> >* pTimeSig = 0;
	string uid_prev;
	int cntSymbol = 0;
	while( getline(ifsTxt, line) )
	{
		vector<string> sl = split(line, '\t');

		int timeIdx = (sl.size() > 2) ? 2 : 1;
		//if( dir.find("\\hfs\\") != string::npos && sample == "booktrade" )
		//	timeIdx = 3;

		string uid = trim(sl[0]);
		int time = ceil(atof(sl[timeIdx].c_str()) * 1000. - 0.5);
		if( uid != "uid" && time > 0 )
		{
			ifs.read( (char*)&v[0], NV * sizeof(float) );
			if( nSymbols > 0 && cntSymbol >= nSymbols && !uids.count(uid) )
				break;

			if( uids.count(uid) && time > min_msecs )
			{
				if( uid_prev != uid )
				{
					++cntSymbol;

					mUidTimeSig[uid];
					pTimeSig = &(mUidTimeSig[uid]);
					uid_prev = uid;
				}
				(*pTimeSig)[time] = v;
			}
		}
	}
}

void get_data_txt(set<string>& uids, map<string, map<int, vector<float> > >& mUidTimeSig, int& NV, vector<string>& varName, string dir, int idate)
{
	// Header.
	string tpath = txt_path(dir, idate);
	ifstream ifsTxt(tpath.c_str());

	if( ifsTxt.is_open() )
	{
		string line;
		getline(ifsTxt, line);
		vector<string> sl = split(line, '\t');
		NV = sl.size();
		varName = sl;
	}
	else
		exit(3);

	// Content.
	vector<float> v(NV);
	string line;
	map<int, vector<float> >* pTimeSig = 0;
	string uid_prev;
	int cntSymbol = 0;
	while( getline(ifsTxt, line) )
	{
		vector<string> sl = split(line, '\t');
		string uid = trim(sl[0]);
		int time = ceil(atof(sl[2].c_str()) * 1000. - 0.5);
		if( uid != "uid" && time > 0 )
		{
			if( nSymbols > 0 && cntSymbol >= nSymbols && !uids.count(uid) )
				break;

			if( uids.count(uid) && time > min_msecs )
			{
				if( uid_prev != uid )
				{
					if( ++cntSymbol > nSymbols )
						break;

					mUidTimeSig[uid];
					pTimeSig = &(mUidTimeSig[uid]);
					uid_prev = uid;
				}
				for( int i = 0; i < NV; ++i )
					v[i] = atof(sl[i].c_str());
				(*pTimeSig)[time] = v;
			}
		}
	}
}

void comp_name(vector<string>& varName1, vector<string>& varName2)
{
	int N = varName1.size();
	if( varName2.size() < N )
		N = varName2.size();

	for( int i = 0; i < N; ++i )
	{
		if( varName1[i] != varName2[i] )
			printf("varName %d %s %s\n", i, varName1[i].c_str(), varName2[i].c_str());
	}
}

bool excluded(string uid)
{
	vector<string> exlist;
	exlist.push_back("20441W10");
	exlist.push_back("01475210");
	exlist.push_back("21987A20");
	exlist.push_back("CPK");
	exlist.push_back("CSS");
	exlist.push_back("23703Q10");
	exlist.push_back("33731L10");
	exlist.push_back("33731K10");
	exlist.push_back("33738C10");
	exlist.push_back("GBL");
	exlist.push_back("36268W10");
	exlist.push_back("GMK");
	exlist.push_back("HUVL");
	exlist.push_back("JMP");
	exlist.push_back("LUB");
	exlist.push_back("LVB");
	exlist.push_back("MPV");
	exlist.push_back("PANR.A");
	exlist.push_back("PKE");
	exlist.push_back("PRO");
	exlist.push_back("76128310");
	exlist.push_back("SGK");
	exlist.push_back("STN.TO");
	exlist.push_back("THO.J");
	exlist.push_back("88688510");
	exlist.push_back("87261Q10");
	exlist.push_back("TRC");
	exlist.push_back("TRK");
	exlist.push_back("TRR");
	exlist.push_back("WHG");

	exlist.push_back("AXR");
	exlist.push_back("44248W20");
	exlist.push_back("46489B10");
	exlist.push_back("MHF");
	exlist.push_back("72200T10");
	exlist.push_back("SJW");
	exlist.push_back("94874184");

	sort(exlist.begin(), exlist.end());
	if( binary_search(exlist.begin(), exlist.end(), uid) )
		return true;
	return false;
}

/* om txt
0 uid
1 ticker
2 time
3 sprd
4 logVolu
5 logPrice
6 qI
7 mret60
8 TOBoff1
9 mret300
10 mret300L
11 TOBqI2
12 TOBqI3
13 TOBoff2
14 BqRat1
15 BqRat2
16 BsRat1
17 BsRat2
18 target1
19 bmpred
20 exchange
21 medVolat
22 hilo
23 BqI2
24 Boff1
25 Boff2
26 BOffmedVol.01
27 BmedSprdqI.5
28 BmedSprdqI1
29 BmedSprdqI2
30 BmedSprdqI4
31 fillImb
32 spxPspx
33 spfPspx
34 lcPlc
35 spfPlc
36 rusfPspx
37 rusfPlc
38 logDolVolu
39 logDolPrice
40 mret5
41 mret15
42 mret30
43 mret120
44 mret600
45 hiloQAI
46 hiloLag1Open
47 hiloLag1Rat
48 hiloLag1
49 hilo900
50 bollinger300
51 bollinger900
52 dmret5
53 dmret15
54 dmret30
55 dmret60
56 dmret120
57 dmret300
58 dmret600
59 ssECpred
60 restar60;0
61 bmpred60;0
62 indxPred60;0
63 indxSig60;0
*/

/* om bin
0       time
1       sprd
2       logVolu
3       logPrice
4       qI
5       mret60
6       TOBoff1
7       mret300
8       mret300L
9       restar
10      bmpred
11      minTick
12      arDown
13      BsRat1
14      BsRat2
15      BqI2
16      arOscil
17      Boff1
18      exchange
19      medVolat
20      hilo
21      Boff2
22      BOffmedVol.01
23      BOffmedVol.02
24      BOffmedVol.04
25      BOffmedVol.08
26      BOffmedVol.16
27      BOffmedVol.32
28      BmedSprdqI.25
29      BmedSprdqI.5
30      BmedSprdqI1
31      BmedSprdqI2
32      BmedSprdqI4
33      spxPspx
34      spfPlc
35      logDolVolu
36      logDolPrice
37      mret5
38      mret15
39      mret30
40      mret120
41      mret600
42      hiloQAI
43      hiloLag1Open
44      hiloLag1Rat
45      hiloLag1
46      hilo900
47      bollinger300
48      bollinger900
49      dmret5
50      dmret15
51      lcPlc
52      spfPlc
53      rusfPspx
54      rusfPlc
55      dmret600
56      BOffmedVol.0025
57      BOffmedVol.005
58      target1UH
59      TOBqI2
60      TOBqI3
61      TOBoff2
62      dark1
63      dark5
64      dark20
65      mI15
66      mI30
67      mI60
68      mI120
69      restar50;0
70      restar60;0
71      restar120;0
72      restar240;0
73      restar360;0
74      restar540;0
75      restar720;0
76      restar900;0
77      restar1080;0
78      restar1320;0
79      restar1680;0
80      bmpred50;0
81      bmpred60;0
82      bmpred120;0
83      bmpred240;0
84      bmpred360;0
85      bmpred540;0
86      bmpred720;0
87      bmpred900;0
88      bmpred1080;0
89      bmpred1320;0
90      bmpred1680;0
91      indx50;0
92      indx60;0
93      indx120;0
94      indx240;0
95      indx360;0
96      indx540;0
97      indx720;0
98      indx900;0
99      indx1080;0
100     indx1320;0
101     indx1680;0
*/

/* tm txt
0 uid
1 ticker
2 time
3 logVolu
4 logPrice
5 logMedSprd
6 mret300
7 mret300L
8 mret600L
9 mret1200L
10 mret2400L
11 mret4800L
12 mretONLag0
13 mretIntraLag1
14 vm600
15 vm3600
16 qIWt
17 qIMax
18 hilo
19 TOBqI2
20 TOBqI3
21 TOBoff1
22 TOBoff2
23 sprd
24 target
25 uhtarget
26 exchange
27 northRST
28 northTRD
29 northBP
30 hiloQAI
31 medVolat
32 prevDayVolu
33 mretOpen
34 qIMax2
35 mretONLag1
36 mretIntraLag2
37 vmIntra
38 isSecTypeF
39 sprdOpen
40 BOffmedVol.01
41 BOffmedVol.02
42 BOffmedVol.04
43 BOffmedVol.08
44 BOffmedVol.16
45 BOffmedVol.32
46 BqRat1
47 BqRat2
48 BsRat1
49 BsRat2
50 BqI2
51 BqI3
52 Boff1
53 Boff2
54 BmedSprdqI.25
55 BmedSprdqI.5
56 BmedSprdqI1
57 BmedSprdqI2
58 BmedSprdqI4
59 BmedSprdqI8
60 BmedSprdqI16
61 mI600
62 mI3600
63 mIIntra
64 qI
65 fillImb
66 mret60
67 target41
68 linpred1
69 logDolVolu
70 logDolPrice
71 hiloLag1Open
72 hiloLag2Open
73 hiloLag1Rat
74 hiloLag2Rat
75 hiloQAI2
76 hiloLag1
77 hiloLag2
78 hilo900
79 bollinger300
80 bollinger900
81 dmret300
82 dmret600
83 BOffmedVol.0025
84 BOffmedVol.005
85 minTick
86 target1
87 mocSig
88 BmedSprdqIWt1
89 BmedSprdqIWt2
90 BmedSprdqIWt4
91 BmedSprdqIWt8
92 BmedSprdqIWt16
93 bollinger1800
94 bollinger3600
95 tar5
96 tar10
97 tar10UH
*/

/* tm bin
0       time
1       logVolu
2       logPrice
3       logMedSprd
4       mret300
5       mret300L
6       mret600L
7       mret1200L
8       m2400L
9       m4800L
10      mretONLag0
11      mretIntraLag1
12      vm600
13      vm3600
14      qIWt
15      qIMax
16      hilo
17      TOBqI2
18      TOBqI3
19      TOBoff1
20      TOBoff2
21      sprd
22      tar
23      bmpred
24      mI600
25      mI3600
26      mIIntra
27      tar5
28      tar10
29      tar60
30      BqRat1
31      BqRat2
32      BsRat1
33      BsRat2
34      exchange
35      northRST
36      northTRD
37      northBP
38      hiloQAI
39      medVolat
40      prevDayVolu
41      mretOpen
42      qIMax2
43      mretONLag1
44      mretIntraLag2
45      vmIntra
46      ompred
47      isSecTypeF
48      intraTar
49      tar1060Intra
50      sprdOpen
51      BqI2
52      BqI3
53      Boff1
54      Boff2
55      BOffmedVol.01
56      fillImb
57      mret60
58      BOffmedVol.0025
59      BOffmedVol.005
60      tar1060Intra2
61      BOffmedVol.02
62      BOffmedVol.04
63      BOffmedVol.08
64      BOffmedVol.16
65      BOffmedVol.32
66      BmedSprdqI.25
67      BmedSprdqI.5
68      BmedSprdqI1
69      BmedSprdqI2
70      BmedSprdqI4
71      BmedSprdqI8
72      BmedSprdqI16
73      omlinpred
74      totaltar
75      minTick
76      logDolVolu
77      logDolPrice
78      hiloLag1Open
79      hiloLag2Open
80      hiloLag1Rat
81      hiloLag2Rat
82      hiloQAI2
83      hiloLag1
84      hiloLag2
85      hilo900
86      bollinger300
87      bollinger900
88      dmret300
89      dmret600
90      tarClose
91      tarON
92      newsIdx
93      news6
94      news7
95      BmedSprdqIWt1
96      BmedSprdqIWt2
97      BmedSprdqIWt4
98      BmedSprdqIWt8
99      BmedSprdqIWt16
100     dark1
101     dark5
102     dark20
103     vwap900
104     vwap60
105     timeFrac
106     g11
107     tar600;50
108     tar600;60
109     tar600;120
110     tar600;240
111     tar600;360
112     tar600;540
113     tar600;720
114     tar600;900
115     tar600;1080
116     tar600;1320
117     tar600;1680
118     tar3610;650
119     tar3600;660
120     tar3540;720
121     tar3420;840
122     tar3300;960
123     tar3120;1140
124     tar2940;1320
125     tar2760;1500
126     tar2580;1680
127     tar2340;1920
128     tar1980;2280
129     tar600;50uh
130     tar600;60uh
131     tar600;120uh
132     tar600;240uh
133     tar600;360uh
134     tar600;540uh
135     tar600;720uh
136     tar600;900uh
137     tar600;1080uh
138     tar600;1320uh
139     tar600;1680uh
140     tar3610;650uh
141     tar3600;660uh
142     tar3540;720uh
143     tar3420;840uh
144     tar3300;960uh
145     tar3120;1140uh
146     tar2940;1320uh
147     tar2760;1500uh
148     tar2580;1680uh
149     tar2340;1920uh
150     tar1980;2280uh
*/
