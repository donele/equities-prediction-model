#include <MOrders/MMercatorTrades.h>
#include <MOrders/TradeQuantileInfo.h>
#include <gtlib_signal/QuoteView.h>
#include <jl_lib/MercatorTrade.h>
#include <MFramework/MEvent.h>
#include <MFramework/MEnv.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <map>
#include <list>
#include <string>
using namespace std;
using namespace gtlib;

MMercatorTrades::MMercatorTrades(const string& moduleName, const multimap<string, string>& conf)
	:MModuleBase(moduleName),
	source_(""),
	groupBy_("order"),
	min_price_(0.),
	max_price_(max_double_),
	min_msso_(0),
	max_msso_(0),
	debug_(false),
	debug_tqi_(false),
	readNofill_(false),
	requireValidQuote_(true),
	fillZeros_(false),
	allowCross_(false),
	msecOpen_(0),
	msecClose_(0)
{
	if( conf.count("source") )
		source_ = conf.find("source")->second;
	if( conf.count("minPrice") )
		min_price_ = atof(conf.find("minPrice")->second.c_str());
	if( conf.count("maxPrice") )
		max_price_ = atof(conf.find("maxPrice")->second.c_str());

	if( conf.count("minMsso") )
		min_msso_ = atof(conf.find("minMsso")->second.c_str());
	if( conf.count("maxMsso") )
		max_msso_ = atof(conf.find("maxMsso")->second.c_str());

	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("debug_tqi") )
		debug_tqi_ = conf.find("debug_tqi")->second == "true";
	if( conf.count("readNofill") )
		readNofill_ = conf.find("readNofill")->second == "true";
	if( conf.count("requireValidQuote") )
		requireValidQuote_ = conf.find("requireValidQuote")->second == "true";
	if( conf.count("fillZeros") )
		fillZeros_ = conf.find("fillZeros")->second == "true";

	if( conf.count("univ") )
	{
		pair<mmit, mmit> univs = conf.equal_range("univ");
		for( mmit mi = univs.first; mi != univs.second; ++mi )
		{
			string m = mi->second;
			vector<string> vm = split(m);
			for( vector<string>::iterator it = vm.begin(); it != vm.end(); ++it )
				vUniv_.push_back(*it);
		}
	}
}

MMercatorTrades::~MMercatorTrades()
{}

void MMercatorTrades::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void MMercatorTrades::beginMarket()
{
	return;
}

void MMercatorTrades::beginDay()
{
	string market = MEnv::Instance()->market;
	int idate = MEnv::Instance()->idate;
	orders_ = read_orders(market, idate);
}

void MMercatorTrades::beginMarketDay()
{
	if(debug_tqi_)
	{
		vVolatSurprise30s_.clear();
		vVolatSurprise300s_.clear();
	}

	int idate = MEnv::Instance()->idate;
	string market = MEnv::Instance()->market;
	sessions_ = Sessions(market, idate);
	msecOpen_ = mto::msecOpen(market, idate);
	msecClose_ = mto::msecClose(market, idate);
	if( !vUniv_.empty() )
		select_univ(market, idate);

	mDayVolat_ = readChara(market, idate, "volatility");
	mMedVolat_ = readChara(market, idate, "medvolatility");
	mDayVolume_ = readChara(market, idate, "volume");
	mMedVolume_ = readChara(market, idate, "medvolume");

	vector<string>& tickers = MEnv::Instance()->tickers;
	for(string& ticker : tickers)
		addTrades(ticker);

	TradeQuantileInfo tqi = getTradeQuantileInfo();
	MEvent::Instance()->add<TradeQuantileInfo>("", (string)"quantileInfo", tqi);

	if(debug_tqi_)
	{
		sort(vVolatSurprise30s_.begin(), vVolatSurprise30s_.end());
		sort(vVolatSurprise300s_.begin(), vVolatSurprise300s_.end());
		int nBins = 40;
		int binSize30s = vVolatSurprise30s_.size() / nBins;
		int binSize300s = vVolatSurprise300s_.size() / nBins;

		printf("30sEdges ");
		for(int i = 1; i < nBins; ++i)
			printf("%6.3f ", vVolatSurprise30s_[binSize30s * i]);
		cout << endl;

		printf("300sEdges ");
		for(int i = 1; i < nBins; ++i)
			printf("%6.3f ", vVolatSurprise300s_[binSize300s * i]);
		cout << endl;
	}
}

void MMercatorTrades::beginTicker(const string& ticker)
{
}

void MMercatorTrades::addTrades(const string& ticker)
{
	vector<MercatorTrade> trades = getMtrades(ticker);
	addTickInfo(ticker, trades);
	MEvent::Instance()->add<vector<MercatorTrade> >(ticker, (string)"mtrades", trades);
	return;
}

void MMercatorTrades::endTicker(const string& ticker)
{
	MEvent::Instance()->remove<vector<MercatorTrade> >(ticker, (string)"mtrades");
	return;
}

void MMercatorTrades::endDay()
{
	return;
}

void MMercatorTrades::endMarket()
{
	return;
}

