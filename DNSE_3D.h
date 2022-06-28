#pragma once

#include <memory>

#include "filter.h"


class DNSE_3D : public Filter
{
public:
    DNSE_3D(int st_eff, int st_rev, int st_hrdel, int sampleRate);
    ~DNSE_3D();

    void filter(int16_t l, const int16_t r,
                int16_t * l_out, int16_t * r_out) override;
private:
    class IIRbiquad3D;
    class FIR4hrtf;
    class Reverb;


    std::unique_ptr<IIRbiquad3D> iir_l_;
    std::unique_ptr<IIRbiquad3D> iir_r_;
    std::unique_ptr<FIR4hrtf>   fir_;
    std::unique_ptr<Reverb>     reverb_;

    int head_[4];
    int khrdel_;
};
