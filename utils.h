#pragma once

#include <string>
#include <vector>


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

//

template<typename T, int size>
inline void copy(T(&dst)[size], const T(&src)[size])
{
    std::copy(src, src + std::size(src), dst);
}

template<typename T, int size>
inline void set(T(&dst)[size], T value)
{
    std::fill(dst, dst + std::size(dst), value);
}
