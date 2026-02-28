#include <MSignal/MMergePred.h>
#include <MFitting.h>
#include <MFramework.h>
#include <gtlib_model/pathFtns.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MMergePred::MMergePred(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	debug_(false),
	fitDescBase_("reg"),
	fitDescAdd_("reg"),
	sigType_(MEnv::Instance()->sigType)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("modelBase") )
		modelBase_ = trim(conf.find("modelBase")->second);
	if( conf.count("modelAdd") )
		modelAdd_ = trim(conf.find("modelAdd")->second);
	if( conf.count("fitDesc") )
	{
		fitDescBase_ = conf.find("fitDesc")->second;
		fitDescAdd_ = conf.find("fitDesc")->second;
	}
	if( conf.count("fitDescBase") )
		fitDescBase_ = conf.find("fitDescBase")->second;
	if( conf.count("fitDescAdd") )
		fitDescAdd_ = conf.find("fitDescAdd")->second;
	if( conf.count("targetName") )
		targetName_ = conf.find("targetName")->second;
	//if( conf.count("sigType") )
	//sigType_ = conf.find("sigType")->second;
}

MMergePred::~MMergePred()
{}

void MMergePred::beginJob()
{
}

void MMergePred::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;

	map<string, map<int, string> > mBase;
	map<string, map<int, string> > mAdd;
	map<string, map<int, string> > mTxtBase;
	map<string, map<int, string> > mTxtAdd;

	read_data(idate, modelBase_, fitDescBase_, mBase, mTxtBase);
	read_data(idate, modelAdd_, fitDescAdd_, mAdd, mTxtAdd);
	write_data(idate, model, fitDescBase_, mBase, mAdd, mTxtBase, mTxtAdd);
}

void MMergePred::endDay()
{
}

void MMergePred::endJob()
{
}

void MMergePred::read_data(int idate, const string& model, const string& fitdesc,
		map<string, map<int, string> >& m, map<string, map<int, string> >& mTxt)
{
	string path = get_pred_path(MEnv::Instance()->baseDir, model, idate, targetName_, fitdesc);
	ifstream ifs(path.c_str());

	string pathTxt = get_sigTxt_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitdesc);
	ifstream ifsTxt(pathTxt.c_str());

	int cnt = 0;
	if( ifs.is_open() && ifsTxt.is_open() )
	{
		cout << "Reading " << path << endl;

		string lineTxt;
		string line;
		getline(ifsTxt, lineTxt); // Read header.
		getline(ifs, line);
		//while( ifsTxt >> lineTxt && ifs >> line )
		while( getline(ifsTxt, lineTxt) && getline(ifs, line) )
		{
			vector<string> sl = split(lineTxt, '\t');
			string uid = trim(sl[0]);
			string ticker = trim(sl[1]);
			int time = ceil(atof(sl[2].c_str()) * 1000. - 0.5);

			m[uid][time] = line;
			mTxt[uid][time] = lineTxt;
			if( debug_ && ++cnt > 1000 )
				break;
		}
	}
	else
		cout << path << " is not open." << endl;
}

