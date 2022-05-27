#pragma once

#include <atlbase.h>
#include <atlcom.h>
#include <combaseapi.h>

#include <string>
#include <ostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef INITGUID
    #define INITGUID
    #include <guiddef.h>
    #undef INITGUID
#else
    #include <guiddef.h>
#endif
