#include <jl_lib/HConfig.h>
#include <jl_lib/HPreProcessor.h>
#include <jl_lib/jlutil.h>
#include <sstream>
#include <boost/algorithm/string.hpp>
using namespace std;

HConfig::HConfig(istream& is, const vector<string>& lines, const string& confDir)
{
	conf_.clear();
	HPreprocessor pre(is, lines, confDir);
	parse_conf(pre.get_is());
}

void HConfig::add_module(const pair<ModuleInfo, multimap<string, string> >& module)
{
	conf_.push_back( module );
}

vector<pair<ModuleInfo, multimap<string, string> > > HConfig::get_block()
{
	return conf_;
}

void HConfig::parse_conf(istream& is)
{
	// Gather the information from the config file.
	conf_.clear();

	// vector of pairs of className and moduleName.
	vector<ModuleInfo> modules;

	// map of moduleName and options.
	map<string, multimap<string, string> > options;

	string line;
	while( !is.eof() )
	{
		getline(is, line);
		boost::trim(line);
		vector<string> tokens = split(line);
		if( tokens.size() == 2 && tokens[0] == "talk" )
		{
			if( trim(tokens[1]) == "main" )
				get_modules( is, modules );
			else
			{
				string moduleName = tokens[1];
				get_options( is, moduleName, options );
			}
		}
	}

	// checks.
	for( auto it = begin(options); it != end(options); ++it )
	{
		string moduleName = it->first;
		bool moduleFound = false;
		for( auto it = begin(modules); it != end(modules); ++it )
		{
			if( it->moduleName == moduleName )
			{
				moduleFound = true;
				break;
			}
		}
		if( !moduleFound )
			cout << "HConfig:Warning Unknown module " << moduleName << "\n";
	}

	// Fill the conf object.
	for( auto it = modules.begin(); it != modules.end(); ++it )
	{
		string moduleName = it->moduleName;
		conf_.push_back( make_pair((*it), options[moduleName]) );
	}

	return;
}

void HConfig::get_modules( istream& is, vector<ModuleInfo>& modules )
{
	string line;
	while( !is.eof() )
	{
		getline(is, line);
		boost::trim(line);
		vector<string> tokens = split(line);
		int ts = tokens.size();
		if( (ts >= 2 && ts <= 4) && tokens[0] == "module" )
		{
			ModuleInfo mi("", "", -1);
			mi.className = tokens[1];
			mi.moduleName = mi.className;
			if( ts == 3 )
			{
				// if numbers
				bool isnum = true;
				for( int i=0; i<tokens[2].size(); ++i )
				{
					char c = tokens[2][i];
					if( c < '0' || c > '9' )
					{
						isnum = false;
						break;
					}
				}
				if( isnum )
					mi.execOrder = atoi( tokens[2].c_str() );
				else
					mi.moduleName = tokens[2];
			}
			else if( ts == 4 )
			{
				mi.moduleName = tokens[2];
				mi.execOrder = atoi(tokens[3].c_str());
			}
			modules.push_back(mi);
		}
		else if( trim(line) == "exit" )
			return; // Should return here.
		else
		{
			vector<string> tokens = split(line);
			if( !tokens.empty() )
			{
				if( !tokens[0].empty() && tokens[0][0] == '#' )
					;
				else
					cout << "HConfig:Warning Unknown option " << tokens[0] << "\n";
			}
		}
	}
	exit(1);
	return;
}

void HConfig::get_options( istream& is, const string& moduleName,
		map<string, multimap<string, string> >& options )
{
	string line;
	while( !is.eof() )
	{
		getline(is, line);
		vector<string> tokens = split(line);
		int tsize = tokens.size();
		if( trim(line) == "exit" )
			return; // Should return here.
		else if( tsize > 1 )
		{
			string option = tokens[0];
			string val = tokens[1];
			for( int i=2; i<tsize; ++i )
				val += (string)" " + tokens[i];

			if( !val.empty() && val[0] == '$' )
				val = "";
			options[moduleName].insert(make_pair(option, val));
		}
		else if( tsize == 1 )
		{
			vector<string> tokens = split(line);
			string option = tokens[0];
			options[moduleName].insert(make_pair(option, ""));
		}
	}
	exit(1);
	return;
}

ostream& operator <<(ostream& os, const vector<pair<ModuleInfo, multimap<string, string> > >& obj)
{
	char buf[200];
	printf("-- Block Configuration -----------------\n");
	for( auto it = obj.begin(); it != obj.end(); ++it )
	{
		const ModuleInfo& modInfo = it->first;
		sprintf(buf, "\n%s %s %d\n",
				modInfo.className.c_str(), modInfo.moduleName.c_str(), modInfo.execOrder);
		cout << buf;

		const multimap<string, string>& options = it->second;
		for( auto ito = options.begin(); ito != options.end(); ++ito )
		{
			sprintf(buf, "  %s %s\n", ito->first.c_str(), ito->second.c_str());
			cout << buf;
		}
	}
	printf("----------------------------------------\n");
	os << endl;

	return os;
}





