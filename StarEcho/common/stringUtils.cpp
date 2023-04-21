#include "pch.h"

#include <combaseapi.h>

#include "stringUtils.h"

void toUpperCase(std::wstring & s)
{
    if (!s.empty())
    {
        _wcsupr_s(s.data(), s.size() + 1);
    }
}

std::wstring toUpperCase(const std::wstring & s)
{
    std::wstring r(s);
    toUpperCase(r);
    return r;
}


std::wstring toString(const GUID & guid)
{
    std::wstring str;
    str.resize(48);
    auto strSize = StringFromGUID2(guid, str.data(), (int)std::size(str));
    str.resize(strSize > 1 ? strSize - 1 : 0);
    return str;
}

std::wstring toString(const PROPERTYKEY & pkey)
{
    return toString(pkey.fmtid) + L',' + std::to_wstring(pkey.pid);
}
