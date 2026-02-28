#include <gtlib_linmod/BiLinearModel.h>
#include <gtlib_linmod/CovMatSet.h>
#include <gtlib_linmod/CovMatSetCollUniv.h>
#include <gtlib_linmod/CovMatSetCollErr.h>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

BiLinearModel::BiLinearModel(){}

BiLinearModel::BiLinearModel(const string& covDir, const string& weightDir,
		int om_univ_fit_days, int om_err_fit_days,
		int minPts, int nHorizon, int num_time_slices, int om_num_sig,
		int om_num_err_sig, double ridgeRegUniv, double ridgeRegSS,
		const set<string>& uids, const vector<int>& allIdates, bool writeWeights, bool debug_wgt, int verbose)
	:enoughDataUniv_(false),
	enoughDataErr_(false),
	goodUnivModel_(false),
	goodErrModel_(false),
	coeffsNeverCalculated_(true),
	use_sigmoid_(false),
	cov_dir_(covDir),
	writeWeights_(writeWeights),
	weight_dir_(weightDir),
	univFitDays_(om_univ_fit_days),
	errFitDays_(om_err_fit_days),
	minPts_(minPts),
	nHorizon_(nHorizon),
	num_time_slices_(num_time_slices),
	om_num_sig_(om_num_sig),
	om_num_err_sig_(om_num_err_sig),
	ridgeRegUniv_(ridgeRegUniv),
	ridgeRegSS_(ridgeRegSS),
	allIdates_(allIdates),
	verbose_(verbose),
	debug_wgt_(debug_wgt)
{
	if( debug_wgt )
		verbose_ = 1;

	// OLS with moving window, by time slices.
	olsmov_ = vector<vector<OLSmovMM> >(nHorizon_, vector<OLSmovMM>(num_time_slices, OLSmovMM(om_num_sig, ridgeRegUniv)));

	// OLS with moving window for error correction, by tickers.
	olsmovErr_ = vector<unordered_map<string, OLSmovMM> >(nHorizon_);
	for( auto it1 = begin(olsmovErr_); it1 != end(olsmovErr_); ++it1 )
	{
		for( auto uid : uids )
			(*it1)[uid] = OLSmovMM(om_num_err_sig, ridgeRegSS);
	}

	mkd(cov_dir_);
	mkd(weight_dir_);
}

void BiLinearModel::endJob()
{
}

void BiLinearModel::set_verbose()
{
	verbose_ = 1;
}

void BiLinearModel::setWriteSigmoid(bool tf)
{
	writeRanges_ = tf;
}

void BiLinearModel::useSigmoid()
{
	use_sigmoid_ = true;
}

void BiLinearModel::beginDay(int idate)
{
	clear_cov(idate);
	if( coeffsNeverCalculated_ )
	{
		read_cov(getPrevIdate(idate));
		calculate_coeffs();
	}
}

void BiLinearModel::endDay(int idate)
{
	write_cov(idate);
	read_cov(idate);
	calculate_coeffs();
	if( writeWeights_ )
		write_weights(idate);
	if( writeRanges_ )
		write_range(idate);
}

int BiLinearModel::getPrevIdate(int idate)
{
	auto it = find(begin(allIdates_), end(allIdates_), idate);
	advance(it, -1);
	int prevIdate = *it;
	return prevIdate;
}

void BiLinearModel::clear_cov(int idate)
{
	// Get ready for today.
	for( auto it = begin(olsmov_); it != end(olsmov_); ++it ) // by time slices.
	{
		for( auto it2 = begin(*it); it2 != end(*it); ++it2 )
			it2->clear();
	}
	for( auto it = begin(olsmovErr_); it != end(olsmovErr_); ++it ) // by tickers.
	{
		for( auto it2 = begin(*it); it2 != end(*it); ++it2 )
			it2->second.clear();
	}
}

