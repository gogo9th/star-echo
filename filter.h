#pragma once

#include <cstdint>
#include <cassert>
#include <vector>

class Filter
{

public:
    int32_t global_max, global_min;
    float normalizer;

    virtual ~Filter() = default;

    explicit Filter(std::vector<int> && sampleRates)
        : sampleRates_(sampleRates)
    {
        global_max = 0x7FFF;
        global_min = -0x8000;
        normalizer = 1.0f;
    }

    virtual void setSamplerate(int sampleRate) = 0;

    virtual void filter(int16_t l, const int16_t r,
                        int16_t * l_out, int16_t * r_out) = 0;

    //

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
        for (int i = 0; i < nSamples; i++)
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
    {
        global_max = 0x7FFF;
        global_min = -0x8000;
    }

    // normalize not to rip the sound
    void normalize(int32_t & l, int32_t & r)
    {
        if (normalizer != 1.0f)
        {
            l = (int16_t)((float)l / normalizer);
            r = (int16_t)((float)r / normalizer);
        }

        if (l > global_max)
            global_max = l;
        if (l < global_min)
            global_min = l;
        if (r > global_max)
            global_max = r;
        if (r < global_min)
            global_min = r;
    }

    int agreeSamplerate(int sr)
    {
        if (sampleRates_.empty())
            return sr;

        int nearestHigh = std::numeric_limits<int>::max();
        int maxAvailable = 0;
        for (auto rate : sampleRates_)
        {
            if (sr == rate)
            {
                return rate;
            }

            if (rate > sr && rate < nearestHigh)
            {
                nearestHigh = rate;
            }
            maxAvailable = std::max(maxAvailable, rate);
        }
        return nearestHigh != std::numeric_limits<int>::max() ? nearestHigh : maxAvailable;
    }
    
protected:
    std::vector<int> sampleRates_;
};
