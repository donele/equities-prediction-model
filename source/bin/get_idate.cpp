#include <optionlibs/TickData.h>
#include <jl_lib.h>
#include <time.h>
#include <map>
#include <string>
using namespace std;

int main(int argc, char* argv[])
{
	map<string, string> am = get_am(argc, argv);

	// reference date, and the difference from the reference date
	if( am.count("d") && am.count("dd") )
	{
		int idate = atoi( am["d"].c_str() );
		int dd = atoi( am["dd"].c_str() );

		// backwards?
		if( am.count("b") )
			dd *= -1;
	 
		// difference in calendar day
		if( am.count("c") )
		{
			QuoteTime today(idate, 120000, "ET");
			int adjusted = (int)(today + RealTime(dd)).Date();
			cout << adjusted << endl; // This is used in Linux.
			return adjusted; // This is used in Windows.
		}

		// difference in trading day
		else if( am.count("t") )
		{
			string m = "";
			if( am.count("m") )
			{
				m = am["m"];
				Exchange ex(mto::ex(m));
				QuoteTime today(idate, 120000, mto::tz(m));
				int adjusted = (int)ex.AddTradeTime(today, dd).Date();
				cout << adjusted << endl;
				return adjusted;
			}
		}

		// list of trading days
		else if( am.count("lt") )
		{
			string m = "";
			if( am.count("m") )
			{
				m = am["m"];
				Exchange ex(mto::ex(m));
				QuoteTime today(idate, 040000, mto::tz(m));
				int idate = (int)ex.NextOpen(today).Date();
				for( int count = 0; count < dd; ++count, idate = (int)ex.NextOpen( QuoteTime(idate, 200000, mto::tz(m)) ).Date() )
				{
					cout << idate << endl;
				}
			}
		}
	}

	// list of holidays or trading days.
	else if( am.count("m") && am.count("d1") && am.count("d2") )
	{
		int d1 = atoi( am["d1"].c_str() );
		int d2 = atoi( am["d2"].c_str() );
		int today = itoday();
		string m = am["m"];
		if( am.count("db") )
		{
			if( d2 > today )
				d2 = today;
			if( "qai" == am["db"] )
			{
				ODBCConnection odbcQAI("qai", "Mercator1", "DBacc101");

				if( am.count("lt") )
				{
					for( int idate = d1; idate < d2; idate = (int)( QuoteTime(idate, 120000, "ET") + RealTime(1) ).Date() )
					{
						int nRowsAll = 0;
						int nRowsGood = 0;
						{
							string cmd = (string)" select count(*) from Gprcval v "
								+ " where v.Date_ = " + quote(itoq(idate))
								+ " and " + mto::selVal(m);
							vector<vector<string> > vv;
							odbcQAI.ReadTable(cmd, &vv);
							nRowsAll = atoi( vv[0][0].c_str() );
						}
						if( nRowsAll > 0 )
						{
					    cout << idate << endl;
						}
					}
				}
				else if( am.count("lh") )
				{
					for( int idate = d1; idate < d2; idate = (int)( QuoteTime(idate, 120000, "ET") + RealTime(1) ).Date() )
					{
						string cmd = (string)" select count(*) from Gprcval v "
							+ " where v.Date_ = " + quote(itoq(idate))
							+ " and " + mto::selVal(m);
						vector<vector<string> > vv;
						odbcQAI.ReadTable(cmd, &vv);
						int nRows = atoi( vv[0][0].c_str() );
						if( 0 == nRows )
							cout << idate << endl;
					}
				}
			}
		}
		else // don't use qai database, for future regular weekdays/weekends only (no speical holidays).
		{
			if ( am.count("lh") )
			{
				for (int idate = d1; idate <= d2; idate = (int)(QuoteTime(idate, 040000, mto::tz(m)) + RealTime(1)).Date() )
				{
					QuoteTime today(idate, 040000, mto::tz(m));
					if (today.WeekDay() == 6 || today.WeekDay() == 0) // Saturday or Sunday
						cout << idate <<endl;
				}
			}

			else if ( am.count("lt") )
			{
				for (int idate = d1; idate <= d2; idate = (int)(QuoteTime(idate, 040000, mto::tz(m)) + RealTime(1)).Date() )
				{
					QuoteTime today(idate, 040000, mto::tz(m));
					if (today.WeekDay() != 6 && today.WeekDay() != 0) // Not Saturday or Sunday
						cout << idate <<endl;
				}
			}
		}
	}

	// 
	else if( am.count("m") && !am.count("d") && !am.count("d1") && !am.count("d2") )
	{
		char syyyymmdd[128];
		char shhmmss[128];
		struct tm* now;
		time_t ltime;
		time( &ltime );
		now = gmtime( &ltime );
		strftime( syyyymmdd, 128, "%Y%m%d", now );
		strftime( shhmmss, 128, "%H%M%S", now );
		int utc_yyyymmdd = atoi(syyyymmdd);
		int utc_hhmmss = atoi(shhmmss);

		QuoteTime qt(utc_yyyymmdd, utc_hhmmss, "UTC"); // idate can be today or yesterday.
		string m = am["m"];
		qt.SetTimeZone(mto::tz(m));
		int idate = (int)qt.Date();

		if( am.count("dd") )
		{
			int dd = atoi( am["dd"].c_str() );

			// backwards?
			if( am.count("b") )
				dd *= -1;
		 
			int ret = 0;
			if( am.count("lt") ) // trading day.
			{
				int ret_date = idate;
				if( dd > 0 )
				{
					Exchange ex(mto::ex(m));
					for( int i = 0; i < dd; ++i )
					{
						QuoteTime today(ret_date, 200000, mto::tz(m));
						ret_date = (int)ex.NextOpen(today).Date();
					}
				}
				else if( dd < 0 )
				{
					Exchange ex(mto::ex(m));
					for( int i = 0; i < -dd; ++i )
					{
						QuoteTime today(ret_date, 040000, mto::tz(m));
						ret_date = (int)ex.PrevOpen(today).Date();
					}
				}
				ret = ret_date;
			}
			else if( am.count("lw") ) // weekday.
			{
				int ret_date = idate;
				int incr = 0;
				if( dd > 0 )
					incr = 1;
				else
					incr = -1;
				for( int i = 0; i < abs(dd); ++i )
				{
					// Add or subtract one day.
					ret_date = (int)(QuoteTime(ret_date, 120000, mto::tz(m)) + RealTime(incr)).Date();

					// Keep adding or subtracting until a weekday.
					QuoteTime today(ret_date, 120000, mto::tz(m));
					while( today.WeekDay() == 0 || today.WeekDay() == 6 )
					{
						ret_date = (int)(today + RealTime(incr)).Date();
						today = QuoteTime(ret_date, 120000, mto::tz(m));
					}
				}
				ret = ret_date;
			}
			else // calendar day.
			{
				ret = (int)(qt + RealTime(dd)).Date();
			}
			cout << ret << endl;
			return ret;
		}
		else
		{
			int ret = (int)qt.Date();
			cout << ret << endl;
			return ret;
		}
	}
	cout << 0 << endl;
	return 0;
}
