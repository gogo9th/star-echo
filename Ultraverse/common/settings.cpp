#include "pch.h"

#include "stringUtils.h"
#include "registry.h"
#include "settings.h"

namespace Settings
{

static const wchar_t settingsName[] = L"Settings";

std::wstring current()
{
    std::wstring value;
    Registry::getValue(appPath, settingsName, value);
    return value;
}

void set(const std::wstring & value)
{
    Registry::setValue(appPath, settingsName, value);
}

void setupAppKey()
{
    if (!Registry::keyExists(appPath))
    {
        Registry::createKey(appPath);
        Registry::setKeyWritable(appPath);
    }

    auto hKey = Registry::openKey(appPath, KEY_SET_VALUE | KEY_WOW64_64KEY);
    if (!Registry::valueExists(hKey, settingsName))
    {
        Registry::setValue(hKey, settingsName, L"");
    }
}

bool Settings::isEnabled(const std::wstring & str)
{
    return !str.empty() && str != L"disable";
}

std::wstring Settings::disabled()
{
    return L"disable";
}

}
