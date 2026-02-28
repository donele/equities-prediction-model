#ifndef __MCPred__
#define __MCPred__
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MCPred {
public:
	MCPred();
	MCPred(std::string pred_dir, std::string market, int idate);
	double get_pred1m(std::string ticker, int msecs);
	double get_pred40m(std::string ticker, int msecs);
	double get_pred41m(std::string ticker, int msecs);

private:
	std::string market_;
	int idate_;
	int N_;
	std::map<std::string, std::vector<double> > mTickerPred1m_;
	std::map<std::string, std::vector<double> > mTickerPred40m_;

	void flush_pred(std::string uid, std::vector<double>& vPred1m, std::vector<double>& vPred40m);
	void add_line(std::vector<std::string>& v, std::vector<double>& vPred1m, std::vector<double>& vPred40m);
	double get_pred(std::map<std::string, std::vector<double> >& m, std::string ticker, int msecs);
};

#endif
