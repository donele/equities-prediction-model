#include <gtlib_model/hff.h>
#include <jl_lib/mto.h>
#include <jl_lib/jlutil.h>
#include <optionlibs/TickData.h>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

namespace hff {

vector<string> markets(const string& model)
{
	vector<string> markets;

	if( model.size() >= 2 )
	{
		string model02 = model.substr(0, 2);

		if( "EU" == model02 )
		{
			markets.push_back("EA");
			markets.push_back("EB");
			markets.push_back("EI");
			markets.push_back("EP");
			markets.push_back("EF");
			markets.push_back("EL");
			markets.push_back("ED");
			markets.push_back("EM");
			markets.push_back("EZ");
			markets.push_back("EO");
			markets.push_back("EX");
			markets.push_back("EC");
			markets.push_back("EW");
			markets.push_back("EY");
		}
		else if( "KR" == model02 )
		{
			markets.push_back("AK");
			markets.push_back("AQ");
		}
		else if( "CA" == model02 )
			markets.push_back("CJ");
		else if( "US" == model02 || "UF" == model02 )
			markets.push_back("US");
		else if( "ASX" == model )
			markets.push_back("AS");
		else
			markets.push_back(model02);
	}
	return markets;
}

IndexFilters::IndexFilters(const string& model_, double clip_index_, double clip_fut_index_, bool use_etf_filter_)
	:model(model_),
	clip_index(clip_index_),
	clip_fut_index(clip_fut_index_),
	use_etf_filter(use_etf_filter_)
{}

void IndexFilters::addHorizon(int horiz, int lag)
{
	string model02 = model.substr(0, 2);

	if( model02[0] == 'E' )
	{
		if( "EU" == model02 )
		{
			vector<string> names1;
			names1.push_back("PANEU_RET"); // target.
			names1.push_back("PANEU_RET"); // predictor.
			filters.push_back(IndexFilter(model, names1, horiz, lag, 300, clip_index, clip_fut_index));
		}
		else
		{
			string name = model.substr(1, 1) + "_RET";
			vector<string> names1;
			names1.push_back(name);
			names1.push_back(name);
			filters.push_back(IndexFilter(model, names1, horiz, lag, 300, clip_index, clip_fut_index));
		}
	}
	else if( "KR" == model02 || "AK" == model02 || "AQ" == model02 )
	{
		string name = "KR_RET";
		vector<string> names1;
		names1.push_back(name);
		names1.push_back(name);
		filters.push_back(IndexFilter(model, names1, horiz, lag, 300, clip_index, clip_fut_index));
	}
	else if( "AX" == model02 )
	{
		string name = "S_RET";
		vector<string> names1;
		names1.push_back(name);
		names1.push_back(name);
		filters.push_back(IndexFilter(model, names1, horiz, lag, 300, clip_index, clip_fut_index));
	}
	else if( "A" == model.substr(0, 1) || "M" == model.substr(0, 1) )
	{
		string name = model.substr(1, 1) + "_RET";
		vector<string> names1;
		names1.push_back(name);
		names1.push_back(name);
		filters.push_back(IndexFilter(model, names1, horiz, lag, 300, clip_index, clip_fut_index));
	}
	else if( "S" == model.substr(0, 1) )
	{
		string name = model.substr(0, 2) + "_RET";
		vector<string> names1;
		names1.push_back(name);
		names1.push_back(name);
		filters.push_back(IndexFilter(model, names1, horiz, lag, 300, clip_index, clip_fut_index));
	}
	else if( "C" == model.substr(0, 1) )
	{
		vector<string> names1;
		names1.push_back("TSX_RET");
		names1.push_back("TSX_RET");
		filters.push_back(IndexFilter(model, names1, horiz, lag, 300, clip_index, clip_fut_index));

		vector<string> names2;
		names2.push_back("TSX_RET");
		names2.push_back("SXF_RET");
		filters.push_back(IndexFilter(model, names2, horiz, lag, 300, clip_index, clip_fut_index));
	}
	else if( "US" == model02 || "UF" == model02 )
	{
		if( use_etf_filter )
		{
			vector<string> names1;
			names1.push_back("SPX_RET");
			names1.push_back("SPX_RET");
			filters.push_back(IndexFilter(model, names1, horiz, lag, 100, clip_index, clip_fut_index));

			vector<string> names2;
			names2.push_back("SPX_RET");
			names2.push_back("SPY_RET");
			filters.push_back(IndexFilter(model, names2, horiz, lag, 100, clip_index, clip_fut_index));

			vector<string> names3;
			names3.push_back("LCX_RET");
			names3.push_back("LCX_RET");
			filters.push_back(IndexFilter(model, names3, horiz, lag, 300, clip_index, clip_fut_index));

			vector<string> names4;
			names4.push_back("LCX_RET");
			names4.push_back("SPY_RET");
			filters.push_back(IndexFilter(model, names4, horiz, lag, 100, clip_index, clip_fut_index));

			vector<string> names5;
			names5.push_back("SPX_RET");
			names5.push_back("IWY_RET");
			filters.push_back(IndexFilter(model, names5, horiz, lag, 300, clip_index, clip_fut_index));

			vector<string> names6;
			names6.push_back("LCX_RET");
			names6.push_back("IWY_RET");
			filters.push_back(IndexFilter(model, names6, horiz, lag, 300, clip_index, clip_fut_index));
		}
		else
		{
			vector<string> names1;
			names1.push_back("SPX_RET");
			names1.push_back("SPX_RET");
			filters.push_back(IndexFilter(model, names1, horiz, lag, 100, clip_index, clip_fut_index));

			vector<string> names2;
			names2.push_back("SPX_RET");
			names2.push_back("ES_RET");
			filters.push_back(IndexFilter(model, names2, horiz, lag, 100, clip_index, clip_fut_index));

			vector<string> names3;
			names3.push_back("LCX_RET");
			names3.push_back("LCX_RET");
			filters.push_back(IndexFilter(model, names3, horiz, lag, 100, clip_index, clip_fut_index));

			vector<string> names4;
			names4.push_back("LCX_RET");
			names4.push_back("ES_RET");
			filters.push_back(IndexFilter(model, names4, horiz, lag, 300, clip_index, clip_fut_index));

			vector<string> names5;
			names5.push_back("SPX_RET");
			names5.push_back("ER2_RET");
			filters.push_back(IndexFilter(model, names5, horiz, lag, 100, clip_index, clip_fut_index));

			vector<string> names6;
			names6.push_back("LCX_RET");
			names6.push_back("ER2_RET");
			filters.push_back(IndexFilter(model, names6, horiz, lag, 300, clip_index, clip_fut_index));
		}
	}
}

int minDataOK(const string& model)
{
	int minDataOK = 1;
	if( "EU" == model.substr(0, 2) )
		minDataOK = 10;
	return minDataOK;
}

IndexFilter::IndexFilter(const string& model, const vector<string>& names_, int horizon_, int targetLag_, int length_, double clip_index, double clip_fut_index)
	:names(names_),
	myTitle(""),
	fitDays(20),
	length(length_),
	horizon(horizon_),
	targetLag(targetLag_),
	skip(300),
	clip_index_(clip_index),
	clip_fut_index_(clip_fut_index)
{
	for( vector<string>::iterator it = names.begin(); it != names.end(); ++it )
		clip.push_back( filtClip(*it) );

	for( vector<string>::const_iterator it = names.begin(); it != names.end(); ++it )
	{
		if( myTitle.empty() )
			myTitle += *it;
		else
			myTitle += xpf(";") + *it;
	}
	myTitle += xpf(";") + itos(horizon);
	myTitle += xpf(";") + itos(targetLag);
	myTitle += xpf(";") + itos(length);
}

int IndexFilter::filtClip(const string& name)
{
	if( name == "ER2_RET" || "ES_RET" )
		return clip_fut_index_;
	else
		return clip_index_;
	return 0;
}

string IndexFilter::title() const
{
	return myTitle;
}

LinearModel::LinearModel(const string& model_)
	:model(model_),
	medFromHfuniv(false),
	nfFromHfuniv(false),
	inUnivFromHfuniv(false),
	partialVM(true),
	useMoreCAEcns(false),
	requirePersistChina(false),
	excludeBlockTrade(0),
	allowNegSizeTo(0),
	primary_only(false),
	exploratoryDelay(1),
	minSpreadMMS(0.),
	maxSpreadMMS(0.),
	gridInterval(30),
	nSlices(13),
	nSig(64),
	nErrSig(16),
	nHorizon(0),
	sampleDelay(1),
	useShortMrets(false),
	writeCompTicker(true),
	allowCross(false),
	nTreesOmFit(0),
	nTreesOmProd(0),
	minNrowsParam(500),
	transientMsecs(5),
	om_bin_sample_freq(30),
	tm_bin_sample_freq(300),
	num_time_slices(13),
	om_univ_fit_days(30),
	om_err_fit_days(30),
	clip_pred_index(1000000.),
	clip_fut_index(25.),
	clip_index(5.),
	clip_ret60(200.),
	clip_ret300(400.),
	on_target_clip(800.),
	om_target_clip(200.),
	tm_target_clip(600.),
	tm_target_60_clip(1000.),
	tm_target_intra_clip(100000.),
	scaleTradable(1.),
	decayTradable(4.),
	om_use_tm_input(false),
	use_pred_index(true),
	use_etf_filter(false),
	use_us_arca_trade_event(true),
	halt_tickers(false),
	use_psx(false)
{
	model02 = model.substr(0, 2);
	mCode = model.substr(1, 1);
	market = hff::markets(model)[0];
	markets = hff::markets(model);
	openMsecs = mto::msecOpen(market, 0);
	closeMsecs = mto::msecClose(market);
	n1sec = (closeMsecs - openMsecs) / 1000 + 1;
	gpts = (n1sec - 1) / gridInterval + 1;
	useom = vector<int>(hff::om_num_sig_, 1);
	useomErr = vector<int>(hff::om_num_err_sig_, 1);
	useom[52] = 0;
	useom[53] = 0;
	useom[54] = 0;
	useom[55] = 0;
	useom[60] = 0;
	useom[61] = 0;

	string region = model.substr(0, 1);

	if( "AA" == model02 )
		requirePersistChina = true;
	if( "US" == model02 )
		tm_bin_sample_freq = 600;

	if( model.substr(0, 1) == "E" )
	{
		mCode = "E";
		excludeBlockTrade = true;
		allowNegSizeTo = 20190901;
	}
	else if( model.substr(0, 2) == "US" )
		mCode = "U";
	else if( model.substr(0, 2) == "UF" )
		mCode = "F";
	else if( model.substr(0, 2) == "UE" )
		mCode = "X";
	else if( model.substr(0, 1) == "C" )
		mCode = "J";
	else if( "KR" == model02 )
		mCode = "K";

	if( model.substr(0, 2) == "US" )
		minNrowsParam = 2650;
	else if( model.substr(0, 2) == "UF" )
		minNrowsParam = 150;
	else if( model.substr(0, 1) == "E" )
		minNrowsParam = 500;
	else if( model.substr(0, 1) == "C" )
		minNrowsParam = 3500;
	else if( "MJ" == model02 || "SS" == model02 )
		minNrowsParam = 50;

	if( "US" == model02 || "UF" == model02 || "CA" == model02 || "E" == model.substr(0, 1) )
		allowCross = true;
	if( "U" == model02.substr(0, 1) )
		halt_tickers = true;

	// 20190529, noLin worldwide
	nTreesOmFit = 60;
	nTreesOmProd = 60;
	if("CA" == model02)
	{
		nTreesOmFit = 30;
		nTreesOmProd = 30;
	}
	else if("AH" == model02 || "AA" == model02)
	{
		nTreesOmFit = 120;
		nTreesOmProd = 120;
	}
	//nTreesOmFit = 30;
	//nTreesOmProd = 30;
	//if( "US" == model02 )
	//{
	//	nTreesOmFit = 60; // noLin
	//	nTreesOmProd = 60;
	//}
	//else if( "UF" == model02 )
	//{
	//	nTreesOmFit = 60; // noLin
	//	nTreesOmProd = 60;
	//}
	//else if( "AT" == model02 )
	//{
	//	nTreesOmFit = 50; // noIndex, 20180406
	//	nTreesOmProd = 50;
	//}
	//else if( "KR" == model02 )
	//{
	//	nTreesOmFit = 50; // noIndex, 20180406
	//	nTreesOmProd = 50;
	//}
}

void LinearModel::setUseMoreCAEcns()
{
	useMoreCAEcns = true;
}

void LinearModel::addHorizon(int horizon, int lag)
{
	vHorizonLag.push_back(make_pair(horizon, lag));
	++nHorizon;
}

void LinearModel::set_idate(int idate)
{
	openMsecs = mto::msecOpen(market, idate);
	closeMsecs = mto::msecClose(market, idate);
	n1sec = (closeMsecs - openMsecs) / 1000 + 1;
	gpts = (n1sec - 1) / gridInterval + 1;
}

vector<string> LinearModel::get_ecns(int idate) const
{
	vector<string> eu_ecns {"ET", "EG", "EH"};

	vector<string> ca_ecns {"CC"};
	if(idate >= 20180312)
		ca_ecns.push_back("CE");
	if(idate >= 20180525)
		ca_ecns.push_back("CD");
	// Alpha shutdown. 20150921.
	//if( udate < 20150921 )
		//ca_ecns.push_back("CH");
	//if( useMoreCAEcns )
	//{
	//	ca_ecns.push_back("CP");
	//	ca_ecns.push_back("CO");
	//}
	//if( idate < 20130402 ) // Pure is taken out since 20130402.
		//ca_ecns.push_back("CP");

	vector<string> us_dests {"UP", "UQ", "UN", "UC", "UD", "UJ", "UB", "UY"};
	if( use_psx )
		us_dests.push_back("UX");

	vector<string> ret;
	// primaryOnly
	if(!primary_only)
	{
		if( model02[0] == 'E' )
			ret = eu_ecns;
		else if( "CA" == model02 )
			ret = ca_ecns;
		else if( "US" == model02 || "UF" == model02 )
			ret = us_dests;
		else if( model.size() > 2 && model.substr(0, 3) == "ASX" )
			ret.push_back("AX");
	}

	return ret;
}

int LinearModel::minPts(const string& model) const
{
	int minPts = 0.4 * om_err_fit_days * (n1sec - 1) / 30;

	string model02 = model.substr(0, 2);
	if( "AT" == model02 || "AH" == model02 || "AG" == model02 || "AA" == model02 )
	{
		// lunch time adjust.
		if( "AT" == model02 )
			minPts *= 4.5 / 6.;
		else if( "AH" == model02 )
			minPts *= 4. / 5.5;
		else if( "AG" == model02 )
			minPts *= 6.5 / 8.;
		else if( "AA" == model02 )
			minPts *= 4. / 5.5;

		// half.
		minPts *= 0.5;
	}
	//else if( "EU" == model02 )
	else if( model02[0] == 'E' )
		minPts *= 6.5 / 8.5;
	else if( "SS" == model02 )
		minPts *= 6.5 / (6.0 + 55./60.);
	return minPts;
}

int LinearModel::minPtsTm(const string& model) const
{
	int minPtsTm = 0.4 * om_err_fit_days * (n1sec - 1) / 300;

	string model02 = model.substr(0, 2);
	if( "AT" == model02 || "AH" == model02 || "AG" == model02 || "AA" == model02 )
	{
		// lunch time adjust.
		if( "AT" == model02 )
			minPtsTm *= 4.5 / 6.;
		else if( "AH" == model02 )
			minPtsTm *= 4. / 5.5;
		else if( "AG" == model02 )
			minPtsTm *= 6.5 / 8.;
		else if( "AA" == model02 )
			minPtsTm *= 4. / 5.5;

		// half.
		minPtsTm *= 0.5;
	}
	//else if( "EU" == model02 )
	else if( model02[0] == 'E' )
		minPtsTm *= 6.5 / 8.5;
	else if( "SS" == model02 )
		minPtsTm *= 6.5 / (6.0 + 55./60.);
	return minPtsTm;
}

NonLinearModel::NonLinearModel(const string& model, const LinearModel& linMod)
	:maxHorizon(0),
	nHorizon(0),
	nTreesTmFit(0),
	nTreesTmProd(0),
	nTrees71ClFit(30),
	nTrees71ClProd(30),
	nTreesClclFit(10),
	nTreesClclProd(5),
	nTreesOpclFit(10),
	nTreesOpclProd(10)
{
	vHorizonLag = linMod.vHorizonLag;
	nHorizon = vHorizonLag.size();

	for( vector<pair<int, int> >::iterator it = vHorizonLag.begin(); it != vHorizonLag.end(); ++it )
		if( it->first + it->second > maxHorizon )
			maxHorizon = it->first + it->second;

	nTreesTmFit = 0;
	nTreesTmProd = 0;
	string model02 = model.substr(0, 2);
	if( "AS" == model02 )
	{
		nTreesTmFit = 45;
		nTreesTmProd = 45;
	}
	else if( "US" == model02 )
	{
		nTreesTmFit = 125; // 20170523: 500d fit. 100->125.
		nTreesTmProd = 125;
	}
	else if( "UF" == model02 )
	{
		nTreesTmFit = 65;
		nTreesTmProd = 65; // 20170530: 500d fit. 45->65.
	}
	else if( "C" == model.substr(0, 1) )
	{
		nTreesTmFit = 65;
		nTreesTmProd = 65; // 20170530: 500d fit. 45->65.
	}
	//else if( "EU" == model02 )
	else if( model02[0] == 'E' )
	{
		nTreesTmFit = 125;
		nTreesTmProd = 70; // 20170530: 500d fit. 65->125. 202002: 125->70
	}
	else if( "AT" == model02 )
	{
		nTreesTmFit = 100;
		nTreesTmProd = 100; // 20170530: 500d fit. 40->100.
	}
	else if( "AH" == model02 )
	{
		nTreesTmFit = 40;
		nTreesTmProd = 40; // 20170530: 500d fit. 40->100. 20170803: back to 40.
	}
	else if( "KR" == model02 )
	{
		nTreesTmFit = 100;
		nTreesTmProd = 100;
	}
	else if( "AA" == model02 )
	{
		nTreesTmFit = 50;
		nTreesTmProd = 30;
	}
	else if( "MJ" == model02 )
	{
		nTreesTmFit = 25;
		nTreesTmProd = 25;
	}
	else if( "SS" == model02 )
	{
		nTreesTmFit = 25;
		nTreesTmProd = 25;
	}
}

void NonLinearModel::addHorizon(int horizon, int lag)
{
	vHorizonLag.push_back(make_pair(horizon, lag));
	++nHorizon;
}

bool CharaContainer::getCharaInfo(const string& market, int idate, const string& uid, CharaInfo& chara) const
{
	auto itm = mMktIdtTkr.find(market);
	if( itm != mMktIdtTkr.end() )
	{
		auto itd = itm->second.find(idate);
		if( itd != itm->second.end() )
		{
			auto itu = itd->second.find(uid);
			if( itu != itd->second.end() )
			{
				chara = itu->second;
				return true;
			}
		}
	}
	return false;
}

void CharaContainer::deleteOlder(const string& market, int idate)
{
	auto& mIdtTkr = mMktIdtTkr[market];
	for( auto it = begin(mIdtTkr); it != end(mIdtTkr); )
	{
		if( it->first < idate )
			mIdtTkr.erase(it++);
		else
			++it;
	}
}

bool CharaContainer::exists(const string& market, int idate)
{
	return mMktIdtTkr[market].count(idate);
}

unordered_map<string, hff::CharaInfo>& CharaContainer::createAndReturn(const string& market, int idate)
{
	mMktIdtTkr[market][idate] = unordered_map<string, hff::CharaInfo>();
	return mMktIdtTkr[market][idate];
}

StateInfo::StateInfo(const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	init_vars();

	sig1 = vector<float>(hff::om_num_basic_sig_);
	sig10 = vector<float>(hff::tm_num_basic_sig_);
	sigBook = vector<float>(hff::max_book_sigs_);
	target = vector<float>(nonLinMod.nHorizon);
	targetUH = vector<float>(nonLinMod.nHorizon);
	targetXsprd = vector<float>(nonLinMod.nHorizon);
	targetXsprdUH = vector<float>(nonLinMod.nHorizon);
	pred = vector<float>(nonLinMod.nHorizon);
	predIndex = vector<float>(linMod.nHorizon);
	sigIndex1 = vector<float>(linMod.nHorizon);
	sigIndex2 = vector<float>(linMod.nHorizon);
	sigIndex3 = vector<float>(linMod.nHorizon);
	sigIndex4 = vector<float>(linMod.nHorizon);
	sigIndex5 = vector<float>(linMod.nHorizon);
	sigIndex6 = vector<float>(linMod.nHorizon);
	predErr = vector<float>(nonLinMod.nHorizon);
	predVar = vector<float>(linMod.nHorizon);
	om = vector<float>(hff::om_num_sig_);
	omErr = vector<float>(hff::om_num_err_sig_);
	targetErr = vector<float>(nonLinMod.nHorizon);
	tm = vector<float>(hff::tm_num_sig_);
}

void StateInfo::init(const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	init_vars();
	init_arrays(linMod, nonLinMod);
}

void StateInfo::init_vars()
{
	msso = 0;
	mstc = 0;
	timeFrac = 0.;
	mqut = 0.;
	absSprd = 0.;
	sprd = 0.;
	costBid = 0.;
	costAsk = 0.;
	bid = 0.;
	ask = 0.;
	bsize = 0.;
	asize = 0.;
	bidEx = 0;
	askEx = 0;
	maxbsize = 0;
	maxasize = 0;
	maxbsize2 = 0;
	maxasize2 = 0;
	mretp5 = 0.;
	mret1 = 0.;
	mret5 = 0.;
	mret15 = 0.;
	mret30 = 0.;
	mret60 = 0.;
	mret120 = 0.;
	mret300 = 0.;
	mret600 = 0.;
	mret1200 = 0.;
	mret2400 = 0.;
	mret4800 = 0.;
	mret300L = 0.;
	mret600L = 0.;
	mret1200L = 0.;
	mret2400L = 0.;
	mret4800L = 0.;
	//selfInt60L60 = 0.;
	//selfInt60L120 = 0.;
	//selfInt60L300 = 0.;
	//selfInt60L600 = 0.;
	//selfInt60L1200 = 0.;
	//selfInt300L300 = 0.;
	//selfInt300L600 = 0.;
	//selfInt300L1200 = 0.;
	//selfInt300L2400 = 0.;
	//cwret15 = 0.;
	cwret30 = 0.;
	cwret60 = 0.;
	cwret120 = 0.;
	cwret300 = 0.;
	cwret600 = 0.;
	cwret300L = 0.;
	//vwret15 = 0.;
	swret30 = 0.;
	swret60 = 0.;
	swret120 = 0.;
	swret300 = 0.;
	swret600 = 0.;
	swret300L = 0.;
	//voluMom5 = 0.;
	voluMom15 = 0.;
	voluMom30 = 0.;
	voluMom60 = 0.;
	voluMom120 = 0.;
	voluMom300 = 0.;
	voluMom600 = 0.;
	voluMom3600 = 0.;
	mretOpen = 0.;
	quimMax2 = 0.;
	intraVoluMom = 0.;
	relativeHiLo = 0.;
	//midCwap = 0.;
	//midVwap = 0.;
	//midCwap600 = 0.;
	//midVwap600 = 0.;
	mI600 = 0.;
	mI3600 = 0.;
	mIIntra = 0.;
	//mI5 = 0.;
	//mI15 = 0.;
	//mI30 = 0.;
	//mI60 = 0.;
	//mI120 = 0.;
	//mI300 = 0.;
	ltrade = 0.;
	//qIm5 = 0.;
	hiloLag1 = 0.;
	hiloLag2 = 0.;
	hilo900 = 0.;
	bollinger300 = 0.;
	bollinger900 = 0.;
	bollinger1800 = 0.;
	bollinger3600 = 0.;
	logBlockVolUsd60 = 0.;
	logBlockVolUsd3600 = 0.;
	logBlockVolUsdIntra = 0.;
	//sdTrd = 0.;
	//sdTrdNorm = 0.;
	volSclBsz = 0.;
	volSclAsz = 0.;
	rtrd = 0.;
	rPastTrd = 0.;
	rCurTrd = 0.;
	rtrd60 = 0.;
	rtrd120 = 0.;
	rtrd3600 = 0.;
	rtrd7200 = 0.;
	rtrdOverSprd = 0.;
	mrtrd = 0.;
	mrtrd60 = 0.;
	mrtrd120 = 0.;
	mrtrd3600 = 0.;
	mrtrd7200 = 0.;
	dnmk = 0.;
	snmk = 0.;
	moP = 0.;
	moA = 0.;
	bidOffA = 0.;
	askOffA = 0.;
	quimP = 0.;
	quimA = 0.;
	tsinceq = 0;
	tsincet = 0;
	isExp = 0.;
	tsincen = 24*60*60*1000.;
	tlastq = 0;
	tlastt = 0;

	//persist1 = 0.;
	//persist2 = 0.;
	//persist5 = 0.;
	//persist10 = 0.;
	//persist20 = 0.;
	//persist50 = 0.;
	//persist100 = 0.;
	//persist200 = 0.;
	//persist500 = 0.;
	//persist1000 = 0.;

	//fIm1 = 0.;
	//fIm2 = 0.;
	//fillImbTm10 = 0.;
	//fillImbTm5 = 0.;
	//fillImbT = 0.;
	//fillImbTp1 = 0.;
	eventTakeRatMkt = 0.;
	eventTakeRatTot = 0.;
	indxPredWS1 = 0.;
	indxPredWS2 = 0.;
	indxPred1Adj = 0.;
	indxPred2Adj = 0.;
	indxPred1Sprd = 0.;
	indxPred2Sprd = 0.;
	//news = 0.;
	//newsSenti = 0.;
	//newsVol = 0.;
	//dswmdsTop = 0.;
	//dswmds = 0.;
	//dswmdp = 0.;
	bidDepth900 = 0.;
	askDepth900 = 0.;
	bestPostTrade = 0.;
	//sipDiffMid = -99999.;
	//sipDiffSide = -99999.;
	isTmSample = false;
	validq = 0;
	validt = 0;
	valid = 0;
	validTm = 0;
	tobOK = 0;
	sampleType = 0;
	bidPersists = 1;
	askPersists = 1;
	//persist1 = 1;
	//persist2 = 1;
	//persist5 = 1;
	//persist10 = 1;
	//persist20 = 1;
	//persist50 = 1;
	//persist100 = 1;
	//persist200 = 1;
	//persist500 = 1;
	//persist1000 = 1;
	target11Close = 0.;
	target71Close = 0.;
	targetClose = 0.;
	targetNextClose = 0.;
	target11CloseUH = 0.;
	target71CloseUH = 0.;
	targetCloseUH = 0.;
	targetNextCloseUH = 0.;
	predOm = 0.;
	predTm = 0.;
	predOm30s = 0.;
	predOm60s = 0.;
	predOm90s = 0.;
	predOm120s = 0.;
	predOm150s = 0.;
	predOm180s = 0.;
	predOm210s = 0.;
	predOm300s = 0.;
	predOm600s = 0.;
	predTm30s = 0.;
	predTm60s = 0.;
	predTm90s = 0.;
	predTm120s = 0.;
	predTm150s = 0.;
	predTm180s = 0.;
	predTm210s = 0.;
	predTm300s = 0.;
	predTm600s = 0.;
	predTm1200s = 0.;
	predTm2400s = 0.;
	predTm4800s = 0.;
	mret30predOm = 0.;
	mret60predOm = 0.;
	mret120predOm = 0.;
	mret300predOm = 0.;
	mret600predOm = 0.;
	mret120predTm = 0.;
	mret300predTm = 0.;
	mret600predTm = 0.;
	mret1200predTm = 0.;
	mret2400predTm = 0.;
	mret4800predTm = 0.;
	mret120predTm1 = 0.;
	mret300predTm1 = 0.;
	mret600predTm1 = 0.;
	mret1200predTm1 = 0.;
	mret2400predTm1 = 0.;
	mret4800predTm1 = 0.;
	predOmIt1 = 0.;
	predOm30sIt1 = 0.;
	predOm60sIt1 = 0.;
	predOm90sIt1 = 0.;
	predOm120sIt1 = 0.;
	predOm150sIt1 = 0.;
	predOm180sIt1 = 0.;
	predOm210sIt1 = 0.;
	predOm300sIt1 = 0.;
	mret30predOmIt1 = 0.;
	mret60predOmIt1 = 0.;
	mret120predOmIt1 = 0.;
	pr1 = 0.;
	pr1err = 0.;
	bmpred = 0.;
	gptOK = 0;
	persistentChina = false;
	relVolat = 0.;
	relSprd = 0.;
	ltrdImb = 0.;
	ltrdPrc = 0.;
}

void StateInfo::init_arrays(const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	fill(sig1.begin(), sig1.end(), 0.);
	fill(sig10.begin(), sig10.end(), 0.);
	fill(sigBook.begin(), sigBook.end(), 0.);
	fill(target.begin(), target.end(), 0.);
	fill(targetUH.begin(), targetUH.end(), 0.);
	fill(targetXsprd.begin(), targetXsprd.end(), 0.);
	fill(targetXsprdUH.begin(), targetXsprdUH.end(), 0.);
	fill(pred.begin(), pred.end(), 0.);
	fill(predIndex.begin(), predIndex.end(), 0.);
	fill(sigIndex1.begin(), sigIndex1.end(), 0.);
	fill(sigIndex2.begin(), sigIndex2.end(), 0.);
	fill(sigIndex3.begin(), sigIndex3.end(), 0.);
	fill(sigIndex4.begin(), sigIndex4.end(), 0.);
	fill(sigIndex5.begin(), sigIndex5.end(), 0.);
	fill(sigIndex6.begin(), sigIndex6.end(), 0.);
	fill(predErr.begin(), predErr.end(), 0.);
	fill(predVar.begin(), predVar.end(), 0.);
	fill(om.begin(), om.end(), 0.);
	fill(omErr.begin(), omErr.end(), 0.);
	fill(targetErr.begin(), targetErr.end(), 0.);
	fill(tm.begin(), tm.end(), 0.);
}

SigC::SigC()
{
	init_vars();
}

SigC::SigC(const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	init_vars();

	int maxGpts = linMod.gpts;
	if( linMod.market == "AH" || linMod.market == "SS" )
		maxGpts += (30 * 60) / linMod.gridInterval;
	sI = vector<StateInfo>(maxGpts, StateInfo(linMod, nonLinMod));
}

SigC::~SigC()
{
	vector<StateInfo>().swap(sI);
}

void SigC::init(const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	init_vars();
	init_si(linMod, nonLinMod);
}

void SigC::init(const LinearModel& linMod, const NonLinearModel& nonLinMod, int n)
{
	init_vars();
	init_si(linMod, nonLinMod, n);
}

void SigC::init(const LinearModel& linMod, const NonLinearModel& nonLinMod, const vector<int>& vMsso)
{
	init_vars();
	init_si(linMod, nonLinMod, vMsso);
}

void SigC::init_vars()
{
	ticker = "";
	logCap = 0.;
	logShr = 0.;
	avgDlyVolume = 0.;
	avgDlyVolat = 0.;
	adjPrevClose = 0.;
	close = 0.;
	medSprd = 0.;
	medNquotes = 0.;
	medNtrades = 0.;
	medNbooktrades = 0.;
	prevDayVolume = 0.;
	prevDayVolat = 0.;
	mretIntraLag1 = 0.;
	mretIntraLag2 = 0.;
	mretIntraLag3 = 0.;
	mretIntraLag4 = 0.;
	mretIntraLag5 = 0.;
	mretIntraLag6 = 0.;
	mretIntraLag7 = 0.;
	mretIntraLag8 = 0.;
	cwretIntraLag1 = 0.;
	cwretIntraLag2 = 0.;
	mretIntraLag3Sprd = 0.;
	mretIntraLag4Sprd = 0.;
	mretIntraLag5Sprd = 0.;
	hiloQAI = 0.;
	hiloQAI2 = 0.;
	hiloQAI3 = 0.;
	hiloQAI4 = 0.;
	hiloQAI5 = 0.;
	hiloQAI6 = 0.;
	hiloQAI7 = 0.;
	hiloQAI8 = 0.;
	hiloQAI3Sprd = 0.;
	hiloQAI4Sprd = 0.;
	hiloQAI5Sprd = 0.;
	hiloLag1Open = 0.;
	hiloLag2Open = 0.;
	hiloLag3Open = 0.;
	hiloLag4Open = 0.;
	hiloLag5Open = 0.;
	hiloLag6Open = 0.;
	hiloLag7Open = 0.;
	hiloLag8Open = 0.;
	hiloLag3OpenSprd = 0.;
	hiloLag4OpenSprd = 0.;
	hiloLag5OpenSprd = 0.;
	hiloLag1Rat = 0.;
	hiloLag2Rat = 0.;
	hiloLag3Rat = 0.;
	hiloLag4Rat = 0.;
	hiloLag5Rat = 0.;
	hiloLag6Rat = 0.;
	hiloLag7Rat = 0.;
	hiloLag8Rat = 0.;
	hiloLag3RatSprd = 0.;
	hiloLag4RatSprd = 0.;
	hiloLag5RatSprd = 0.;
	mretONLag0 = 0.;
	mretONLag1 = 0.;
	mretONLag2 = 0.;
	mretONLag3 = 0.;
	mretONLag4 = 0.;
	mretONLag5 = 0.;
	mretONLag6 = 0.;
	mretONLag7 = 0.;
	cwretONLag1 = 0.;
	mretONLag2Sprd = 0.;
	mretONLag3Sprd = 0.;
	mretONLag4Sprd = 0.;
	hiLag1 = 0.;
	loLag1 = 0.;
	hiLag2 = 0.;
	loLag2 = 0.;
	closeNextCloseRet = 0.;
	firstMidQt = 0.;
	firstQtGpt = 0;
	sprdOpen = 0.;
	fxRate = 1.;
	logVolu = 0.;
	logPrice = 0.;
	logPriceUsd = 0.;
	logMedSprd = 0.;
	northRST = 0.;
	northTRD = 0.;
	northBP = 0.;
	isSecTypeF = 0.;
	tarMax = 0.;
	tarMaxuh = 0.;
	tarON = 0.;
	tarONuh = 0.;
	tarClcl = 0.;
	tarClcluh = 0.;
	exchangeChar = "";
	sectype = "";
	exchange = 0.;
	isSecTypeF = 0.;
}

void SigC::init_si(const LinearModel& linMod, const NonLinearModel& nonLinMod)
{
	if( sI.size() != linMod.gpts )
		sI.resize(linMod.gpts, StateInfo(linMod, nonLinMod));

	for( vector<StateInfo>::iterator it = sI.begin(); it != sI.end(); ++it )
		it->init(linMod, nonLinMod);
}

void SigC::init_si(const LinearModel& linMod, const NonLinearModel& nonLinMod, int n)
{
	sI.resize(n, StateInfo(linMod, nonLinMod));
	for( vector<StateInfo>::iterator it = sI.begin(); it != sI.end(); ++it )
		it->init(linMod, nonLinMod);
}

void SigC::init_si(const LinearModel& linMod, const NonLinearModel& nonLinMod, const vector<int>& vMsso)
{
	sI.resize(vMsso.size(), StateInfo(linMod, nonLinMod));
	int indx = 0;
	for( vector<StateInfo>::iterator it = sI.begin(); it != sI.end(); ++it, ++indx )
	{
		it->init(linMod, nonLinMod);
		it->msso = vMsso[indx];
		it->mstc = linMod.closeMsecs - linMod.openMsecs + it->msso;
	}
}

StateInfoH::StateInfoH(const StateInfo& sI)
	:gptOK(sI.gptOK),
	target(sI.target),
	target11Close(sI.target11Close),
	target71Close(sI.target71Close),
	targetClose(sI.targetClose),
	targetNextClose(sI.targetNextClose)
{
}

SigH::SigH(const string& ticker_, int inUnivFit_, int n1sec_, int nTargets)
	:ticker(ticker_),
	northBP(0.),
	northTRD(0.),
	northRST(0.),
	inUnivFit(inUnivFit_)
{
	sIH = vector<StateInfoH>(n1sec_);
	for( auto& s : sIH )
	{
		s.target = vector<float>(nTargets, 0.);
		s.gptOK = 0;
	}
}

SigH::SigH(const SigC& sig, const string& ticker_)
	:ticker(ticker_),
	northBP(0.),
	northTRD(0.),
	northRST(0.),
	tarON(sig.tarON),
	//tarMax(sig.tarMax),
	tarClcl(sig.tarClcl),
	inUnivFit(sig.inUnivFit)
{
	int N = sig.sI.size();
	sIH = vector<StateInfoH>(N);
	for( int k = 0; k < N; ++k )
		sIH[k] = StateInfoH(sig.sI[k]);
}

float SigH::get_hedged_target(int sec, int len, int lag, bool debug) const
{
	float ret = 0.;
	int iSecMax = sIH.size();
	for( int iSec = sec + lag; iSec < sec + len + lag && iSec < iSecMax - 1; ++iSec )
	{
		ret += sIH[iSec].target[0];
		if( debug )
			printf("%d %.4f\n", iSec, sIH[iSec].target[0]);
	}
	return ret;
}

bool SigHLess::operator()(const SigH& lhs, const SigH& rhs) const
{
	if( lhs.ticker < rhs.ticker )
		return true;
	return false;
}

void SignalLabel::print(ostream& os)
{
	vector<string>::iterator itBeg = labels.begin();
	for( vector<string>::iterator it = itBeg; it != labels.end(); ++it )
	{
		if( it != itBeg )
			os << "\t";
		os << *it;
	}
	os << "\t";
}

void SignalContent::print(ostream& os)
{
	char buf[100];
	vector<float>::iterator vBeg = v.begin();
	for( vector<float>::iterator it = vBeg; it != v.end(); ++it )
	{
		if( it != vBeg )
			os << "\t";
		sprintf(buf, "%.6f", *it);
		os << buf;
	}
	os << "\n";
}

} // namespace hff

