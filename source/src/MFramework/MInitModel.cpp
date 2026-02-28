#include <MFramework/MInitModel.h>
#include <MFramework/MEnv.h>
#include <MSignal/flt.h>
#include <gtlib/util.h>
#include <gtlib_model/pathFtns.h>
#include <gtlib_model/mFtns.h>
#include <jl_lib.h>
#include <optionlibs/TickData.h>
#include <vector>
#include <string>
#include <map>
using namespace std;
using namespace gtlib;

MInitModel::MInitModel(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	verbose_(0),
	cntDay_(0),
	d1_(0),
	d2_(0),
	multiThreadModule_(false),
	multiThreadTicker_(false),
	nMaxThreadTicker_(4),
	elapsed_prev_(0),
	requireDataOK_(true),
	ndatesFront_(0),
	udate_(0),
	ndates_(0),
	noosdates_(0),
	ticker_choice_("univ")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("debugODBC") )
		GODBC::Instance()->set_debug(true);

	// Multithreading
	if( conf.count("multiThreadModule") )
		multiThreadModule_ = conf.find("multiThreadModule")->second == "true";
	if( conf.count("multiThreadTicker") )
		multiThreadTicker_ = conf.find("multiThreadTicker")->second == "true";
	if( conf.count("nMaxThreadTicker") )
		nMaxThreadTicker_ = atoi(conf.find("nMaxThreadTicker")->second.c_str());
	MEnv::Instance()->multiThreadModule = multiThreadModule_;
	MEnv::Instance()->multiThreadTicker = multiThreadTicker_;
	MEnv::Instance()->nMaxThreadTicker = nMaxThreadTicker_;

	if( conf.count("debugFlag") )
		MEnv::Instance()->debugFlag = atoi(conf.find("debugFlag")->second.c_str());
	if( conf.count("excludeDates") )
	{
		pair<mmit, mmit> exRng = conf.equal_range("excludeDates");
		for( mmit mi = exRng.first; mi != exRng.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 1 )
				excludeRange_.push_back(make_pair(atoi(vs[0].c_str()), atoi(vs[0].c_str())));
			else if( vs.size() == 2 )
				excludeRange_.push_back(make_pair(atoi(vs[0].c_str()), atoi(vs[1].c_str())));
		}
	}

	// Model.
	if( conf.count("model") )
		model_ = conf.find("model")->second;
	MEnv::Instance()->model = model_;
	MEnv::Instance()->allIdates = getAllIdates(model_);

	// Markets.
	vector<string> markets = getConfigValuesString(conf, "market");
	if(markets.empty())
		markets = hff::markets(model_);
	MEnv::Instance()->markets = markets;
	if( !markets.empty() )
		MEnv::Instance()->market = markets[0];

	// Base dir.
	string baseDir;
	if( conf.count("baseDir") )
		baseDir = conf.find("baseDir")->second;
	assert(!baseDir.empty());
	MEnv::Instance()->baseDir = baseDir;

	// Conf dir.
	string confDir;
	if( conf.count("confDir") )
		confDir = conf.find("confDir")->second;
	MEnv::Instance()->confDir = confDir;

	if( conf.count("tickerChoice") )
		ticker_choice_ = conf.find("tickerChoice")->second;

	// Looping order.
	MEnv::Instance()->loopingOrder = "dmt";

	if( conf.count("sourceFlag" ) )
		MEnv::Instance()->sourceFlag = conf.find("sourceFlag")->second;
	if( conf.count("fitDesc" ) )
		MEnv::Instance()->fitDesc = conf.find("fitDesc")->second;
	if( conf.count("sigType" ) )
		MEnv::Instance()->sigType = conf.find("sigType")->second;
	if( conf.count("requireDataOK") )
		requireDataOK_ = conf.find("requireDataOK")->second != "false";

	// Tickers debug.
	vector<string> vticker;
	if( conf.count("ticker") )
	{
		pair<mmit, mmit> tickers = conf.equal_range("ticker");
		for( mmit mi = tickers.first; mi != tickers.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			if( vs.size() == 1 )
				vticker.push_back(vs[0]);
		}
		MEnv::Instance()->tickersDebug = vticker;
	}

	if( conf.count("nTickerMax") )
		MEnv::Instance()->nTickerMax = atoi(conf.find("nTickerMax")->second.c_str());

	// Dates.
	if( conf.count("ndatesFront") )
		ndatesFront_ = atoi( conf.find("ndatesFront")->second.c_str() );
	if( conf.count("udate") )
	{
		udate_ = arg_idate( conf.find("udate")->second );
		if(udate_ != 0 && udate_ < 19990000 || udate_ > 30000000)
			exit(38);
	}
	MEnv::Instance()->udate = udate_;

	// Linear Model.
	hff::LinearModel linMod(model_);
	if( conf.count("allowCross") )
		linMod.allowCross = conf.find("allowCross")->second == "true";
	if( conf.count("gridInterval") )
	{
		linMod.gridInterval = atoi(conf.find("gridInterval")->second.c_str());
		linMod.om_bin_sample_freq = linMod.gridInterval;
	}
	if( conf.count("primaryOnly") )
		linMod.primary_only = conf.find("primaryOnly")->second == "true";
	if( conf.count("clipIndex") )
		linMod.clip_index = atof(conf.find("clipIndex")->second.c_str());
	if( conf.count("clipFutIndex") )
		linMod.clip_fut_index = atof(conf.find("clipFutIndex")->second.c_str());
	if( conf.count("clipPredIndex") )
		linMod.clip_pred_index = atof(conf.find("clipPredIndex")->second.c_str());
	if( conf.count("clipRet60") )
		linMod.clip_ret60 = atof(conf.find("clipRet60")->second.c_str());
	if( conf.count("clipRet300") )
		linMod.clip_ret300 = atof(conf.find("clipRet300")->second.c_str());
	if( conf.count("onTargetClip") )
		linMod.on_target_clip = atof(conf.find("onTargetClip")->second.c_str());
	if( conf.count("omTargetClip") )
		linMod.om_target_clip = atof(conf.find("omTargetClip")->second.c_str());
	if( conf.count("tmTargetClip") )
		linMod.tm_target_clip = atof(conf.find("tmTargetClip")->second.c_str());
	if( conf.count("tmTarget60Clip") )
		linMod.tm_target_60_clip = atof(conf.find("tmTarget60Clip")->second.c_str());
	if( conf.count("tmTargetIntraClip") )
		linMod.tm_target_intra_clip = atof(conf.find("tmTargetIntraClip")->second.c_str());
	if( conf.count("haltTickers") )
		linMod.halt_tickers = conf.find("haltTickers")->second == "true";
	if( conf.count("partialVM") )
		linMod.partialVM = conf.find("partialVM")->second == "true";
	if( conf.count("medFromHfuniv") )
		linMod.medFromHfuniv = conf.find("medFromHfuniv")->second == "true";
	if( conf.count("nfFromHfuniv") )
		linMod.nfFromHfuniv = conf.find("nfFromHfuniv")->second == "true";
	if( conf.count("inUnivFromHfuniv") )
		linMod.inUnivFromHfuniv = conf.find("inUnivFromHfuniv")->second == "true";
	if( conf.count("useMoreCAEcns") )
		linMod.setUseMoreCAEcns();
	if( conf.count("requirePersist") )
		linMod.requirePersistChina = conf.find("requirePersist")->second == "true";
	if( conf.count("useEtfFilter") )
		if( conf.find("useEtfFilter")->second == "true" )
			linMod.use_etf_filter = true;
	if( conf.count("useIndexPrediction") )
		if( conf.find("useIndexPrediction")->second == "false" )
			linMod.use_pred_index = false;
	if( conf.count("useom") )
	{
		pair<mmit, mmit> p = conf.equal_range("useom");
		for( mmit mi = p.first; mi != p.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			int vs_size = vs.size();
			if( vs_size == 2 )
			{
				int indx = atoi(vs[0].c_str());
				int val = atoi(vs[1].c_str());
				if( val == 1 || val == 0 )
					linMod.useom[indx] = val;
			}
		}
	}
	if( conf.count("useomErr") )
	{
		pair<mmit, mmit> p = conf.equal_range("useomErr");
		for( mmit mi = p.first; mi != p.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			int vs_size = vs.size();
			if( vs_size == 2 )
			{
				int indx = atoi(vs[0].c_str());
				int val = atoi(vs[1].c_str());
				if( val == 1 || val == 0 )
					linMod.useomErr[indx] = val;
			}
		}
	}
	if( !linMod.use_pred_index )
	{
		for( int i = 0; i < 4; ++i )
		{
			linMod.useom[4 + i * 16] = 0;
			linMod.useom[5 + i * 16] = 0;
			linMod.useom[6 + i * 16] = 0;
			linMod.useom[7 + i * 16] = 0;
			linMod.useom[12 + i * 16] = 0;
			linMod.useom[13 + i * 16] = 0;
		}
		linMod.useomErr[4] = 0;
		linMod.useomErr[5] = 0;
		linMod.useomErr[6] = 0;
		linMod.useomErr[7] = 0;
		linMod.useomErr[12] = 0;
		linMod.useomErr[13] = 0;
	}
	if( conf.count("horizShort") )
	{
		pair<mmit, mmit> horizs = conf.equal_range("horizShort");
		for( mmit mi = horizs.first; mi != horizs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			int vs_size = vs.size();
			int horiz = 0;
			int lag = 0;
			if( vs_size >= 1 )
			{
				horiz = atoi(vs[0].c_str());
				if( vs_size >= 2 )
					lag = atoi(vs[1].c_str());
				linMod.addHorizon(horiz, lag);
			}
		}
	}
	if( conf.count("omUnivFitDays") )
	{
		linMod.om_univ_fit_days = atoi(conf.find("omUnivFitDays")->second.c_str());
		if( linMod.om_univ_fit_days == 0 )
			linMod.num_time_slices = 0;
	}
	if( conf.count("omErrFitDays") )
		linMod.om_err_fit_days = atoi(conf.find("omErrFitDays")->second.c_str());
	if( conf.count("sampleDelay") )
		linMod.sampleDelay = atoi(conf.find("sampleDelay")->second.c_str());
	if( conf.count("omUseTmInput") )
		linMod.om_use_tm_input = conf.find("omUseTmInput")->second == "true";
	if( conf.count("useUSarcaTradeEvent") )
		linMod.use_us_arca_trade_event = conf.find("useUSarcaTradeEvent")->second == "true";
	if( conf.count("transientMsecs") )
		linMod.transientMsecs = atoi(conf.find("transientMsecs")->second.c_str());
	if( conf.count("exploratoryDelay") )
		linMod.exploratoryDelay = atoi(conf.find("exploratoryDelay")->second.c_str());
	if( conf.count("minSpreadMMS") )
		linMod.minSpreadMMS = atoi(conf.find("minSpreadMMS")->second.c_str());
	if( conf.count("maxSpreadMMS") )
		linMod.maxSpreadMMS = atoi(conf.find("maxSpreadMMS")->second.c_str());
	if( conf.count("omBinSampleFreq") )
		linMod.om_bin_sample_freq = atoi(conf.find("omBinSampleFreq")->second.c_str());
	if( conf.count("tmBinSampleFreq") )
		linMod.tm_bin_sample_freq = atoi(conf.find("tmBinSampleFreq")->second.c_str());
	if( conf.count("usePsx") && conf.find("usePsx")->second == "true" )
		linMod.use_psx = true;
	MEnv::Instance()->linearModel = linMod;

	// NonLinear Model.
	hff::NonLinearModel nonLinMod(model_, MEnv::Instance()->linearModel);
	if( conf.count("horizLong") )
	{
		pair<mmit, mmit> horizs = conf.equal_range("horizLong");
		for( mmit mi = horizs.first; mi != horizs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			int vs_size = vs.size();
			int horiz = 0;
			int lag = 0;
			if( vs_size >= 1 )
			{
				horiz = atoi(vs[0].c_str());
				if( vs_size >= 2 )
					lag = atoi(vs[1].c_str());
				nonLinMod.addHorizon(horiz, lag);
			}
		}
	}
	MEnv::Instance()->nonLinearModel = nonLinMod;

	// Filters.
	hff::IndexFilters filters(model_, linMod.clip_index, linMod.clip_fut_index, linMod.use_etf_filter);
	if( conf.count("horizShort") )
	{
		pair<mmit, mmit> horizs = conf.equal_range("horizShort");
		for( mmit mi = horizs.first; mi != horizs.second; ++mi )
		{
			vector<string> vs = split(mi->second);
			int vs_size = vs.size();
			int horiz = 0;
			int lag = 0;
			if( vs_size >= 1 )
			{
				horiz = atoi(vs[0].c_str());
				//if( horiz != 60 )
				{
					if( vs_size >= 2 )
						lag = atoi(vs[1].c_str());
					filters.addHorizon(horiz, lag);
				}
			}
		}
	}
	MEnv::Instance()->indexFilters = filters;

	// Dates autodetect.
	if( conf.count("dateFrom") && conf.count("dateTo") )
	{
		d1_ = arg_idate( conf.find("dateFrom")->second );
		d2_ = arg_idate( conf.find("dateTo")->second );
		if( d1_ < 20000000 || d2_ > 21000000 || d2_ < d1_ )
		{
			cerr << "Invalid date range " << d1_ << ' ' << d2_ << endl;
			exit(21);
		}
	}
	if( conf.count("dateAutoDetect") )
	{
		string dateAutoDetect = conf.find("dateAutoDetect")->second;
		d1_ = getFirstDayToWrite(linMod.market, dateAutoDetect);
		d2_ = getFinalSigDay(linMod.market);

		cout << d1_ << '\t' << d2_ << endl;
		if( d1_ > d2_ )
			exit(0);
	}
	if( conf.count("useRecentDates") && conf.find("useRecentDates")->second == "true" )
	{
		d2_ = getFinalSigDay(linMod.market);
		d1_ = d2_;
	}
	if( conf.count("ndates") )
		ndates_ = atoi( conf.find("ndates")->second.c_str() );
	if( conf.count("noosdates") )
		noosdates_ = atoi( conf.find("noosdates")->second.c_str() );
}

