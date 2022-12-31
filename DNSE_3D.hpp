#pragma once

#include <boost/circular_buffer.hpp>

#include <memory>

#include "filter.h"
#include "utils.h"


template<typename sampleType, typename wideSampleType>
class DNSE_3D : public Filter<sampleType, wideSampleType>
{
    using Filter<sampleType, wideSampleType>::smulw;
    using Filter<sampleType, wideSampleType>::normalize;
public:
    using sample_t = typename Filter<sampleType, wideSampleType>::sample_t;
    using samplew_t = typename Filter<sampleType, wideSampleType>::samplew_t;
    using intfloat_t = typename Filter<sampleType, wideSampleType>::intfloat_t;
    using int16float_t = typename Filter<sampleType, wideSampleType>::int16float_t;


    DNSE_3D(int st_eff, int st_rev, int st_hrdel)
        : Filter<sampleType, wideSampleType>({ 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 })
        , st_eff_(st_eff)
        , st_rev_(st_rev)
        , st_hrdel_(st_hrdel)
    {}

    void setSamplerate(int sampleRate) override
    {
        int mhrdelOff;
        switch (sampleRate)
        {
            case  8000:
                mhrdelOff = 0;
                break;
            case 11025:
                mhrdelOff = 1;
                break;
            case 12000:
                mhrdelOff = 2;
                break;
            case 16000:
                mhrdelOff = 3;
                break;
            case 22050:
                mhrdelOff = 4;
                break;
            case 24000:
                mhrdelOff = 5;
                break;
            case 32000:
                mhrdelOff = 6;
                break;
            case 48000:
                mhrdelOff = 8;
                break;
            case 44100:
            default:
                mhrdelOff = 7;
                break;
        }

        const int16_t hiir[9][4] = {
            { 0x2281, -0x7CD,  0x194F, -0x12A },
            { 0x23FF, -0xC41,  0x2438, -0x2C1 },
            { 0x2470, -0xDA0,  0x271F, -0x35F },
            { 0x2624, -0x12AF, 0x30CD, -0x60F },
            { 0x2824, -0x1872, 0x3B10, -0xA0B },
            { 0x28A8, -0x19DE, 0x3D95, -0xB2F },
            { 0x2A02, -0x1DEF, 0x454F, -0xF12 },
            { 0x2AAB, -0x2000, 0x4AE2, -0x11DA },
            { 0x2AAB, -0x2000, 0x4BAB, -0x121D },
        };
        static const int mhrdel[] = { 0x2E7, 0x400, 0x45B, 0x5CE, 0x800, 0x8B5, 0xB9C, 0x1000, 0x116A };
        static const int16_t head[9][4] = {
            0x568D, -0x2C75, 0x1958, 0x10C0, 0x58C9, -0x3737, 0x1582, 0xC11,
            0x5954, -0x39CF, 0x1495, 0xAEF, 0x5AFD, -0x41CD, 0x11BC, 0x774,
            0x5C87, -0x4935, 0xF18, 0x43A, 0x5CE2, -0x4AE9, 0xE7D, 0x37C,
            0x5DEF, -0x4FF7, 0xCB0, 0x148, 0x5EDB, -0x5468, 0xB1A, -0xA7,
            0x5F10, -0x5566, 0xAC0, -0x116,
        };

        // max value is 17
        auto hrdel = (mhrdel[mhrdelOff] * st_hrdel_ + 0x800) >> 12;

        iir_l_ = IIRbiquad3D(to_array(hiir[mhrdelOff]));
        iir_r_ = IIRbiquad3D(to_array(hiir[mhrdelOff]));
        fir_ = FIR4hrtf(hrdel, to_array(head[mhrdelOff]));

        std::array<int, 4> fincoef = { 0 };
        if (st_eff_ < 9 && st_rev_ < 9)
        {
            // The value of n >> s is n right-shifted s bit positions with sign-extension.
            // The resulting value is floor(n / 2s).
            // by replacing >> with division we've got one bit difference in some of the coeffs comparing to native arm values

            auto eff = (0x332000 * st_eff_) >> 12;
            fincoef[2] = (-120 * eff) >> 10;
            fincoef[1] = (113 * eff) >> 7;

            auto effectstrength = 409 * st_eff_;
            auto fc3 = -((((effectstrength << 13) >> 12) * 409 * st_rev_) >> 12);
            fincoef[0] = 4 * (-effectstrength - fc3 + 4096);
            fincoef[3] = 4 * fc3;
        }
        else
        {
            //effectstrength = 0;
            //copy(fincoef, { 0, 0, 0, 0 });
        }

        reverb_ = Reverb(fincoef);
    }

