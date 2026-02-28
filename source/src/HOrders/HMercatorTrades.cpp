#include <HOrders/HMercatorTrades.h>
#include <gtlib_signal/QuoteSample.h>
#include <jl_lib/MercatorTrade.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
#include <jl_lib/GODBC.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/mto.h>
#include <map>
#include <list>
#include <string>
using namespace std;
using namespace gtlib;

HMercatorTrades::HMercatorTrades(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
execType_("MT"),
source_(""),
source_ext_(""),
min_price_(0.),
max_price_(max_double_),
min_msso_(0),
max_msso_(0),
debug_(false)
{
	if( conf.count("execType") )
		execType_ = conf.find("execType")->second;

	if( conf.count("source") )
		source_ = conf.find("source")->second;

	if( !source_.empty() )
		source_ext_ = (string)"_" + source_;

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

HMercatorTrades::~HMercatorTrades()
{}

void HMercatorTrades::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void HMercatorTrades::beginMarket()
{
	return;
}

void HMercatorTrades::beginDay()
{
	string market = HEnv::Instance()->market();
	int idate = HEnv::Instance()->idate();

	read_trades(market, idate);
	read_orders(market, idate);
}

void HMercatorTrades::beginMarketDay()
{
	int idate = HEnv::Instance()->idate();
	string market = HEnv::Instance()->market();
	open_msecs_ = mto::msecOpen(market, idate);
	if( !vUniv_.empty() )
		select_univ(market, idate);

}

void HMercatorTrades::beginTicker(const string& ticker)
{
	int idate = HEnv::Instance()->idate();
	vector<MercatorTrade> trades = getMtrades(ticker);
	HEvent::Instance()->add<vector<MercatorTrade> >(ticker, (string)"mtrades" + source_ext_, trades);
	vector<MercatorTrade> orders = getMorders(ticker);
	HEvent::Instance()->add<vector<MercatorTrade> >(ticker, (string)"morders" + source_ext_, orders);

	return;
}

void HMercatorTrades::endTicker(const string& ticker)
{
	HEvent::Instance()->remove<vector<MercatorTrade> >(ticker, (string)"mtrades" + source_ext_);
	return;
}

void HMercatorTrades::endDay()
{
	return;
}

void HMercatorTrades::endMarket()
{
	return;
}

void HMercatorTrades::endJob()
{
	return;
}

vector<MercatorTrade> HMercatorTrades::getMtrades(const string& ticker)
{
	//const vector<QuoteInfo>* pQuotes = static_cast<const vector<QuoteInfo>*>(HEvent::Instance()->get(ticker, "quotes"));
	//const QuoteSample* pqs = static_cast<const QuoteSample*>(HEvent::Instance()->get(ticker, "qSample"));
	vector<MercatorTrade> v;
	//for( vector<vector<string> >::iterator it = trades_.begin(); it != trades_.end(); ++it )
	//{
	//	//bool isMT = (*it).size() >= 6 && (*it)[5] == "L";
	//	//bool isMM = (*it).size() >= 6 && (*it)[5] == "A";

	//	//if( (execType_ == "MT" && isMT) || (execType_ == "MM" && isMM) || execType_ == "MTMM" )
	//	{
	//		string dbTicker = trim( (*it)[1] );
	//		if( ticker == dbTicker )
	//		{
	//			double price = atof( (*it)[4].c_str() );
	//			double ms = atof( (*it)[0].c_str() );
	//			int msso = ms - open_msecs_;
	//			if( min_price_ <= price && price <= max_price_ && (min_msso_ < msso) && (max_msso_ == 0 || msso < max_msso_) )
	//			{
	//				string side = trim( (*it)[2] );
	//				int sign = 0;
	//				if( side[0] == 'B' )
	//					sign = 1;
	//				else if( side[0] == 'S' )
	//					sign = -1;
	//				int qty = atoi( (*it)[3].c_str() );
	//				double pred1 = atof( (*it)[7].c_str() ) * basis_pts_;
	//				double pred10 = atof( (*it)[8].c_str() ) * basis_pts_;
	//				double bid = atof( (*it)[9].c_str() );
	//				double ask = atof( (*it)[10].c_str() );
	//				double mid = 0.;
	//				if( bid > 0. && ask > 0. )
	//					mid = (bid + ask) / 2.;
	//				if( ms > 1000000 )
	//				{
	//					char execType = (*it)[5][0];
	//					int schedType = atoi((*it)[6].c_str());
	//					MercatorTrade mt(ms, sign, price, qty, pred1, pred10, mid, execType, schedType, qty);
	//					mt.getTickInfo(*pQuotes, *pqs);
	//					v.push_back(mt);
	//				}
	//			}
	//		}
	//	}
	//}
	return v;
}

vector<MercatorTrade> HMercatorTrades::getMorders(const string& ticker)
{
	//const vector<QuoteInfo>* pQuotes = static_cast<const vector<QuoteInfo>*>(HEvent::Instance()->get(ticker, "quotes"));
	//const QuoteSample* pqs = static_cast<const QuoteSample*>(HEvent::Instance()->get(ticker, "qSample"));
	vector<MercatorTrade> v;
	//for( vector<vector<string> >::iterator it = orders_.begin(); it != orders_.end(); ++it )
	//{
	//	//bool isMT = (*it).size() >= 6 && (*it)[5] == "L";
	//	//bool isMM = (*it).size() >= 6 && (*it)[5] == "A";

	//	//if( (execType_ == "MT" && isMT) || (execType_ == "MM" && isMM) || execType_ == "MTMM" )
	//	{
	//		string dbTicker = trim( (*it)[1] );
	//		string tickerBase = baseTicker(ticker);
	//		if( tickerBase == dbTicker )
	//		{
	//			//double price = atof( (*it)[4].c_str() );
	//			int qty = atoi( (*it)[3].c_str() );
	//			double price = atof( (*it)[4].c_str() );
	//			double ms = atof( (*it)[0].c_str() );
	//			int msso = ms - open_msecs_;
	//			if( min_price_ <= price && price <= max_price_ && (min_msso_ < msso) && (max_msso_ == 0 || msso < max_msso_) )
	//			{
	//				string side = trim( (*it)[2] );
	//				int sign = 0;
	//				if( side[0] == 'B' )
	//					sign = 1;
	//				else if( side[0] == 'S' )
	//					sign = -1;
	//				double pred1 = atof( (*it)[7].c_str() ) * basis_pts_;
	//				double pred10 = atof( (*it)[8].c_str() ) * basis_pts_;
	//				double bid = atof( (*it)[9].c_str() );
	//				double ask = atof( (*it)[10].c_str() );
	//				double mid = 0.;
	//				if( bid > 0. && ask > 0. )
	//					mid = (bid + ask) / 2.;
	//				if( ms > 1000000 )
	//				{
	//					char execType = (*it)[5][0];
	//					int schedType = atoi((*it)[6].c_str());
	//					MercatorTrade mt(ms, sign, price, qty, pred1, pred10, mid, execType, schedType, qty);
	//					mt.getTickInfo(*pQuotes, *pqs);
	//					v.push_back(mt);
	//				}
	//			}
	//		}
	//	}
	//}
	return v;
}

void HMercatorTrades::select_univ(const string& market, int idate)
{
	if( !vUniv_.empty() )
	{
		string sExch = mto::code(market);

		string selMarket = "";
		if( mto::isInternational(market) && !isECN(market) )
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
			HEnv::Instance()->tickers(tickers_);
		}
	}
}

void HMercatorTrades::read_orders(const string& market, int idate)
{
	//string selMarket = "";
	//if( mto::isInternational(market) )
	//	selMarket = (string)" and e.exchange = " + quote(mto::code(market));

	//string selSource = "";
	//if( mto::region(market) == "U" && !source_.empty() )
	//	selSource = (string)" and o.dest = " + itos(static_cast<int>(source_[0]));

	//string dbSymbol = "o.symbol";
	////if( mto::isInternational(market) )
	//	//dbSymbol = "o.exchange + ':' + o.symbol";

	//string cmd = (string)" select o.orderMsecs, " + dbSymbol
	//	+ ", e.side, sum(e.qty), sum(e.qty * e.price)/sum(e.qty), o.exectype, o.orderschedtype, "
	//	+ " min(o.onempred), min(o.tenmpred), min(o.bid), min(o.ask) "
	//	+ " from hfOrderEvents e inner join hfOrders o on e.orderID = o.orderID "
	//	+ " where o.idate = " + itos(idate)
	//	+ " and e.idate = " + itos(idate)
	//	+ selSource
	//	+ selMarket
	//	+ " group by o.orderMsecs, o.symbol, e.side, o.exectype, o.orderschedtype "
	//	+ " order by o.orderMsecs ";
	//orders_.clear();
	//GODBC::Instance()->read(mto::hfo(market, idate), cmd, orders_);

	return;
}

void HMercatorTrades::read_trades(const string& market, int idate)
{
	//string selMarket = "";
	//if( mto::isInternational(market) )
	//	selMarket = (string)" and e.exchange = " + quote(mto::code(market));

	//string selSource = "";
	//if( mto::region(market) == "U" && !source_.empty() )
	//	selSource = (string)" and o.dest = " + itos(static_cast<int>(source_[0]));

	//string dbSymbol = "o.symbol";
	////if( mto::isInternational(market) )
	//	//dbSymbol = "o.exchange + ':' + o.symbol";

	//string cmd = (string)" select e.eventMsecs, " + dbSymbol + ", e.side, e.qty, e.price, o.exectype, o.orderschedtype, "
	//	+ " o.onempred, o.tenmpred "
	//	+ " from hfOrderEvents e inner join hfOrders o on e.orderID = o.orderID "
	//	+ " where eventType = 2 "
	//	+ " and o.idate = " + itos(idate)
	//	+ " and e.idate = " + itos(idate)
	//	+ selSource
	//	+ selMarket
	//	+ " order by eventMsecs ";
	//trades_.clear();
	//GODBC::Instance()->read(mto::hfo(market, idate), cmd, trades_);

	return;
}