void MMercatorTrades::endJob()
{
	return;
}

vector<MercatorTrade> MMercatorTrades::getMtrades(const string& ticker)
{
	vector<MercatorTrade> v;
	for(auto order : orders_)
	{
		string dbTicker = trim( order[1] );
		string tickerBase = baseTicker(ticker);
		if( tickerBase == dbTicker )
		{
			double price = atof( order[4].c_str() );
			double ms = atof( order[0].c_str() );
			int msso = ms - msecOpen_;
			if( min_price_ <= price && price <= max_price_ && (min_msso_ < msso) && (max_msso_ == 0 || msso < max_msso_) )
			{
				string side = trim( order[2] );
				int sign = 0;
				if( side[0] == 'B' )
					sign = 1;
				else if( side[0] == 'S' )
					sign = -1;
				int qty = atoi( order[3].c_str() );
				double pred1 = atof( order[7].c_str() ) * basis_pts_;
				double pred10 = atof( order[8].c_str() ) * basis_pts_;
				double bid = atof( order[9].c_str() );
				double ask = atof( order[10].c_str() );
				int oqty = atoi( order[11].c_str() );
				double oprice = atof(order[12].c_str());
				double mid = 0.;
				if( bid > 0. && ask > 0. )
					mid = (bid + ask) / 2.;
				if( ms > 1000000 )
				{
					char execType = order[5][0];
					char schedType = atoi(order[6].c_str());
					MercatorTrade mt(ms, sign, price, qty, pred1, pred10, mid, execType,
							schedType, oqty, oprice);
					v.push_back(mt);
				}
			}
		}
	}
	return v;
}

void MMercatorTrades::select_univ(const string& market, int idate)
{
	string sExch = mto::code(market);

	string selMarket = "";
	if( market[0] == 'U' )
		selMarket = " and sectype = '" + sExch + "' ";
	else if( mto::isInternational(market) && !isECN(market) )
		selMarket = " and exchange = '" + sExch + "' ";

	string dbSymbol = "symbol";
	if( mto::isInternational(market) )
		dbSymbol = "exchange + ':' + symbol";

	vector<string> selectedTickers;
	string univs = (string)"(" + vUniv_[0];
	int nUniv = vUniv_.size();
	for( int i = 1; i < nUniv; ++i )
	{
		univs += (string)"," + vUniv_[i];
	}
	univs += ")";

	char cmd[400];
	sprintf(cmd, "select %s from hfuniverse where idate = (select max(idate) from hfuniverse where idate <= %d %s) and inuniverse in %s %s ",
			dbSymbol.c_str(), idate, selMarket.c_str(), univs.c_str(), selMarket.c_str());

	vector<vector<string> > vv;
	GODBC::Instance()->read(mto::hf(market), cmd, vv);
	for( vector<vector<string> >::iterator it = vv.begin(); it != vv.end(); ++it )
		selectedTickers.push_back(trim((*it)[0]));

	sort(selectedTickers.begin(), selectedTickers.end());

	{
		string cmd = (string)" select distinct (" + dbSymbol + ") "
			+ " from hfOrders o "
			+ " where idate = " + itos(idate)
			+ selMarket;
		vector<vector<string> > vvs;
		GODBC::Instance()->read(mto::hfo(market, idate), cmd, vvs);

		tickers_.clear();
		int cnt = 0;
		for( vector<vector<string> >::iterator it = vvs.begin(); it != vvs.end(); ++it )
		{
			string symbol = trim((*it)[0]);
			if( !symbol.empty() )
			{
				if( vUniv_.empty() || binary_search(selectedTickers.begin(), selectedTickers.end(), symbol) )
					tickers_.push_back(symbol);
			}
		}
		sort(tickers_.begin(), tickers_.end());
		MEnv::Instance()->tickers = tickers_;
	}
}

vector<vector<string>> MMercatorTrades::read_orders(const string& market, int idate)
{
	string selMarket = "";
	if( mto::isInternational(market) )
		selMarket = (string)" and e.exchange = " + quote(mto::code(market));

	string selSource = "";
	if( mto::region(market) == "U" && !source_.empty() )
		selSource = (string)" and o.dest = " + itos(static_cast<int>(source_[0]));

	string selFill = " eventType = 2 ";
	if(readNofill_)
		selFill = " eventType >= 1 ";

	string cmd;
	if(groupBy_ == "exec")
	{
		cmd = (string)" select e.eventMsecs, o.symbol "
			+ ", e.side, e.qty, e.price, o.exectype, o.orderschedtype, "
			+ " o.onempred, o.tenmpred, o.bid, o.ask, o.qty, o.price "
			+ " from hfOrderEvents e inner join hfOrders o on e.orderID = o.orderID "
			+ " where "
			+ selFill
			+ " and o.idate = " + itos(idate)
			+ " and e.idate = " + itos(idate)
			+ selSource
			+ selMarket
			+ " order by eventMsecs ";
	}
	else if(groupBy_ == "order")
	{
		cmd = (string)" select o.orderMsecs, o.symbol "
			+ ", e.side, sum(e.qty), sum(e.qty * e.price)/sum(e.qty), o.exectype, o.orderschedtype, "
			+ " min(o.onempred), min(o.tenmpred), min(o.bid), min(o.ask), min(o.qty), min(o.price) "
			+ " from hfOrderEvents e inner join hfOrders o on e.orderID = o.orderID "
			+ " where "
			+ selFill
			+ " and o.idate = " + itos(idate)
			+ " and e.idate = " + itos(idate)
			+ selSource
			+ selMarket
			+ " group by o.orderMsecs, o.symbol, e.side, o.exectype, o.orderschedtype "
			+ " order by o.orderMsecs ";
	}
	vector<vector<string>> orders;
	GODBC::Instance()->read(mto::hfo(market, idate), cmd, orders);
	return orders;
}

