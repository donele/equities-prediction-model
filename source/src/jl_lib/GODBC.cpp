#include <jl_lib/GODBC.h>
#include <jl_lib/jlutil.h>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
using namespace std;

GODBC* GODBC::instance_ = 0;

GODBC* GODBC::Instance()
{
	static GODBC::Cleaner cleaner;
	if( instance_ == 0 )
		instance_ = new GODBC();
	return instance_;
}

GODBC::Cleaner::~Cleaner() {
	for( map<string, ODBCConnection*>::iterator it = GODBC::Instance()->m_.begin(); it != GODBC::Instance()->m_.end(); ++it )
		delete it->second;
	delete GODBC::instance_;
	GODBC::instance_ = 0;
}

GODBC::GODBC()
:debug_(false)
{}

void GODBC::set_debug(bool debug)
{
	debug_ = debug;
}

ODBCConnection* GODBC::get(string db)
{
	boost::algorithm::to_lower(db);
	boost::mutex::scoped_lock lock(odbc_mutex_);
	if( !m_.count(db) )
	{
		if( "hfOrderSumm" == db )
			m_[db] = new ODBCConnection(db.c_str(), "postgres", "DBacc101");
		else if( db.find("windstar") != string::npos )
			m_[db] = new ODBCConnection(db.c_str(), "windstar", "puddle");
		else
			m_[db] = new ODBCConnection(db.c_str(), "mercator1", "DBacc101");
		if( debug_ )
			cout << "GODBC::get(): New connection to " << db << " created, " << timestamp() << endl;
	}

	return m_[db];
}

void GODBC::close(string db)
{
	boost::algorithm::to_lower(db);
	if( debug_ )
		cout << "GODBC::close(): Closing connection to " << db << endl;
	if( m_.count(db) )
	{
		delete m_[db];
		m_.erase(db);
		if( debug_ )
			cout << " " << timestamp() << endl;
	}
	else
		if( debug_ )
			cout << " Connection to close was not found, " << timestamp() << endl;

}

void GODBC::close_all()
{
	vector<string> vdb;
	for( map<string, ODBCConnection*>::iterator it = m_.begin(); it != m_.end(); ++it )
		vdb.push_back(it->first);
	for( vector<string>::iterator it = vdb.begin(); it != vdb.end(); ++it )
		close(*it);
}

void GODBC::exec(string db, const string& query)
{
	boost::algorithm::to_lower(db);
	if( debug_ )
		cout << ">>> " << timestamp() << " " << db << " <<< " << query << endl;
	get(db)->ExecDirect(query.c_str());
	if( debug_ )
		cout << " Query finished, " << timestamp() << endl;
}

void GODBC::read(string db, const string& query, vector<vector<string> >& vv)
{
	boost::algorithm::to_lower(db);
	if( debug_ )
		cout << ">>> " << timestamp() << " " << db << " <<< " << query << endl;
	get(db)->ReadTable(query, &vv);
	if( debug_ )
		cout << " Query finished, " << timestamp() << endl;
}
