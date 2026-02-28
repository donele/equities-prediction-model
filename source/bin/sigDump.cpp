#include <gtlib_sigread/SignalBufferBin.h>
#include <gtlib_sigread/SignalBufferDecoratorHeader.h>
#include <jl_lib/Arg.h>
#include <vector>
#include <map>
#include <fstream>
#include <boost/filesystem.hpp>
using namespace std;
using namespace gtlib;

void usage()
{
	cout << "Usage:\n";
	cout << "  sigDump -p <path> -d <idate>\n";
	cout << "  sigDump -p <path> -d <idate> -v <varNames...>\n";
	cout << "  sigDump -p <path> -d <idate> -v <varNames...> -t <ticker>\n";
	exit(0);
}

class SigDump {
public:
	SigDump(){}
	SigDump(const Arg& arg);
	~SigDump();
	void run();
private:
	int NR_;
	int idate_;
	int msecs_;
	string dir_;
	string ticker_;
	string binPath_;
	vector<string> varNames_; 
	SignalBuffer* pSigBuf_;
	bool printTickers_;
	bool printVarNames_;
	bool printSelectVars_;
	bool printTickerSelectVarsTime_;
	bool printTickerSelectVars_;

	void parseArgs(const Arg& arg);
	void chooseAction(const Arg& arg);
	void printVarNames();
	void printSelectVars();
	void printTickerSelectVarsTime();
	void printTickerSelectVars();
	void printTickerSelectVarsHeader();
	void printTickers();
	string getBinPath();
};

SigDump::~SigDump()
{
	delete pSigBuf_;
}

// ----------------------------------------------------------------------------
SigDump::SigDump(const Arg& arg)
	:NR_(5),
	idate_(0),
	msecs_(0),
	dir_("."),
	printTickers_(false),
	printVarNames_(false),
	printSelectVars_(false),
	printTickerSelectVarsTime_(false),
	printTickerSelectVars_(false)
{
	parseArgs(arg);
	chooseAction(arg);
	binPath_ = getBinPath();
}

void SigDump::parseArgs(const Arg& arg)
{
	if( !arg.isGiven("d") )
		usage();

	string sdate = arg.getVal("d");
	idate_ = atoi(sdate.c_str());
	if( arg.isGiven("p") )
		dir_ = arg.getVal("p");
	varNames_ = arg.getVals("v");
	if( arg.isGiven("n") )
		NR_ = atoi(arg.getVal("n").c_str());
	ticker_ = arg.getVal("t");
	msecs_ = atoi(arg.getVal("s").c_str());
}

void SigDump::chooseAction(const Arg& arg)
{
	if( varNames_.empty() && ticker_.empty() )
	{
		if( arg.isGiven("t") )
			printTickers_ = true;
		else
			printVarNames_ = true;
	}
	else if( !varNames_.empty() && ticker_.empty() )
		printSelectVars_ = true;
	else if( !varNames_.empty() && !ticker_.empty() )
	{
		if( msecs_ > 0 )
			printTickerSelectVarsTime_ = true;
		else
			printTickerSelectVars_ = true;
	}
}

string SigDump::getBinPath()
{
	string path;
	if( idate_ > 19000000 && idate_ < 30000000 )
	{
		namespace fs = boost::filesystem;
		fs::path dir(dir_);
		fs::directory_iterator end_iter;
		for( fs::directory_iterator itd(dir); itd != end_iter; ++itd )
		{
			string filename = itd->path().filename().string();
			if( filename.find("sig") == 0 && filename.find(".bin") != string::npos )
			{
				int pos = filename.find("20");
				int filedate = atoi(filename.substr(pos, 8).c_str());
				if( filedate == idate_ )
				{
					path = dir_ + "/" + filename;
					break;
				}
			}
		}
	}
	return path;
}

// ----------------------------------------------------------------------------
void SigDump::run()
{
	pSigBuf_ = new gtlib::SignalBufferDecoratorHeader(
			new gtlib::SignalBufferBin(binPath_, varNames_), binPath_);
	if( pSigBuf_->is_open() )
	{
		if( printTickers_ )
			printTickers();
		else if( printVarNames_ )
			printVarNames();
		else if( printSelectVars_ )
			printSelectVars();
		else if( printTickerSelectVarsTime_ )
			printTickerSelectVarsTime();
		else if( printTickerSelectVars_ )
			printTickerSelectVars();
	}
}

void SigDump::printVarNames()
{
	vector<string> labels = pSigBuf_->getLabels();
	int cnt = 0;
	for( string label : labels )
		cout << ++cnt << "\t" << label << "\n";
}

void SigDump::printSelectVars()
{
	for( string name : varNames_ )
		cout << name << "\t";
	cout << "\n";

	int NI = varNames_.size();
	int cnt = 0;
	while( cnt < NR_ && pSigBuf_->next() )
	{
		for( int i = 0; i < NI; ++i )
			cout << pSigBuf_->getInput(i) << "\t";
		cout << "\n";
		++cnt;
	}
}

void SigDump::printTickerSelectVarsTime()
{
	int NI = varNames_.size();
	while( pSigBuf_->next() )
	{
		if( ticker_ == pSigBuf_->getTicker() )
		{
			int msecs = pSigBuf_->getMsecs();
			if( msecs == msecs_ )
			{
				printTickerSelectVarsHeader();

				cout << idate_ << "\t" << ticker_ << "\t" << msecs << "\t";
				for( int i = 0; i < NI; ++i )
					cout << pSigBuf_->getInput(i) << "\t";
				cout << "\n";
				break;
			}
		}
	}
}

void SigDump::printTickerSelectVars()
{
	int NI = varNames_.size();
	int cnt = 0;
	while( pSigBuf_->next() )
	{
		if( ticker_ == pSigBuf_->getTicker() )
		{
			if( cnt == 0 )
				printTickerSelectVarsHeader();

			int msecs = pSigBuf_->getMsecs();
			cout << idate_ << "\t" << ticker_ << "\t" << msecs << "\t";
			for( int i = 0; i < NI; ++i )
				cout << pSigBuf_->getInput(i) << "\t";
			cout << "\n";

			++cnt;
			if( cnt >= NR_ )
				break;
		}
	}
}

void SigDump::printTickerSelectVarsHeader()
{
	cout << "idate\tticker\tmsecs\t";
	for( string name : varNames_ )
		cout << name << "\t";
	cout << "\n";
}

void SigDump::printTickers()
{
	int cnt = 0;
	string prevTicker;
	while( pSigBuf_->next() )
	{
		string ticker = pSigBuf_->getTicker();
		if( ticker != prevTicker )
		{
			prevTicker = ticker;
			cout << ticker << " ";
			++cnt;
			if( cnt >= NR_ )
				break;
		}
	}
	cout << endl;
}

// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	if( argc < 2 )
		usage();
	Arg arg(argc, argv);
	SigDump sigDump(arg);
	sigDump.run();
	return 0;
}
