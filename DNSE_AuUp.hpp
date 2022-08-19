#pragma once

#include "DNSE_AuUp_params.h"


template<typename sampleType, typename wideSampleType>
class DNSE_AuUp : public Filter<sampleType, wideSampleType>
{
    using Filter<sampleType, wideSampleType>::smulw;
    using Filter<sampleType, wideSampleType>::normalize;
public:
    using sample_t = typename Filter<sampleType, wideSampleType>::sample_t;
    using samplew_t = typename Filter<sampleType, wideSampleType>::samplew_t;
    using intfloat_t = typename Filter<sampleType, wideSampleType>::intfloat_t;
    using int16float_t = typename Filter<sampleType, wideSampleType>::int16float_t;


    DNSE_AuUp(int v1, int v2)
        : Filter({ 44100, })
    {}

    void setSamplerate(int sampleRate) override
    {
        samplerate_ = sampleRate;
        e_low = 5000 / (samplerate_ >> 9);
        e_point = e_point_prev = 0x80;
        e_point_d = 0x400000;
        e_direction = e_direction_prev = false;

        psrL_.setup(e_point, samplerate_);
        psrR_.setup(e_point, samplerate_);
    }

    void filter(sample_t l, const sample_t r,
                sample_t * l_out, sample_t * r_out) override
    {
        if (fftToSkip == 0)
        {
            // [block] (fft * 9 rounds) [9 * block] [permute] [energy] [psr update]

            if (fft.in(l, r))
            {
                // then we skip 13 rounds of 128 bytes input
                fftToSkip = 13 * 128;

                auto e = fft.energy();

                e_slider = e_direction == e_direction_prev ? e_slider * 2 : e_slider / 2;
                e_slider = std::max(1, std::min(e_slider, 0x10));
                e_direction_prev = e_direction;

                // energy values are approximately of same degree for 16 and 32bit samples

                if constexpr (std::is_integral_v<sample_t>)
                    e_direction = e[e_point] > 0x863;
                else if constexpr (std::is_floating_point_v<sample_t>)
                    // 0x863 / ccdffff = 0.000099
                    // actual value might be up to 0.000005 probably
                    e_direction = e[e_point] > 0.0000099;

                e_point = e_direction ? e_point + e_slider : e_point - e_slider;
                e_point = std::max(e_low, std::min(e_point, 0xFF));

                auto epd_next = (0x3334LL * (e_point << 15) + 0x4CCCLL * e_point_d) >> 15;
                e_point_new = epd_next >> 15;
                e_point_d = epd_next;
            }
        }
        else
        {
            if (fftToSkip == 128)
            {
                // point is updated at the end of 'last' block so last block should be filtered with updated coefs
                if (std::abs(e_point_new - e_point_prev) > 3)
                {
                    e_point_prev = e_point_new;

                    psrL_.setup(e_point_new, samplerate_);
                    psrR_.setup(e_point_new, samplerate_);
                }
            }

            --fftToSkip;
        }

        auto pl = psrL_.filter(l);
        auto pr = psrR_.filter(r);

        normalize(pl, pr);

        *l_out = pl;
        *r_out = pr;
    }

private:
    class FFT
    {
    public:
        FFT(int power = 9)
            : power_(power)
        {
            assert(power > 0);

            auto size = 1 << power;
            perm.resize(size);
            complex.resize(2 * size);
            energy_.resize(size / 2);

            assert(std::size(DNSE_AuUp_Params::fft_window) == size / 2);

            // bit reversed index
            for (auto i = 0; i < size; ++i)
            {
                int p = 0;
                for (size_t shift = 0; shift < power; shift++)
                {
                    p |= ((i >> shift) & 1) << (power - shift - 1);
                }
                perm[i] = p;
            }
        }

        bool in(sample_t l, const sample_t r)
        {
            //complex[2 * offset] = ((samplew_t)l + r) >> 1;
            complex[2 * offset] = ((samplew_t)l + r) / 2;
            complex[2 * offset + 1] = 0;

            if (++offset < complex.size() / 2)
            {
                return false;
            }
            offset = 0;

            // complex is filled in

            // apply window
            for (size_t i = 0; i < complex.size() / 2; i += 2)
            {
                sample_t wnd = DNSE_AuUp_Params::fft_window[i / 2];

                if constexpr (std::is_integral_v<sample_t>)
                {
                    complex[i] = (complex[i] * wnd) >> 15;
                    complex[complex.size() - i - 2] = (complex[complex.size() - i - 2] * wnd) >> 15;
                }
                else if constexpr (std::is_floating_point_v<sample_t>)
                {
                    complex[i] = (complex[i] * wnd) / 0x8000;
                    complex[complex.size() - i - 2] = (complex[complex.size() - i - 2] * wnd) / 0x8000;
                }
            }

            for (size_t round = 0; round < power_; round++)
            {
                fft_dfreq_frwd(round);
            }

            fft_permute();
            fft_energy();

            return true;
        }

