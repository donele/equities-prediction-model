#include <jl_lib/QueryAggregator.h>
#include <jl_lib/jlutil.h>
#include <jl_lib/GODBC.h>
#include <boost/algorithm/string.hpp>
using namespace std;

void QueryAggregator::clear() {
	m_.clear();
}

void QueryAggregator::add(const string& query) {
	vector<string> split_query = split(query);
	string id;
	if( split_query[0] == "insert" || split_query[0] == "INSERT" )
	{
		int pos1 = query.find(")");
		int pos2 = query.find("values", pos1);
		if( pos2 == string::npos )
			pos2 = query.find("VALUES", pos1);
		int pos = query.find("(", pos2);
		id = query.substr(0, pos); // Not include "(" in the id.
		boost::trim(id);
		m_[id].push_back(query.substr(pos));
	}
	else if( split_query[0] == "update" || split_query[0] == "UPDATE" )
	{
		int pos1 = query.find("set");
		if( pos1 == string::npos )
			pos1 = query.find("SET");
		int pos = query.find("=", pos1);
		id = query.substr(0, pos + 1); // Include "=" in the id.
		boost::trim(id);
		m_[id].push_back(query.substr(pos + 1));
	}
	else
	{
		cerr << "ERROR: QueryAggregator::add() parsing failure.\n";
		exit(10);
	}
}

void QueryAggregator::exec(const string& dbname) {
	for( map<string, vector<string> >::iterator it = m_.begin(); it != m_.end(); )
	{
		const string& id = it->first;
		const vector<string>& cmds = it->second;
		vector<string> split_id = split(id);
		if( split_id[0] == "insert" || split_id[0] == "INSERT" )
		{
			exec_insert(dbname, id, cmds);
			m_.erase(it++);
		}
		else if( split_id[0] == "update" || split_id[0] == "UPDATE" )
		{
			exec_update(dbname, id, cmds);
			m_.erase(it++);
		}
		else
			++it;
	}
}

void QueryAggregator::exec_insert(const string& dbname, const string& id, const vector<string>& cmds)
{
	string cmd = id;
	vector<string>::const_iterator cmdsB = cmds.begin();
	vector<string>::const_iterator cmdsE = cmds.end();
	for( vector<string>::const_iterator it = cmdsB; it != cmdsE; ++it )
	{
		if( it == cmdsB )
			cmd += *it;
		else
			cmd += "," + *it;
	}
	GODBC::Instance()->exec(dbname, cmd);
}

void QueryAggregator::exec_update(const string& dbname, const string& id, const vector<string>& cmds)
{
	string col_name = exec_update_get_col_name(id);

	// If idates are the same in all the conditions.
	string common_idate_cond;
	{
		int pos_where = cmds[0].find("where");
		if( pos_where == string::npos )
			pos_where = cmds[0].find("WHERE");
		if( pos_where != string::npos )
		{
			int pos_idate = cmds[0].find("idate", pos_where);
			if( pos_idate != string::npos )
			{
				int pos_end_cond = cmds[0].find(" ", pos_idate + 13);
				if( pos_end_cond != string::npos )
				{
					string idate_cond = cmds[0].substr(pos_idate, pos_end_cond - pos_idate);
					bool idate_cond_is_common = true;
					for( vector<string>::const_iterator it = cmds.begin(); it != cmds.end(); ++it )
						if( it->find(idate_cond) == string::npos )
						{
							idate_cond_is_common = false;
							break;
						}
					if( idate_cond_is_common )
						common_idate_cond = idate_cond;
				}
			}
		}
	}
	int common_cond_len = common_idate_cond.size();

	if( !col_name.empty() )
	{
		string cmd = id + " CASE ";
		vector<string>::const_iterator cmdsB = cmds.begin();
		vector<string>::const_iterator cmdsE = cmds.end();
		for( vector<string>::const_iterator it = cmdsB; it != cmdsE; ++it )
		{
			const string& original = *it;
			vector<string> split_original = split(original);
			string val = split_original[0]; // Extract the value.
			int pos = original.find(split_original[1]) + split_original[1].size(); // Remove 'where'.
			string processed = " WHEN " + original.substr(pos) + " THEN " + val;
			if( common_cond_len > 0 )
			{
				size_t pos_cond = processed.find(common_idate_cond);
				if( pos_cond != string::npos )
					processed.erase(pos_cond, common_cond_len);
				processed = exec_update_remove_and(processed);
			}
			cmd += processed;
		}
		cmd += " ELSE " + col_name;
		cmd += " END ";
		cmd += " WHERE " + common_idate_cond;
		GODBC::Instance()->exec(dbname, cmd);
	}
	else
	{
		cerr << "ERROR: QueryAggregator::exec_update() parsing failure.\n";
		exit(10);
	}
}

string QueryAggregator::exec_update_get_col_name(const string& id)
{
	string col_name;
	int pos_set = id.find("set");
	if( pos_set == string::npos )
		pos_set = id.find("SET");
	int pos_eq = id.find("=");
	int pos_col_name = pos_set + 3;
	if( pos_set > 0 && pos_col_name < pos_eq && pos_eq < id.size() )
	{
		col_name = id.substr(pos_col_name, pos_eq - (pos_col_name));
		boost::trim(col_name);
	}
	return col_name;
}

string QueryAggregator::exec_update_remove_and(const string& str)
{
	vector<string> split_str = split(str);
	vector<string>::iterator it_and = find(split_str.begin(), split_str.end(), "and");
	size_t index_and = distance(split_str.begin(), it_and);
	int N = split_str.size();
	if( index_and > 0 && index_and < N - 1 && (split_str[index_and - 1] == "and"
				|| split_str[index_and - 1] == "WHEN" || split_str[index_and + 1] == "THEN") )
	{
		string ret;
		for( int i = 0; i < N; ++i )
			if( i != index_and )
				ret += " " + split_str[i];
		return exec_update_remove_and(ret);

	}
	return str;
}

