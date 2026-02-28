#include <jl_lib/HPreProcessor.h>
#include <string>
#include <jl_lib/jlutil.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
using namespace std;

HPreprocessor::HPreprocessor(istream& is, const vector<string>& lines, const string& confDir)
	:confDir_(confDir)
{
	keywords_.insert("set");
	keywords_.insert("foreach");
	keywords_.insert("call");
	keywords_.insert("if");

	if( 1 )
	{
		if( !lines.empty() )
		{
			for( auto line : lines )
			{
				if( !line.empty() )
					lines_.push_back(line);
			}
		}

		string line;
		while( !is.eof() )
		{
			getline(is, line);
			boost::trim(line);
			if(!line.empty() && line[0] != '#')
				lines_.push_back(line);
		}

		while( needs_processing() )
			preprocess();
	}
}

bool HPreprocessor::needs_processing()
{
	for( auto line : lines_ )
	{
		vector<string> sl = split(line);
		if( !sl.empty() && keywords_.count(sl[0]) )
			return true;
	}
	return false;
}

void HPreprocessor::preprocess()
{
	{
		vector<string> lines_copied = lines_;
		lines_.clear();
		auto linesEnd = end(lines_copied);
		for( auto it = begin(lines_copied); it != linesEnd; ++it )
		{
			string line = *it;
			vector<string> sl = split(line);
			int sl_size = sl.size();
			if( sl_size > 0 )
			{
				if( sl[0] == "set" )
					process_set(line);
				else
					lines_.push_back(line);
			}
		}
	}

	{
		vector<string> lines_copied = lines_;
		lines_.clear();
		auto linesEnd = end(lines_copied);
		for( auto it = begin(lines_copied); it != linesEnd; ++it )
		{
			string line = *it;
			vector<string> sl = split(line);
			int sl_size = sl.size();
			if( sl_size > 0 )
			{
				if( sl[0] == "call" )
					process_call(line);
				else
					lines_.push_back(line);
			}
		}
	}

	{
		vector<string> lines_copied = lines_;
		lines_.clear();
		auto linesEnd = end(lines_copied);
		for( auto it = begin(lines_copied); it != linesEnd; ++it )
		{
			string line = *it;
			vector<string> sl = split(line);
			int sl_size = sl.size();
			if( sl_size > 0 )
			{
				if( sl[0] == "set" )
					process_set(line);
				else
					lines_.push_back(line);
			}
		}
	}

	{
		vector<string> lines_copied = lines_;
		lines_.clear();
		auto linesEnd = end(lines_copied);
		for( auto it = begin(lines_copied); it != linesEnd; ++it )
		{
			string line = *it;
			vector<string> sl = split(line);
			int sl_size = sl.size();
			if( sl_size > 0 )
			{
				if( line.find("$") != string::npos )
					lines_.push_back(substitute(line));
				else
					lines_.push_back(line);
			}
		}
	}

	{
		vector<string> lines_copied = lines_;
		lines_.clear();
		auto linesEnd = end(lines_copied);
		for( auto it = begin(lines_copied); it != linesEnd; ++it )
		{
			string line = *it;
			vector<string> sl = split(line);
			int sl_size = sl.size();
			if( sl_size > 0 )
			{
				if( sl[0] == "if" )
				{
					vector<string> block;
					add_block(block, it, linesEnd);

					string nextLine = get_next_line(it, linesEnd);
					vector<string> nsl = split(nextLine);

					vector<string> blockElse;
					if( !nsl.empty() && nsl[0] == "else" )
					{
						++it;
						add_block(blockElse, it, linesEnd);
					}

					process_if(block, blockElse);
				}
				else if( sl[0] == "foreach" )
				{
					vector<string> block;
					add_block(block, it, linesEnd);

					process_foreach(block);
				}
				else
					lines_.push_back(line);
			}
		}
	}
	return;
}

