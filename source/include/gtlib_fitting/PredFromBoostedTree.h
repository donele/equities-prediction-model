#ifndef __gtlib_fitting_PredFromBoostedTree__
#define __gtlib_fitting_PredFromBoostedTree__
#include <gtlib_fitting/FitData.h>
#include <gtlib_fitting/BoostedDecisionTree.h>
#include <vector>
#include <string>
#include <ctime>
#include <iostream>

namespace gtlib {

class PredFromBoostedTree {
public:
	PredFromBoostedTree();
	PredFromBoostedTree(FitData* pFitData, const std::string& fitDir, const std::string& coefFitDir, int udate, bool costWgt = false);
	PredFromBoostedTree(FitData* pFitData, const std::string& fitDir, const std::string& dbname, const std::string& dbtable, const std::string& mkt, int udate, bool costWgt);
	~PredFromBoostedTree();

	void calculatePred();
	void writePred();
private:
	int udate_;
	bool useOnlyOneModel_;
	bool costWgt_;
	std::string fitDir_;
	std::string coefFitDir_;
	std::string dbname_;
	std::string dbtable_;
	std::string mkt_;
	std::string modelSource_;
	std::vector<int> fitDates_;
	std::map<int, BoostedDecisionTree*> mUdateFitter_;
	int lenPredSeries_;
	int sprdIndex_;
	int bidSizeIndex_;
	int bidIndex_;
	int askIndex_;
	int askSizeIndex_;
	int intraTarIndex_;
	FitData* pFitData_;
	std::vector<std::vector<float>>* pvvPred_;

	void readModels();
	int getMinModelDate(const std::vector<int>& modelDates, int minFitDate);
	void calculatePredDay(int idate);
	std::vector<int> getUsedModelDates();
	BoostedDecisionTree* getFitter(int idate);
	int getModelDate(int idate);
	std::string getPredHeader();
	void printPredRow(std::ostream& os, int iSample);
};

} // namespace gtlib

#endif
