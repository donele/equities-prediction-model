#ifndef __jlutil__
#define __jlutil__
#include <string>
#include <map>
#include <vector>
#include <math.h>
#include <optionlibs/TickData.h>
#include <limits>
#include <float.h>
#include <jl_lib/MSessions.h>

typedef std::multimap<std::string, std::string>::const_iterator mmit;

extern const double ltmb_; // a number less than minimum bid increment of any known market.
extern const double basis_pts_;
//extern const double min_double_;
extern const double max_double_;
//extern const float min_float_;
extern const float max_float_;
extern const int max_int_;
extern const std::string tickC_dir_;
extern const std::string tickR_dir_;
extern const std::string tickMon_dir_;

void mkd(const std::string path);
std::string xpf(const std::string& path);
bool is_number(const std::string& s);
bool isPowerOfTwo(int n);
int sign(double d);
double clip_off(double d, double clip);
void clip(double& d, double clip);
void clip(float& d, float clip);
std::string itos(int i);
std::string dtos(double d, int p);
std::string dtosN0n(double d, int p);
std::string itoq(int idate);
int qtoi(const std::string& qdate);
std::string quote(const std::string& s);
std::string quoteN(const std::string& s);
std::string unquote(const std::string& s);
std::string trim(const std::string& s);
std::multimap<std::string, std::string> get_amm(int argc, char* argv[]);
std::map<std::string, std::string> get_am( int argc, char* argv[] );
int getFinalSigDay(const std::string& market);
int itoday(const std::string& market);
int itoday();
std::string timestamp();
std::vector<std::string> split( const std::string& s );
std::vector<std::string> split( const std::string& s, const char d );
std::vector<std::string> splitN( const std::string& s, const char d = ' ' );
int MMMtomm(const std::string& MMM);
int Mmmtomm(const std::string& Mmm);
double get_mid(const double& bid, const double& ask);
double get_mid(const QuoteInfo& quote);
double get_sprd(double bid, double ask);
int prevClose(const std::string& market, int idate);
int nextClose(const std::string& market, int idate);
int nextOpen(const std::string& market, int idate, int n = 1);
int advance_idate(const std::string& market, int idate, int dd);
void print_mr(MultiRegress& mr);
struct inRange:
	public std::binary_function<std::pair<int, int>, int, bool> {
		bool operator()(const std::pair<int, int>& p, const int& t) const;
	};
struct inRangeStrict:
	public std::binary_function<std::pair<int, int>, int, bool> {
		bool operator()(const std::pair<int, int>& p, const int& t) const;
	};
void getLotSizeCA( std::map<std::string, int>& m, int idate );
std::string HK_4digit( const std::string& ticker );
std::string HK_QAI( const std::string& ticker );
std::string HK_ticker( const std::string& ticker, int idate );
std::string QHtoBloomberg( const std::string& market, const std::string& ticker );
std::string removeColon( std::string ticker );
std::string baseTicker(std::string ticker);
std::string currCharaToQAI( const std::string& currChara );
std::vector<std::string> base_names( const std::vector<std::string>& tickers );
std::string base_name( const std::string& ticker );
char ExecFeesPrimex(const std::string& model, const std::string& ticker);;
std::vector<std::string> comp_ticker( const std::vector<std::string>& tickers, const std::string& market );
std::vector<std::string> comp_ticker( const std::vector<std::string>& tickers, const std::string& market, int idate );
std::vector<std::string> get_univ_uids( const std::string& market, int idate );
std::vector<std::string> get_univ_tickers( const std::string& market, int idate );
std::vector<std::string> get_univ_tickers( const std::string& market, int idate1, int idate2 );
std::vector<std::string> get_univ_tickers( const std::string& market, int idate, const std::vector<int>& univ );
class SampleCounter {
public:
	SampleCounter():step_(0),cnt_(0){}
	SampleCounter(int step);
	void update();
	bool doSample();
private:
	int step_;
	int cnt_;
};
class TickersVolat {
public:
	TickersVolat():market_("US"), meanVolat_(0.){}
	~TickersVolat(){}
	void read(int idate);
	float getVolat(const std::string& ticker) const;
private:
	std::map<std::string, float> mTickerVolat_;
	std::string market_;
	float meanVolat_;
};
std::map<std::string, float> readChara(const std::string& market, int idate, const std::string& col);
void get_close( std::map<std::string, double>& m, const std::string& market, int idate);
void check_and_delete(const std::string& dbname, const std::string& chk, const std::string& cmd);
void check_and_insert_or_update(const std::string& dbname, const std::string& chk, const std::string& ins, const std::string& upt);
void check_and_exit(const std::string& dbname, const std::string& chk);
std::vector<std::string> primaries();
std::vector<std::string> euEcns();
std::vector<std::string> caEcns();
std::vector<std::string> asEcns();
std::vector<std::string> ecns();
std::vector<std::string> usDests();
bool isUSDest(const std::string& market);
bool isECN( const std::string& market );
bool isECN_EU( const std::string& market );
double volatility_n
	( const std::vector<double>& vMsec, const std::vector<double>& vBid, const std::vector<double>& vAsk,
	  const std::string& market, int n = 100, int idate = 99999999 );
