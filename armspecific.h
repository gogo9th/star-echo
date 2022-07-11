#pragma once

#include "filter.h"

inline Filter::samplew_t smulw(Filter::samplew_t a, int b)
{
    // 32x16 multiplication may overflow here and gives incorrect results for SMUL* operations
    return (int64_t(a) * b) >> 16;
}


