#pragma once

#include <memory>

#include "filter.h"


class DNSE_BE: public Filter
{
public:
    DNSE_BE(int level, int fc);
    ~DNSE_BE();

    void setSamplerate(int sampleRate) override;

    void filter(int16_t l, const int16_t r,
                int16_t * l_out, int16_t * r_out) override;
private:
    class Biquad16
    {
    public:
        Biquad16();
        Biquad16(const int16_t(&hIIR)[5]);

        int filter(int16_t in);

    private:
        int16_t hIIR_[5];
        int     iirb_[2];
    };

    //

    int fc_;

    int KBass_ExecMode_;
    int KBass_pt_ = 0;
    int KBass_B1_poly_coef[24];
    int KBass_B2_poly_coef[24];
    int16_t KBass_B1_poly_param[6];
    int16_t KBass_B2_poly_param[6];

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
};
