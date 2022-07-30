#pragma once

#include <iostream>
#include <mutex>

template<typename charT>
class LogI
{
public:
    LogI(std::basic_ostream<charT, std::char_traits<charT>> & os)
        : os_(os)
    {
        mtx_.lock();
    }

    ~LogI()
    {
        os_ << std::endl;
        mtx_.unlock();
    }

    template<typename T>
    LogI & operator <<(const T & v)
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
    std::basic_ostream<charT, std::char_traits<charT>> & os_;
    inline static std::mutex mtx_;
};

using Log = LogI<char>;
using WLog = LogI<wchar_t>;

inline Log err() {  return Log(std::cerr); }

inline Log msg() {  return Log(std::cout); }

inline WLog werr() { return WLog(std::wcerr); }

inline WLog wmsg() { return WLog(std::wcout); }
