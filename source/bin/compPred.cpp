#include <map>
#include <string>
#include <vector>
#include <jl_lib.h>
using namespace std;

// Compare two pred files.

vector<string> varName;
string get_hffit_pred_dir(string model, string machine, string disk, string udate);
string get_hfs_pred_dir(string model, string machine, string disk);
string get_hffit_sig_dir(string model, string machine, string disk);
string get_hfs_sig_dir(string model, string machine, string disk);
string pred_path(string pdir, int idate);
string sig_path(string sdir, int idate);
void comp(int idate, Corr& corrPreds, vector<Corr>& vCorrTar, string pdir1, string sdir1, string pdir2, string sdir2);
void get_data(map<string, map<int, pair<float, float> > >& mUidTimeSig, string pdir, string sdir, int idate);

const int min_time = 0;
string type = "tm"; // om or tm.
string mCode = "S";
string target = "restar60;0"; // restar, restar60;0, restar720;0, etc.

int main(int argc, char* argv[])
{
	string framework1 = "hffit";
	string framework2 = "hfs";
	string model1 = "AH";
	string model2 = "hk";
	string machine1 = "40";
	string machine2 = "45";
	string drive1 = "l";
	string drive2 = "fdrive";
	string udate1, udate2;
	vector<int> idates;

	if( 0 )
	{
		Arg arg(argc, argv);

		if( arg.isGiven("s1") )
		{
			vector<string> sig = arg.getVals("s1");
			framework1 = sig[0];
			model1 = sig[1];
			machine1 = sig[2];
			drive1 = sig[3];
			udate1 = sig[4];
		}

		if( arg.isGiven("s2") )
		{
			vector<string> sig = arg.getVals("s1");
			framework2 = sig[0];
			model2 = sig[1];
			machine2 = sig[2];
			drive2 = sig[3];
			udate2 = sig[4];
		}

		if( arg.isGiven("g") )
			target = arg.getVal("g");

		if( arg.isGiven("m") )
			mCode = arg.getVal("m");

		if( arg.isGiven("t") )
			type = arg.getVal("t");

		if( arg.isGiven("d") )
		{
			vector<string> dates = arg.getVals("d");
			for( vector<string>::iterator it = dates.begin(); it != dates.end(); ++it )
				idates.push_back(atoi((*it).c_str()));
		}
	}
	else if( argc >= 15 )
	{
		framework1 = argv[1];
		model1 = argv[2];
		machine1 = argv[3];
		drive1 = argv[4];
		udate1 = argv[5];

		framework2 = argv[6];
		model2 = argv[7];
		machine2 = argv[8];
		drive2 = argv[9];
		udate2 = argv[10];

		target = argv[11];
		mCode = argv[12];
		type = argv[13];
		for( int i = 14; i < argc; ++i )
		{
			if( (string)argv[i] == ">" )
				break;
			idates.push_back(atoi(argv[i]));
		}
	}
	else
	{
		cout << "> compPred.exe hffit AH 48 e 20120523 hfs hk 45 fdrive 20120921 restar60;0 H tm 20120505 20120506 20120507\n";
		exit(1);
	}

	string pdir1 = (framework1 == "hffit") ? get_hffit_pred_dir(model1, machine1, drive1, udate1) : get_hfs_pred_dir(model1, machine1, drive1);
	string sdir1 = (framework1 == "hffit") ? get_hffit_sig_dir(model1, machine1, drive1) : get_hfs_sig_dir(model1, machine1, drive1);
	string pdir2 = (framework2 == "hffit") ? get_hffit_pred_dir(model2, machine2, drive2, udate2) : get_hfs_pred_dir(model2, machine2, drive2);
	string sdir2 = (framework2 == "hffit") ? get_hffit_sig_dir(model2, machine2, drive2) : get_hfs_sig_dir(model2, machine2, drive2);

	Corr corrPreds;
	vector<Corr> vCorrTar(4);
	char buf[200];
 
	// Do compare.
	for( vector<int>::iterator it = idates.begin(); it != idates.end(); ++it )
	{
		int idate = *it;
		cout << idate << endl;

		comp(idate, corrPreds, vCorrTar, pdir1, sdir1, pdir2, sdir2);

		sprintf(buf, "ndata %.0f pred1-pred2 %.4f pred1-tar %.4f pred2-tar %.4f",
				vCorrTar[0].accX.n, corrPreds.corr(), vCorrTar[0].corr(), vCorrTar[3].corr());
		cout << buf << endl;
	}

	for( int i = 0; i < 4; ++i )
	{
		sprintf(buf, "%.4f\n", vCorrTar[i].corr());
		cout << buf;
	}
	cout << endl;
}

string get_hffit_pred_dir(string model, string machine, string disk, string udate)
{
	char dir[1000];
	if( machine == "mnt" )
		sprintf(dir, "/home/jelee/%s/%s/fit/%s/stat_%s/preds", disk.c_str(), model.c_str(), target.c_str(), udate.c_str());
	else
		sprintf(dir, "\\\\smrc-ltc-mrct%s\\%s\\hffit\\%s\\fit\\%s\\stat_%s\\preds", machine.c_str(), disk.c_str(), model.c_str(), target.c_str(), udate.c_str());
	return dir;
}

string get_hfs_pred_dir(string model, string machine, string disk)
{
	if( model == "eu" )
		model = "lseEn";

	string subdir = (type == "om") ? "29sigsB" : "new70L";
	char dir[1000];
	sprintf(dir, "\\\\smrc-ltc-mrct%s\\%s\\hfs\\resStatDir\\%s\\%s\\%s\\preds", machine.c_str(), disk.c_str(), type.c_str(), model.c_str(), subdir.c_str());
	return dir;
}

