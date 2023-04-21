#include "pch.h"

#include "stringUtils.h"
#include "registry.h"
#include "settings.h"

namespace Settings
{

static const auto autorunKey = L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"sv;
static const auto autorunApprovedKey = L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run"sv;
static const auto settingsValueName = L"Settings"sv;


std::wstring currentEffect()
{
    std::wstring value;
    if (Registry::keyExists(appPath))
    {
        auto k = Registry::openKey(appPath);
        if (Registry::valueExists(k, settingsValueName))
        {
            Registry::getValue(k, settingsValueName, value);
        }
    }
    return value;
}

void setEffect(const std::wstring & value)
{
    Registry::setValue(appPath, settingsValueName, value);
}

bool Settings::isAnyEffectEnabled(const std::wstring & str)
{
    return !str.empty() && str != L"disable";
}

std::wstring Settings::effectDisabledString()
{
    return L"disable";
}

bool isAutostart(std::wstring_view valueName, const std::wstring & value)
{
    auto hKey = Registry::openKey(autorunKey);
    if (Registry::valueExists(hKey, valueName))
    {
        std::wstring rv;
        Registry::getValue(hKey, valueName, rv);
        return toUpperCase(std::as_const(rv)) == toUpperCase(value);
    }
    return false;
}

void setAutostart(std::wstring_view valueName, const std::wstring & value)
{
    Registry::setValue(autorunKey, valueName, value);
    if (Registry::keyExists(autorunApprovedKey))
    {
        auto hkey = Registry::openKey(autorunApprovedKey, KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY);
        if (Registry::valueExists(hkey, valueName))
        {
            std::vector<uint8_t> arData;
            Registry::getValue(hkey, valueName, arData);
            if (!arData.empty() && arData[0] == 1)
            {
                arData[0] = 2;
                Registry::setValue(hkey, valueName, arData);
            }
        }
    }
}

void removeAutostart(std::wstring_view valueName)
{
    Registry::deleteValue(autorunKey, valueName);
}

void setupAppKey()
{
    if (!Registry::keyExists(appPath))
    {
        Registry::createKey(appPath);
    }
    Registry::setKeyWritable(appPath);

    auto hKey = Registry::openKey(appPath, KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY);
    if (!Registry::valueExists(hKey, settingsValueName))
    {
        Registry::setValue(hKey, settingsValueName, L"");
    }
}


}
