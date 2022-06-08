#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <CommCtrl.h>
#include <combaseapi.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cassert>
#include <string>
#include <string_view>
#include <exception>
#include <stdexcept>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

#include "common/error.h"

HINSTANCE hInstance();
const wchar_t * windowNameW();
const wchar_t * appNameW();
const char * appName();
