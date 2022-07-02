
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
    set(iirb_, 0);
}

int DNSE_BE::Biquad16::filter(int16_t in)
{
    int nxt = smulw(iirb_[1], hIIR_[4]) + smulw(iirb_[0], hIIR_[3]) + in;
    int out = smulw(4 * (smulw(iirb_[1], hIIR_[2]) + smulw(iirb_[0], hIIR_[1]) + nxt), hIIR_[0]);
    iirb_[1] = iirb_[0];
    iirb_[0] = 8 * nxt;
    return out;
}

//

static int16_t power_poly(int16_t in, const int (&poly_coeff)[24], const int16_t (&poly_param)[6])
{
    if (in == 0) return 0;

    const int *poly_coeff1 = poly_coeff;
    auto pv = poly_param + 3;
    if (in < 0)
    {
        poly_coeff1 = poly_coeff + 4;
        in = -in;
    }
    if (in > poly_param[0])
    {
        poly_coeff1 += 8;
        pv = poly_param + 4;
    }
    if (in > poly_param[1])
    {
        poly_coeff1 += 8;
        ++pv;
    }
    if (in > poly_param[2])
    {
        in = poly_param[2];
    }

    auto v10 = 2 * smulw(*poly_coeff1, in) + poly_coeff1[1];
    auto v11 = 2 * smulw(v10, in) + poly_coeff1[2];
    return (2 * smulw(v11, in) + poly_coeff1[3]) << *pv >> 16;
}

//

DNSE_BE::DNSE_BE(int level, int fc)
    : Filter({ 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 })
    , fc_(fc)
{
    if (level > 0 && level <= 15)
    {
        copy(KBass_B1_poly_coef, KBass_B1_poly_coef_all[15 - level]);
        copy(KBass_B2_poly_coef, KBass_B2_poly_coef_all[15 - level]);
        copy(KBass_B1_poly_param, KBass_B1_poly_param_all[15 - level]);
        copy(KBass_B2_poly_param, KBass_B2_poly_param_all[15 - level]);
    }
    else
    {
        assert(0);
        set(KBass_B1_poly_coef, 0);
        set(KBass_B2_poly_coef, 0);
        set(KBass_B1_poly_param, int16_t(0));
        set(KBass_B2_poly_param, int16_t(0));
    }
}

void DNSE_BE::setSamplerate(int sampleRate)
{
    int Kbass_Fs;
    switch (sampleRate)
    {
        case 8000:
            KBass_ExecMode_ = 0;
            Kbass_Fs = 0;
            break;
        case 11025:
            KBass_ExecMode_ = 0;
            Kbass_Fs = 1;
            break;
        case 12000:
            KBass_ExecMode_ = 0;
            Kbass_Fs = 2;
            break;
        case 16000:
            KBass_ExecMode_ = 1;
            Kbass_Fs = 3;
            break;
        case 22050:
            KBass_ExecMode_ = 1;
            Kbass_Fs = 4;
            break;
        case 24000:
            KBass_ExecMode_ = 1;
            Kbass_Fs = 5;
            break;
        case 32000:
            KBass_ExecMode_ = 1;
            Kbass_Fs = 6;
            break;
        default:
        case 44100:
            KBass_ExecMode_ = 1;
            Kbass_Fs = 7;
            break;
        case 48000:
            KBass_ExecMode_ = 1;
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

void DNSE_BE::filter(int16_t l, const int16_t r, int16_t * l_out, int16_t * r_out)
{

    auto l1 = KBass_Hpf_L.filter(l);
    auto r1 = KBass_Hpf_R.filter(r);

    int16_t b1, b2;
    if (KBass_ExecMode_)
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
    if (!KBass_ExecMode_ || (KBass_pt_++ & 3) == 0)
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

    if (KBass_ExecMode_)
    {
        out = 8 * KBass_AntiUpSub.filter(KBass_AntiUp.filter(out));
        //vr.push_back(o);
    }
    else
    {
        out *= 2;
    }

    *l_out = std::min(0x7FFF, std::max(-0x8000, l1 + (l1 >> 1) + out));
    *r_out = std::min(0x7FFF, std::max(-0x8000, r1 + (r1 >> 1) + out));
}