void MMergePred::write_data(int idate, const string& model, const string& fitdesc,
		map<string, map<int, string> >& mBase, map<string, map<int, string> >& mAdd,
		map<string, map<int, string> >& mTxtBase, map<string, map<int, string> >& mTxtAdd)
{
	string path = get_pred_path(MEnv::Instance()->baseDir, model, idate, targetName_, fitdesc);
	string pathTxt = get_sigTxt_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitdesc);

	ofstream ofs( path.c_str(), ios::out);
	ofstream ofsTxt( pathTxt.c_str(), ios::out );

	if( ofs.is_open() && ofsTxt.is_open() )
	{
		// Header for txt file.
		ofsTxt.precision(6);
		ofsTxt << "uid\tticker\ttime\n";

		// Header for pred file.
		ofs.precision(6);
		ofs << "target\tbmpred\ttbpred\tsprd\n";

		// Contents.
		for( map<string, map<int, string> >::iterator itu = mBase.begin(); itu != mBase.end(); ++itu )
		{
			string uid = itu->first;
			map<string, map<int, string> >::iterator ituTxt = mTxtBase.find(uid);

			if( mAdd.count(uid) ) // ticker found in both sources.
			{
				map<string, map<int, string> >::iterator ituAdd = mAdd.find(uid);
				map<string, map<int, string> >::iterator ituAddTxt = mTxtAdd.find(uid);

				for( map<int, string>::iterator itt = ituTxt->second.begin(); itt != ituTxt->second.end(); ++itt )
				{
				}

				map<int, string>::iterator itt = itu->second.begin();
				map<int, string>::iterator ittTxt = ituTxt->second.begin();

				map<int, string>::iterator ittAdd = ituAdd->second.begin();
				map<int, string>::iterator ittAddTxt = ituAddTxt->second.begin();

				while(1)
				{
					if( ittAdd == ituAdd->second.end() && itt != itu->second.end() )
					{
						write_all(ofs, ofsTxt, itt, ittTxt, itu->second.end());
						break;
					}
					else if( itt == itu->second.end() && ittAdd != ituAdd->second.end() )
					{
						write_all(ofs, ofsTxt, ittAdd, ittAddTxt, ituAdd->second.end());
						break;
					}
					else if( itt == itu->second.end() && ittAdd == ituAdd->second.end() )
					{
						break;
					}
					else if( itt->first <= ittAdd->first ) // time of itt is smaller.
					{
						write_one(ofs, ofsTxt, itt, ittTxt);
						++itt;
						++ittTxt;
					}
					else
					{
						write_one(ofs, ofsTxt, ittAdd, ittAddTxt);
						++ittAdd;
						++ittAddTxt;
					}
				}
			}
			else
			{
				write_all(ofs, ofsTxt, itu->second.begin(), ituTxt->second.begin(), itu->second.end());
			}
		}
	}

	//if( ofs.is_open() && ofsTxt.is_open() )
	//{
	//	// Header for txt file.
	//	ofsTxt.precision(6);
	//	if( sigType_ == "om" )
	//		ofsTxt << "uid\tticker\ttime\n";
	//	else if( sigType_ == "tm" )
	//		ofsTxt << "uid\tticker\ttime\tbsize\tbid\task\tasize\n";

	//	// Header for bin file.
	//	// nrows.
	//	int nrows = shBase.nrows;
	//	ofs.write((char*)&(nrows), sizeof(int));

	//	// ncols.
	//	int ncols = shBase.labels.size() + shAdd.labels.size();
	//	ofs.write((char*)&(ncols), sizeof(int));

	//	// labels.
	//	string labels;
	//	for( vector<string>::iterator it = shBase.labels.begin(); it != shBase.labels.end(); ++it )
	//	{
	//		if( it == shBase.labels.begin() )
	//			labels += *it;
	//		else
	//			labels += (string)"," + *it;
	//	}
	//	for( vector<string>::iterator it = shAdd.labels.begin(); it != shAdd.labels.end(); ++it )
	//		labels += (string)"," + *it + "_Add";
	//	long long int labels_len = labels.size() + 1; // 8 bytes on 32 and 64 bit OS for MSDN only; does not include terminating zero so add 1 
	//	ofs.write((char*)&labels_len, sizeof(labels_len));
	//	ofs.write(labels.c_str(), labels_len);

	//	// Contents.
	//	vector<float> vtmp(shAdd.ncols, 0.);
	//	for( map<string, map<int, vector<float> > >::iterator itu = mBase.begin(); itu != mBase.end(); ++itu )
	//	{
	//		string uid = itu->first;
	//		map<string, map<int, vector<float> > >::iterator ituAdd = mAdd.find(uid);
	//		map<string, map<int, string> >::iterator itTxt = mTxt.find(uid);

	//		for( map<int, vector<float> >::iterator itt = itu->second.begin(); itt != itu->second.end(); ++itt )
	//		{
	//			// Base.
	//			vector<float>& v = itt->second;
	//			ofs.write((char*)(&(v[0])), shBase.ncols * sizeof(float));

	//			// Added.
	//			int time = itt->first;
	//			if( ituAdd != mAdd.end() )
	//			{
	//				map<int, vector<float> >::iterator ittAdd = ituAdd->second.find(time);
	//				if( ittAdd != ituAdd->second.end() )
	//				{
	//					vector<float>& vAdd = ittAdd->second;
	//					ofs.write((char*)(&(vAdd[0])), shAdd.ncols * sizeof(float));
	//				}
	//				else
	//					ofs.write((char*)(&(vtmp[0])), shAdd.ncols * sizeof(float));
	//			}
	//			else
	//				ofs.write((char*)(&(vtmp[0])), shAdd.ncols * sizeof(float));

	//			string txtLine = itTxt->second[time];
	//			//ofsTxt << uid << "\t" << uid << "\t" << time << endl;
	//			ofsTxt << txtLine << "\n";
	//		}
	//	}
	//}
}

void MMergePred::write_all(ofstream& ofs, ofstream& ofsTxt, map<int, string>::iterator itt, map<int, string>::iterator ittTxt, map<int, string>::iterator theEnd)
{
	// Reference to pointer doesn't work on Linux.
	// 	while( itt != theEnd )
	// 	{
	// 		ofs << itt->second << "\n";
	// 		ofsTxt << ittTxt->second << "\n";
	// 		++itt;
	// 		++ittTxt;
	// 	}
}

void MMergePred::write_one(ofstream& ofs, ofstream& ofsTxt, map<int, string>::iterator itt, map<int, string>::iterator ittTxt)
{
	// 	ofs << itt->second << "\n";
	// 	ofsTxt << ittTxt->second << "\n";
}

