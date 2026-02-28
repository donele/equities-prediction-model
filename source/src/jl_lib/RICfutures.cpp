#include <jl_lib/RICfutures.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GEX.h>
#include <jl_lib/mto.h>
#include <jl_lib/GODBC.h>
#include <optionlibs/TickData.h>
#include <map>
using namespace std;

RICfutures* RICfutures::instance_ = 0;

RICfutures* RICfutures::Instance()
{
	static RICfutures::Cleaner cleaner;
	if( instance_ == 0 )
		instance_ = new RICfutures();
	return instance_;
}

RICfutures::Cleaner::~Cleaner() {
	delete RICfutures::instance_;
	RICfutures::instance_ = 0;
}

RICfutures::RICfutures()
{
	if( mMarketBase_.empty() )
	{
		mMarketBase_["EA"] = "AEX";
		mMarketBase_["EB"] = "BFX";
		mMarketBase_["EP"] = "FCE";
		mMarketBase_["EF"] = "FDX";
		mMarketBase_["EL"] = "FFI";
		mMarketBase_["ED"] = "MFXI";
		mMarketBase_["EM"] = "IFS";
		mMarketBase_["EZ"] = "FSMI";
		mMarketBase_["E5"] = "STXX";
		mMarketBase_["AT"] = "JNI";
		mMarketBase_["AS"] = "YAP";
		mMarketBase_["AH"] = "HSI";
		mMarketBase_["AK"] = "KS";
		mMarketBase_["AG"] = "SSG";
		mMarketBase_["AW"] = "TX";
	}

	if( mMonthCode_.empty() )
	{
		mMonthCode_[1] = "F";
		mMonthCode_[2] = "G";
		mMonthCode_[3] = "H";
		mMonthCode_[4] = "J";
		mMonthCode_[5] = "K";
		mMonthCode_[6] = "M";
		mMonthCode_[7] = "N";
		mMonthCode_[8] = "Q";
		mMonthCode_[9] = "U";
		mMonthCode_[10] = "V";
		mMonthCode_[11] = "X";
		mMonthCode_[12] = "Z";
	}
}

string RICfutures::most_traded_ticker(const string& market, int idate)
{
	// Futures of the next expiration start to trade more on n days before the expiration day.
	int adj = 0;
	if( "EB" == market || "AT" == market )
		adj = 1;
	else if( "AK" == market || "AW" == market ) // The expiring contract is traded most actively until the expiration day in Korea.
		adj = -1;

	int idate_adjusted = next_trading_day(market, idate, adj);
	int idate_exp = next_expiration(market, idate_adjusted);

	int month = idate_exp/100%100;
	int year = idate_exp/10000%10;
	string ticker = mMarketBase_[market] + mMonthCode_[month] + itos(year);
	return ticker;
}

//string RICfutures::most_active_ticker(string market, int idate)
//{
//	int rollover = next_rollover(market, idate);
//	int month = rollover/100%100;
//	int year = rollover/10000%10;
//	string ticker = mMarketBase_[market] + mMonthCode_[month] + itos(year);
//	return ticker;
//}
//
//int RICfutures::next_rollover(string market, int idate)
//{
//	// Futures of the next expiration start to trade more on n days before the expiration day.
//	int adj = 0;
//	if( "EB" == market || "AT" == market )
//		adj = 1;
//	else if( "AK" == market ) // The expiring contract is traded most actively until the expiration day in Korea.
//		adj = -1;
//
//	//int idate_exp = idate;
//	//int idate_rollover = 0;
//	//do {
//	//	idate_exp = next_expiration(market, idate_exp);
//	//	idate_rollover = next_trading_day(market, idate_exp, -adj);
//	//} while ( idate_rollover <= idate );
//
//	//int idate_rollover = 0;
//	//for( int idate_exp = idate; idate_rollover < idate; idate_exp = next_trading_day(market, idate_exp) )
//	//{
//	//	idate_exp = next_expiration(market, idate_exp);
//	//	idate_rollover = next_trading_day(market, idate_exp, -adj);
//	//}
//
//	int idate_rollover = 0;
//	int idate_seed = idate;
//	for( int idate_seed = idate; idate_rollover < idate; idate_seed = next_trading_day(market, idate_seed) )
//	{
//		int idate_exp = next_expiration(market, idate_seed);
//		idate_rollover = next_trading_day(market, idate_exp, -adj);
//	}
//
//	return idate_rollover;
//}

