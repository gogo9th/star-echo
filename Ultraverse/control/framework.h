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


HINSTANCE hInstance();
const wchar_t * appNameW();


class Error : public std::runtime_error
{
public:
    Error(const char * msg)
        : runtime_error(msg)
    {}

    Error(const char * msg, int errorCode)
        : runtime_error(std::string(msg) + " (" + std::to_string(errorCode) + ")")
    {}

    static Error gle(const char * msg)
    {
        return Error(msg, GetLastError());
    }
};
