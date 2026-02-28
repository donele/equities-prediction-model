#include <HFitting/HMakeSampleIndex.h>
#include <HLib.h>
#include <map>
#include <string>
#include "TFile.h"
using namespace std;

HMakeSampleIndex::HMakeSampleIndex(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
debug_(false)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
}

HMakeSampleIndex::~HMakeSampleIndex()
{}

void HMakeSampleIndex::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	string model = HEnv::Instance()->model();

	// Debug.
	if( debug_ )
	{
		ar_.clear();
		const vector<hff::IndexFilter>& filters = HEnv::Instance()->indexFilters().filters;
		for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it )
			ar_.push_back(ARMulti(it->names.size(), it->length, it->horizon));
	}
}

void HMakeSampleIndex::beginDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	string model = HEnv::Instance()->model();

	int n = 0;
	const vector<hff::IndexFilter>& filters = HEnv::Instance()->indexFilters().filters;
	for( vector<hff::IndexFilter>::const_iterator it = filters.begin(); it != filters.end(); ++it, ++n )
		make_sample(*it, n);
}

void HMakeSampleIndex::endDay()
{
}

void HMakeSampleIndex::endJob()
{
	// Debug.
	if( debug_ )
	{
		for( vector<ARMulti>::iterator it = ar_.begin(); it != ar_.end(); ++it )
			it->getCoeffs();
	}
}

void HMakeSampleIndex::make_sample(const hff::IndexFilter& filter, int n)
{
	if( valid_data(filter.names) )
	{
		string model = HEnv::Instance()->model();
		int idate = HEnv::Instance()->idate();
		int lagMax = filter.length + filter.horizon;

		int NF = filter.names.size();
		vector<const vector<ReturnData>*> vp(NF);
		for( int i = 0; i < NF; ++i )
			vp[i] = static_cast<const vector<ReturnData>*>(HEvent::Instance()->get("", filter.names[i] + "_bps"));

		int totlen = vp[0]->size();

		// Debug.
		if( debug_ )
		{
			for( int iSer1 = 0; iSer1 < NF; ++iSer1 ) // series 1.
			{
				for( int iSer2 = 0; iSer2 < NF; ++iSer2 ) // series 2.
				{
					for( int i = filter.skip; i < totlen; ++i ) // index of series 1.
					{
						double val1 = clip_off( (*vp[iSer1])[i].ret, filter.clip[iSer1] );

						int jBegin = i;
						int jEnd = i + lagMax; // exclusive.
						if( jEnd > totlen )
							jEnd = totlen;

						for( int j = jBegin; j < jEnd; ++j ) // index of series 2.
						{
							double val2 = clip_off( (*vp[iSer2])[j].ret, filter.clip[iSer2] );
							int lag = j - i;
							ar_[n].add(iSer1, iSer2, lag, val1, val2);
						}
					}
				}
			}
		}

		// Make sample.
		vector<hff::Signal> vSig;
		int iSerTarget = 0;
		int ncol = NF * filter.length + 1;
		for( int i = filter.skip; i < totlen - filter.length - filter.horizon; ++i )
		{
			// Predictor series.
			vector<float> v;
			v.reserve(ncol);
			for( int iSer = 1; iSer < NF; ++iSer )
			{
				vector<ReturnData>::const_iterator itBegin = vp[iSer]->begin() + i;
				vector<ReturnData>::const_iterator itEnd = vp[iSer]->begin() + i + filter.length;
				for( vector<ReturnData>::const_iterator it = itBegin; it != itEnd; ++it )
					v.push_back(it->ret);
			}

			// Target is the sum of future returns within horizon.
			float target = 0.;
			vector<ReturnData>::const_iterator itBegin = vp[iSerTarget]->begin() + i + filter.length;
			vector<ReturnData>::const_iterator itEnd = vp[iSerTarget]->begin() + i + filter.length + filter.horizon;
			for( vector<ReturnData>::const_iterator it = itBegin; it != itEnd; ++it )
				target += it->ret;
			v.push_back(target);

			// Signal.
			vSig.push_back(hff::Signal(v));
		}

		// Signal Header.
		vector<string> labels;
		for( int iSer = 1; iSer < NF; ++iSer )
		{
			for( int n = 0; n < filter.length; ++n )
				labels.push_back(filter.names[iSer] + "_" + itos(n));
		}
		labels.push_back("ret_" + itos(filter.horizon));
		hff::SignalLabel sigHeader(vSig.size(), ncol, labels);

		// Write text file.
		string base_dir = HEnv::Instance()->baseDir();
		char dirTxt[400];
		sprintf(dirTxt, "%s\\%s\\txtSig\\%s", base_dir.c_str(), model.c_str(), filter.title().c_str());
		ForceDirectory(dirTxt);

		char filenameTxt[400];
		sprintf(filenameTxt, "%s\\%d.txt", dirTxt, idate);
		ofstream ofsTxt( filenameTxt, ios::out );

		sigHeader.print(ofsTxt);
		for( vector<hff::Signal>::iterator it = vSig.begin(); it != vSig.end(); ++it )
			it->print(ofsTxt);

		// Write binary file.
		char dirBin[400];
		sprintf(dirBin, "%s\\%s\\binSig\\%s", base_dir.c_str(), model.c_str(), filter.title().c_str());
		ForceDirectory(dirBin);

		char filenameBin[400];
		sprintf(filenameBin, "%s\\%d.bin", dirBin, idate);
		ofstream ofsBin( filenameBin, ios::out | ios::binary );

		ofsBin << sigHeader;
		for( vector<hff::Signal>::iterator it = vSig.begin(); it != vSig.end(); ++it )
			ofsBin << (*it);
	}
}

bool HMakeSampleIndex::valid_data(const vector<string>& filter)
{
	bool valid = true;
	int NF = filter.size();
	if( NF > 0 )
	{
		// Get the pointers.
		vector<const vector<ReturnData>*> vp(NF);
		for( int i = 0; i < NF; ++i )
		{
			vp[i] = static_cast<const vector<ReturnData>*>(HEvent::Instance()->get("", filter[i] + "_bps"));
			if( vp[i] == 0 )
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