HConfig2::HConfig2(istream& is, const vector<string>& lines)
{
	conf_.clear();
	HPreprocessor pre(is, lines, "");
	parse_conf(pre.get_is());
}

void HConfig2::parse_conf(istream& is)
{
	// Gather the information from the config file.
	conf_.clear();

	// vector of pairs of className and moduleName.
	vector<ModuleInfo> modules;

	// map of moduleName and options.
	map<string, multimap<string, string> > options;

	string line;
	while( !is.eof() )
	{
		getline(is, line);
		boost::trim(line);
		vector<string> tokens = split(line);
		if( tokens.size() == 2 && tokens[0] == "talk" )
		{
			if( trim(tokens[1]) == "main" )
				modules = find_modules(is);
			else
			{
				string moduleName = tokens[1];
				get_options( is, moduleName, options );
			}
		}
	}

	// checks.
	for( auto it = begin(options); it != end(options); ++it )
	{
		string moduleName = it->first;
		bool moduleFound = false;
		for( auto it = begin(modules); it != end(modules); ++it )
		{
			if( it->moduleName == moduleName )
			{
				moduleFound = true;
				break;
			}
		}
		if( !moduleFound )
			cout << "HConfig2:Warning Unknown module " << moduleName << "\n";
	}

	// Fill the conf object.
	for( auto it = modules.begin(); it != modules.end(); ++it )
	{
		string moduleName = it->moduleName;
		//conf_.push_back( make_pair((*it), options[moduleName]) );
	}

	return;
}

vector<ModuleInfo> HConfig2::find_modules(istream& is)
{
	vector<ModuleConfig> modules;
	bool success = false;

	string line;
	while( !is.eof() )
	{
		getline(is, line);
		boost::trim(line);
		vector<string> tokens = split(line);
		int ts = tokens.size();
		if( (ts >= 2 && ts <= 4) && tokens[0] == "module" )
		{
			ModuleConfig mi("", "", -1);
			mi.className = tokens[1];
			mi.moduleName = mi.className;
			if( ts == 3 )
			{
				// if numbers
				bool isnum = true;
				for( int i=0; i<tokens[2].size(); ++i )
				{
					char c = tokens[2][i];
					if( c < '0' || c > '9' )
					{
						isnum = false;
						break;
					}
				}
				if( isnum )
					mi.execOrder = atoi( tokens[2].c_str() );
				else
					mi.moduleName = tokens[2];
			}
			else if( ts == 4 )
			{
				mi.moduleName = tokens[2];
				mi.execOrder = atoi(tokens[3].c_str());
			}
			modules.push_back(mi);
		}
		else if( trim(line) == "exit" )
		{
			success = true;
			break;
		}
		else
		{
			vector<string> tokens = split(line);
			if( !tokens.empty() )
			{
				if( !tokens[0].empty() && tokens[0][0] == '#' )
					;
				else
					cout << "HConfig2:Warning Unkown option " << tokens[0] << "\n";
			}
		}
	}
	if( !success )
		exit(1);
	return vector<ModuleInfo>();
}

void HConfig2::get_options( istream& is, const string& moduleName,
		map<string, multimap<string, string> >& options )
{
	string line;
	while( !is.eof() )
	{
		getline(is, line);
		vector<string> tokens = split(line);
		int tsize = tokens.size();
		if( trim(line) == "exit" )
			return; // Should return here.
		else if( tsize > 1 )
		{
			string option = tokens[0];
			string val = tokens[1];
			for( int i=2; i<tsize; ++i )
				val += (string)" " + tokens[i];

			if( !val.empty() && val[0] == '$' )
				val = "";
			options[moduleName].insert(make_pair(option, val));
		}
		else if( tsize == 1 )
		{
			vector<string> tokens = split(line);
			string option = tokens[0];
			options[moduleName].insert(make_pair(option, ""));
		}
	}
	exit(1);
	return;
}

ostream& operator <<(ostream& os, const vector<ModuleConfig>& obj)
{
	printf("-- Block Configuration -----------------\n");
	for( auto it = begin(obj); it != end(obj); ++it )
	{
		//printf("\n%s %s %d\n", obj.className.c_str(), obj.moduleName.c_str(), obj.execOrder);

		//const multimap<string, string>& options = it->second;
		//for( multimap<string, string>::const_iterator ito = options.begin(); ito != options.end(); ++ito )
		//{
		//	printf("  %s %s\n", ito->first.c_str(), ito->second.c_str());
		//}
	}
	printf("----------------------------------------\n");
	os << endl;

	return os;
}
