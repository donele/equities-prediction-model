#ifndef __gtlib_tickdump_DumpImpl__
#define __gtlib_tickdump_DumpImpl__
#include <gtlib_tickdump/DumpFtns.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <optionlibs/TickData.h>

namespace gtlib {

template <typename T>
class DumpImpl {
public:
	DumpImpl(){}
	DumpImpl(int idate, const std::vector<std::string>& dirs, int n);
	~DumpImpl(){}
	void summ();
	void dump(const std::string& ticker, int msecs = 0);
private:
	TickAccessMulti<T> ta_;
	std::vector<std::string> names_;
	int idate_;
	std::vector<std::string> dirs_;
	int nPrint_;

	void init_access();
	void terminate_dirs();
	void terminate_names();
	void terminate_ticker(const std::string& ticker);

	void dump_ticker(const std::string& ticker);
	void dump_ticker_time(const std::string& ticker, int msecs);
};

template <typename T>
DumpImpl<T>::DumpImpl(int idate, const std::vector<std::string>& dirs, int n)
	:idate_(idate),
	dirs_(dirs),
	nPrint_((n > 0) ? n : 5)
{
	if( dirs_.empty() )
		terminate_dirs();
	else
	{
		init_access();
		if( names_.empty() )
			terminate_names();
	}
}

template <typename T>
void DumpImpl<T>::init_access()
{
	for( const std::string& dir : dirs_ )
		ta_.AddRoot(dir);
	ta_.GetNames(idate_, &names_);
	std::sort(begin(names_), end(names_));
}

template <typename T>
void DumpImpl<T>::terminate_dirs()
{
	std::cerr << "Directories not determined.\n";
	exit(1);
}

template <typename T>
void DumpImpl<T>::terminate_names()
{
	std::cerr << "No data for " << idate_ << " in";
	for( const auto& dir : dirs_ )
		std::cerr << " " << dir;
	std::cerr << '\n';
	exit(1);
}

template <typename T>
void DumpImpl<T>::summ()
{
	char buf[200];
	sprintf(buf, "%12s %7s\n", "ticker", "ntick");
	std::cout << buf;
	for( const std::string& ticker : names_ )
	{
		TickSeries<T> ts;
		ta_.GetTickSeries(ticker, idate_, &ts);
		int nt = ts.NTicks();
		sprintf(buf, "%12s%7d\n", ticker.c_str(), nt);
		std::cout << buf;
	}
}

template <typename T>
void DumpImpl<T>::dump(const std::string& ticker, int msecs)
{
	if( binary_search(begin(names_), end(names_), ticker) )
	{
		if( msecs == 0 )
			dump_ticker(ticker);
		else
			dump_ticker_time(ticker, msecs);
	}
	else
		terminate_ticker(ticker);
}

template <typename T>
void DumpImpl<T>::dump_ticker(const std::string& ticker)
{
	TickSeries<T> ts;
	ta_.GetTickSeries(ticker, idate_, &ts);
	int nticks = ts.NTicks();

	std::cout << getDumpMeta(idate_, ticker, nticks) << '\n';
	std::cout << getDumpHeader<T>() << '\n';
	ts.StartRead();
	T t;
	for( int i = 0; i < nticks; ++i )
	{
		ts.Read(&t);
		std::cout << getDumpContent(t) << '\n';
	}
}

template <typename T>
void DumpImpl<T>::terminate_ticker(const std::string& ticker)
{
	std::cerr << "ticker " << ticker << " was not found in ";
	for( const std::string& dir : dirs_ )
		std::cerr << " " << dir;
	std::cerr << '\n';
	exit(1);
}

template <typename T>
void DumpImpl<T>::dump_ticker_time(const std::string& ticker, int msecs)
{
	TickSeries<T> ts;
	ta_.GetTickSeries(ticker, idate_, &ts);
	int nticks = ts.NTicks();

	std::vector<std::string> vContent;
	std::cout << getDumpMeta(idate_, ticker, nticks) << " [prior to " << msecs << "]\n";
	std::cout << getDumpHeader<T>() << '\n';
	ts.StartRead();
	T t;
	for( int i = 0; i < nticks; ++i )
	{
		ts.Read(&t);
		if( t.msecs >= msecs )
			break;
		vContent.push_back(getDumpContent(t));
	}

	int n = vContent.size();
	if( n > nPrint_ )
		vContent = std::vector<std::string>(vContent.begin() + n - nPrint_, vContent.end());
	for( const auto& content : vContent )
		std::cout << content << '\n';
}

} // namespace gtlib

#endif
