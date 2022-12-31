#pragma once

#include <boost/circular_buffer.hpp>
#include <array>

#include "filter.h"


template<typename sampleType, typename wideSampleType>
class DNSE_EQ : public Filter<sampleType, wideSampleType>
{
    static constexpr int16_t eqGains[25] = { -0x17F6, -0x16FB, -0x15E1, -0x14A5, -0x1343, -0x11B5, -0xFF6, -0xE01, -0xBCF, -0x959, -0x695, -0x37B,
                                        0,
                                        0x3E8, 0x849, 0xD34, 0x12B7, 0x18E8, 0x1FD9, 0x27A4, 0x3061, 0x3A30, 0x4531, 0x518A, 0x5F65 };

    using Filter<sampleType, wideSampleType>::smulw;
    using Filter<sampleType, wideSampleType>::normalize;
public:
    using sample_t = typename Filter<sampleType, wideSampleType>::sample_t;
    using samplew_t = typename Filter<sampleType, wideSampleType>::samplew_t;
    using int16float_t = typename Filter<sampleType, wideSampleType>::int16float_t;


    DNSE_EQ(const std::array<int16_t, 7> & gains)
        : Filter<sampleType, wideSampleType>({ 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 })
    {
        int iGain = 0;
        for (auto g : gains)
        {
            auto ng = std::max<int16_t>(0, std::min<int16_t>(int16_t(std::size(eqGains)) - 1, g));
            gains_[iGain++] = eqGains[ng];
        }
    }

    void setSamplerate(int sampleRate) override
    {
        int srIndex;
        if (sampleRate == 8000 || sampleRate == 11025 || sampleRate == 12000)
        {
            srIndex = 0;
            gains_[6] = eqGains[12];
        }
        else if (sampleRate == 16000 || sampleRate == 22050 || sampleRate == 24000)
        {
            srIndex = 1;
            gains_[6] = eqGains[12];
        }
        else if (sampleRate == 32000)
        {
            srIndex = 2;
        }
        else if (sampleRate == 48000)
        {
            srIndex = 4;
        }
        else
        {
            srIndex = 3;
        }

        static const int16_t eqFilter0[5][3] = { {0x8B , -0x3EEA, 0x7ED7}, {0x46, -0x3F75, 0x7F70}, {0x30, -0x3FA0, 0x7F9E}, {0x20, -0x3FC0, 0x7FBF}, {0x20, -0x3FC1, 0x7FC0} };
        static const int16_t eqFilter1[5][3] = { {0x157, -0x3D52, 0x7CDD}, {0xAD, -0x3EA6, 0x7E88}, {0x78, -0x3F10, 0x7F02}, {0x41, -0x3F7F, 0x7F78}, {0x41, -0x3F80, 0x7F78} };
        static const int16_t eqFilter2[5][3] = { {0x373, -0x3919, 0x75F7}, {0x1C6, -0x3C74, 0x7BA5}, {0x13C, -0x3D89, 0x7D26}, {0xE0, -0x3E40, 0x7E08}, {0xE0, -0x3E40, 0x7E10} };
        static const int16_t eqFilter3[5][3] = { {0x8AC, -0x2EA8, 0x599E}, {0x4A7, -0x36B3, 0x70EA}, {0x347, -0x3971, 0x769F}, {0x240, -0x3B80, 0x7A00}, {0x1E0, -0x3C40, 0x7B00} };
        static const int16_t eqFilter4[5][3] = { {0x1314, -0x19D7, -0x24F3}, {0xC6E, -0x2724, 0x37F5}, {0x962, -0x2D3C, 0x5470}, {0x800, -0x3000, 0x6200}, {0x800, -0x3000, 0x6400} };
        static const int16_t eqFilter5[5][3] = { {0x216C, 0x2D8, -0x955}, {0x10C7, -0x1E72, -0x3D7A}, {0xDD1, -0x245E, 0}, {0xC00, -0x2800, 0x2800}, {0x800, -0x3000, 0x3800} };
        static const int16_t eqFilter6[5][3] = { {0x3D15, 0x3A2B, -0xBA}, {0x3B4B, 0x3697, -0x63A}, {0x13F0, -0x181F, -0x516A}, {0x1400, -0x1800, -0x2400}, {0x1200, -0x1C00, -0x1800} };

        bq0l_ = bq0r_ = BiQuadFilter(to_array(eqFilter0[srIndex]), gains_[0]);
        bq1l_ = bq1r_ = BiQuadFilter(to_array(eqFilter1[srIndex]), gains_[1]);
        bq2l_ = bq2r_ = BiQuadFilter(to_array(eqFilter2[srIndex]), gains_[2]);
        bq3l_ = bq3r_ = BiQuadFilter(to_array(eqFilter3[srIndex]), gains_[3]);
        bq4l_ = bq4r_ = BiQuadFilter(to_array(eqFilter4[srIndex]), gains_[4]);
        bq5l_ = bq5r_ = BiQuadFilter(to_array(eqFilter5[srIndex]), gains_[5]);
        bq6l_ = bq6r_ = BiQuadFilter(to_array(eqFilter6[srIndex]), gains_[6]);
    }

    virtual void filter(sample_t l, sample_t r,
                        sample_t * l_out, sample_t * r_out) override
    {
        samplew_t lb = bq6l_.filter(l) + bq5l_.filter(l) + bq4l_.filter(l) + bq3l_.filter(l) + bq2l_.filter(l) + bq1l_.filter(l) + bq0l_.filter(l) + l;
        samplew_t rb = bq6r_.filter(r) + bq5r_.filter(r) + bq4r_.filter(r) + bq3r_.filter(r) + bq2r_.filter(r) + bq1r_.filter(r) + bq0r_.filter(r) + r;

        normalize(lb, rb);

        *l_out = sample_t(lb);
        *r_out = sample_t(rb);
    }

private:
    class BiQuadFilter
    {
    public:
        BiQuadFilter() = default;
        BiQuadFilter(const std::array<int16_t, 3> & filterCoeff, int16float_t gain)
        {
            if constexpr (std::is_integral_v<sample_t>)
            {
                fc_ = filterCoeff;
                fc_[0] = (samplew_t(fc_[0]) * gain) >> 13;
            }
            else if constexpr (std::is_floating_point_v<sample_t>)
            {
                fc_ = filterCoeff / float(0x10000);
                fc_[0] = fc_[0] * gain / 0x2000;
            }
        }

        samplew_t filter(sample_t in)
        {
            auto v0 = delay_[0];
            auto v1 = delay_[1];
            delay_.push_back(4 * (smulw(v0, fc_[1]) + smulw(v1, fc_[2]) + in));
            return smulw(delay_[1] - v0, fc_[0]);
        }

    private:
        std::array<int16float_t, 3>         fc_ { 0 };
        boost::circular_buffer<samplew_t>   delay_ { 2, 0 };
    };

    BiQuadFilter    bq0l_, bq0r_;
    BiQuadFilter    bq1l_, bq1r_;
    BiQuadFilter    bq2l_, bq2r_;
    BiQuadFilter    bq3l_, bq3r_;
    BiQuadFilter    bq4l_, bq4r_;
    BiQuadFilter    bq5l_, bq5r_;
    BiQuadFilter    bq6l_, bq6r_;

    std::array<short, 7>    gains_;
};