string get_hffit_sig_dir(string model, string machine, string disk)
{
	string subdir = "binSig";
	char dir[1000];
	if( machine == "mnt" )
		sprintf(dir, "/home/jelee/%s/%s/%s/%s", disk.c_str(), model.c_str(), subdir.c_str(), type.c_str());
	else
		sprintf(dir, "\\\\smrc-ltc-mrct%s\\%s\\hffit\\%s\\%s\\%s", machine.c_str(), disk.c_str(), model.c_str(), subdir.c_str(), type.c_str());
	return dir;
}

string get_hfs_sig_dir(string model, string machine, string disk)
{
	if( model == "eu" )
		model = "lseEuEcn";

	string subdir = "resbinsigs";
	char dir[1000];
	sprintf(dir, "\\\\smrc-ltc-mrct%s\\%s\\hfs\\%s\\%s\\%s", machine.c_str(), disk.c_str(), subdir.c_str(), type.c_str(), model.c_str());
	return dir;
}

void comp(int idate, Corr& corrPreds, vector<Corr>& vCorrTar, string pdir1, string sdir1, string pdir2, string sdir2)
{
	map<string, map<int, pair<float, float> > > mUidTimeSig1;
	map<string, map<int, pair<float, float> > > mUidTimeSig2;
	get_data(mUidTimeSig1, pdir1, sdir1, idate);
	get_data(mUidTimeSig2, pdir2, sdir2, idate);

	for( map<string, map<int, pair<float, float> > >::iterator it1 = mUidTimeSig1.begin(); it1 != mUidTimeSig1.end(); ++it1 )
	{
		string uid = it1->first;
		for( map<int, pair<float, float> >::iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2 )
		{
			int time = it2->first;

			map<string, map<int, pair<float, float> > >::iterator it3 = mUidTimeSig2.find(uid);
			if( it3 != mUidTimeSig2.end() )
			{
				map<int, pair<float, float> >::iterator it4 = it3->second.find(time);
				if( it4 != it3->second.end() )
				{
					double pred1 = it2->second.first;
					double target1 = it2->second.second;
					double pred2 = it4->second.first;
					double target2 = it4->second.second;

					corrPreds.add(pred1, pred2);
					vCorrTar[0].add(pred1, target1);
					vCorrTar[1].add(pred1, target2);
					vCorrTar[2].add(pred2, target1);
					vCorrTar[3].add(pred2, target2);

					it3->second.erase(it4);
				}
			}
		}
	}
}

string pred_path(string dir, int idate)
{
	char path[200];
	if( dir.find("hffit") != string::npos )
		sprintf(path, "%s\\pred%d.txt", dir.c_str(), idate);
	else if( dir.find("hfs") != string::npos )
	{
		if( dir.find("lseEn") != string::npos || dir.find("\\us\\") != string::npos
			|| dir.find("\\br\\") != string::npos || dir.find("\\tsx\\") != string::npos
			|| dir.find("\\uscef\\") != string::npos )
		{
			int nTree = 6;
			if( type == "tm" )
			{
				nTree = 13;
				if( dir.find("\\us\\") != string::npos )
					nTree = 20;
				else if( dir.find("\\br\\") != string::npos )
					nTree = 5;
				else if( dir.find("\\tsx\\") != string::npos || dir.find("\\uscef\\") != string::npos )
					nTree = 9;
			}

			sprintf(path, "%s\\%dpred%d%s.txt", dir.c_str(), nTree, idate, mCode.c_str());
		}
		else
			sprintf(path, "%s\\pred%d%s%s.txt", dir.c_str(), idate, mCode.c_str(), type.c_str());
	}
	return xpf(path);
}

string sig_path(string dir, int idate)
{
	char path[200];
	sprintf(path, "%s\\sig%d%s%s.txt", dir.c_str(), idate, mCode.c_str(), type.c_str());
	return xpf(path);
}

void get_data(map<string, map<int, pair<float, float> > >& mUidTimeSig, string pdir, string sdir, int idate)
{
	// Header.
	string ppath = pred_path(pdir, idate);
	ifstream ifsPred(ppath.c_str());

	int NV = 0;
	if( ifsPred.is_open() )
	{
		string line;
		getline(ifsPred, line);
		vector<string> sl = split(line, '\t');
		NV = sl.size();
	}
	else
	{
		cout << ppath << " is not open.\n";
		exit(3);
	}

	// Content.
	if( NV == 7 )
	{
		string line;
		while( getline(ifsPred, line) )
		{
			vector<string> sl = split(line, '\t');
			string uid = trim(sl[0]);
			int time = atoi(sl[2].c_str());
			if( uid != "uid" && time > min_time )
			{
				float pred = atof(sl[5].c_str()) + atof(sl[6].c_str());
				float target = atof(sl[4].c_str());
				mUidTimeSig[uid].insert( make_pair(time, make_pair(pred, target)) );
			}
		}
	}
	else if( NV == 4 )
	{
		string spath = sig_path(sdir, idate);
		ifstream ifsSig(spath.c_str());

		if( ifsSig.is_open() )
		{
			string line;
			getline(ifsSig, line);
		}

		string linePred, lineSig;
		while( getline(ifsPred, linePred) && getline(ifsSig, lineSig) )
		{
			vector<string> ssl = split(lineSig, '\t');
			string uid = trim(ssl[0]);
			int time = atoi(ssl[2].c_str());
			if( uid != "uid" && time > min_time )
			{
				vector<string> psl = split(linePred, '\t');
				float pred = atof(psl[2].c_str());
				float target = atof(psl[0].c_str());
				mUidTimeSig[uid].insert( make_pair(time, make_pair(pred, target)) );
			}
		}
	}
}