bool MInitModel::isExcludeDate(int idate)
{
	for(auto datePair : excludeRange_)
	{
		if(idate >= datePair.first && idate <= datePair.second)
			return true;
	}
	return false;
}

int MInitModel::getFirstDayToWrite(const string& market, const string& type)
{
	int nextToWrite = 0;
	if( type == "flt" )
	{
		string dir = flt::get_filter_dir(MEnv::Instance()->indexFilters.filters[0], model_);
		nextToWrite = getIdateToBeginWith(dir, market);
	}
	else if( type == "cov" )
	{
		string dir = get_cov_dir(MEnv::Instance()->baseDir, model_);
		int nPast = 61;
		vector<int> idateRange = vector<int>(MEnv::Instance()->allIdates.end() - nPast, MEnv::Instance()->allIdates.end());
		nextToWrite = getIdateToBeginWith(dir, market, idateRange);
	}
	else if( type.size() >= 2 && type[1] == 'm' ) // om, tm, ome
	{
		string sigType = type.substr(0, 2);
		bool isEvt = type.size() == 3 && type[2] == 'e';
		string fitDesc = isEvt ? "tevt" : "reg";
		string dir = get_sig_dir(MEnv::Instance()->baseDir, model_, sigType, fitDesc, "bin");
		nextToWrite = getIdateToBeginWith(dir, market);
	}
	else if( type.size() >= 3 && type[0] == 'p' && type[2] == 'm' ) // pom, ptm, pome
	{
		string sigType = type.substr(1, 2);
		bool isEvt = type.size() == 4 && type[3] == 'e';
		string fitDesc = isEvt ? "tevt" : "reg";
		string dir = get_pred_dir(MEnv::Instance()->baseDir, model_, itoday(), getTargetName(sigType, fitDesc), fitDesc, 0);
		nextToWrite = getIdateToBeginWithPred(dir, market);
	}
	return nextToWrite;
}

