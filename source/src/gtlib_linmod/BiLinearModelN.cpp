#include <gtlib_linmod/BiLinearModelN.h>
#include <boost/filesystem.hpp>
using namespace std;

BiLinearModelN::BiLinearModelN(){}

BiLinearModelN::BiLinearModelN(const string& covDir, const string& weightDir,
		int om_univ_fit_days, int om_err_fit_days,
		int minPts, int nHorizon, int num_time_slices, int om_num_sig,
		int om_num_err_sig, double ridgeRegUniv, double ridgeRegSS,
		const set<string>& uids, const vector<int>& allIdates, bool writeWeights, bool debug_wgt, int verbose)
	:cov_dir_(covDir),
	writeWeights_(writeWeights),
	univFitDays_(om_univ_fit_days),
	errFitDays_(om_err_fit_days),
	weight_dir_(weightDir),
	minPts_(minPts),
	allIdates_(allIdates),
	verbose_(verbose),
	debug_wgt_(debug_wgt)
{
	// OLS with moving window, by time slices.
	olsmov_ = vector<vector<OLSmov> >(nHorizon, vector<OLSmov>(num_time_slices, OLSmov(om_num_sig, ridgeRegUniv)));

	// OLS with moving window for error correction, by tickers.
	olsmovErr_ = vector<map<string, OLSmov> >(nHorizon);
	for( auto it1 = olsmovErr_.begin(); it1 != olsmovErr_.end(); ++it1 )
	{
		for( auto it2 = uids.begin(); it2 != uids.end(); ++it2 )
		{
			string uid = *it2;
			(*it1)[uid] = OLSmov(om_num_err_sig, ridgeRegSS);
		}
	}

	mkd(cov_dir_);
	mkd(weight_dir_);
}

void BiLinearModelN::endJob()
{
}

void BiLinearModelN::set_verbose()
{
	verbose_ = 1;
}

void BiLinearModelN::beginDay(int idate)
{
	// Get ready for today.
	for( auto it = olsmov_.begin(); it != olsmov_.end(); ++it ) // by time slices.
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
			it2->clear();
	for( auto it = olsmovErr_.begin(); it != olsmovErr_.end(); ++it ) // by tickers.
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
			it2->second.clear();
	sIdate_.insert(idate);
}

void BiLinearModelN::endDay(int idate)
{
	write_cov(idate);
	read_cov(idate);
	calculate_coeffs();
}

string BiLinearModelN::get_cov_file(int idate)
{
	string filename = cov_dir_ + "/" + itos(idate) + ".cov";
	return filename;
}

void BiLinearModelN::write_cov(int idate)
{
	ofstream ofs(get_cov_file(idate).c_str());
	for( auto it = olsmov_.begin(); it != olsmov_.end(); ++it ) // by time slices.
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
			it2->saveCorrs(ofs);
	for( auto it = olsmovErr_.begin(); it != olsmovErr_.end(); ++it ) // by tickers.
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
			it2->second.saveCorrs(ofs);
}

void BiLinearModelN::init_OLS()
{
	for( auto it = olsmov_.begin(); it != olsmov_.end(); ++it ) // by time slices.
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
			it2->initOLS();
	for( auto it = olsmovErr_.begin(); it != olsmovErr_.end(); ++it ) // by tickers.
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
			it2->second.initOLS();
}

void BiLinearModelN::read_cov(int idate)
{
	init_OLS();

	//int idateFirstUniv = getFirstDate(allIdates_, idate, univFitDays_);
	//int idateFirstErr = getFirstDate(allIdates_, idate, errFitDays_);
	int idateFirstUniv = 0;
	int idateFirstErr = 0;
	auto itFrom = find(begin(allIdates_), end(allIdates_), min(idateFirstUniv, idateFirstErr));
	auto itTo = find(begin(allIdates_), end(allIdates_), idate);
	vector<int> idatesToRead = vector<int>(itFrom, itTo);

	for( auto idate : idatesToRead )
	{
		bool doReadUniv = idate >= idateFirstUniv;
		bool doReadErr = idate >= idateFirstErr;
		ifstream ifs(get_cov_file(idate).c_str());
		if( ifs.is_open() )
		{
			for( auto it = olsmov_.begin(); it != olsmov_.end(); ++it ) // by time slices.
				for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
					it2->loadCorrs(ifs, doReadUniv);
			for( auto it = olsmovErr_.begin(); it != olsmovErr_.end(); ++it ) // by tickers.
				for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
					it2->second.loadCorrs(ifs, doReadErr);
		}
	}
}

