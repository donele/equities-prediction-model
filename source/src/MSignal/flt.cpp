#include <MSignal/flt.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
using namespace std;

namespace flt {
vector<const vector<ReturnData>*> get_data(const vector<string>& names, int idate)
{
	int NF = names.size();
	vector<const vector<ReturnData>*> vp(NF);
	for( int i = 0; i < NF; ++i )
	{
		string name = names[i] + "_bps" + "_" + itos(idate);
		vp[i] = static_cast<const vector<ReturnData>*>(MEvent::Instance()->get("", name));
		if( vp[i] == 0 )
			cout << name << " is not found.\n";
	}
	return vp;
}

vector<float> get_data(const string& name, int idate)
{
	vector<float> v;
	string this_name = name + "_bps" + "_" + itos(idate);
	const vector<ReturnData>* p = static_cast<const vector<ReturnData>*>(MEvent::Instance()->get("", this_name));
	if( p == 0 )
		cout << name << " is not found.\n";
	else
	{
		for( auto it = p->begin(); it != p->end(); ++it )
			v.push_back(it->ret);
	}
	return v;
}

bool valid_data(vector<const vector<ReturnData>*>& vp)
{
	bool valid = true;
	int NF = vp.size();
	if( NF > 0 )
	{
		for( vector<const vector<ReturnData>*>::iterator it = vp.begin(); it != vp.end(); ++it )
		{
			if( *it == 0 )
			{
				valid = false;
				break;
			}
		}

		if( valid )
		{
			// check the sizes and msecs[0].
			int totlen = vp[0]->size();
			if( totlen > 0 )
			{
				int msecs0 = (*vp[0])[0].msecs;
				for( int i = 1; i < NF; ++ i )
				{
					if( vp[i]->size() != totlen )
					{
						valid = false;
						break;
					}
					if( (*vp[i])[0].msecs != msecs0 )
					{
						valid = false;
						break;
					}
				}
			}
			else
				valid = false;
		}
	}
	return valid;
}

vector<int> get_idates(int ndays, int idate)
{
	const vector<int>& idates = MEnv::Instance()->idates;
	vector<int>::const_iterator itLB = upper_bound(idates.begin(), idates.end(), idate);

	vector<int> ret;
	vector<int>::const_iterator itBegin = idates.begin();
	int dist = distance(itBegin, itLB);
	if( dist >= ndays )
	{
		for( vector<int>::const_iterator it = itLB - ndays; it != itLB; ++it )
			ret.push_back(*it);
	}

	return ret;
}

string get_filter_dir(const hff::IndexFilter& filter, const string& model)
{
	char dir[1000];
	sprintf( dir, "%s\\%s\\filter\\%s", MEnv::Instance()->baseDir.c_str(), model.c_str(), filter.title().c_str() );
	return xpf(dir);
}

string get_filter_path(const hff::IndexFilter& filter, const string& model, int idate)
{
	namespace fs = boost::filesystem;
	string filter_dir = get_filter_dir(filter, model);
	fs::path dir(filter_dir);

	int filter_date = 0;
	if( fs::exists(dir) )
	{
		fs::directory_iterator end_iter;
		for( fs::directory_iterator itd(dir); itd != end_iter; ++itd )
		{
			string filename = itd->path().filename().string();
			if( filename.size() == 15 )
			{
				int fdate = atoi(filename.substr(3, 8).c_str());
				if( fdate > filter_date && fdate < idate )
					filter_date = fdate;
			}
		}
	}
	else
	{
		cerr << "Path " << dir << " does not exists." << endl;
		exit(5);
	}

	char path[200];
	if( filter_date > 0 )
		sprintf(path, "%s\\flt%d.txt", filter_dir.c_str(), filter_date);
	return xpf(path);
}

string get_beta_dir(const string& indexName, const string& model)
{
	string dir = MEnv::Instance()->baseDir + "/" + model + "/filter/" + indexName;
	return dir;
}

string get_beta_path(const string& indexName, const string& model, int idate)
{
	namespace fs = boost::filesystem;
	string beta_dir = get_beta_dir(indexName, model);
	fs::path dir(beta_dir);

	int beta_date = 0;
	if( fs::exists(dir) )
	{
		fs::directory_iterator end_iter;
		for( fs::directory_iterator itd(dir); itd != end_iter; ++itd )
		{
			string filename = itd->path().filename().string();
			if( filename.size() == 16 )
			{
				int fdate = atoi(filename.substr(4, 8).c_str());
				if( fdate > beta_date && fdate < idate )
					beta_date = fdate;
			}
		}
	}
	else
	{
		cerr << "Path " << dir << " does not exists." << endl;
		exit(5);
	}

	string path = beta_dir + "/beta" + itos(beta_date) + ".txt";
	return path;
}
string get_filter_path_ols(const hff::IndexFilter& filter, const string& model, int idate, int lag0, int lagMult)
{
	namespace fs = boost::filesystem;
	string filter_dir = get_filter_dir(filter, model);
	fs::path dir(filter_dir);

	int filter_date = 0;
	if( fs::exists(dir) )
	{
		fs::directory_iterator end_iter;
		for( fs::directory_iterator itd(dir); itd != end_iter; ++itd )
		{
			string filename = itd->path().filename().string();
			if( filename.size() == 18 )
			{
				int fdate = atoi(filename.substr(6, 8).c_str());
				if( fdate > filter_date && fdate < idate )
					filter_date = fdate;
			}
		}
	}
	else
	{
		cerr << "Path " << dir << " does not exists." << endl;
		exit(5);
	}

	char path[200];
	if( filter_date > 0 )
	{
		if( lag0 > 0 && lagMult > 0 )
			sprintf(path, "%s\\weight%d_%d_%d.txt", filter_dir.c_str(), filter_date, lag0, lagMult);
		else
			sprintf(path, "%s\\weight%d.txt", filter_dir.c_str(), filter_date);
	}
	return xpf(path);
}
}