void MMercatorTrades::addTickInfo(const string& ticker, vector<MercatorTrade>& trades)
{
	const float& dayVolat = mDayVolat_[ticker];
	const float& medVolat = mMedVolat_[ticker];
	const float& dayVolume = mDayVolume_[ticker];
	const float& medVolume = mMedVolume_[ticker];

	const vector<QuoteInfo>* pQuotes = static_cast<const vector<QuoteInfo>*>(MEvent::Instance()->get(ticker, "nbbo"));
	const vector<TradeInfo>* pTrades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));
	QuoteView qv(pQuotes, pTrades, sessions_, msecOpen_, msecClose_, 100, requireValidQuote_, fillZeros_, allowCross_);
	if(debug_tqi_ && medVolat > 0.)
	{
		int n30sec = (msecClose_ - msecOpen_) / 1000 / 30;
		for(int i = 0; i < n30sec; ++i)
		{
			int msecs = msecOpen_ + i * 30 * 1000;
			double volat = qv.getHiLo(msecs);
			vVolatSurprise30s_.push_back(volat / medVolat);
			if(i % 10 == 0)
				vVolatSurprise300s_.push_back(volat / medVolat);
		}
	}

	int cumTradedShare = 0;
	for(auto& mt : trades)
	{
		cumTradedShare += mt.qty;
		double timeFrac = (double)(msecClose_ - msecOpen_) / (mt.msecs - msecOpen_);

		if(mt.mid == 0)
		{
			printf("addTickInfo %s\n", ticker.c_str());
			mt.mid = qv.getMidExact(mt.msecs);
		}
		if(mt.mid == 0)
			mt.mid = mt.price;

		mt.cv.dayVolat = dayVolat;
		mt.cv.dayVolume = dayVolume;
		mt.cv.volat = qv.getHiLo(mt.msecs);
		mt.cv.dv = qv.getDv(mt.msecs);
		mt.cv.share = qv.getShare(mt.msecs);
		mt.cv.mercatorTrade = timeFrac * cumTradedShare;

		if(medVolat > 0.)
		{
			mt.cv.dayVolatSurprise = dayVolat / medVolat;
			mt.cv.volatSurprise = mt.cv.volat / medVolat;
		}
		if(medVolume > 0.)
		{
			mt.cv.dayVolumeSurprise = dayVolume / medVolume;
			mt.cv.dvSurprise = mt.cv.dv / mt.mid / medVolume;
			mt.cv.shareSurprise = mt.cv.share / medVolume;
			mt.cv.mercatorTradeSurprise = timeFrac * cumTradedShare / medVolume;
			mt.cv.share1s = qv.getShare(mt.msecs, 1) * getTimeFrac(1) / medVolume;
			mt.cv.share60s = qv.getShare(mt.msecs, 60) * getTimeFrac(60) / medVolume;
			mt.cv.share3600s = qv.getShare(mt.msecs, 3600) * getTimeFrac(3600) / medVolume;
		}

		float mid1 = qv.getMid(mt.msecs + 60*1000);
		float mid61 = qv.getMid(mt.msecs + 3660*1000);
		float midC = qv.getMid(24*60*60*1000);
		if( mt.price > ltmb_ && mid1 > ltmb_ )
			mt.gain1 = mt.sign * (mid1 / mt.price - 1.) * basis_pts_;
		if( mt.price > ltmb_ && mid61 > ltmb_ )
			mt.gain61 = mt.sign * (mid61 / mt.price - 1.) * basis_pts_;
		if( mt.price > ltmb_ && midC > ltmb_ )
			mt.gainC = mt.sign * (midC / mt.price - 1.) * basis_pts_;
	}
}

double MMercatorTrades::getTimeFrac(int sec)
{
	double timeFrac = (double)(msecClose_ - msecOpen_) / (sec * 1000);
	return timeFrac;
}

TradeQuantileInfo MMercatorTrades::getTradeQuantileInfo()
{
	TradeQuantileInfo tqi(10);
	vector<string>& tickers = MEnv::Instance()->tickers;
	for(string& ticker : tickers)
		tqi.addTrades(MEvent::Instance()->get<vector<MercatorTrade>>(ticker, (string)"mtrades"));
	tqi.finish(debug_tqi_);
	return tqi;
}
