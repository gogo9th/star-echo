#pragma once

#include <cstdint>
#include <cmath>
#include <cassert>
#include <vector>
#include <cstdlib>


template<typename sampleType, typename wideSampleType>
class Filter
{
public:
    template<typename Type, typename E = void>
    struct intfloat_if;

    template<typename Type>
    struct intfloat_if<Type, std::enable_if_t<std::is_integral_v<Type>>>
    {
        using intfloat = int;
        using int16float = int16_t;
    };
    template<typename Type>
    struct intfloat_if<Type, std::enable_if_t<std::is_floating_point_v<Type>>>
    {
        using intfloat = float;
        using int16float = float;
    };

    using intfloat_t = typename intfloat_if<sampleType>::intfloat;
    using int16float_t = typename intfloat_if<sampleType>::int16float;

    using sample_t = sampleType;
    using samplew_t = wideSampleType;

    //static constexpr sample_t MaxValue = std::is_floating_point_v<sample_t> ? 1.f : std::numeric_limits<sample_t>::max();
    //static constexpr sample_t MinValue = std::is_floating_point_v<sample_t> ? 5.f : std::numeric_limits<sample_t>::min();

    static inline samplew_t smulw(samplew_t a, intfloat_t b)
    {
        if constexpr (std::is_integral_v<sample_t>)
        {
            // 32x16 multiplication may overflow here and gives incorrect results for SMUL* operations
            return (int64_t(a) * b) >> 16;
        }
        else if constexpr (std::is_floating_point_v<sample_t>)
        {
            return a * b;
        }
        else
        {
            // keep extra assertion here
            static_assert(!std::is_integral_v<sample_t> && !std::is_floating_point_v<sample_t>, "Incompatible sample type");
        }
    }


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
        std::vector<sample_t> vl, vr;
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

    template<typename T>
    static inline T limit(T v)
    {
        if constexpr (std::is_floating_point_v<sample_t>)
        {
            return std::max<T>(-1., std::min<T>(1., v));
        }
        else
        {
            return std::max<T>(std::numeric_limits<sample_t>::min(), std::min<T>(std::numeric_limits<sample_t>::max(), v));
        }
    }

    // normalize not to rip the sound
    void normalize(samplew_t & l, samplew_t & r)
    {
        if (normalizer != 1.0f)
        {
            if constexpr (std::is_floating_point_v<sample_t>)
            {
                l /= normalizer;
                r /= normalizer;
            }
            else
            {
                // TODO is any performance impact made by round?
                // note lround zeroes output for -+Max value which in theory may happen when overflowing samplew type
                if constexpr (sizeof(samplew_t) == 8)
                {
                    l = std::llround((float)l / normalizer);
                    r = std::llround((float)r / normalizer);
                }
                else
                {
                    l = std::lround((float)l / normalizer);
                    r = std::lround((float)r / normalizer);
                }
            }
        }

        if (l > global_max)
            global_max = l;
        if (l < global_min)
            global_min = l;
        if (r > global_max)
            global_max = r;
        if (r < global_min)
            global_min = r;

        l = limit<samplew_t>(l);
        r = limit<samplew_t>(r);
    }

    float normFactor() const
    {
        return normalizer;
    }

    void setNormFactor(float n)
    {
        normalizer = n;
    }

    float calcNormFactor() const
    {
        float factor_max = 1.0f, factor_min = 1.0f;

        if constexpr (std::is_floating_point_v<sample_t>)
        {
            if (global_max > 1.)
            {
                // division is quite useless 
                factor_max = global_max / 1.;
            }
            if (global_min < -1.)
            {
                factor_min = float(global_min) / -1.;
            }
        }
        else
        {
            if (global_max > std::numeric_limits<sample_t>::max())
            {
                factor_max = float(global_max) / std::numeric_limits<sample_t>::max();
            }
            if (global_min < std::numeric_limits<sample_t>::min())
            {
                factor_min = float(global_min) / std::numeric_limits<sample_t>::min();
            }
        }
        return std::max(factor_max, factor_min);
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
#if defined(_DEBUG)
    int sCount = 0;
#endif
    samplew_t   global_max = 0;
    samplew_t   global_min = 0;
    float       normalizer = 1.0;

    std::vector<int> sampleRates_;
};
