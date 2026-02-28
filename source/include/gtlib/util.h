#ifndef __gtlib_util__
#define __gtlib_util__
#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace gtlib {

std::string concat(const std::vector<std::string>& v);
std::string firstToUpper(const std::string& str);
std::vector<float> getNextLineSplitFloat(std::istream& is);
std::vector<std::string> getNextLineSplitString(std::istream& is);
std::vector<char> getConfigValuesChar(const std::multimap<std::string, std::string>& conf, std::string id);
std::vector<std::string> getConfigValuesString(const std::multimap<std::string, std::string>& conf, std::string id);
std::vector<int> getConfigValuesInt(const std::multimap<std::string, std::string>& conf, std::string id);
std::vector<float> getConfigValuesFloat(const std::multimap<std::string, std::string>& conf, std::string id);
std::vector<double> getConfigValuesDouble(const std::multimap<std::string, std::string>& conf, std::string id);

} // namespace gtlib

#endif
