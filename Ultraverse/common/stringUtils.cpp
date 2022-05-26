#include "pch.h"

#include "stringUtils.h"

void toUpperCase(std::wstring & s)
{
    if (!s.empty())
    {
        _wcsupr_s(s.data(), s.size() + 1);
    }
}
