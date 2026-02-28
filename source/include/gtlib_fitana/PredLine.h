#ifndef __gtlib_fitana_PredLine__
#define __gtlib_fitana_PredLine__
#include <vector>
#include <string>
#include <iostream>

namespace gtlib {

class PredLine {
public:
	PredLine(){}
	PredLine(const std::string& header);
	float getSprd();
	float getTarget();
	float getBmpred();
	std::vector<float> getPredSeries();
	std::vector<std::string> getPredLabels();
	friend std::istream& operator >>(std::istream& is, gtlib::PredLine& rhs);
private:
	int sprdIndex_;
	int targetIndex_;
	int bmpredIndex_;
	std::vector<int> vPredIndex_;
	std::vector<std::string> vPredLabel_;
	std::vector<float> v_;

	void initializeStorage(const std::vector<std::string>& tokens);
	void findIndexes(const std::vector<std::string>& tokens);
	void findPredLabelsIndexes(const std::vector<std::string>& tokens);
};

} // namespace gtlib


#endif
