#pragma once

#include <memory>
#include <boost/circular_buffer.hpp>

#include "filter.h"
#include "DNSE_BE_params.h"


template<typename sampleType, typename wideSampleType>
class DNSE_BE: public Filter<sampleType, wideSampleType>
{
    using Filter<sampleType, wideSampleType>::smulw;
    using Filter<sampleType, wideSampleType>::normalize;
public:
    using sample_t = typename Filter<sampleType, wideSampleType>::sample_t;
    using samplew_t = typename Filter<sampleType, wideSampleType>::samplew_t;
    using int16float_t = typename Filter<sampleType, wideSampleType>::int16float_t;


    DNSE_BE(int level, int fc)
        : Filter<sampleType, wideSampleType>({ 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 })
        , fc_(fc)
    {
        if (level > 0 && level <= 15)
        {
            auto & B1_poly_coef = KBass_B1_poly_coef_all[15 - level];
            auto & B2_poly_coef = KBass_B2_poly_coef_all[15 - level];
            for (size_t i = 0; i < std::size(B1_poly_coef); i++)
            {
                // coef: <= param0 ( [mul][add][add][add], negative [][][][] ), <= param1 (...., ....), <= param2 (...., ....) 
                if constexpr (std::is_same_v<sample_t, float>)
                {
                    KBass_B1_poly_coef[i] = float(B1_poly_coef[i]) / 0x100000000;
                    KBass_B2_poly_coef[i] = float(B2_poly_coef[i]) / 0x100000000;
                }
                else
                {
                    KBass_B1_poly_coef[i] = B1_poly_coef[i];
                    KBass_B2_poly_coef[i] = B2_poly_coef[i];
                }
            }

            auto & B1_poly_param = KBass_B1_poly_param_all[15 - level];
            auto & B2_poly_param = KBass_B2_poly_param_all[15 - level];
            for (size_t i = 0; i < std::size(B1_poly_param); i++)
            {
                // [][][] params, [][][] coefs
                if (i < std::size(KBass_B1_poly_param) / 2)
                {
                    if constexpr (std::is_same_v<sample_t, int16_t>)
                    {
                        KBass_B1_poly_param[i] = B1_poly_param[i];
                        KBass_B2_poly_param[i] = B2_poly_param[i];
                    }
                    else if constexpr (std::is_same_v<sample_t, int32_t>)
                    {
                        KBass_B1_poly_param[i] = B1_poly_param[i] << 16;
                        KBass_B2_poly_param[i] = B2_poly_param[i] << 16;
                    }
                    else if constexpr (std::is_same_v<sample_t, float>)
                    {
                        KBass_B1_poly_param[i] = float(B1_poly_param[i]) / 0x8000;
                        KBass_B2_poly_param[i] = float(B2_poly_param[i]) / 0x8000;
                    }
                }
                else
                {
                    if constexpr (std::is_same_v<sample_t, float>)
                    {
                        KBass_B1_poly_param[i] = 1 << (1 + int(B1_poly_param[i]));
                        KBass_B2_poly_param[i] = 1 << (1 + int(B2_poly_param[i]));
                    }
                    else
                    {
                        KBass_B1_poly_param[i] = B1_poly_param[i];
                        KBass_B2_poly_param[i] = B2_poly_param[i];
                    }
                }
            }
        }
        else
        {
            assert(0);
            set(KBass_B1_poly_coef, samplew_t(0));
            set(KBass_B2_poly_coef, samplew_t(0));
            set(KBass_B1_poly_param, sample_t(0));
            set(KBass_B2_poly_param, sample_t(0));
        }
    }