MInitModel::~MInitModel()
{
}

void MInitModel::beginJob()
{
	cout << getTimerInfoSimple() << endl;

	if( requireDataOK_ )
		set_idate_list_ok();
	else
		set_idate_list_trade();

	set_ticker_list();
	set_uid_list();

	return;
}

void MInitModel::beginDay()
{
	++cntDay_;
	int idate = MEnv::Instance()->idate;
	MEnv::Instance()->linearModel.set_idate(idate);
	if( verbose_ > 0 )
	{
		char buf[200];
		sprintf(buf, "\nMInitModel::beginDay() %d %d ", cntDay_, idate);
		cout << buf;
		cout << getMemoryInfoSimple() << ' ' << getTimerInfoSimple() << endl;
	}
}

void MInitModel::beginMarket()
{
	cntDay_ = 0;
	string market = MEnv::Instance()->market;
	if( marketTickers_.count(market) )
		MEnv::Instance()->tickers = marketTickers_[market];

	if( verbose_ > 2 )
	{
		vector<string>& tickers = MEnv::Instance()->tickers;
		int nT = tickers.size();
		printf("MInitModel::beginMarket() %s", market.c_str());
		if( nT > 0 )
		{
			printf(", %d tickers:", nT);
			int n = (nT > 3)? 3: nT;
			for( int i = 0; i < n; ++i )
				printf(" %s", tickers[i].c_str());
			if( nT > 3 )
				printf(", ...");
		}
		cout << endl;
	}
}

