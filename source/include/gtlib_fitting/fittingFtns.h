#ifndef __gtlib_fitting_fittingFtns__
#define __gtlib_fitting_fittingFtns__
#include <string>
#include <vector>

namespace gtlib {

std::string getStatDir(const std::string& fitDir, int udate);
std::string getCoefDir(const std::string& fitDir);
std::string getPredDir(const std::string& fitDir, int udate);
std::string getPredPath(const std::string& fitDir, int udate, int idate);
std::string getPredPath(const std::string& fitDir, const std::string& coefFitDir, int udate, int idate);
std::string getPredPath(const std::string& fitDir, int idate);
std::string getTxtPath(const std::string& binPath);
std::vector<int> get_fitdates(const std::vector<int>& idates, int udate = 0);
std::vector<int> getModelDates(const std::string& fitDir);
int getRecentModelDate(const std::string& fitDir, int idate); 
std::string getModelPath(const std::string& fitDir, int modelDate);
std::string getMarket(const std::string& path);
std::string getSigType(const std::string& targetName);

} // namespace gtlib

#endif
