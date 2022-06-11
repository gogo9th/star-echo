#pragma once

#include <cstdint>

class Filter
{
public:
    virtual ~Filter() = default;

    void filter(const int16_t * lb, const int16_t * rb,
                int16_t * lb_out, int16_t * rb_out,
                int nSamples)
    {
    #if defined(_DEBUG)
        static int sCount = 0;
    #endif
        for (size_t i = 0; i < nSamples; i++)
        {
            filter(lb[i], rb[i], lb_out + i, rb_out + i);
        #if defined(_DEBUG)
            ++sCount;
        #endif
        }
    }

    virtual void filter(int16_t l, const int16_t r,
                        int16_t * l_out, int16_t * r_out) = 0;
};