void HPreprocessor::add_block(vector<string>& block, vector<string>::iterator& it, vector<string>::iterator itEnd)
{
	for( ; it != itEnd; ++it )
	{
		string line = *it;
		block.push_back(substitute(line));
		if( end_of_block(block) )
			break;
	}
}

string HPreprocessor::get_next_line(vector<string>::iterator it, vector<string>::iterator itEnd)
{
	string ret;
	if( ++it != itEnd )
		ret = *it;
	return ret;
}

bool HPreprocessor::end_of_block(const vector<string>& lines)
{
	int cntAll = 0;
	int cntLeft = 0;
	int cntRight = 0;
	int trailing = 0;
	for( auto line : lines )
	{
		int N = line.size();
		for( int i=0; i<N; ++i )
		{
			char c = line[i];
			if( c == '{' )
			{
				++cntAll;
				++cntLeft;
			}
			else if( c == '}' )
			{
				++cntAll;
				++cntRight;
				trailing = 0;
			}
			else if( c == ' ' || c == '\t' )
				++trailing;
		}
	}

	if( cntAll > 1 && cntLeft > 0 && cntRight > 0 )
	{
		if( cntLeft == cntRight && trailing == 0 )
			return true;
		else if( cntLeft < cntRight )
		{
			cerr << "HPreprocessor: Brackets do not match.\n";
			exit(3);
		}
		else if( cntLeft == cntRight && trailing > 0 )
		{
			cerr << "HPreprocessor: Trailing characters.\n";
			exit(3);
		}
	}
	return false;
}

istringstream& HPreprocessor::get_is()
{
	lines_flattened_.clear();
	for( auto line : lines_ )
		lines_flattened_ += line + "\n";
	iss_.str(lines_flattened_);
	return iss_;
}

void HPreprocessor::process_set(const string& line)
{
	vector<string> sl = split(line);
	int sl_size = sl.size();
	if( sl_size >= 2 )
	{
		string name = sl[1];
		if( sl_size == 2 )
			mv_[name] = vector<string>(0);
		else if( sl_size > 2 )
		{
			vector<string> val;
			for( int i = 2; i < sl_size; ++i )
			{
				string substituted = substitute(sl[i]);
				vector<string> temp_val = split(substituted);
				for( auto it = begin(temp_val); it != end(temp_val); ++it )
					val.push_back(*it);
			}
			mv_[name] = val;
		}
	}
	return;
}

void HPreprocessor::process_if(const vector<string>& lines, const vector<string>& linesElse)
{
	bool cond = get_cond(lines[0]);

	if( cond )
	{
		int nLines = lines.size();
		for( int i = 1; i < nLines - 1; ++i )
			lines_.push_back(lines[i]);
	}
	else
	{
		int nLines = linesElse.size();
		for( int i = 1; i < nLines - 1; ++i )
			lines_.push_back(linesElse[i]);
	}

	return;
}

bool HPreprocessor::get_cond(const string& line)
{
	vector<string> sl = split(line);
	int sl_size = sl.size();
	if( sl_size >= 4 && sl[0] == "if" )
	{
		if( sl[2] == "begins" )
		{
			string word = substitute(sl[1]);
			string pattern = unquote(sl[3]);
			if( word.size() >= pattern.size() && word.substr(0, pattern.size()) == pattern )
				return true;
		}
	}
	return false;
}

void HPreprocessor::process_foreach(const vector<string>& lines)
{
	vector<string> sl = split(lines[0]);
	int sl_size = sl.size();
	if( sl_size < 3 )
	{
		cerr << "HPreprocessor: Unknown parsing error.\n";
		exit(3);
	}

	string token = sl[1];
	//string temp_list = substitute(sl[2]);
	//vector<string> list = split(temp_list);
	string temp_list;
	for( int i = 2; i < sl_size - 1; ++i )
		append(temp_list, substitute(sl[i]));
	vector<string> list = split(temp_list);

	vector<string> body;
	int nLines = lines.size();
	for( int i=1; i<nLines - 1; ++i )
		body.push_back(lines[i]);

	for( auto replacement : list )
	{
		for( auto line : body )
		{
			string line2;
			vector<string> sl = split(line);
			for( auto word : sl )
			{
				string substituted = substitute(word, token, replacement);
				append(line2, substituted);
			}
			lines_.push_back(line2);
		}
	}

	return;
}