ostream& operator <<(ostream& os, const hff::SignalLabel& obj)
{
	os.write( (char*)&obj.nrows, sizeof(int) );
	os.write( (char*)&obj.ncols, sizeof(int) );

	int len = 0;
	for( vector<string>::const_iterator it = obj.labels.begin(); it != obj.labels.end(); ++it )
		len += it->size();
	os.write( (char*)&len, sizeof(int) );

	vector<string>::const_iterator itBeg = obj.labels.begin();
	char comma = ',';
	for( vector<string>::const_iterator it = itBeg; it != obj.labels.end(); ++it )
	{
		char clabel[100];
		sprintf(clabel, "%s", it->c_str());
		int lsize = it->size();
		if( it != itBeg )
			os.write( (char*)&comma, sizeof(char) );
		os.write( (char*)&clabel, lsize * sizeof(char) );
	}

	return os;
}

ostream& operator <<(ostream& os, const hff::SignalContent& obj)
{
	for( vector<float>::const_iterator it = obj.v.begin(); it != obj.v.end(); ++it )
	{
		float v = *it;
		os.write( (char*)&v, sizeof(float) );
	}
	return os;
}

istream& operator >>(istream& is, hff::SignalLabel& obj)
{
	is.read( (char*)&obj.nrows, sizeof(int) );
	is.read( (char*)&obj.ncols, sizeof(int) );

	long long int len = 0;
	is.read( (char*)&len, sizeof(len) );

	vector<char> str;
	str.resize(len);
	is.read(&str[0], len);

	obj.labels.clear();
	vector<char>::iterator itFrom = str.begin();
	vector<char>::iterator itEnd = str.end();
	vector<char>::iterator itTo = itFrom;
	while( itTo != itEnd )
	{
		itTo = find(itFrom, itEnd, ',');
		string label = string(itFrom, itTo).c_str(); // Drop '\0' if exists.
		obj.labels.push_back(label);
		if( itEnd != itTo )
			itFrom = itTo + 1;
	}

	return is;
}

istream& operator >>(istream& is, hff::SignalContent& obj)
{
	obj.v.clear();
	obj.v.resize(obj.ncols);
	is.read( (char*)&obj.v[0], obj.ncols * sizeof(float) );

	return is;
}

istream& operator >>(istream& is, hff::Prediction& obj)
{
	obj.v.clear();
	obj.v.resize(obj.ncols);
	for( int i = 0; i < obj.ncols; ++i )
		is >> obj.v[i];

	return is;
}