    void filter(sample_t l, const sample_t r,
                sample_t * l_out, sample_t * r_out) override
    {
        auto lbIIR = iir_l_.filter(l);
        auto rbIIR = iir_r_.filter(r);

        auto [lbFIR, rbFIR] = fir_.filter(lbIIR, rbIIR);
        auto [lbRev, rbRev] = reverb_.filter(l, r, lbFIR, rbFIR);

        normalize(lbRev, rbRev);

        *l_out = sample_t(lbRev);
        *r_out = sample_t(rbRev);
    }

private:
    class IIRbiquad3D
    {
    public:
        IIRbiquad3D() = default;
        IIRbiquad3D(const std::array<int16_t, 4> & hIIR)
        {
            if constexpr (std::is_integral_v<sample_t>)
                hIIR_ = hIIR;
            else if constexpr (std::is_floating_point_v<sample_t>)
                hIIR_ = hIIR / float(0x10000);
        }

        samplew_t filter(sample_t in)
        {
            auto nxt = smulw(32 * samplew_t(in), hIIR_[0]) + smulw(iirb_[1], hIIR_[3]) + smulw(iirb_[0], hIIR_[2]);
            auto out = nxt + smulw(iirb_[0], hIIR_[1]);
            iirb_.push_front(4 * nxt);
            return out;
        }

    private:
        std::array<int16float_t, 4>         hIIR_ { 0 };
        boost::circular_buffer<samplew_t>   iirb_ { 2, 0 };
    };

    class FIR4hrtf
    {
    public:
        FIR4hrtf(int hrdel = 0, const std::array<int16_t, 4> & head = { 0 })
        {
            if constexpr (std::is_integral_v<sample_t>)
                head_ = head;
            else if constexpr (std::is_floating_point_v<sample_t>)
                head_ = head / float(0x10000);

            assert(hrdel >= 0 && hrdel <= 17);
            delayL_.resize(hrdel + 1);
            delayR_.resize(hrdel + 1);
        }

        std::pair<samplew_t, samplew_t> filter(samplew_t l, samplew_t r)
        {
            auto r1 = smulw(l, head_[0]) + smulw(delayL_.back(), head_[1]) + smulw(delayR_[1], head_[2]) + smulw(delayR_[0], head_[3]);
            auto r2 = smulw(r, head_[0]) + smulw(delayR_.back(), head_[1]) + smulw(delayL_[1], head_[2]) + smulw(delayL_[0], head_[3]);
            delayL_.push_back(l);
            delayR_.push_back(r);

            return { r1, r2 };
        }

    private:
        boost::circular_buffer<samplew_t>   delayL_;
        boost::circular_buffer<samplew_t>   delayR_;
        std::array<int16float_t, 4>         head_;
    };

    class Reverb
    {
        // reverb_and_effect
    public:
        Reverb() = default;
        Reverb(const std::array<int, 4> & fincoef)
        {
            if constexpr (std::is_integral_v<sample_t>)
                fincoef_ = fincoef;
            else if constexpr (std::is_floating_point_v<sample_t>)
                fincoef_ = fincoef / float(0x10000);
        }

        std::pair<samplew_t, samplew_t> filter(samplew_t l, samplew_t r, samplew_t iir_l, samplew_t iir_r)
        {
            auto r1 = smulw(4 * l, fincoef_[0]) + smulw(4 * iir_l, fincoef_[1]) + smulw(4 * iir_r, fincoef_[2]) + smulw(4 * r, fincoef_[3]);
            auto r2 = smulw(4 * r, fincoef_[0]) + smulw(4 * iir_r, fincoef_[1]) + smulw(4 * iir_l, fincoef_[2]) + smulw(4 * l, fincoef_[3]);
            return { r1, r2 };
        }

    private:
        std::array<intfloat_t, 4> fincoef_ = { 0 };
    };

    //

    int st_eff_;
    int st_rev_;
    int st_hrdel_;

    IIRbiquad3D iir_l_;
    IIRbiquad3D iir_r_;
    FIR4hrtf    fir_;
    Reverb      reverb_;
};
