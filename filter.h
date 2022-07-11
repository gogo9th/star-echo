#pragma once

#include <cstdint>
#include <cassert>
#include <vector>
#include <cstdlib>


class Filter
{
public:
    //using sample_t = int16_t;
    //using samplew_t = int32_t;
    using sample_t = int32_t;
    using samplew_t = int64_t;

    static constexpr sample_t Max = std::numeric_limits<sample_t>::max();
    static constexpr sample_t Min = std::numeric_limits<sample_t>::min();


    samplew_t   global_max = 0;
    samplew_t   global_min = 0;
    float       normalizer = 1.0;

    
    explicit Filter(std::vector<int> && sampleRates)
        : sampleRates_(sampleRates)
    {}
    virtual ~Filter() = default;

    virtual void setSamplerate(int sampleRate) = 0;

    virtual void filter(sample_t l, const sample_t r,
                        sample_t * l_out, sample_t * r_out) = 0;

    //

    void filter(const sample_t * lb, const sample_t * rb,
                sample_t * lb_out, sample_t * rb_out,
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

    // normalize not to rip the sound
    void normalize(samplew_t & l, samplew_t & r)
    {
        if (normalizer != 1.0f)
        {
            auto ln = ((float)l / normalizer);
            // correct sign swap when result jitters over 0x800*
            l = ln - ((ln < 0) ^ (l < 0));
            auto rn = (sample_t)((float)r / normalizer);
            r = rn - ((rn < 0) ^ (l < 0));
        }

        if (l > global_max)
            global_max = l;
        if (l < global_min)
            global_min = l;
        if (r > global_max)
            global_max = r;
        if (r < global_min)
            global_min = r;

        l = std::min<samplew_t>(Max, std::max<samplew_t>(Min, l));
        r = std::min<samplew_t>(Max, std::max<samplew_t>(Min, r));
    }

    int agreeSamplerate(int proposed)
    {
        if (sampleRates_.empty())
            return proposed;

        int nearestHigh = std::numeric_limits<int>::max();
        int maxAvailable = 0;
        for (auto rate : sampleRates_)
        {
            if (proposed == rate)
            {
                return rate;
            }

            if (rate > proposed && rate < nearestHigh)
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