string BiLinearModel::get_cov_file(int idate)
{
	string filename = cov_dir_ + "/" + itos(idate) + ".cov";
	return filename;
}

void BiLinearModel::write_cov(int idate)
{
	ofstream ofs(get_cov_file(idate).c_str());
	ofs << nHorizon_ << '\t' << num_time_slices_ << '\t' << om_num_sig_ << '\n';
	for( auto it = begin(olsmov_); it != end(olsmov_); ++it ) // by time slices.
	{
		for( auto it2 = begin(*it); it2 != end(*it); ++it2 )
			it2->saveCorrs(ofs);
	}
	int nTicker = olsmovErr_[0].size();
	ofs << nHorizon_ << '\t' << nTicker << '\t' << om_num_err_sig_<< '\n';
	for( auto it = begin(olsmovErr_); it != end(olsmovErr_); ++it ) // by tickers.
	{
		for( auto it2 = begin(*it); it2 != end(*it); ++it2 )
		{
			const string& ticker = it2->first;
			ofs << ticker << '\n';
			it2->second.saveCorrs(ofs);
		}
	}
	ofs << flush;
}

void BiLinearModel::init_OLS()
{
	for( auto it = begin(olsmov_); it != end(olsmov_); ++it ) // by time slices.
	{
		for( auto it2 = begin(*it); it2 != end(*it); ++it2 )
			it2->initOLS();
	}
	for( auto it = begin(olsmovErr_); it != end(olsmovErr_); ++it ) // by tickers.
	{
		for( auto it2 = begin(*it); it2 != end(*it); ++it2 )
			it2->second.initOLS();
	}
}

void BiLinearModel::read_range(int idate)
{
	ifstream ifs(get_aggregated_range_file(idate).c_str());
	double x;
	vector<float> v;
	while(ifs >> x)
		v.push_back(x);
	if( v.size() == om_num_sig_ )
	{
		range_ = v;
		rangeErr_ = vector<float>(v.begin(), v.begin() + om_num_err_sig_);
	}
}

void BiLinearModel::read_cov(int idate)
{
	init_OLS();

	int idateFirstUniv = getFirstDateInclusive(allIdates_, idate, univFitDays_);
	int idateFirstErr = getFirstDateInclusive(allIdates_, idate, errFitDays_);
	auto itFrom = find(begin(allIdates_), end(allIdates_), min(idateFirstUniv, idateFirstErr));
	auto itTo = upper_bound(begin(allIdates_), end(allIdates_), idate);
	vector<int> idatesToRead = vector<int>(itFrom, itTo);

	enoughDataUniv_ = false;
	enoughDataErr_ = false;
	int nDataDayUniv = 0;
	int nDataDayErr = 0;
	for( auto idate : idatesToRead )
	{
		bool doReadUniv = idate >= idateFirstUniv;
		bool doReadErr = idate >= idateFirstErr;

		ifstream ifs(get_cov_file(idate).c_str());
		if( ifs.is_open() )
		{
			// Read univ model.
			bool univNonEmpty = false;
			CovMatSetCollUniv cUniv;
			ifs >> cUniv;
			if( doReadUniv )
			{
				assert(cUniv.nHorizon == nHorizon_);
				assert(cUniv.nTimeSlice == num_time_slices_);
				assert(cUniv.nSig == om_num_sig_);
				for( int iH = 0; iH < nHorizon_; ++iH )
				{
					for( int iT = 0; iT < cUniv.nTimeSlice; ++iT )
					{
						const CovMatSet& cSet = cUniv.getCovMatSet(iH, iT);
						if( !cSet.empty() )
						{
							olsmov_[iH][iT].loadCorrs(cSet);
							univNonEmpty = true;
						}
					}
				}
			}
			if( nDataDayUniv > 0 && !univNonEmpty ) // Missing data?
			{
				cerr << "BiLinearModel::read_cov(): Read error.\n";
				exit(21);
			}
			if( univNonEmpty )
				++nDataDayUniv;

			// Read ticker specifit error correction model.
			bool errNonEmpty = false;
			CovMatSetCollErr cErr;
			ifs >> cErr;
			if( doReadErr )
			{
				assert(cErr.nHorizon == nHorizon_);
				assert(cErr.nSig == om_num_err_sig_);
				for( int iH = 0; iH < nHorizon_; ++iH )
				{
					for( auto ticker : cErr.vTicker )
					{
						auto it = olsmovErr_[iH].find(ticker);
						if( it != olsmovErr_[iH].end() )
						{
							const CovMatSet& cSet = cErr.getCovMatSet(iH, ticker);
							if( !cSet.empty() )
							{
								it->second.loadCorrs(cSet);
								errNonEmpty = true;
							}
						}
					}
				}
			}
			if( errNonEmpty )
				++nDataDayErr;
		}
	}
	cout << nDataDayUniv << '/' << univFitDays_ << '\t' << nDataDayErr << '/' << errFitDays_ << endl;
	enoughDataUniv_ = nDataDayUniv == univFitDays_;
	enoughDataErr_ = nDataDayErr == errFitDays_;
}

