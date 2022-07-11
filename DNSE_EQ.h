#pragma once

#include <array>

#include "filter.h"


class DNSE_EQ : public Filter
{
public:

    DNSE_EQ(const std::array<int16_t, 7> & gains);
    ~DNSE_EQ();

    void setSamplerate(int sampleRate) override;

    virtual void filter(sample_t l, sample_t r,
                        sample_t * l_out, sample_t * r_out) override;

private:
    class BiQuadFilter
    {
    public:
        BiQuadFilter() = default;
        BiQuadFilter(const int16_t (&filterCoeff)[3], int16_t gain);

        samplew_t filter(sample_t in);

    private:
        int16_t     fc_[3] = {};
        samplew_t   delay_[2] = {};
    };


    std::array<short, 7>    gains_;

    BiQuadFilter    bq0l_, bq0r_;
    BiQuadFilter    bq1l_, bq1r_;
    BiQuadFilter    bq2l_, bq2r_;
    BiQuadFilter    bq3l_, bq3r_;
    BiQuadFilter    bq4l_, bq4r_;
    BiQuadFilter    bq5l_, bq5r_;
    BiQuadFilter    bq6l_, bq6r_;
};
