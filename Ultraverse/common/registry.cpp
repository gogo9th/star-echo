#include "pch.h"

#include <aclapi.h>

#include "registry.h"
#include "error.h"
#include "stringUtils.h"

namespace Registry
{

static std::tuple<HKEY, std::wstring_view> splitKey(std::wstring_view key)
{
    size_t pos = key.find(L'\\');
    if (pos == std::wstring::npos)
        throw WError(L"Key " + key + L" has invalid format");

    auto pathPart = key.substr(pos + 1);
    auto rootPart = std::wstring(key.substr(0, pos));
    toUpperCase(rootPart);

    HKEY rootKey;
    if (rootPart == L"HKEY_CLASSES_ROOT")
        rootKey = HKEY_CLASSES_ROOT;
    else if (rootPart == L"HKEY_CURRENT_CONFIG")
        rootKey = HKEY_CURRENT_CONFIG;
    else if (rootPart == L"HKEY_CURRENT_USER")
        rootKey = HKEY_CURRENT_USER;
    else if (rootPart == L"HKEY_LOCAL_MACHINE")
        rootKey = HKEY_LOCAL_MACHINE;
    else if (rootPart == L"HKEY_USERS")
        rootKey = HKEY_USERS;
    else
        throw WError(L"Unknown root key " + rootPart);

    return { rootKey, pathPart };
}

HKey openKey(std::wstring_view key, REGSAM samDesired)
{
    auto [rootKey, subKey] = splitKey(key);

    HKey hkey;
    auto status = RegOpenKeyExW(rootKey, subKey.data(), 0, samDesired, &hkey);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to open registry key " + key + L": " + status);

    return hkey;
}

HKey openKey(const HKey & hkey, std::wstring_view subKey, REGSAM samDesired)
{
    HKey hSubKey;
    auto status = RegOpenKeyExW(hkey, subKey.data(), 0, samDesired, &hSubKey);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to open registry subkey " + subKey + L": " + status);

    return hSubKey;
}

void createKey(std::wstring_view key)
{
    auto [rootKey, subKey] = splitKey(key);

    HKey hkey;
    auto status = RegCreateKeyExW(rootKey, subKey.data(), 0, NULL, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &hkey, NULL);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to create registry key " + key + L": " + status);
}

std::vector<std::wstring> Registry::subKeys(std::wstring_view key)
{
    std::vector<std::wstring> r;

    auto hkey = openKey(key, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY);

    int i = 0;
    LSTATUS status;
    wchar_t subkey[256];
    DWORD keyLength = (DWORD)std::size(subkey);
    while ((status = RegEnumKeyExW(hkey, i++, subkey, &keyLength, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
    {
        r.push_back(subkey);
        keyLength = (DWORD)std::size(subkey);
    }

    if (status != ERROR_NO_MORE_ITEMS)
        throw WError(L"Failed to enumerate subkeys of registry key " + key + L": " + status);

    return r;
}

void setKeyWritable(std::wstring_view key)
{
    auto hkey = openKey(key, READ_CONTROL | WRITE_DAC | KEY_WOW64_64KEY);

    DWORD descriptorSize = 0;
    RegGetKeySecurity(hkey, DACL_SECURITY_INFORMATION, NULL, &descriptorSize);

    TypedBuffer<PSECURITY_DESCRIPTOR> oldSd(descriptorSize);
    LSTATUS status = RegGetKeySecurity(hkey, DACL_SECURITY_INFORMATION, oldSd, &descriptorSize);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to get security information for registry key " + key + L": " + status);

    BOOL aclPresent, aclDefaulted;
    PACL oldAcl = NULL;
    if (!GetSecurityDescriptorDacl(oldSd, &aclPresent, &oldAcl, &aclDefaulted))
        throw WError(L"Failed GetSecurityDescriptorDacl while ensuring writability");

    PSid usersSid;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS,
                                  0, 0, 0, 0, 0, 0, &usersSid))
        throw WError(L"Error in AllocateAndInitializeSid while ensuring writability");

    EXPLICIT_ACCESS ea;
    ea.grfAccessPermissions = KEY_ALL_ACCESS;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = (LPWSTR)(PSID)usersSid;

    scoped_ptr<ACL> acl([&ea, &oldAcl] ()
                        {
                            ACL *acl;
                            if (ERROR_SUCCESS != SetEntriesInAcl(1, &ea, oldAcl, &acl))
                                throw WError(L"Failed SetEntriesInAcl while ensuring writability");
                            return acl;
                        }(), [] (auto * p) { LocalFree(p); });

    SECURITY_DESCRIPTOR sd;
    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        throw WError(L"Failed InitializeSecurityDescriptor while ensuring writability");

    if (!SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE))
        throw WError(L"Failed SetSecurityDescriptorDacl while ensuring writability");

    status = RegSetKeySecurity(hkey, DACL_SECURITY_INFORMATION, &sd);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to set security information for registry key " + key + L": " + status);
}



bool keyExists(std::wstring_view key)
{
    auto [rootKey, subKey] = splitKey(key);

    HKey keyHandle;
    auto result = RegOpenKeyExW(rootKey, subKey.data(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle);

    return result == ERROR_SUCCESS;
}

bool keyExists(const HKey & hkey, std::wstring_view subKey)
{
    HKey keyHandle;
    auto result = RegOpenKeyExW(hkey, subKey.data(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle);
    return result == ERROR_SUCCESS;
}

bool valueExists(std::wstring_view key, std::wstring_view value)
{
    auto hkey = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    return valueExists(hkey, value);
}

bool valueExists(const HKey & hkey, std::wstring_view value)
{
    auto result = RegQueryValueExW(hkey, value.data(), NULL, NULL, NULL, NULL);
    return result == ERROR_SUCCESS;
}

void setValue(std::wstring_view key, std::wstring_view name, std::wstring_view value)
{
    auto hKey = openKey(key, KEY_SET_VALUE | KEY_WOW64_64KEY);
    setValue(hKey, name, value);
}

void setValue(std::wstring_view key, std::wstring_view name, unsigned long value)
{
    auto hKey = openKey(key, KEY_SET_VALUE | KEY_WOW64_64KEY);
    setValue(hKey, name, value);
}

void setValue(const HKey & key, std::wstring_view name, std::wstring_view value)
{
    auto status = RegSetValueExW(key, name.data(), 0, REG_SZ, (const BYTE *)value.data(), (DWORD)((value.size() + 1) * sizeof(wchar_t)));
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to write to registry value " + name + L": " + status);
}

void setValue(const HKey & key, std::wstring_view name, unsigned long value)
{
    auto status = RegSetValueExW(key, name.data(), 0, REG_DWORD, (const BYTE *)&value, sizeof(unsigned long));
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to write to registry value " + name + L": " + status);
}


void getValue(std::wstring_view key, std::wstring_view name, std::wstring & value)
{
    auto hKey = openKey(key);
    return getValue(hKey, name, value);
}

void getValue(std::wstring_view key, std::wstring_view name, unsigned long & value)
{
    auto hKey = openKey(key);
    return getValue(hKey, name, value);
}

void getValue(const HKey & key, std::wstring_view name, std::wstring & value)
{
    DWORD type;
    DWORD bufSize;
    auto status = RegQueryValueExW(key, name.data(), NULL, &type, NULL, &bufSize);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to read registry value " + name + L": " + status);

    if (type != REG_SZ)
        throw WError(L"Registry value " + name + L" has unexpected type " + type);

    if (bufSize < sizeof(wchar_t))
    {
        value.clear();
        return;
    }

    value.resize(bufSize / sizeof(wchar_t) + 1);
    status = RegQueryValueExW(key, name.data(), NULL, NULL, (LPBYTE)value.data(), &bufSize);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to read registry value " + name + L": " + status);

    while (!value.empty() && value.back() == L'\0')
        value.pop_back();
}

void getValue(const HKey & hkey, std::wstring_view name, unsigned long & value)
{
    DWORD type;
    DWORD bufSize;
    auto status = RegQueryValueExW(hkey, name.data(), NULL, &type, NULL, &bufSize);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to read registry value " + name + L": " + status);

    if (type != REG_DWORD)
        throw WError(L"Registry value " + name + L" has unexpected type " + type);
    if (bufSize < sizeof(unsigned long))
        throw WError(L"Registry value " + name + L" has unexpected size " + bufSize);

    std::unique_ptr<byte[]> buf(new byte[bufSize]);
    status = RegQueryValueExW(hkey, name.data(), NULL, NULL, buf.get(), &bufSize);
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to read registry value " + name + L": " + status);

    value = *(unsigned long *)buf.get();
}

void deleteValue(std::wstring_view key, std::wstring_view value)
{
    auto hkey = openKey(key, KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    deleteValue(hkey, value);
}

void deleteValue(const HKey & hkey, std::wstring_view value)
{
    auto status = RegDeleteValueW(hkey, value.data());
    if (status != ERROR_SUCCESS)
        throw WError(L"Failed to delete registry value " + value + L": " + status);
}


}
