#include <HFitting/flt.h>
#include <HLib.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <string>
#include <vector>
using namespace std;

namespace flt {
	vector<const vector<ReturnData>*> get_data(const vector<string>& names, int idate)
	{
		int NF = names.size();
		vector<const vector<ReturnData>*> vp(NF);
		for( int i = 0; i < NF; ++i )
			vp[i] = static_cast<const vector<ReturnData>*>(HEvent::Instance()->get("", names[i] + "_bps" + "_" + itos(idate)));
		return vp;
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
		return valid;
	}

	vector<int> get_idates(int ndays, int idate)
	{
		const vector<int>& idates = HEnv::Instance()->idates();
		vector<int>::const_iterator itLB = lower_bound(idates.begin(), idates.end(), idate);

		vector<int> ret;
		vector<int>::const_iterator itBegin = idates.begin();
		if( distance(itBegin, itLB) >= ndays )
		{
			for( vector<int>::const_iterator it = itLB - 20; it != itLB; ++it )
				ret.push_back(*it);
		}

		return ret;
	}

	string get_model_dir(const hff::IndexFilter& filter, string fitAlg)
	{
		char dir[1000];
		sprintf( dir, "%s\\%s\\model\\%s\\%s", HEnv::Instance()->baseDir().c_str(), HEnv::Instance()->model().c_str(), filter.title().c_str(), fitAlg.c_str() );
		return (string)dir;
	}

	string get_model_path(const hff::IndexFilter& filter, string fitAlg, int idate)
	{
		string dir = get_model_dir(filter, fitAlg);
		ifstream ifsMap( (dir + "\\mapping.txt").c_str() );
		int idateFrom = 0;
		int idateTo = 0;
		string filename;
		string path = "";
		while( ifsMap >> idateFrom >> idateTo >> filename )
		{
			if( idateFrom <= idate && idate <= idateTo )
				path = dir + "\\" + filename;
		}
		return path;
	}
}