void BiLinearModel::write_range(int idate)
{
	int idateFirstUniv = getFirstDateInclusive(allIdates_, idate, univFitDays_);
	int idateFirstErr = getFirstDateInclusive(allIdates_, idate, errFitDays_);
	auto itFrom = find(begin(allIdates_), end(allIdates_), min(idateFirstUniv, idateFirstErr));
	auto itTo = upper_bound(begin(allIdates_), end(allIdates_), idate);
	vector<int> idatesToRead = vector<int>(itFrom, itTo);

	const int N = 64;
	vector<Acc> vAcc(N);
	bool enoughData = false;
	int nDataDay = 0;
	for( auto idate : idatesToRead )
	{
		bool doReadUniv = idate >= idateFirstUniv;
		bool doReadErr = idate >= idateFirstErr;
		bool doRead = doReadUniv || doReadErr;

		ifstream ifs(get_daily_range_file(idate).c_str());
		if( ifs.is_open() )
		{
			bool nonEmpty = false;
			if( doRead )
			{
				for(int i = 0; i < N; ++i)
				{
					double x = 0.;
					ifs >> x;
					vAcc[i].add(x);
				}
			}
			if( nDataDay > 0 && !nonEmpty ) // Missing data?
			{
				cerr << "BiLinearModel::write_range(): Read error.\n";
				exit(21);
			}
			if( nonEmpty )
				++nDataDay;
		}
	}
	cout << nDataDay << '/' << max(univFitDays_, errFitDays_) << endl;
	enoughData = nDataDay == max(univFitDays_, errFitDays_);

	ofstream ofs(get_aggregated_range_file(idate).c_str());
	for(auto x : vAcc)
		ofs << x.mean() << '\n';
	ofs << flush;
}

string BiLinearModel::get_daily_range_file(int idate)
{
	string filename = cov_dir_ + "/drange" + itos(idate) + ".txt";
	return filename;
}

string BiLinearModel::get_aggregated_range_file(int idate)
{
	char filename[200];
	sprintf(filename, "%s/range%d.txt", weight_dir_.c_str(), idate);
	return filename;
}

int BiLinearModel::getFirstDateInclusive(const vector<int>& idates, int lastIdate, int length)
{
	auto itIdate = find(begin(idates), end(idates), lastIdate);
	advance(itIdate, -(length - 1));
	return *itIdate;
}

