#pragma once

namespace Settings
{
    const auto appPath = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Ultraverse Control"sv;

    std::wstring currentEffect();
    void setEffect(const std::wstring & value);

    bool isAnyEffectEnabled(const std::wstring &);
    std::wstring effectDisabledString();

    bool isAutostart(std::wstring_view valueName, const std::wstring & value);
    void setAutostart(std::wstring_view valueName, const std::wstring & value);
    void removeAutostart(std::wstring_view valueName);

    void setupAppKey();
}
