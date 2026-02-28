#include <jl_lib/cme.h>
#include <cstdio>
#include <algorithm>
using namespace std;

namespace cme
{
	string month_code(int month)
	{
		string code;
		if( 1 == month )
			code = "F";
		else if( 2 == month )
			code = "G";
		else if( 3 == month )
			code = "H";
		else if( 4 == month )
			code = "J";
		else if( 5 == month )
			code = "K";
		else if( 6 == month )
			code = "M";
		else if( 7 == month )
			code = "N";
		else if( 8 == month )
			code = "Q";
		else if( 9 == month )
			code = "U";
		else if( 10 == month )
			code = "V";
		else if( 11 == month )
			code = "X";
		else if( 12 == month )
			code = "Z";
		return code;
	}

	vector<int> contract_months(string product)
	{
		vector<int> months;

		if( product == "CL" || product == "RB" || product == "HO" || product == "NG" || product == "QM" )
		{
			months.push_back(1); // F
			months.push_back(2); // G
			months.push_back(3); // H
			months.push_back(4); // J
			months.push_back(5); // K
			months.push_back(6); // M
			months.push_back(7); // N
			months.push_back(8); // Q
			months.push_back(9); // U
			months.push_back(10); // V
			months.push_back(11); // X
			months.push_back(12); // Z
		}
		else if( product == "ZC" || product == "ZW" )
		{
			months.push_back(3); // H
			months.push_back(5); // K
			months.push_back(7); // N
			months.push_back(9); // U
			months.push_back(12); // Z
		}
		else if( product == "ZS" )
		{
			months.push_back(1); // F
			months.push_back(3); // H
			months.push_back(5); // K
			months.push_back(7); // N
			months.push_back(8); // Q
			months.push_back(9); // U
			months.push_back(11); // X
		}
		else if( product == "ZL" || product == "ZM" )
		{
			months.push_back(1); // F
			months.push_back(3); // H
			months.push_back(5); // K
			months.push_back(7); // N
			months.push_back(8); // Q
			months.push_back(9); // U
			months.push_back(10); // V
			months.push_back(12); // Z
		}
		else if( product == "LE" )
		{
			months.push_back(2); // G
			months.push_back(4); // J
			months.push_back(6); // M
			months.push_back(8); // Q
			months.push_back(10); // V
			months.push_back(12); // Z
		}

		return months;
	}

	string front_ticker(string product, int idate)
	{
		string ticker;
		int month = idate / 100 % 100;
		int year = idate / 10000;

		vector<int> months = contract_months(product);
		vector<int>::iterator it = lower_bound(months.begin(), months.end(), month);
		if( it == months.end() )
		{
			it = months.begin();
			++year;
		}

		int day = idate % 100;
		if( day > 10 )
			++it;
		if( it == months.end() )
		{
			it = months.begin();
			++year;
		}

		char buf[10];
		sprintf(buf, "%s%s%d", product.c_str(), month_code(month).c_str(), year % 10);

		return buf;
	}
}
