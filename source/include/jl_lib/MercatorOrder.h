#ifndef __MercatorOrder__
#define __MercatorOrder__

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC MercatorOrder {
public:
	MercatorOrder();
	MercatorOrder(double p, int q, int qe, char s, int f, int o, int e, int bx, int bs, double b, double a , int as, int ax,
		char et = '\0', int st = 0, int ft = 0, double pr1 = 0., double pr10 = 0.);
	bool isFilledFull() const;
	bool isFilledIncl() const;

	double price;
	int qty;
	int qtyExec;
	char side;

	int feedMsecs;
	int orderMsecs;
	int eventMsecs;
	int quoteMsecs;
	int detMsecs; // < 0 if price improves.
	int matMsecs;
	int trdMsecs;
	int insertMsecs;
	int executeMsecs;

	int bidEx;
	int bidsize;
	double bid;
	double ask;
	int asksize;
	int askEx;

	char execType;
	int orderSchedType;
	int fillType;
	int quoteMatch; // 0: match not found. 1: prices and size in future. 2: prices match. 3: prices and size match.

	double gain;
	double ret1;
	double ret10;
	double retR;
	double retON;
	double pred1;
	double pred10;
};

#endif