void MInitModel::beginTicker(const string& ticker)
{
}

void MInitModel::endTicker(const string& ticker)
{
}

void MInitModel::endMarket()
{
}

void MInitModel::endDay()
{
	int idate = MEnv::Instance()->idate;
}

void MInitModel::endJob()
{
	MEvent::Instance()->remove<map<string, vector<string> > >("", "marketTickers");
	cout << "MInitModel::endJob() " << getMemoryInfoSimple() << ' ' << getTimerInfoSimple() << endl;
}

int MInitModel::arg_idate(const string& sdate)
{
	int ret = 0;
	if( sdate == "today" )
		ret = itoday();
	else if( !sdate.empty() )
	{
		char s = sdate[0];
		if( s == '-' || (s >= 48 && s <= 57) ) // minus sign and numbers.
		{
			int n = atoi( sdate.c_str() );
			if( n == 0 || n > 20000000 )
				ret = atoi( sdate.c_str() );
			else // relative date.
			{
				ret = itoday();
				string market = MEnv::Instance()->markets[0];
				if( n > 0 )
				{
					for( int i = 0; i < n; ++i )
						ret = nextClose(market, ret);
				}
				else if( n < 0 )
				{
					for( int i = 0; i < abs(n); ++i )
						ret = prevClose(market, ret);
				}
			}
		}
	}

	return ret;
}

