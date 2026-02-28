#include <MSignal/MPasteSample.h>
#include <MFitting.h>
#include <gtlib_model/pathFtns.h>
#include <MFramework.h>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

MPasteSample::MPasteSample(const string& moduleName, const multimap<string, string>& conf)
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
	//if( conf.count("sigType") )
	//sigType_ = conf.find("sigType")->second;
}

MPasteSample::~MPasteSample()
{}

void MPasteSample::beginJob()
{
}

void MPasteSample::beginDay()
{
	int idate = MEnv::Instance()->idate;
	string model = MEnv::Instance()->model;

	map<string, map<int, vector<float> > > mBase;
	map<string, map<int, vector<float> > > mAdd;
	hff::SignalLabel shBase;
	hff::SignalLabel shAdd;
	map<string, map<int, string> > mTxtBase;
	map<string, map<int, string> > mTxtAdd;

	read_data(idate, modelBase_, fitDescBase_, shBase, mBase, mTxtBase);
	read_data(idate, modelAdd_, fitDescAdd_, shAdd, mAdd, mTxtAdd);
	write_data(idate, model, fitDescBase_, shBase, shAdd, mBase, mAdd, mTxtBase);
}

void MPasteSample::endDay()
{
}

void MPasteSample::endJob()
{
}

void MPasteSample::read_data(int idate, const string& model, const string& fitdesc, hff::SignalLabel& sh,
		map<string, map<int, vector<float> > >& m, map<string, map<int, string> >& mTxt)
{
	string path = get_sig_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitdesc);
	ifstream ifs(path.c_str(), ios::binary);

	string pathTxt = get_sigTxt_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitdesc);
	ifstream ifsTxt(pathTxt.c_str());

	if( ifs.is_open() && ifsTxt.is_open() )
	{
		cout << "Reading " << path << endl;

		// Read the headers.
		ifs >> sh;
		string line;
		getline(ifsTxt, line);

		hff::SignalContent aSignal(sh.ncols);
		int nrow_day = 0;

		for( int r = 0; r < sh.nrows && ifs.rdstate() == 0; ++r )
		{
			ifs >> aSignal; // Read bin.
			getline(ifsTxt, line); // Read txt.

			vector<string> sl = split(line, '\t');
			string uid = trim(sl[0]);
			string ticker = trim(sl[1]);
			int timeIdx = (sl.size() > 2) ? 2 : 1;
			int time = ceil(atof(sl[timeIdx].c_str()) * 1000. - 0.5);

			if( uid != "uid" && time > 0 )
			{
				m[uid][time] = aSignal.v;
				mTxt[uid][time] = line;
			}
		}
		cout << idate << " " << nrow_day << endl;
	}
	else
		cout << path << " is not open." << endl;
}

void MPasteSample::write_data(int idate, const string& model, const string& fitdesc, const hff::SignalLabel& shBase, const hff::SignalLabel& shAdd,
		map<string, map<int, vector<float> > >& mBase, map<string, map<int, vector<float> > >& mAdd,
		map<string, map<int, string> >& mTxt)
{
	string path = get_sig_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitdesc);
	string pathTxt = get_sigTxt_path(MEnv::Instance()->baseDir, model, idate, sigType_, fitdesc);

	ofstream ofs( path.c_str(), ios::out | ios::binary );
	ofstream ofsTxt( pathTxt.c_str(), ios::out );

	if( ofs.is_open() && ofsTxt.is_open() )
	{
		// Header for txt file.
		ofsTxt.precision(6);
		if( sigType_ == "om" )
			ofsTxt << "uid\tticker\ttime\n";
		else if( sigType_ == "tm" )
			ofsTxt << "uid\tticker\ttime\tbsize\tbid\task\tasize\n";

		// Header for bin file.
		// nrows.
		int nrows = shBase.nrows;
		ofs.write((char*)&(nrows), sizeof(int));

		// ncols.
		int ncols = shBase.labels.size() + shAdd.labels.size();
		ofs.write((char*)&(ncols), sizeof(int));

		// labels.
		string labels;
		for( auto it = shBase.labels.begin(); it != shBase.labels.end(); ++it )
		{
			if( it == shBase.labels.begin() )
				labels += *it;
			else
				labels += (string)"," + *it;
		}
		for( auto it = shAdd.labels.begin(); it != shAdd.labels.end(); ++it )
			labels += (string)"," + *it + "_Add";
		long long int labels_len = labels.size() + 1; // 8 bytes on 32 and 64 bit OS for MSDN only; does not include terminating zero so add 1 
		ofs.write((char*)&labels_len, sizeof(labels_len));
		ofs.write(labels.c_str(), labels_len);

		// Contents.
		vector<float> vtmp(shAdd.ncols, 0.);
		for( auto itu = mBase.begin(); itu != mBase.end(); ++itu )
		{
			string uid = itu->first;
			auto ituAdd = mAdd.find(uid);
			auto itTxt = mTxt.find(uid);

			for( auto itt = itu->second.begin(); itt != itu->second.end(); ++itt )
			{
				// Base.
				vector<float>& v = itt->second;
				ofs.write((char*)(&(v[0])), shBase.ncols * sizeof(float));

				// Added.
				int time = itt->first;
				if( ituAdd != mAdd.end() )
				{
					auto ittAdd = ituAdd->second.find(time);
					if( ittAdd != ituAdd->second.end() )
					{
						vector<float>& vAdd = ittAdd->second;
						ofs.write((char*)(&(vAdd[0])), shAdd.ncols * sizeof(float));
					}
					else
						ofs.write((char*)(&(vtmp[0])), shAdd.ncols * sizeof(float));
				}
				else
					ofs.write((char*)(&(vtmp[0])), shAdd.ncols * sizeof(float));

				string txtLine = itTxt->second[time];
				//ofsTxt << uid << "\t" << uid << "\t" << time << endl;
				ofsTxt << txtLine << "\n";
			}
		}
	}
}
