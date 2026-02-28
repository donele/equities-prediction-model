#include <optionlibs/TickData.h>
#include <jl_lib.h>
#include <vector>
#include <algorithm>
#include <boost/range/algorithm_ext/iota.hpp>
#include <boost/bind.hpp>
#include <string>
#include <thread>
#include <future>
using namespace std;

void thread_function(int x)
{
	cout << x;
}

string print_part(vector<int>& v, int i, int n)
{
	int from = v.size() / n * i;
	int end = (i == n - 1) ? v.size() : v.size() / n * (i + 1);
	for( int x = from; x < end; ++x )
		cout << v[x] << ",";
	return (string)"thread" + itos(i);
}

int main()
{
  //TOCEntryStyle x = TES_XLONG;
  //TOCEntryStyle y = TES_64;
  //SetDefaultTocAuto(true);
	auto i = 1;
	cout << typeid(i).name() << endl;
	auto x = 1.;
	cout << typeid(x).name() << endl;

	map<string, Corr> m;
	m["a"] = Corr();
	for( auto it = begin(m); it != end(m); ++it )
		cout << it->first << it->second << endl;
	for( auto it = m.cbegin(); it != m.cend(); ++it )
		cout << it->first << it->second << endl;

	vector<int> v;
	v.push_back(1);
	v.push_back(2);
	for( auto& i: v )
		cout << i << endl;

	for( const auto& keyValue: m )
		cout << keyValue.first << keyValue.second << endl;

	shared_ptr<int> p1(new int(123));
	cout << *p1 << endl;
	cout << p1.get() << endl;
	auto p2 = make_shared<int>(321);
	cout << *p2 << endl;
	cout << p2.get() << endl;

	for_each(begin(v), end(v), [](int n) {cout << n << "\t";});

	auto is_odd = [](int n) {return n % 2 == 1;};
	auto pos = find_if(begin(v), end(v), is_odd);
	if( pos != end(v) )
		cout << *pos << endl;

	vector<thread> vt;
	for( auto i = 0; i < 10; ++i )
		vt.push_back(thread(thread_function, i));
	for( auto i = 0; i < 10; ++i )
		vt[i].join();
	cout << endl;

	vector<int> v2(10);
	iota(begin(v2), end(v2), 10);
	for_each( begin(v2), end(v2), [](int n) {cout << n << ",";});
	cout << endl;
	int n = 3;
	//for( int i = 0; i < n; ++i )
		//std::async(print_part, ref(v2), i, n);
	auto f1 = async(print_part, ref(v2), 0, 3);
	auto f2 = async(print_part, ref(v2), 1, 3);
	auto f3 = async(print_part, ref(v2), 2, 3);
	cout << f1.get();
	f2.wait();
	f3.wait();
	
	const int& ri = 4;
	cout << ri << endl;
}
