#include <HLearnObj/InputInfo.h>
#include <fstream>
using namespace std;

namespace hnn {
	InputInfo::InputInfo(const InputInfo& nni)
	{
		n = nni.n;
		mean = nni.mean;
		stdev = nni.stdev;
	}

	InputInfo::InputInfo(string filename)
	{
		ifstream ifs(filename.c_str());
		ifs >> n;
		mean = vector<double>(n);
		stdev = vector<double>(n);
		for( int i = 0; i < n; ++i )
			ifs >> mean[i];
		for( int i = 0; i < n; ++i )
			ifs >> stdev[i];
	}
}

ostream& operator <<(ostream& os, hnn::InputInfo& obj)
{
	os << obj.n << endl;

	char buf[100];
	for( int i = 0; i < obj.n; ++i )
	{
		sprintf(buf, "%15.10f\t", obj.mean[i]);
		os << buf;
	}
	os << endl;
	for( int i = 0; i < obj.n; ++i )
	{
		sprintf(buf, "%15.10f\t", obj.stdev[i]);
		os << buf;
	}
	os << endl;

	return os;
}
