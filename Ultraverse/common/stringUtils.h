#pragma once

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

inline std::wstring operator+(std::wstring_view l, std::wstring_view r)
{
    return std::wstring(l).operator+=(r);
}
inline std::wstring operator+(std::wstring_view l, wchar_t c)
{
    return std::wstring(l).operator+=(c);
}

inline std::wstring operator+(std::wstring l, long v)
{
    return std::wstring(l).operator+=(std::to_wstring(v));
}


void toUpperCase(std::wstring & s);
