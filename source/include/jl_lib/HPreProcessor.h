#ifndef __HPreprocessor__
#define __HPreprocessor__
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HPreprocessor {
public:
	HPreprocessor(std::istream& is, const std::vector<std::string>& lines, const std::string& confDir);
	std::istringstream& get_is();

private:
	std::string confDir_;
	std::vector<std::string> lines_;
	std::string lines_flattened_;
	std::istringstream iss_;

	std::set<std::string> keywords_;
	std::map<std::string, std::vector<std::string> > mv_;

	bool needs_processing();
	void preprocess();
	std::string substitute(std::string input, const std::string& token, const std::string& replacement);
	std::string substitute(std::string input);
	std::string substitute(std::string input, std::map<std::string, std::vector<std::string> >& m);
	void add_block(std::vector<std::string>& block,
			std::vector<std::string>::iterator& it, std::vector<std::string>::iterator itEnd);
	std::string get_next_line(std::vector<std::string>::iterator it, std::vector<std::string>::iterator itEnd);
	bool end_of_block(const std::vector<std::string>& lines);
	void append(std::string& output, const std::string& word);

	void process_set(const std::string& line);
	void process_if(const std::vector<std::string>& lines, const std::vector<std::string>& linesElse);
	void process_foreach(const std::vector<std::string>& lines);
	void process_call(const std::string& line);
	bool get_cond(const std::string& line);
};

#endif
