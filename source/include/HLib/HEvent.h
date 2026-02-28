#ifndef __HEvent__
#define __HEvent__
#include <map>
#include <string>
#include <boost/thread.hpp>

#ifdef _WIN32
#define CLASS_DECLSPEC __declspec(dllexport)
#define FUNC_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC
#define FUNC_DECLSPEC
#endif

class CLASS_DECLSPEC HEvent {
public:
	static HEvent* Instance();
	void clear();

	bool find(std::string ticker, std::string desc) const;
	const void* get(std::string ticker, std::string desc) const;
	template<class T> void add(std::string ticker, std::string desc, T* p);
	template<class T> void add(std::string ticker, std::string desc, const T& v);
	template<class T> void remove(std::string ticker, std::string desc);
	template<class T> const T& get(std::string ticker, std::string desc);

private:
	static HEvent* instance_;
	struct Cleaner { ~Cleaner(); };
	friend struct Cleaner;
	HEvent();
	HEvent( const HEvent& );
	const HEvent& operator=( const HEvent& );

	mutable boost::mutex mp_mutex_;
	std::map<std::string, std::map<std::string, void*> > mp_;
};

template <class T>
void HEvent::add(std::string ticker, std::string desc, T* p) // Add a pointer. Do no make a copy of T.
{
	boost::mutex::scoped_lock lock(mp_mutex_);
	mp_[ticker][desc] = static_cast<void*>(p);

	return;
}

template <class T>
void HEvent::add(std::string ticker, std::string desc, const T& v) // Make a copy.
{
	// Delete if exists.
	boost::mutex::scoped_lock lock(mp_mutex_);
	if( mp_[ticker].count(desc) )
	{
		T* oldp = static_cast<T*>(mp_[ticker][desc]);
		if( oldp != 0 )
			delete oldp;
	}

	// Add.
	T* p = new T(v);
	mp_[ticker][desc] = static_cast<void*>(p);

	return;
}

template <class T>
void HEvent::remove(std::string ticker, std::string desc)
{
	// Delete if exists.
	boost::mutex::scoped_lock lock(mp_mutex_);
	if( mp_.count(ticker) && mp_[ticker].count(desc) )
	{
		T* oldp = static_cast<T*>(mp_[ticker][desc]);
		if( oldp != 0 )
			delete oldp;
		mp_[ticker].erase(mp_[ticker].find(desc));
		if( mp_[ticker].size() == 0 )
			mp_.erase(mp_.find(ticker));
	}

	return;
}

template<class T>
const T& HEvent::get(std::string ticker, std::string desc)
{
	const T& ret = *static_cast<const T*>(HEvent::Instance()->get(ticker, desc));
	return ret;
}

#endif