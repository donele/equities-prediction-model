#include <HNews.h>
#include <HLib/HEvent.h>
#include <HLib/HEnv.h>
//#include <jl_lib/NewsCatRP.h>
#include <optionlibs/TickData.h>
#include <map>
#include <jl_lib.h>
#include <string>
#include "TFile.h"
using namespace std;

HRavenPackRead::HRavenPackRead(const string& moduleName, const multimap<string, string>& conf)
:HModule(moduleName),
verbose_(0),
input_dir_("\\\\smrc-nas09\\gf1\\tickR\\RavenPack\\v1.4")
{
	if( conf.count("verbose") )
		verbose_ = atoi( conf.find("verbose")->second.c_str() );
	if( conf.count("input_file") )
		input_file_ = conf.find("input_file")->second;
}

HRavenPackRead::~HRavenPackRead()
{}

void HRavenPackRead::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;

	return;
}

void HRavenPackRead::beginMarket()
{
	return;
}

void HRavenPackRead::beginDay()
{
	int idate = HEnv::Instance()->idate();
	read_text(idate);

	return;
}

void HRavenPackRead::endDay()
{
	return;
}

void HRavenPackRead::endMarket()
{
	return;
}

void HRavenPackRead::endJob()
{
	return;
}

void HRavenPackRead::read_text(int idate)
{
	lines_.clear();
	if( !temp_line_.empty() )
		lines_.push_back(temp_line_);
	temp_line_ = "";

	if( !ifs_.is_open() )
	{
		char filename[200];
		int yyyy = idate / 10000;
		int mm = idate / 100 % 100;
		sprintf(filename, "%s\\%d\\%d-%02d.csv", input_dir_.c_str(), yyyy, yyyy, mm);
		ifs_.open(filename);
	}

	string line = "";
	while( getline(ifs_, line) )
	{
		int line_idate = get_idate(line);
		if( line_idate < idate )
			continue;
		else if( line_idate == idate )
			lines_.push_back(line);
		else if( line_idate > idate )
		{
			temp_line_ = line;
			break;
		}
	}

	if( ifs_.eof() )
	{
		ifs_.close();
		ifs_.clear();
	}

	HEvent::Instance()->add("", "lines", lines_);
	return;
}

int HRavenPackRead::get_idate(string& line)
{
	int idate = 0;
	if( line.size() > 10 )
		idate = atoi(line.substr(0,4).c_str())*10000 + atoi(line.substr(5,2).c_str())*100 + atoi(line.substr(8,2).c_str());
	return idate;
}
