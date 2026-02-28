#ifndef __gtlib_fitting_DTFtns__
#define __gtlib_fitting_DTFtns__
#include <string>
#include <vector>
#include <memory>

namespace gtlib {

class FitData;
class DecisionTree;
class DTNode;

std::shared_ptr<DTNode> make_node(std::vector<std::string>::const_iterator& itFrom,
		std::vector<std::string>::const_iterator& itTo);
void DTreadModel(const std::string& path, std::vector<DecisionTree*>& vpT, std::vector<double>& wts);
void DTreadModel(const std::string& dbname, const std::string& dbtable, const std::string& mkt,
		int udate, std::vector<DecisionTree*>& vpT, std::vector<double>& wts);
int getPredStep(int ntrees);
void DTupdateTarget(FitData* pFitData, int iTree, std::vector<DecisionTree*>& vpT, std::vector<double>& wts);
void DTwriteModel(const std::string& path, std::vector<DecisionTree*>& vpT, std::vector<double>& wts);
void writeNode(std::ofstream& ofs, std::shared_ptr<DTNode> z, int iTree, double weight);
void DTstat(FitData* pFitData, std::vector<DecisionTree*>& vpT, const std::string& fitDir, int udate);
float DTgetPred(const std::vector<float>& input, std::vector<DecisionTree*>& vpT, std::vector<double>& wts);
std::vector<float> DTgetPredSeries(int iSample, FitData* p,
		std::vector<DecisionTree*>& vpT, std::vector<double>& wts);
std::vector<std::string> DTgetPredSeriesNames(int ntrees);
std::vector<std::string>::const_iterator getPositionFrom(const std::vector<std::string>& lines, int iTree);
std::vector<std::string>::const_iterator getPositionTo(const std::vector<std::string>& lines, int iTree);

} // namespace gtlib

#endif
