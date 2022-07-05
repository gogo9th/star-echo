#pragma once

#include <iostream>
#include <mutex>

class Log
{
public:
	Log(std::ostream & os)
		: os_(os)
	{
		mtx_.lock();
	}

	~Log()
	{
		os_ << std::endl;
		mtx_.unlock();
	}

	template<typename T>
	Log & operator <<(const T & v)
	{
		os_ << v;
		return *this;
	}

	//Log & operator<<(const std::wstring & s)
	//{
	//	os_ << wstringToString(v);
	//	return *this;
	//}

private:
	std::ostream & os_;
	inline static std::mutex mtx_;
};

inline Log err()
{
	return Log(std::cerr);
}

inline Log msg()
{
	return Log(std::cout);
}