void BiLinearModel::calculate_coeffs()
{
	goodUnivModel_ = false;
	if( enoughDataUniv_ && univFitDays_ > 0 )
	{
		cout << "Calculating univ model." << endl;
		for( auto& vOlsmov : olsmov_ )
		{
			for( auto& olsmov : vOlsmov )
				olsmov.getCoeffs(true, false, verbose_);
		}
		goodUnivModel_ = valid_univ_weights();
		if( !goodUnivModel_ )
			cout << "ERROR univ weights invalid." << endl;
	}

	goodErrModel_ = false;
	if( enoughDataErr_ && errFitDays_ > 0 )
	{
		cout << "Calculating err model." << endl;
		for( auto& mOlsmov : olsmovErr_ )
		{
			for( auto& kv : mOlsmov )
			{
				if( kv.second.nPts() >= minPts_ )
					kv.second.getCoeffs(true, false, verbose_);
			}
		}
		goodErrModel_ = valid_err_weights();
		if( !goodErrModel_ )
			cout << "ERROR err weights invalid." << endl;
	}

	coeffsNeverCalculated_ = false;
}

bool BiLinearModel::goodUnivModel()
{
	return univFitDays_ == 0 || goodUnivModel_;
}

bool BiLinearModel::goodErrModel()
{
	return errFitDays_ == 0 || goodErrModel_;
}

bool BiLinearModel::goodModel()
{
	//return goodUnivModel_ && goodErrModel_;
	return goodUnivModel() && goodErrModel();
}

void BiLinearModel::write_weights(int idate)
{
	if( goodModel() && !weight_dir_.empty() )
	{
		mkd(weight_dir_);

		//char wfilename[200];
		//sprintf(wfilename, "%s\\weight%d.txt", weight_dir_.c_str(), idate);
		string wfilename = get_weight_file(idate);
		ofstream ofs(wfilename.c_str());

		int length = (num_time_slices_ > 0) ? olsmov_[0][0].getOLS().length() : 0;
		ofs << nHorizon_ << "\t" << num_time_slices_ << "\t" << length << endl;

		for( auto& vOlsmov : olsmov_ )
		{
			for( auto& olsmov : vOlsmov )
				olsmov.write_coeffs(ofs);
		}

		int nTickers = olsmovErr_[0].size();
		int lengthErr = olsmovErr_[0].begin()->second.getOLS().length();
		ofs << nTickers << "\t" << lengthErr << endl;

		for( auto& mOlsmov : olsmovErr_ )
		{
			for( auto& kv : mOlsmov )
			{
				ofs << kv.first << '\t';
				kv.second.write_coeffs(ofs);
			}
		}
		cout << "Writing weights for " << idate << "\n";
	}
}

string BiLinearModel::get_weight_file(int idate)
{
	char wfilename[200];
	sprintf(wfilename, "%s/weight%d.txt", weight_dir_.c_str(), idate);
	return wfilename;
}

bool BiLinearModel::valid_univ_weights()
{
	double totUniv = 0.;
	double okUniv = 0.;
	for( auto& vOlsmov : olsmov_ )
	{
		for( auto& olsmov : vOlsmov )
		{
			++totUniv;
			if( olsmov.getOLS().valid_coeffs() )
				++okUniv;
		}
	}
	bool validUniv = olsmov_.empty() || olsmov_[0].empty() || okUniv > totUniv * .2;
	return validUniv;
}

bool BiLinearModel::valid_err_weights()
{
	double totErr = 0.;
	double okErr = 0.;
	for( auto& mOlsmov : olsmovErr_ )
	{
		for( auto& kv : mOlsmov )
		{
			++totErr;
			if( kv.second.getOLS().valid_coeffs() )
				++okErr;
		}
	}
	bool validErr = olsmovErr_.empty() || okErr > totErr * .2;
	return validErr;
}

BiLinearModel::LinRegStatTicker::LinRegStatTicker(int num_time_slices, int om_num_sig, int om_num_err_sig, double ridgeRegUniv, double ridgeRegSS)
{
	olsmov = vector<OLSmovMM>(num_time_slices, OLSmovMM(om_num_sig, ridgeRegUniv));
	olsmovErr = OLSmovMM(om_num_err_sig, ridgeRegSS);
}