int RICfutures::next_expiration(const string& market, int idate)
{
	// 0: Sunday, 6: Saturday.
	if( "EA" == market || "EB" == market || "EP" == market || "ED" == market )
		return next_expiration(market, idate, 3, 5, 1); // 3rd Friday(5).
	else if( "EF" == market || "EL" == market || "EM" == market || "EZ" == market || "E5" == market )
		return next_expiration(market, idate, 3, 5, 3); // 3rd Friday(5) of Mar, Jun, Sep, and Dec.
	else if( "AT" == market )
		return next_expiration(market, idate, 2, 5, 3); // 2nd Friday(5) of Mar, Jun, Sep, and Dec.
	else if( "AH" == market || "AG" == market )
		return next_expiration(market, idate, 2); // Second last trading day of the month.
	else if( "AS" == market )
		return next_expiration(market, idate, 3, 4, 3); // 3rd Thursday(4) of Mar, Jun, Sep, and Dec.
	else if( "AK" == market )
		return next_expiration(market, idate, 2, 4, 3); // 3rd Thursday(4) of Mar, Jun, Sep, and Dec.
	else if( "AW" == market )
		return next_expiration(market, idate, 3, 3, 1); // 3rd Wednesday(3).
	else
		exit(2);

	return 0;
}

int RICfutures::next_expiration(const string& market, int idate, int expWeek, int expWeekDay, int freq)
{
	QuoteTime qt(idate, 200000, mto::tz(market));

	// Go to the next expiring weekday.
	int offset = expWeekDay - qt.WeekDay();
	while( offset <= 0 )
		offset += 7;
	qt += RealTime( offset );

	// Add a week until the day of expiration.
	while( true )
	{
		int day = (((int)qt.Date())%100);
		int month = (((int)qt.Date())/100%100);
		if( (day - 1) / 7 + 1 == expWeek && month % freq == 0 )
			break;
		qt += RealTime(7.0);
	}

	// Last closing day.
	qt = GEX::Instance()->get(market)->PrevClose(qt);
	if( (int)qt.Date() <= idate )
		qt = next_expiration(market, next_trading_day(market, idate), expWeek, expWeekDay, freq);
	int ret = (int)qt.Date();
	return ret;
}

int RICfutures::next_trading_day(const string& market, int idate, int n)
{
	int ret = idate;
	if( n > 0 )
	{
		QuoteTime qt(idate, 200000, mto::tz(market));
		for( int i=0; i<n; ++i )
			qt = (int)GEX::Instance()->get(market)->NextClose(qt).Date();
		ret = (int)qt.Date();
	}
	else if( n < 0 )
	{
		QuoteTime qt(idate, 040000, mto::tz(market));
		for( int i=0; i<abs(n); ++i )
			qt = (int)GEX::Instance()->get(market)->PrevClose(qt).Date();
		ret = (int)qt.Date();
	}
	return ret;
}

int RICfutures::next_expiration(const string& market, int idate, int nBeforeEndOfMonth)
{
	//QuoteTime ret = QuoteTime(idate, 200000, mto::tz(market));
	QuoteTime qt(idate, 200000, mto::tz(market));

	// Start by adding nBeforeEndOfMonth days.
	for( int i=0; i<nBeforeEndOfMonth; ++i )
		qt = GEX::Instance()->get(market)->NextClose(qt);

	// Move to the first trading day of the next month.
	int month0 = ((int)qt.Date())/100%100;
	for( int month = month0; month == month0; month = ((int)qt.Date())/100%100 )
		qt = GEX::Instance()->get(market)->NextClose(qt);

	// Step back to the expiration day.
	for( int i=0; i<nBeforeEndOfMonth; ++i )
		qt = GEX::Instance()->get(market)->PrevClose(qt);

	int ret = (int)qt.Date();
	return ret;
}
