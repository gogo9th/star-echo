#pragma once

#include <string>
#include <vector>
#include <array>


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


template<typename char_type>
inline std::basic_string<char_type> stringJoin(const std::vector<std::basic_string<char_type>> & arr, std::basic_string_view<char_type> delimiter)
{
    std::basic_string<char_type> r;
    auto sCount = arr.size();
    for (auto s : arr)
    {
        r += s;
        if (--sCount > 0)
        {
            r += delimiter;
        }
    }
    return r;
}

template<typename char_type, size_t size>
inline std::basic_string<char_type> stringJoin(const std::vector<std::basic_string<char_type>> & arr, const char_type(&delimiter)[size])
{
    return stringJoin(arr, std::basic_string_view<char_type>(delimiter, size - 1));
}


inline std::wstring stringToWstring(const std::string & s)
{
    return std::wstring(s.begin(), s.end());
}
inline std::string wstringToString(const std::wstring & s)
{
    std::string r;
    for (auto & c : s)
    {
        r.push_back((char)c);
    }
    return r;
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

template<typename T, std::size_t N>
std::array<std::remove_cv_t<T>, N> to_array(T(&src)[N])
{
    std::array<std::remove_cv_t<T>, N> r;
    std::copy(src, src + std::size(src), r.begin());
    return r;
}

template<typename T, size_t size, typename R>
std::array<R, size> operator/(const std::array<T, size> & arr, R div)
{
    std::array<R, size> r;
    for (auto i {0}; i < size; ++i)
    {
        r[i] = arr[i] / div;
    }
    return r;
}

template<typename T, std::size_t N, class Fn>
void for_each(T (&arr)[N], Fn fn)
{
    for (auto & v : arr)
    {
        v = fn(v);
    }
}