BiLinearModel::LinRegStatTicker* BiLinearModel::getNewLinRegStatTicker()
{
	LinRegStatTicker* linRegStat = new LinRegStatTicker(num_time_slices_, om_num_sig_, om_num_err_sig_, ridgeRegUniv_, ridgeRegSS_);
	return linRegStat;
}

void BiLinearModel::update(LinRegStatTicker* linRegStat, const string& uid)
{
	for( int i = 0; i < num_time_slices_; ++i )
		olsmov_[0][i].add(linRegStat->olsmov[i]);
	olsmovErr_[0][uid].add(linRegStat->olsmovErr);
}

void BiLinearModel::update(int iT, int timeIdx, const vector<float>& v, double target)
{
	olsmov_[iT][timeIdx].add(v, target);
}

void BiLinearModel::updateErr(int iT, const string& uid, const vector<float>& v, double target)
{
	olsmovErr_[iT][uid].add(v, target);
}

double BiLinearModel::pred(int iT, int timeIdx, const vector<float>& v)
{
	double pr = 0.;
	if( timeIdx < num_time_slices_ )
	{
		//pr = olsmov_[iT][timeIdx].getOLS().pred(v);

		const vector<float>& wgts = olsmov_[iT][timeIdx].getOLS().copyCoeffs();
		int Ni = v.size();
		for( int i = 0; i < Ni; ++i )
		{
			if( wgts[i] > -99 )
			{
				float partial = getPartialPred(i, wgts, v);
				pr += partial;
			}
		}
	}
	return pr;
}

double BiLinearModel::predIndex(int iT, int timeIdx, const vector<float>& v)
{
	vector<float> vLocal = v;
	for( int i = 0; i < hff::om_num_basic_sig_; ++i )
	{
		if( i != 4 && i != 5 && i != 6 && i != 7 && i != 12 && i != 13 )
			vLocal[i] = 0.;
	}

	double pr = 0.;
	if( timeIdx < num_time_slices_ )
		pr = olsmov_[iT][timeIdx].getOLS().pred(vLocal);
	return pr;
}

double BiLinearModel::predErr(int iT, const string& uid, const vector<float>& v)
{
	double prErr = 0.;
	//prErr = olsmovErr_[iT][uid].getOLS().pred(v);
	const vector<float>& wgts = olsmovErr_[iT][uid].getOLS().copyCoeffs();
	int Ni = v.size();
	for( int i = 0; i < Ni; ++i )
	{
		if( wgts[i] > -99 )
		{
			float partial = getPartialPredErr(i, wgts, v);
			prErr += partial;
		}
	}
	return prErr;
}

double BiLinearModel::predErrIndex(int iT, const string& uid, const vector<float>& v)
{
	vector<float> vLocal = v;
	for( int i = 0; i < hff::om_num_basic_sig_; ++i )
	{
		if( i != 4 && i != 5 && i != 6 && i != 7 && i != 12 && i != 13 )
			vLocal[i] = 0.;
	}

	double prErr = olsmovErr_[iT][uid].getOLS().pred(vLocal);
	return prErr;
}

double BiLinearModel::getPartialPred(int i, const vector<float>& wgts, const vector<float>& v)
{
	if( use_sigmoid_ )
	{
		if( !range_.empty() && range_[i] != 0. )
			return range_[i] * wgts[i] * tanh(v[i] / range_[i]);
		else
			return 0.;
	}
	return wgts[i] * v[i];
}

double BiLinearModel::getPartialPredErr(int i, const vector<float>& wgts, const vector<float>& v)
{
	if( use_sigmoid_ )
	{
		if( !rangeErr_.empty() && rangeErr_[i] != 0. )
			return rangeErr_[i] * wgts[i] * tanh(v[i] / rangeErr_[i]);
		else
			return 0.;
	}
	return wgts[i] * v[i];
}
