#pragma once


inline int smulw(int a, int b)
{
    // 32x16 multiplication overflows here and gives incorrect results for SMUL* operations
    return (int64_t(a) * b) >> 16;
}


