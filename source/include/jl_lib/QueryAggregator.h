#ifndef __QueryAggregator__
#define __QueryAggregator__
#include <string>
#include <vector>
#include <map>
#include <jl_lib/crossCompile.h>

class CLASS_DECLSPEC QueryAggregator {
public:
	QueryAggregator(){}
	void clear();
	void add(const std::string&);
	void exec(const std::string&);

private:
	std::map<std::string, std::vector<std::string> > m_;

	void exec_insert(const std::string& dbname, const std::string& id, const std::vector<std::string>& cmds);
	void exec_update(const std::string& dbname, const std::string& id, const std::vector<std::string>& cmds);
	std::string exec_update_get_col_name(const std::string& id);
	std::string exec_update_remove_and(const std::string& str);
};

#endif
