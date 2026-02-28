#ifndef __GODBC__
#define __GODBC__
#include "optionlibs/TickData.h"
#include <string>
#include <vector>
#include <map>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC GODBC {
public:
	static GODBC* Instance();

	void set_debug(bool debug);
	ODBCConnection* get(std::string db);
	void close(std::string db);
	void close_all();
	void exec(std::string db, const std::string& query);
	void read(std::string db, const std::string& query, std::vector<std::vector<std::string> >& vv);

private:
	bool debug_;
	static GODBC* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	GODBC();
	boost::mutex odbc_mutex_;

	std::map<std::string, ODBCConnection*> m_;
};

#endif
