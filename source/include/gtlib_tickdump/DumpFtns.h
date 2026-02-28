#ifndef __gtlib_tickdump_DumpFtns__
#define __gtlib_tickdump_DumpFtns__
#include <string>
#include <optionlibs/TickData.h>

namespace gtlib {

std::string getDumpMeta(int idate, const std::string& ticker, int nticks);
template <typename T>
std::string getDumpHeader();
template <typename T>
std::string getDumpContent(const T& t);

} // namespace gtlib

#endif