double volatility_day
	( const std::vector<double>& vMsec, const std::vector<double>& vBid, const std::vector<double>& vAsk,
	  const std::string& market, int idate );
double volatility_session
	( const std::vector<double>& vMsec, const std::vector<double>& vBid, const std::vector<double>& vAsk,
	  const std::vector<double>& vt );
std::string sedol6to7(const std::string& sedol6);
void get_intersection(const std::vector<std::string>& v1, const std::vector<std::string>& v2, std::vector<std::string>& v3);
void get_union(const std::vector<std::string>& v1, const std::vector<std::string>& v2, std::vector<std::string>& v3);
void get_ma(std::vector<double>& ma, const std::vector<double>& v, int n);

/*
std::string paste_symbol_string_sql( const std::vector<std::string>& vect_symbol, 
												  const std::vector<std::string>& vect_var,  std::string var_type );
*/
std::string paste_key_value_sql( const std::vector<std::string>& vect_key,
			  const std::vector<std::string>& vect_value, const std::string& key_name, const std::string& value_type);
struct Acc
{
	double sum;
	double sum2;
	double n;

	Acc();
	Acc(const std::vector<float>& vv, const std::vector<int>& vi);
	void add(double x);
	double mean() const;
	double var() const;
	double stdev() const;
	double RMS() const;
	void clear();
	Acc& operator+=(const Acc &rhs);
	const Acc operator+(const Acc &rhs) const;
};
std::ostream& operator <<(std::ostream& os, const Acc& obj);
std::istream& operator >>(std::istream& is, Acc& obj);
struct WAcc
{
	double sum;
	double sum2;
	double wsum;
	double n;

	WAcc();
	void add(double x, double w);
	double mean() const;
	double var() const;
	double stdev() const;
	double RMS() const;
	void clear();
};
struct Corr {
	Corr(){}
	Corr(const std::vector<float>& v1, const std::vector<float>& v2, const std::vector<int>& vi);
	Acc accX;
	Acc accY;
	Acc accXY;

	void add(double x, double y);
	double cov() const;
	double corr() const;
	void clear();
	Corr& operator+=(const Corr &rhs);
	const Corr operator+(const Corr &rhs) const;
};
struct CorrInfo {
	CorrInfo(){}
	CorrInfo(int nInputs);
	void clear();
	void add(const std::vector<float>& input, float target);
	std::vector<Corr> vCorr;
	std::vector<std::vector<Corr> > vvCorr;
	std::vector<Acc> vAcc;
	Acc accY;
};
std::ostream& operator <<(std::ostream& os, const Corr& obj);
std::istream& operator >>(std::istream& is, Corr& obj);
struct EconVal {
	Acc accTargetBottom;
	Acc accPredBottom;
	Acc accTargetTop;
	Acc accPredTop;

	void addBottom(double target, double pred);
	void addTop(double target, double pred);
	void clear();
	double econVal();
	double bias();
	double malpred();
};
struct CharaProfile {
	std::vector<float> v;
	std::map<std::string, int> mSymbolIndx;
	std::vector<int> vIndx;

	void read(const std::string& market, int idate, const std::string& condVar, int nP, const std::string& symbol);
	void read(const std::vector<std::string>& market, int idate, const std::string& condVar, int nP, const std::string& symbol);
	int get_indx(const std::string& symbol);
};
void printMemoryInfoSimple();
std::string getMemoryInfoSimple();
std::string getTimerInfoSimple();
void PrintMemoryInfoSimple();
unsigned long get_pid();
int change_timezone( int msecs, const std::string& tzFrom, const std::string& tzTo, int idate );
size_t getPeakRSS( );
size_t getCurrentRSS( );


#endif