        const std::vector<samplew_t> & energy()
        {
            return energy_;
        };

    private:
        void fft_dfreq_frwd(int round)
        {
            auto psz = 1 << power_;
            auto blocksz = (1 << round);


            if (round < power_ - 2)
            {
                auto pc = complex.data();
                auto coff = 2 * (psz >> (round + 1));
                auto pcoff = complex.data() + coff;
                for (size_t iOuter = 0; iOuter < (1 << round); iOuter++)
                {
                    auto d0 = pc[0] - pcoff[0];
                    auto d1 = pc[1] - pcoff[1];
                    auto s0 = pc[0] + pcoff[0];
                    auto s1 = pc[1] + pcoff[1];
                    pc[0] = s0;
                    pc[1] = s1;
                    pcoff[0] = d0;
                    pcoff[1] = d1;

                    if (blocksz < (psz >> 1))
                    {
                        auto ptw = DNSE_AuUp_Params::fft_twiddle + 2 * blocksz;

                        for (size_t iInner = 2 * blocksz; iInner <= (psz >> 1); iInner += blocksz)
                        {
                            auto d2 = pc[2] - pcoff[2];
                            auto d3 = pc[3] - pcoff[3];
                            auto s2 = pc[2] + pcoff[2];
                            auto s3 = pc[3] + pcoff[3];
                            pc[2] = s2;
                            pc[3] = s3;
                            auto c1 = ((d2 * (int64_t)ptw[0]) / 0x8000) - ((d3 * (int64_t)ptw[1]) / 0x8000);
                            auto c2 = ((d2 * (int64_t)ptw[1]) / 0x8000) + ((d3 * (int64_t)ptw[0]) / 0x8000);
                            pcoff[2] = c1;
                            pcoff[3] = c2;

                            pcoff += 2;
                            pc += 2;
                            ptw += 2 * blocksz;
                        }
                        pc += 2;
                        pcoff += 2;
                    }

                    pc += coff;
                    pcoff += coff;
                }
            }
            else if (round == power_ - 2)
            {
                for (auto * pc = complex.data(); pc < complex.data() + complex.size(); pc += 8)
                {
                    const auto c0 = pc[0];
                    const auto c1 = pc[1];
                    const auto c2 = pc[2];
                    const auto c3 = pc[3];
                    const auto c4 = pc[4];
                    const auto c5 = pc[5];
                    const auto c6 = pc[6];
                    const auto c7 = pc[7];
                    pc[0] = c0 + c4;
                    pc[1] = c1 + c5;
                    pc[2] = c2 + c6;
                    pc[3] = c3 + c7;
                    pc[4] = c0 - c4;
                    pc[5] = c1 - c5;
                    pc[6] = c3 - c7;
                    pc[7] = c6 - c2;
                }
            }
            else if (round == power_ - 1)
            {
                for (auto * pc = complex.data(); pc < complex.data() + complex.size(); pc += 4)
                {
                    const auto c0 = pc[0];
                    const auto c1 = pc[1];
                    const auto c2 = pc[2];
                    const auto c3 = pc[3];
                    pc[0] = c0 + c2;
                    pc[1] = c1 + c3;
                    pc[2] = c0 - c2;
                    pc[3] = c1 - c3;
                }
            }
        }

        void fft_permute()
        {
            auto psz = 1 << power_;
            for (size_t j = 0; j < (psz >> 1); j++)
            {
                auto ip = perm[j];
                if (j < ip)
                {
                    std::swap(complex[2 * j], complex[2 * ip]);
                    std::swap(complex[2 * j + 1], complex[2 * ip + 1]);
                }
            }
        }

        void fft_energy()
        {
            auto psz = 1 << power_;
            for (size_t i = 0; i < (psz >> 1); i++)
            {
                if constexpr (std::is_integral_v<sample_t>)
                {
                    int64_t e;
                    if constexpr (sizeof(sample_t) >= 4)
                    {
                        // we sligtly dont fit x64 for squares of x32 type
                        auto v1 = int64_t(complex[2 * i]) >> 16;
                        auto v2 = int64_t(complex[2 * i + 1]) >> 16;
                        e = v1 * v1 + v2 * v2;
                    }
                    else
                    {
                        auto v1 = int64_t(complex[2 * i]);
                        auto v2 = int64_t(complex[2 * i + 1]);
                        e = v1 * v1 + v2 * v2;
                        if (e >= std::numeric_limits<samplew_t>::max()) e = std::numeric_limits<samplew_t>::max();
                    }
                    
                    energy_[i] = (e * 0xCCE + (int64_t)energy_[i] * 0x7332) >> 15;
                }
                else if constexpr (std::is_floating_point_v<sample_t>)
                {
                    auto v1 = complex[2 * i];
                    auto v2 = complex[2 * i + 1];
                    auto e = v1 * v1 + v2 * v2;
                    energy_[i] = (e * 0.1 + energy_[i] * 0.9);
                }
            }
        }

        const int power_;