    void setSamplerate(int sampleRate) override
    {
        int Kbass_Fs;
        switch (sampleRate)
        {
            case 8000:
                KBass_Downrate_ = 0;
                Kbass_Fs = 0;
                break;
            case 11025:
                KBass_Downrate_ = 0;
                Kbass_Fs = 1;
                break;
            case 12000:
                KBass_Downrate_ = 0;
                Kbass_Fs = 2;
                break;
            case 16000:
                KBass_Downrate_ = 1;
                Kbass_Fs = 3;
                break;
            case 22050:
                KBass_Downrate_ = 1;
                Kbass_Fs = 4;
                break;
            case 24000:
                KBass_Downrate_ = 1;
                Kbass_Fs = 5;
                break;
            case 32000:
                KBass_Downrate_ = 1;
                Kbass_Fs = 6;
                break;
            default:
            case 44100:
                KBass_Downrate_ = 1;
                Kbass_Fs = 7;
                break;
            case 48000:
                KBass_Downrate_ = 1;
                Kbass_Fs = 8;
                break;
        }

        KBass_Hpf_L     = Biquad16(to_array(KBass_Hpf_coef_allFs[fc_][Kbass_Fs]));
        KBass_Hpf_R     = Biquad16(to_array(KBass_Hpf_coef_allFs[fc_][Kbass_Fs]));
        KBass_AntiDown  = Biquad16(to_array(KBass_AntiDown_coef_allFs[Kbass_Fs]));
        KBass_AntiUp    = Biquad16(to_array(KBass_AntiUp_coef_allFs[Kbass_Fs]));
        KBass_AntiUpSub = Biquad16(to_array(KBass_AntiUp_coef_allFs[Kbass_Fs]));
        KBass_B1_Lpf1   = Biquad16(to_array(KBass_B1_Lpf1_coef_allFs[fc_][Kbass_Fs]));
        KBass_B1_Lpf2   = Biquad16(to_array(KBass_B1_Lpf2_coef_allFs[fc_][Kbass_Fs]));
        KBass_B1_Bpf1   = Biquad16(to_array(KBass_B1_Bpf1_coef_allFs[fc_][Kbass_Fs]));
        KBass_B1_Bpf2   = Biquad16(to_array(KBass_B1_Bpf2_coef_allFs[fc_][Kbass_Fs]));
        KBass_B2_Lpf1   = Biquad16(to_array(KBass_B2_Lpf1_coef_allFs[fc_][Kbass_Fs]));
        KBass_B2_Lpf2   = Biquad16(to_array(KBass_B2_Lpf2_coef_allFs[fc_][Kbass_Fs]));
        KBass_B2_Bpf1   = Biquad16(to_array(KBass_B2_Bpf1_coef_allFs[fc_][Kbass_Fs]));
        KBass_B2_Bpf2   = Biquad16(to_array(KBass_B2_Bpf2_coef_allFs[fc_][Kbass_Fs]));
    }

