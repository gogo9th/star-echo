#pragma once

#include <iomanip>
#include <utility>
#include <array>

#include "stdafx.h"


template<typename CharT, std::size_t Length>
struct str_array : public std::array<CharT, Length>
{};

namespace detail
{
    template< typename ResT, typename SrcT >
    constexpr ResT static_cast_ascii(SrcT x)
    {
        if (!(x >= 0 && x <= 127))
            throw std::out_of_range("Character value must be in basic ASCII range (0..127)");
        return static_cast<ResT>(x);
    }

    template< typename ResElemT, typename SrcElemT, std::size_t N, std::size_t... I >
    constexpr str_array<ResElemT, N> do_str_array_cast(const SrcElemT(&a)[N], std::index_sequence<I...>)
    {
        return { static_cast_ascii<ResElemT>(a[I])..., 0 };
    }
}

template<typename T, size_t N>
std::basic_ostream<T> & operator<<(std::basic_ostream<T> & os, const str_array<T, N> & a)
{
    return os << a.data();
}


template< typename ResElemT, typename SrcElemT, std::size_t N, typename Indices = std::make_index_sequence< N - 1 > >
constexpr str_array<ResElemT, N> LS(const SrcElemT(&a)[N])
{
    return detail::do_str_array_cast<ResElemT>(a, Indices{});
}

//

class LogInit
{
    // To force OutputDebugStringW to correctly output Unicode strings, debuggers are required to call WaitForDebugEventEx
    // to opt into the new behavior. 
public:
    LogInit()
    {
        std::call_once(init_, [] () { DEBUG_EVENT e; WaitForDebugEventEx(&e, 0); });
    }

private:
    std::once_flag init_;
};


class Log
{
public:
    Log(const wchar_t * pref)
    {
        os_ << pref;
    }

    ~Log()
    {
        OutputDebugStringW(os_.str().c_str());
    }

    template<typename T>
    Log & operator <<(const T & v)
    {
        os_ << v;
        return *this;
    }

private:
    std::wostringstream os_;
};

inline Log err()
{
    return Log((L"{" + std::to_wstring(GetCurrentThreadId()) + L"} [Q2APO] Error: ").c_str());
}

inline Log msg()
{
    return Log((L'{' + std::to_wstring(GetCurrentThreadId()) + L"} [Q2APO] ").c_str());
}

//

template<typename T>
std::basic_ostream<T> & operator<<(std::basic_ostream<T> & os, const GUID & guid)
{
    wchar_t szGUID[64] = { 0 };
    StringFromGUID2(guid, szGUID, 64);
    return os << szGUID;
}
