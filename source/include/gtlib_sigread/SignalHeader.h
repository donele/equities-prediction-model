#ifndef __gtlib_sigread_SignalHeader_
#define __gtlib_sigread_SignalHeader_
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

namespace gtlib {

class SignalHeader {
public:
	SignalHeader(){}
	SignalHeader(const std::string& txtPath);

	bool is_open();
	bool next();

	const std::string& getTicker();
	const std::string& getUid();
	int getMsecs();
	char getBidEx();
	char getAskEx();
private:
	std::ifstream ifsTxt_;
	std::vector<std::string> labels_;
	std::map<std::string, int> mIndex_;
	std::string uid_;
	std::string ticker_;
	int time_;
	std::string market_;
	char bidEx_;
	char askEx_;
};

} // namespace gtlib

#endif
