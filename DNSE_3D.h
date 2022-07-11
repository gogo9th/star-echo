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

    void filter(sample_t l, const sample_t r,
                sample_t * l_out, sample_t * r_out) override;
private:
    class IIRbiquad3D
    {
    public:
        IIRbiquad3D(const int16_t(&hIIR)[4] = { 0 });

        samplew_t filter(sample_t in);

    private:
        int16_t     hIIR_[4];
        samplew_t   iirb_[2];
    };

    class FIR4hrtf
    {
    public:
        FIR4hrtf(int hrdel = 0, const int16_t(&head)[4] = { 0 });

        std::pair<samplew_t, samplew_t> filter(samplew_t l, samplew_t r);

    private:
        boost::circular_buffer<samplew_t> delayL_;
        boost::circular_buffer<samplew_t> delayR_;
        int16_t head_[4];
    };

    class Reverb
    {
    public:
        Reverb(const int(&fincoef)[4] = { 0 });

        std::pair<samplew_t, samplew_t> filter(samplew_t l, samplew_t r, samplew_t iir_l, samplew_t iir_r);

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