        size_t offset = 0;
        std::vector<samplew_t> complex;
        std::vector<samplew_t> energy_;

        std::vector<int> perm;
    };

    class PsrBiquad
    {
    public:
        PsrBiquad() = default;

        void setup(const std::array<int16_t, 6> & coef)
        {
            if constexpr (std::is_integral_v<sample_t>)
                coef_ = coef;
            else if constexpr (std::is_floating_point_v<sample_t>)
                coef_ = coef / float(0x10000);
        }

        samplew_t filter(samplew_t in)
        {
            auto nxt = 2 * (smulw(2 * in, coef_[0])
                            + smulw(delay_[0], coef_[1]) + smulw(delay_[1], coef_[2])
                            + smulw(delay_[2], coef_[4]) + smulw(delay_[3], coef_[5]));
            delay_.push_front(2 * in);
            delay_[2] = 2 * nxt;
            return nxt;
        }

    private:
        boost::circular_buffer<samplew_t>   delay_ { 4, 0 };
        std::array<int16float_t, 6>         coef_ { 0 };
    };


    static inline int fr31div(int a1, int a2)
    {
        return ((int64_t)a1 << 31) / a2;
    }

    static inline int fr31mul(int a1, int a2)
    {
        return ((int64_t)a1 * a2) >> 31;
    }


    class Psr
    {
        static int auup_cos(int v)
        {
            const static int coef[] = { 0x6487ED50, -0x295779CB, 0x519AF19, -0x4CB4B3, };

            if (std::abs(v) > 0x3FFFFFFF)
            {
                v = v >= 0 ? 0x7FFFFFFF - v : -0x7FFFFFFF - v;
            }

            int v2 = 2 * v;
            int vQ = fr31mul(v2, v2);
            int r = 0;
            for (auto c : coef)
            {
                r += fr31mul(c, v2);
                v2 = fr31mul(v2, vQ);
            }
            return 2 * r;
        }

        std::array<int16_t, 6> psrCoef(int ep, int uq, int samplerate)
        {
            std::array<int16_t, 6> coef;

            auto pt = 2 * fr31div(ep, samplerate);

            auto v6 = fr31mul(uq, auup_cos(pt)) / 2;
            auto p2 = fr31div(0x3FFFFFFF, v6 + 0x3FFFFFFF);   // + 1/2
            auto vsin = auup_cos(pt + 0x3FFFFFFF);
            coef[0] = fr31mul(vsin / 4 + 0x1FFFFFFF, p2) >> 16;   // + 1/4
            coef[1] = fr31mul(-0x3FFFFFFF - vsin / 2, p2) >> 16;
            coef[2] = fr31mul(vsin / 4 + 0x1FFFFFFF, p2) >> 16;
            coef[3] = 0x3FFF;
            coef[4] = -fr31mul(-vsin, p2) >> 16;
            coef[5] = -fr31mul(0x3FFFFFFF - v6, p2) >> 16;

            return coef;
        }

    public:
        void setup(int ep, int samplerate)
        {
            auto epr = (samplerate >> 9) * ep;
            auto lv = (0x8A3 * epr - 0x10DE5C0) / 1000 + 0x2CCC;
            if constexpr (std::is_integral_v<sample_t>)
                level = std::max(0x2CCC, std::min(lv, 0x71EC));
            else if constexpr (std::is_floating_point_v<sample_t>)
                level = float(std::max(0x2CCC, std::min(lv, 0x71EC))) / 0x10000;

            bq1.setup(psrCoef(epr, 0x7FFFFFFF, samplerate));
            bq2.setup(psrCoef(epr, 0x2AAAAAAA, samplerate));
        }

        samplew_t filter(sample_t in)
        {
            delay_.push_back(in);

            // originally ix is step by +2 but we are moving against shifting delay buffer
            auto ix0 = (0xFF + ix) & 0xFF;
            auto ix1 = (0x7F + ix) & 0xFF;

            ix = (ix + 1) & 0xFF;
            auto mx = ix >= 0x80 ? 0xFF - ix : ix;

            auto v0 = delay_[ix0];
            auto v1 = delay_[ix1];

            auto v = (((samplew_t(v0) * mx) / 0x80) + ((samplew_t(v1) * (0x7F - mx)) / 0x80)) * 0x4000;

            auto q1 = bq1.filter(v);
            auto q2 = bq2.filter(q1);

            return in + (smulw(q2, level) / 0x2000);
        }

    private:
        intfloat_t level = 0;
        PsrBiquad bq1;
        PsrBiquad bq2;

        boost::circular_buffer<sample_t>   delay_ { 256, 0 };
        int ix = 0;
    };


    int samplerate_ = 0;
    FFT fft;
    int fftToSkip = 0;

    bool e_direction = false;
    bool e_direction_prev = false;
    int e_slider = 2;
    int e_point = 0x80;
    int e_point_new = 0x80;
    int e_point_prev = 0x80;
    int e_point_d = 0x400000;
    int e_low = 0;

    Psr psrL_, psrR_;
};