void HPreprocessor::process_call(const string& line)
{
	vector<string> sl = split(line);
	int sl_size = sl.size();
	if( sl_size == 2 )
	{
		string temp_filename = xpf(sl[1]);
		string filename = substitute(temp_filename);

		string path = filename;
		if( !boost::filesystem::exists(filename) && !confDir_.empty() )
			path = confDir_ + '/' + filename;

		ifstream ifs(path.c_str());
		if( ifs.is_open() )
		{
			string input;
			while( getline(ifs, input) )
			{
				boost::trim(input);
				if( input.find("$") != string::npos )
					lines_.push_back(substitute(input));
				else
					lines_.push_back(input);
			}
		}
	}
	return;
}

string HPreprocessor::substitute(string input, const string& token, const string& replacement)
{
	map<string, vector<string> > m;
	m[token] = vector<string>(1, replacement);
	string ret = substitute(input, m);
	return ret;
}

string HPreprocessor::substitute(string input)
{
	string ret = substitute(input, mv_);
	return ret;
}

string HPreprocessor::substitute(string input, map<string, vector<string> >& m)
{
	if( input.find('$') != string::npos )
	{
		string temp_output;

		// Match the whole.
		bool whole_match = false;
		if( input[0] == '$' )
		{
			int input_size = input.size();
			string key = "";
			if( input[1] == '(' && input[input_size - 1] == ')' )
				key = input.substr(2, input_size - 3);
			else
				key = input.substr(1, input_size - 1);
			if( m.count(key) )
			{
				whole_match = true;

				vector<string> vals = m[key];
				for( auto aVal : vals )
					append(temp_output, aVal);
			}
		}

		// Partial match.
		if( !whole_match )
		{
			for( int pos = input.find('$'); pos != string::npos; pos = input.find('$', pos + 1) )
			{
				if( input[pos + 1] == '(' )
				{
					int pos_left = pos + 1;
					int pos_right = input.find(')', pos_left);
					if( pos_right != string::npos )
					{
						string key = input.substr(pos_left + 1, pos_right - pos_left - 1);
						if( m.count(key) )
						{
							//vector<string> val = m[key];
							//for( auto str : val )
							//{
							//	string substituted = input.replace(pos, pos_right - pos + 1, str);
							//	append(temp_output, substituted);
							//}
							vector<string> val = m[key];
							string valStr;
							for( auto str : val )
								valStr += (str + ' ');
							string substituted = input.replace(pos, pos_right - pos + 1, trim(valStr));
							temp_output = substituted;
						}
					}
				}
				else
				{
					int input_size = input.size();
					for( int posTo = pos + 1; posTo < input_size; ++posTo )
					{
						string key = input.substr(pos + 1, posTo - pos);
						if( m.count(key) )
						{
							//vector<string> val = m[key];
							//for( auto str : val )
							//{
							//	string substituted = input.replace(pos, posTo - pos + 1, str);
							//	append(temp_output, substituted);
							//}
							vector<string> val = m[key];
							string valStr;
							for( auto str : val )
								valStr += (str + ' ');
							string substituted = input.replace(pos, posTo - pos + 1, trim(valStr));
							temp_output = substituted;
						}
					}
				}
			}
		}

		if( !temp_output.empty() )
		{
			string output;
			vector<string> temp_split = split(temp_output);
			for( auto word : temp_split )
			{
				append(output, substitute(word, m));
			}
			return output;
		}
	}
	return input;
}

void HPreprocessor::append(string& output, const string& word)
{
	if( output.empty() )
		output = word;
	else
		output += (string)" " + word;
	return;
}
