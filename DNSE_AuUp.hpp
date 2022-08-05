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
    {

    }

    void setSamplerate(int sampleRate) override
    {
        auto lv = (0x8A3 * (sampleRate >> 9) * 0x80/*dword_1C8910*/ - 0x10DE5C0) / 1000 + 0x2CCC;
        level = std::max(0x2CCC, std::min(lv, 0x71EC));
    }

    void filter(sample_t l, const sample_t r,
                sample_t * l_out, sample_t * r_out) override
    {
        fft.in(l, r);

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

        template<typename T = sample_t, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        bool in(sample_t l, const sample_t r)
        {
            complex[2 * offset] = ((samplew_t)l + r) >> 1;
            complex[2 * offset + 1] = 0;

            if (++offset < complex.size() / 2)
            {
                return false;
            }
            offset = 0;

            // complex is filled in

            // apply window
            auto pWnd = DNSE_AuUp_Params::fft_window;
            for (size_t i = 0; i < complex.size() / 2; i += 2)
            {
                complex[i] = (samplew_t(complex[i]) * *pWnd) >> 15;
                complex[complex.size() - i - 2] = (samplew_t(complex[complex.size() - i - 2]) * *pWnd) >> 15;
                ++pWnd;
            }

            for (size_t round = 0; round < power_; round++)
            {
                fft_dfreq_frwd(round);
            }
            return true;
        }

        template<typename T = sample_t, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
        bool in(sample_t l, const sample_t r)
        {
            return false;
        }

    private:
        void fft_dfreq_frwd(int round)
        {
            auto psz = 1 << power_;
            auto blocksz = (1 << round);

            if (round < power_ - 2)
            {
                auto coff = 2 * (psz >> (round + 1));
                auto pc = complex.data();
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

                        for (size_t iInner = 2 * blocksz; iInner < (psz >> 1); iInner += blocksz)
                        {
                            auto d2 = pc[2] - pcoff[2];
                            auto d3 = pc[3] - pcoff[3];
                            auto s2 = pc[2] + pcoff[2];
                            auto s3 = pc[3] + pcoff[3];
                            pc[2] = s2;
                            pc[3] = s3;
                            auto c1 = ((d2 * (int64_t)ptw[0]) >> 15) - ((d3 * (int64_t)ptw[1]) >> 15);
                            auto c2 = ((d2 * (int64_t)ptw[1]) >> 15) + ((d3 * (int64_t)ptw[0]) >> 15);
                            pcoff[2] = c1;
                            pcoff[3] = c2;

                            pcoff += 2;
                            pc += 2;
                        }
                    }

                    pc += 2 + coff;
                    pcoff += 2 + coff;
                }
            }
            else if (round == power_ - 2)
            { }
            else if (round == power_ - 1)
            { }
        }

        const int power_;

        size_t offset = 0;
        std::vector<int> complex;

        std::vector<int> perm;
    };

    int level;
    FFT fft;
};