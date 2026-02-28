#ifndef __gtlib_model_pathFtns__
#define __gtlib_model_pathFtns__
#include <vector>
#include <fstream>

namespace gtlib {

// Paths.
//std::string get_np_dir(const std::string& baseDir, const std::string& model, const std::string& fitDesc);
//std::string get_np_path(const std::string& baseDir, const std::string& model,
		//int idate, const std::string& fitDesc);
std::string get_sig_dir(const std::string& baseDir, const std::string& model,
		const std::string& sigType, const std::string& fitDesc, const std::string& sigFormat);
std::string get_txt_path(const std::string& baseDir, const std::string& model,
		int idate, const std::string& sigType, const std::string& fitDesc);
std::string get_sig_path(const std::string& baseDir, const std::string& model,
		int idate, const std::string& sigType, const std::string& fitDesc);
std::string get_sigTxt_path(const std::string& baseDir, const std::string& model,
		int idate, const std::string& sigType, const std::string& fitDesc);
std::string get_fit_dir(const std::string& baseDir, const std::string& model,
		const std::string& targetName, const std::string& fitDesc);
std::string getTargetName(const std::string& sigType, const std::string& fitDesc);
std::string get_pred_dir(const std::string& baseDir, const std::string& model,
		int idate, const std::string& targetName, const std::string& fitDesc, int udate);
std::string get_pred_path(const std::string& baseDir, const std::string& model,
		int idate, const std::string& targetName, const std::string& fitDesc,
		int udate = 0, bool is_oos = true);
std::string get_predvar_path(const std::string& baseDir, const std::string& model,
		int idate, const std::string& targetName);
std::string get_cov_dir(const std::string& baseDir, const std::string& model);
std::string get_tmCov_dir(const std::string& baseDir, const std::string& model);
std::string get_desc(const std::string& fitDesc);
std::string get_weight_dir(const std::string& baseDir, const std::string& model);
std::string get_tmWeight_dir(const std::string& baseDir, const std::string& model);
std::vector<int> get_modelDates(const std::string& coefDir);
int get_modelDate(const std::string& coefDir, int testDate);
std::vector<int> getDiskIdates(const std::string& path);
int getIdateToBeginWith(const std::string& dir, const std::string& market);
int getIdateToBeginWith(const std::string& dir, const std::string& market,
		const std::vector<int>& idateRange);
int getIdateToBeginWithPred(const std::string& predDir, const std::string& market);
};

#endif
