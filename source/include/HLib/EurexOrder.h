#ifndef __EurexOrder__
#define __EurexOrder__

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC EurexOrder {
public:
	EurexOrder();
	EurexOrder(double p, int q, char s, int o, int e);
	bool match() const;

	double price;
	int qty;
	char side;
	int orderMsecs;
	int eventMsecs;
	int tradeMsecs;
	int quoteMsecs;

	std::string tickerA;
	double priceA;
	int qtyA;
	int orderMsecsA;
	int eventMsecsA;
	int tradeMsecsA;
	int quoteMsecsA;

	double rpnl;
	//double loss;
};

#endif