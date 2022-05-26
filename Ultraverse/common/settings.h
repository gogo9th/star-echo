#pragma once

namespace Settings
{
    const auto appPath = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Ultraverse Control"sv;

    std::wstring current();
    void set(const std::wstring & value);

    bool isEnabled(const std::wstring &);
    std::wstring disabled();

    void setupAppKey();

}
