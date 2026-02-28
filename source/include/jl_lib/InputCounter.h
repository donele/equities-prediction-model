#ifndef __InputCounter__
#define __InputCounter__
#include <string>
#include <iostream>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC InputCounter {
public:
	InputCounter(){}
	InputCounter(const std::string& desc, double base = 2);
	InputCounter(std::ostream &os, const std::string& desc, double base = 2);
	~InputCounter();

	void operator++();

private:
	std::string desc_;
	double base_;
	double nInputs_;
	double N_;
	double p_;
	std::ostream* os_;

	void print();
};

#endif
