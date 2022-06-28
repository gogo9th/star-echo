#pragma once

#include <cstdint>
#include <vector>

class Filter
{
   
public:
    int32_t global_max, global_min;
    float normalizer;
    virtual ~Filter() = default;

    Filter()
    {
        global_max = 0x7FFF;
        global_min = -0x8000;
        normalizer = 1.0f;
    }

    void filter(const int16_t * lb, const int16_t * rb,
                int16_t * lb_out, int16_t * rb_out,
                int nSamples)
    {
    #if defined(_DEBUG)
        static int sCount = 0;
        std::vector<int> vl, vr;
        vl.resize(nSamples);
        vr.resize(nSamples);
    #endif
        for (size_t i = 0; i < nSamples; i++)
        {
            filter(lb[i], rb[i], lb_out + i, rb_out + i);
        #if defined(_DEBUG)
            ++sCount;
            vl[i] = lb_out[i];
            vr[i] = rb_out[i];
        #endif
        }
    }

    void reset_global_max_min()
    {   global_max = 0x7FFF;
        global_min = -0x8000;
    }

    virtual void filter(int16_t l, const int16_t r,
                        int16_t * l_out, int16_t * r_out) = 0;
};