void MInitModel::set_idate_list_ok()
{
	vector<string> markets = MEnv::Instance()->markets;
	string model2 = model_.substr(0, 2);

	if( !markets.empty() )
	{
		vector<int> idates;
		vector<int>& allIdates = MEnv::Instance()->allIdates;
		if( udate_ > 0 && (ndates_ > 0 || noosdates_ > 0) )
		{
			// fit dates.
			{
				vector<int>::iterator itTo = lower_bound(allIdates.begin(), allIdates.end(), udate_);
				vector<int>::iterator itFrom = itTo;
				int maxN = distance(allIdates.begin(), itFrom);
				if( ndates_ > 0 )
				{
					int temp_ndates = ndates_ + ndatesFront_;
					if( temp_ndates <= maxN )
					{
						advance(itFrom, -temp_ndates);
						for( ; itFrom != itTo; ++itFrom )
						{
							if(isExcludeDate(*itFrom))
								continue;
							else
								idates.push_back(*itFrom);
						}
					}
					else
					{
						cerr << "MInitModel::beginJob() Not enough fit dates.\n";
						exit(4);
					}
					cout << "Fit from " << idates[0] << " to " << idates[idates.size() - 1] << endl;
				}
			}

			// oos dates.
			{
				vector<int>::iterator itFrom = lower_bound(allIdates.begin(), allIdates.end(), udate_);
				vector<int>::iterator itTo = itFrom;
				int maxNoos = distance(itFrom, allIdates.end());
				if( noosdates_ > 0 )
				{
					if( noosdates_ <= maxNoos )
					{
						advance(itTo, noosdates_);
						for( ; itFrom != itTo; ++itFrom )
						{
							if(isExcludeDate(*itFrom))
								continue;
							else
								idates.push_back(*itFrom);
						}
					}
					else
					{
						cerr << "MInitModel::beginJob() Not enough oos dates.\n";
						exit(4);
					}
					cout << "OOS to " << idates[idates.size() - 1] << endl;
				}
			}
		}
		else if( d2_ >= d1_ )
		{
			vector<int>::iterator itd = lower_bound(allIdates.begin(), allIdates.end(), d1_);
			int dist = distance(begin(allIdates), itd);
			if(dist < ndatesFront_)
			{
				cerr << "not enought days " << ndatesFront_ - dist << '\n';
				exit(62);
			}
			advance(itd, -ndatesFront_);
			vector<int>::iterator itEnd = upper_bound(allIdates.begin(), allIdates.end(), d2_);
			for( ; itd != itEnd; ++itd )
			{
				if(isExcludeDate(*itd))
					continue;
				else
					idates.push_back(*itd);
			}
			cout << "Dates from " << idates[0] << " to " << idates[idates.size() - 1] << endl;
		}

		sort(idates.begin(), idates.end());
		MEnv::Instance()->idates = idates;
	}
}

