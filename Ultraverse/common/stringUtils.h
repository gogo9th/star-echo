#pragma once

#include <vector>


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


template<typename char_type>
std::vector<std::basic_string<char_type>> stringSplit(const std::basic_string<char_type> & str, std::basic_string_view<char_type> delimiter)
{
    std::vector<std::basic_string<char_type>> r;

    size_t endpos = 0;
    size_t pos = 0;
    do
    {
        endpos = str.find(delimiter, pos);
        auto tk = str.substr(pos, endpos - pos);
        if (!tk.empty())
        {
            r.push_back(tk);
        }
        pos = endpos + delimiter.size();
    } while (endpos != std::string::npos);
    return r;
}

template<typename char_type, size_t size>
inline std::vector<std::basic_string<char_type>> stringSplit(const std::basic_string<char_type> & str, const char_type(&delimiter)[size])
{
    return stringSplit(str, std::basic_string_view<char_type>(delimiter, size - 1));
}

void toUpperCase(std::wstring & s);
