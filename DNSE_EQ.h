#pragma once

#include <array>

#include "filter.h"


class DNSE_EQ : public Filter
{
public:
    DNSE_EQ(const std::array<int16_t, 7> & gains, int sampleRate);
    ~DNSE_EQ();

    virtual void filter(int16_t l, const int16_t r,
                        int16_t * l_out, int16_t * r_out) override;

private:
    class BiQuadFilter
    {
    public:
        BiQuadFilter()
        {}
        BiQuadFilter(const int16_t(&filterCoeff)[3], int16_t gain);

        int32_t filter(int16_t in);

    private:
        int16_t fc_[3] = {};
        int     delay_[2] = {};
    };

    void EQbiquad_sh2int(int16_t * input, int size, const int16_t(&filterCoeff)[3]);

    std::array<short, 7>    gains_;
    short                   finalGain_;

    BiQuadFilter    bq0l_, bq0r_;
    BiQuadFilter    bq1l_, bq1r_;
    BiQuadFilter    bq2l_, bq2r_;
    BiQuadFilter    bq3l_, bq3r_;
    BiQuadFilter    bq4l_, bq4r_;
    BiQuadFilter    bq5l_, bq5r_;
    BiQuadFilter    bq6l_, bq6r_;
};