void MInitModel::set_idate_list_trade()
{
	set<int> dates;
	vector<string> markets = MEnv::Instance()->markets;
	if( udate_ > 0 && (ndates_ > 0 || noosdates_ > 0) )
	{
		for(auto& market : markets)
		{
			{
				int idate = udate_;
				for(int i = 0; i < ndates_; ++i)
				{
					idate = prevClose(market, idate);
					dates.insert(idate);
				}
			}
			{
				int idate = udate_;
				for(int i = 0; i < noosdates_; ++i)
				{
					dates.insert(idate);
					idate = nextOpen(market, idate);
				}
			}
		}
	}
	else if( d2_ >= d1_ )
	{
		for(auto& market : markets)
		//for( vector<string>::iterator it = markets.begin(); it != markets.end(); ++it )
		{
			//string market = *it;
			{
				int idate = d1_;
				for(int i = 0; i < ndatesFront_; ++i)
				{
					idate = prevClose(market, idate);
					dates.insert(idate);
				}
			}
			{
				int idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(d1_, 040000, mto::tz(market)) ).Date();
				for( ; idate <= d2_; idate = (int)GEX::Instance()->get(market)->NextOpen( QuoteTime(idate, 200000, mto::tz(market)) ).Date() )
					dates.insert(idate);
			}
		}
	}
	vector<int> idates(dates.begin(), dates.end());
	MEnv::Instance()->idates = idates;
}