void BiLinearModelN::calculate_coeffs()
{
	for( auto it = olsmov_.begin(); it != olsmov_.end(); ++it ) // by time slices.
	{
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
		{
			if( verbose_ )
				cout << "Univ ";
			it2->getCoeffs(true, false, verbose_);
		}
	}
	for( auto it = olsmovErr_.begin(); it != olsmovErr_.end(); ++it ) // by tickers.
	{
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
		{
			if( it2->second.nPts() >= minPts_ )
			{
				if( verbose_ )
					cout << it2->first << " ";
				it2->second.getCoeffs(true, false, verbose_);
				if( debug_wgt_ )
					cout << "1 " << it2->first << "\t" << it2->second.nPts() << "\t" << minPts_ << endl;
			}
			else if( debug_wgt_ )
				cout << "0 " << it2->first << "\t" << it2->second.nPts() << "\t" << minPts_ << endl;
		}
	}
}

bool BiLinearModelN::haveEnoughData()
{
	return false;
}

void BiLinearModelN::write_weights(int idate, bool check_valid)
{
	// Write weights.
	if( !check_valid || valid_weights() )
	{
		if( !weight_dir_.empty() )
		{
			mkd(weight_dir_);

			char wfilename[200];
			sprintf(wfilename, "%s\\weight%d.txt", weight_dir_.c_str(), idate);
			ofstream ofs(xpf(wfilename).c_str());

			int nHorizon = olsmov_.size();
			int nTimeSlice = (nHorizon > 0) ? olsmov_[0].size() : 0;
			int length = (nTimeSlice > 0) ? olsmov_[0][0].getOLS().length() : 0;
			ofs << nHorizon << "\t" << nTimeSlice << "\t" << length << endl;

			for( auto it = olsmov_.begin(); it != olsmov_.end(); ++it ) // by time slices.
			{
				for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
				{
					it2->write_coeffs(ofs);
				}
			}

			int nTickers = olsmovErr_[0].size();
			int lengthErr = olsmovErr_[0].begin()->second.getOLS().length();
			ofs << nTickers << "\t" << lengthErr << endl;

			for( auto it = olsmovErr_.begin(); it != olsmovErr_.end(); ++it ) // by tickers.
			{
				for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
				{
					ofs << it2->first << "\t";
					it2->second.write_coeffs(ofs);
				}
			}
		}
	}
	else
		cerr << "Abort writing weights for " << idate << "\n";
}

bool BiLinearModelN::valid_weights()
{
	double totUniv = 0.;
	double okUniv = 0.;
	for( auto it = olsmov_.begin(); it != olsmov_.end(); ++it ) // by time slices.
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
		{
			++totUniv;
			if( it2->getOLS().valid_coeffs() )
				++okUniv;
		}

	double totErr = 0.;
	double okErr = 0.;
	for( auto it = olsmovErr_.begin(); it != olsmovErr_.end(); ++it ) // by tickers.
		for( auto it2 = it->begin(); it2 != it->end(); ++it2 )
		{
			++totErr;
			if( it2->second.getOLS().valid_coeffs() )
				++okErr;
		}

	bool validUniv = olsmov_.empty() || okUniv > totUniv * .5;
	bool validErr = olsmovErr_.empty() || okErr > totErr * .5;
	bool valid = validUniv && validErr;
	return valid;
}

void BiLinearModelN::update(int iT, int timeIdx, const vector<float>& v, double target)
{
	olsmov_[iT][timeIdx].add(v, target);
}

void BiLinearModelN::updateErr(int iT, const string& uid, const vector<float>& v, double target)
{
	olsmovErr_[iT][uid].add(v, target);
}

double BiLinearModelN::pred(int iT, int timeIdx, const vector<float>& v)
{
	double pr = olsmov_[iT][timeIdx].pred(v);
	return pr;
}

double BiLinearModelN::predIndex(int iT, int timeIdx, const vector<float>& v)
{
	vector<float> vLocal = v;
	for( int i = 0; i < hff::om_num_basic_sig_; ++i )
	{
		if( i != 4 && i != 5 && i != 6 && i != 7 && i != 12 && i != 13 )
			vLocal[i] = 0.;
	}

	double pr = olsmov_[iT][timeIdx].pred(vLocal);
	return pr;
}

double BiLinearModelN::predErr(int iT, const string& uid, const vector<float>& v)
{
	double prErr = olsmovErr_[iT][uid].pred(v);
	return prErr;
}

double BiLinearModelN::predErrIndex(int iT, const string& uid, const vector<float>& v)
{
	vector<float> vLocal = v;
	for( int i = 0; i < hff::om_num_basic_sig_; ++i )
	{
		if( i != 4 && i != 5 && i != 6 && i != 7 && i != 12 && i != 13 )
			vLocal[i] = 0.;
	}

	double prErr = olsmovErr_[iT][uid].pred(vLocal);
	return prErr;
}
