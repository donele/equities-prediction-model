#include <boost/format.hpp>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;
using namespace boost;

int main()
{
	int n = -17;
	float avg = 3.142;
	float rat = 0.9899;
	float gpt = -1.3;
	string name = "name";

	string s1 = str(format("%|+10i| |%-10.2f| %010.5f %f %5.2f%|60T^|%s %s %s\n") % n % avg % rat % gpt % gpt % name % gpt % to_string(gpt));
	cout << s1;
}