void MInitModel::set_ticker_list()
{
	const vector<int>& idates = MEnv::Instance()->idates;
	int nDates = idates.size();
	string model2 = model_.substr(0, 2);
	const vector<string>& markets = MEnv::Instance()->markets;
	int nTickerMax = MEnv::Instance()->nTickerMax;

	int idateFrom = get_idate_from(markets[0], idates[0]);
	int idateTo = get_idate_to(markets[0], idates[nDates - 1]);
	marketTickers_ = getMarketTickers(model2, markets, idateFrom, idateTo, nTickerMax);
	//if( nTickerMax > 0 )
		//limitTickerList(nTickerMax);
	MEvent::Instance()->add<map<string, vector<string> > >("", "marketTickers", marketTickers_);
}

void MInitModel::limitTickerList(int nTickerMax)
{
	// Modify marketTickers_ and reduce the number of tickers.
}

void MInitModel::set_uid_list()
{
	const vector<int>& idates = MEnv::Instance()->idates;
	int nDates = idates.size();
	string model2 = model_.substr(0, 2);
	const vector<string>& markets = MEnv::Instance()->markets;
	int idateFrom = get_idate_from(markets[0], idates[0]);
	int idateTo = get_idate_to(markets[0], idates[nDates - 1]);

	set<string> uids;
	if( !idates.empty() )
	{
		char cmd[1000];
		if( "US" == model2 || "CA" == model2 )
		{
			string market = markets[0];
			sprintf( cmd, "select distinct uniqueID from stockcharacteristics "
					" where idate >= %d and idate <= %d "
					" %s "
					" and sectype >= 'A' and sectype <= 'Z' and sectype != 'P' and sectype != 'F' and sectype != 'X' "
					" and uniqueID is not null ",
					idateFrom,
					idateTo,
					sel_univ().c_str() );

			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string uid = trim((*it)[0]);
				if( !uid.empty() )
					uids.insert(uid);
			}
		}
		else if( "UF" == model2 )
		{
			string market = markets[0];
			sprintf( cmd, "select distinct uniqueID from stockcharacteristics "
					" where idate >= %d and idate <= %d "
					" %s "
					" and sectype >= 'A' and sectype <= 'Z' and sectype = 'F' "
					" and uniqueID is not null ",
					idateFrom,
					idateTo,
					sel_univ().c_str() );

			vector<vector<string> > vv;
			GODBC::Instance()->read(mto::hf(market), cmd, vv);

			for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
			{
				string uid = trim((*it)[0]);
				if( !uid.empty() )
					uids.insert(uid);
			}
		}
		else
		{
			for( vector<string>::const_iterator it = markets.begin(); it != markets.end(); ++it )
			{
				string market = *it;
				sprintf( cmd, "select /*distinct*/ uniqueID from stockcharacteristics "
						" where market = '%s' and idate >= %d and idate <= %d %s and uniqueID is not null ",
						mto::code(market).c_str(),
						idateFrom,
						idateTo,
						sel_univ().c_str() );

				vector<vector<string> > vv;
				GODBC::Instance()->read(mto::hf(market), cmd, vv);

				for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
				{
					string uid = trim((*it)[0]);
					if( !uid.empty() )
						uids.insert(uid);
				}
			}
		}
	}
	MEvent::Instance()->add<set<string> >("", "allUids", uids);
}

string MInitModel::sel_univ()
{
	string ret = "";
	string model2 = model_.substr(0, 2);
	if( ticker_choice_ == "univ" )
	{
		if( "US" == model2 || "CA" == model2 )
			ret = " and (inuniverse = 1 or (close_ > .5 and close_ * medvolume > 60000 and medVolatility > .005 and close_ < 900 and medmedsprd >= .00008 and medmedsprd < .04))";
		else
			ret = " and inuniverse = 1 ";
	}
	return ret;
}

int MInitModel::get_idate_from(const string& market, int idate)
{
	for( int i = 0; i < 20; ++i )
		idate = prevClose(market, idate);
	idate = prevClose(market, idate);
	return idate;
}

int MInitModel::get_idate_to(const string& market, int idate)
{
	idate = prevClose(market, idate);
	return idate;
}
