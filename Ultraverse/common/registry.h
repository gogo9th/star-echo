#pragma once

#include <vector>

#include "scopedResource.h"

namespace Registry
{

HKey openKey(std::wstring_view key, REGSAM samDesired = KEY_QUERY_VALUE | KEY_WOW64_64KEY);
HKey openKey(const HKey & hkey, std::wstring_view subKey, REGSAM samDesired = KEY_QUERY_VALUE | KEY_WOW64_64KEY);
void createKey(std::wstring_view key);
std::vector<std::wstring> subKeys(std::wstring_view key);

void setKeyWritable(std::wstring_view key);

bool keyExists(std::wstring_view key);
bool keyExists(const HKey & hkey, std::wstring_view subKey);
bool valueExists(std::wstring_view key, std::wstring_view valuename);
bool valueExists(const HKey & hkey, std::wstring_view valuename);

void setValue(std::wstring_view key, std::wstring_view name, std::wstring_view value);
void setValue(std::wstring_view key, std::wstring_view name, unsigned long value);
void setValue(const HKey & key, std::wstring_view name, std::wstring_view value);
void setValue(const HKey & key, std::wstring_view name, unsigned long value);


void getValue(std::wstring_view key, std::wstring_view name, std::wstring & value);
void getValue(std::wstring_view key, std::wstring_view name, unsigned long & value);
void getValue(const HKey & key, std::wstring_view name, std::wstring & value);
void getValue(const HKey & key, std::wstring_view name, unsigned long & value);

}