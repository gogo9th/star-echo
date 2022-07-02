#pragma once

#include <boost/circular_buffer.hpp>

#include <memory>

#include "filter.h"


class DNSE_3D : public Filter
{
public:
    DNSE_3D(int st_eff, int st_rev, int st_hrdel);
    ~DNSE_3D();

    void setSamplerate(int sampleRate) override;

    void filter(int16_t l, const int16_t r,
                int16_t * l_out, int16_t * r_out) override;
private:
    class IIRbiquad3D
    {
    public:
        IIRbiquad3D(const int16_t(&hIIR)[4] = { 0 });

        int filter(int16_t in);

    private:
        int16_t hIIR_[4];
        int     iirb_[2];
    };

    class FIR4hrtf
    {
    public:
        FIR4hrtf(int hrdel = 0, const int(&head)[4] = { 0 });

        std::pair<int, int> filter(int l, int r);

    private:
        int16_t lPrev_;
        int16_t rPrev_;
        boost::circular_buffer<int> delayL_;
        boost::circular_buffer<int> delayR_;
        int head_[4];
    };

    class Reverb
    {
    public:
        Reverb(const int(&fincoef)[4] = { 0 });

        std::pair<int, int> filter(int l, int r, int iir_l, int iir_r);

    private:
        int fincoef_[4];
    };

    //

    int st_eff_;
    int st_rev_;
    int st_hrdel_;

    IIRbiquad3D iir_l_;
    IIRbiquad3D iir_r_;
    FIR4hrtf    fir_;
    Reverb      reverb_;

    int head_[4];
    int khrdel_;
};
