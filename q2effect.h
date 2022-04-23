#pragma once

#if defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>
#endif

#include <iostream>
#include <cassert>
