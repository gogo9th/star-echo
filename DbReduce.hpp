#pragma once

#include "filter.h"


template<typename sampleType, typename wideSampleType>
class DbReduce : public Filter<sampleType, wideSampleType>
{
    using Filter<sampleType, wideSampleType>::smulw;
public:
    using sample_t = typename Filter<sampleType, wideSampleType>::sample_t;
    using samplew_t = typename Filter<sampleType, wideSampleType>::samplew_t;
    using intfloat_t = typename Filter<sampleType, wideSampleType>::intfloat_t;


    DbReduce(int db)
        : Filter<sampleType, wideSampleType>({})
    {
        static const int dbReduceCoeff[] = { 0x4000, 0x390A, 0x32D6, 0x2D4F, 0x2862, 0x23FD, 0x2013, 0x1C96, 0x197B, 0x16B5,
                                             0x143D, 0x120A, 0x1013, 0xE54, 0xCC5, 0xB62, 0xA25, 0x90A, 0x80F, 0x72E };

        db = std::max(0, std::min((int)std::size(dbReduceCoeff) - 1, db));
        if constexpr (std::is_integral_v<sampleType>)
        {
            gain_ = 4 * dbReduceCoeff[db];
        }
        else if constexpr (std::is_floating_point_v<sampleType>)
        {
            gain_ = dbReduceCoeff[db] / float(0x4000);
        }
    }

    void setSamplerate(int sampleRate) override
    {}

    void filter(sample_t l, const sample_t r,
                sample_t * l_out, sample_t * r_out) override
    {
        *l_out = smulw(l, gain_);
        *r_out = smulw(r, gain_);
    }

private:
    intfloat_t gain_;
};
