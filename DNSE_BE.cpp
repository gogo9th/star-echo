
#include <cassert>

#include "utils.h"
#include "armspecific.h"
#include "DNSE_BE.h"

#include "DNSE_BE_params.cpp"


DNSE_BE::Biquad16::Biquad16()
    : hIIR_ {0}
{
}

DNSE_BE::Biquad16::Biquad16(const int16_t(&hIIR)[5])
{
    copy(hIIR_, hIIR);
    set(iirb_, samplew_t(0));
}

Filter::samplew_t DNSE_BE::Biquad16::filter(sample_t in)
{
    auto nxt = smulw(iirb_[1], hIIR_[4]) + smulw(iirb_[0], hIIR_[3]) + in;
    auto out = smulw(4 * (smulw(iirb_[1], hIIR_[2]) + smulw(iirb_[0], hIIR_[1]) + nxt), hIIR_[0]);
    iirb_[1] = iirb_[0];
    iirb_[0] = 8 * nxt;
    return out;
}

//

static Filter::sample_t power_poly(Filter::sample_t in, const Filter::samplew_t (&poly_coeff)[24], const Filter::sample_t (&poly_param)[6])
{
    if (in == 0) return 0;

    // coef: <= param0 ( [mul][add][add][add], negative [][][][] ), <= param1 (...., ....), <= param2 (...., ....) 

    const auto *coeff = poly_coeff;
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

    // as we multiply 32x32 we should shift back to sample_t width
    auto v10 = 2 * (smulw(*coeff, in) >> (sizeof(Filter::sample_t) * 8 - 16)) + coeff[1];
    auto v11 = 2 * (smulw(v10, in) >> (sizeof(Filter::sample_t) * 8 - 16)) + coeff[2];
    auto r = (2 * (smulw(v11, in) >> (sizeof(Filter::sample_t) * 8 - 16)) + coeff[3]) << *pv;
    return r >> (32 - (sizeof(Filter::sample_t) * 8));
}

//

DNSE_BE::DNSE_BE(int level, int fc)
    : Filter({ 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 })
    , fc_(fc)
{
    if (level > 0 && level <= 15)
    {
        auto & B1_poly_coef = KBass_B1_poly_coef_all[15 - level];
        for (size_t i = 0; i < std::size(B1_poly_coef); i++) KBass_B1_poly_coef[i] = B1_poly_coef[i];

        auto & B2_poly_coef = KBass_B2_poly_coef_all[15 - level];
        for (size_t i = 0; i < std::size(B2_poly_coef); i++) KBass_B2_poly_coef[i] = B2_poly_coef[i];

        auto & B1_poly_param = KBass_B1_poly_param_all[15 - level];
        for (size_t i = 0; i < std::size(B1_poly_param); i++) KBass_B1_poly_param[i] =  B1_poly_param[i];

        auto & B2_poly_param = KBass_B2_poly_param_all[15 - level];
        for (size_t i = 0; i < std::size(B2_poly_param); i++) KBass_B2_poly_param[i] = B2_poly_param[i];

        if (std::is_same_v<Filter::sample_t, int32_t>)
        {
            // [][][] params, [][][] coefs
            for (size_t i = 0; i < std::size(KBass_B1_poly_param) / 2; i++) KBass_B1_poly_param[i] <<= 16;
            for (size_t i = 0; i < std::size(KBass_B2_poly_param) / 2; i++) KBass_B2_poly_param[i] <<= 16;
        }
    }
    else
    {
        assert(0);
        set(KBass_B1_poly_coef, Filter::samplew_t(0));
        set(KBass_B2_poly_coef, Filter::samplew_t(0));
        set(KBass_B1_poly_param, Filter::sample_t(0));
        set(KBass_B2_poly_param, Filter::sample_t(0));
    }
}

void DNSE_BE::setSamplerate(int sampleRate)
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

    KBass_Hpf_L     = Biquad16(KBass_Hpf_coef_allFs[fc_][Kbass_Fs]);
    KBass_Hpf_R     = Biquad16(KBass_Hpf_coef_allFs[fc_][Kbass_Fs]);
    KBass_AntiDown  = Biquad16(KBass_AntiDown_coef_allFs[Kbass_Fs]);
    KBass_AntiUp    = Biquad16(KBass_AntiUp_coef_allFs[Kbass_Fs]);
    KBass_AntiUpSub = Biquad16(KBass_AntiUp_coef_allFs[Kbass_Fs]);
    KBass_B1_Lpf1   = Biquad16(KBass_B1_Lpf1_coef_allFs[fc_][Kbass_Fs]);
    KBass_B1_Lpf2   = Biquad16(KBass_B1_Lpf2_coef_allFs[fc_][Kbass_Fs]);
    KBass_B1_Bpf1   = Biquad16(KBass_B1_Bpf1_coef_allFs[fc_][Kbass_Fs]);
    KBass_B1_Bpf2   = Biquad16(KBass_B1_Bpf2_coef_allFs[fc_][Kbass_Fs]);
    KBass_B2_Lpf1   = Biquad16(KBass_B2_Lpf1_coef_allFs[fc_][Kbass_Fs]);
    KBass_B2_Lpf2   = Biquad16(KBass_B2_Lpf2_coef_allFs[fc_][Kbass_Fs]);
    KBass_B2_Bpf1   = Biquad16(KBass_B2_Bpf1_coef_allFs[fc_][Kbass_Fs]);
    KBass_B2_Bpf2   = Biquad16(KBass_B2_Bpf2_coef_allFs[fc_][Kbass_Fs]);
}

DNSE_BE::~DNSE_BE()
{}

void DNSE_BE::filter(sample_t l, const sample_t r, sample_t * l_out, sample_t * r_out)
{

    auto l1 = KBass_Hpf_L.filter(l);
    auto r1 = KBass_Hpf_R.filter(r);

    sample_t b1, b2;
    if (KBass_Downrate_)
    {
        // downRate
        int sum = (r >> 1) + (l >> 1);
        b1 = b2 = KBass_AntiDown.filter(sum);
    }
    else
    {
        // normRate
        b1 = b2 = (r >> 1) + (l >> 1);
    }

    int out;
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

    //static std::vector<int16_t> vr;

    if (KBass_Downrate_)
    {
        out = 8 * KBass_AntiUpSub.filter(KBass_AntiUp.filter(out));
        //vr.push_back(o);
    }
    else
    {
        out *= 2;
    }

    samplew_t lOut = l1 + (l1 >> 1) + out;
    samplew_t rOut = r1 + (r1 >> 1) + out;

    normalize(lOut, rOut);

    *l_out = lOut;
    *r_out = rOut;
}
