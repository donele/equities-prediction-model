#include <MCore/MCore.h>
#include <jl_lib/jlutil.h>
#include <string>
#include <vector>
using namespace std;

int main(int argc, char* argv[])
{
	if( argc > 1 )
	{
		multimap<string, string> amm = get_amm(argc, argv);
		typedef multimap<string, string>::iterator mmit;

		// Assignments.
		vector<string> lines;
		if( amm.count("a") )
		{
			pair<mmit, mmit> assignments = amm.equal_range("a");
			for( mmit mi = assignments.first; mi != assignments.second; ++mi )
			{
				string line = mi->second;
				lines.push_back(line);
			}
		}

		// Print and do nothing.
		bool print = false;
		if( amm.count("p") )
		{
			print = true;
			for( multimap<string, string>::iterator it = amm.begin(); it != amm.end(); ++it )
				cout << it->first << "\t" << it->second << endl;
		}

		string confDir = "/home/jelee/scripts/gtConf";
		if( amm.count("d") )
			confDir = amm.find("d")->second;

		string filename = argv[1];
		MCore core(confDir, filename, print, lines);
		core.run();
	}
}