    void filter(sample_t l, const sample_t r,
                sample_t * l_out, sample_t * r_out) override
    {
        auto l1 = KBass_Hpf_L.filter(l);
        auto r1 = KBass_Hpf_R.filter(r);

        sample_t b1, b2;
        if (KBass_Downrate_)
        {
            // downRate
            //sample_t sum = (r >> 1) + (l >> 1);
            sample_t sum = (r / 2) + (l / 2);
            b1 = b2 = KBass_AntiDown.filter(sum);
        }
        else
        {
            // normRate
            b1 = b2 = (r / 2) + (l / 2);
        }

        sample_t out;
        if (!KBass_Downrate_ || (KBass_pt_++ & 3) == 0)
        {
            b1 = KBass_B1_Bpf2.filter(KBass_B1_Bpf1.filter(b1));
            b2 = KBass_B2_Bpf2.filter(KBass_B2_Bpf1.filter(b2));

            b1 = power_poly(b1, KBass_B1_poly_coef, KBass_B1_poly_param);
            b2 = power_poly(b2, KBass_B2_poly_coef, KBass_B2_poly_param);

            b1 = KBass_B1_Lpf2.filter(KBass_B1_Lpf1.filter(b1));
            b2 = KBass_B2_Lpf2.filter(KBass_B2_Lpf1.filter(b2));

            out = b1 + b2;
        }
        else
        {
            out = 0;
        }

        if (KBass_Downrate_)
        {
            out = 8 * KBass_AntiUpSub.filter(KBass_AntiUp.filter(out));
        }
        else
        {
            out *= 2;
        }

        //samplew_t lOut = l1 + (l1 >> 1) + out;
        //samplew_t rOut = r1 + (r1 >> 1) + out;
        samplew_t lOut = l1 + (l1 / 2) + out;
        samplew_t rOut = r1 + (r1 / 2) + out;

        normalize(lOut, rOut);

        *l_out = lOut;
        *r_out = rOut;
    }

private:
    static sample_t power_poly(sample_t in, const samplew_t(&poly_coeff)[24], const sample_t(&poly_param)[6])
    {
        if (in == 0) return 0;

        const auto * coeff = poly_coeff;
        auto pv = poly_param + 3;
        if (in < 0)
        {
            coeff = poly_coeff + 4;
            in = -in;
        }
        if (in > poly_param[0])
        {
            coeff += 8;
            ++pv;
        }
        if (in > poly_param[1])
        {
            coeff += 8;
            ++pv;
        }
        if (in > poly_param[2])
        {
            in = poly_param[2];
        }

        if constexpr (std::is_same_v<sample_t, int16_t>)
        {
            auto v10 = 2 * smulw(coeff[0], in) + coeff[1];
            auto v11 = 2 * smulw(v10, in) + coeff[2];
            auto r = (2 * smulw(v11, in) + coeff[3]) << *pv;
            return r >> 16;
        }
        else if constexpr (std::is_same_v<sample_t, int32_t>)
        {
            // as we multiply 32x32 we should shift back to sample_t width
            auto v10 = 2 * (smulw(coeff[0], in) >> 16) + coeff[1];
            auto v11 = 2 * (smulw(v10, in) >> 16) + coeff[2];
            auto r = (2 * (smulw(v11, in) >> 16) + coeff[3]) << *pv;
            return r;
        }
        else if constexpr (std::is_same_v<sample_t, float>)
        {
            // coeffs are scaled to same dimension here
            auto v10 = smulw(coeff[0], in) + coeff[1];
            auto v11 = smulw(v10, in) + coeff[2];
            auto r = (smulw(v11, in) + coeff[3]) * *pv;
            return r;
        }
    }

    class Biquad16
    {
    public:
        Biquad16() = default;
        Biquad16(const std::array<int16_t, 5> & hIIR)
        {
            if constexpr (std::is_integral_v<sample_t>)
                hIIR_ = hIIR;
            else if constexpr (std::is_floating_point_v<sample_t>)
                hIIR_ = hIIR / float(0x10000);
        }

        samplew_t filter(sample_t in)
        {
            auto nxt = smulw(iirb_[1], hIIR_[4]) + smulw(iirb_[0], hIIR_[3]) + in;
            auto out = smulw(4 * (smulw(iirb_[1], hIIR_[2]) + smulw(iirb_[0], hIIR_[1]) + nxt), hIIR_[0]);
            iirb_.push_front(8 * nxt);
            return out;
        }

    private:
        boost::circular_buffer<samplew_t>   iirb_ { 2, 0 };
        std::array<int16float_t, 5>         hIIR_ { 0 };
    };

    //

    Biquad16 KBass_Hpf_L;
    Biquad16 KBass_Hpf_R;
    Biquad16 KBass_AntiDown;
    Biquad16 KBass_AntiUp;
    Biquad16 KBass_AntiUpSub;
    Biquad16 KBass_B1_Lpf1;
    Biquad16 KBass_B1_Lpf2;
    Biquad16 KBass_B1_Bpf1;
    Biquad16 KBass_B1_Bpf2;
    Biquad16 KBass_B2_Lpf1;
    Biquad16 KBass_B2_Lpf2;
    Biquad16 KBass_B2_Bpf1;
    Biquad16 KBass_B2_Bpf2;

    int fc_;

    int KBass_Downrate_ = 0;
    int KBass_pt_ = 0;
    samplew_t KBass_B1_poly_coef[24] {};
    samplew_t KBass_B2_poly_coef[24] {0};
    sample_t KBass_B1_poly_param[6] {0};
    sample_t KBass_B2_poly_param[6] {0};
};
