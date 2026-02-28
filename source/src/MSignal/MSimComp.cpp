#include <MSignal/MSimComp.h>
#include <MSignal/SigSet.h>
#include <MSignal.h>
#include <MFramework.h>
#include <jl_lib.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
using namespace std;

MSimComp::MSimComp(const string& moduleName, const multimap<string, string>& conf)
:MModuleBase(moduleName),
debug_(false),
comp_mm_(false),
mmfit_("TB130"),
interval_(0)
{
	if( conf.count("debug") )
		debug_ = conf.find("debug")->second == "true";
	if( conf.count("compMM") )
		comp_mm_ = conf.find("compMM")->second == "true";
	if( conf.count("interval") )
		interval_ = atoi(conf.find("interval")->second.c_str());
	if( conf.count("event") )
		event_ = conf.find("event")->second;
	if( conf.count("mmfit") )
		mmfit_ = conf.find("mmfit")->second.c_str();
}

MSimComp::~MSimComp()
{}

void MSimComp::beginJob()
{
	cout << moduleName_ << "::beginJob()" << endl;
}

void MSimComp::beginDay()
{
	int idate = MEnv::Instance()->idate;

	if( event_ == "trade" )
	{
	}
	else if( comp_mm_ )
	{
		mSymbolMsecs_.clear();
		string filename = xpf("\\\\smrc-ltc-mrct43\\f\\jelee\\work\\mx\\hffit\\KR\\simPred\\hford_KR" + itos(idate) + mmfit_ + "_KQ2_3.txt");
		ifstream ifs(filename.c_str());
		if( ifs.is_open() )
		{
			string line;
			while( getline(ifs, line) )
			{
				vector<string> vs = split(line, '\t');
				if( vs[0] != "id" )
				{
					string ticker = vs[1].substr(2, 6);
					int msecs = ceil(atof(vs[3].c_str()) * 1000. - 0.5);
					mSymbolMsecs_[ticker].push_back(msecs);
				}
			}
		}
	}
}

void MSimComp::beginTicker(const string& ticker)
{
	const hff::LinearModel& linMod = MEnv::Instance()->linearModel;
	if( comp_mm_ )
	{
		string baseName = base_name(ticker);
		if( mSymbolMsecs_.count(baseName) )
		{
			vector<int>& vMsso = mSymbolMsecs_[baseName];
			MEvent::Instance()->add<vector<int> >(ticker, "vmsso", vMsso);
		}
	}
	else if( interval_ > 0 )
	{
		vector<int> vMsso;
		for( int sec = interval_; sec <= linMod.n1sec - interval_; sec += interval_ )
			vMsso.push_back( sec * 1000 );
		MEvent::Instance()->add<vector<int> >(ticker, "vmsso", vMsso);
	}
	else if( event_ == "trade" )
	{
		vector<int> vMsso;
		const vector<TradeInfo>* trades = static_cast<const vector<TradeInfo>*>(MEvent::Instance()->get(ticker, "trades"));
		for( vector<TradeInfo>::const_iterator it = trades->begin(); it != trades->end(); ++it )
		{
			if( it->msecs >= linMod.openMsecs && it->msecs <= linMod.closeMsecs )
			{
				vMsso.push_back(it->msecs);
			}
		}
		MEvent::Instance()->add<vector<int> >(ticker, "vmsso", vMsso);
	}
}

void MSimComp::endTicker(const string& ticker)
{
	MEvent::Instance()->remove<vector<int> >(ticker, "vmsso");
}

void MSimComp::endDay()
{
}

void MSimComp::endJob()
{
}
