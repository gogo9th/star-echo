#pragma once

#include <memory>
#include <boost/circular_buffer.hpp>

#include "filter.h"
#include "utils.h"
#include "DNSE_CH_params.h"


template<typename sampleType, typename wideSampleType>
class DNSE_CH : public Filter<sampleType, wideSampleType>
{
#pragma pack(4)
    struct PresetFilter
    {
        int32_t tone;
        int32_t absorb;
        int32_t d1c785c;
        int32_t gains[4];
        int32_t er_delays[7];
        int32_t er_ap1_delay;
        int32_t er_ap2_delay;
        int32_t ap3_delay;
        int32_t ap4_delay;
        int32_t ap5_delay;
        int32_t vbr1_delay;
        int32_t delay1_delay;
        int32_t ap6_delay;
        int32_t ap7_delay;
        int32_t ap8_delay;
        int32_t vbr2_delay;
        int32_t delay2_delay;
        int32_t er_ap1_gain;
        int32_t er_ap2_gain;
        int32_t ap3_gain;
        int32_t ap4_gain;
        int32_t ap5_gain;
        int32_t vbr1_gain;
        int32_t delay1_gain;
        int32_t ap6_gain;
        int32_t ap7_gain;
        int32_t ap8_gain;
        int32_t vbr2_gain;
        int32_t delay2_gain;
    };

#pragma pack(4)
    struct PresetGain
    {
        int32_t d_gain;
        int32_t er_gain;
        int32_t r_gain;
    };

    using Filter<sampleType, wideSampleType>::normalize;
public:
    using sample_t = typename Filter<sampleType, wideSampleType>::sample_t;
    using samplew_t = typename Filter<sampleType, wideSampleType>::samplew_t;
    using intfloat_t = typename Filter<sampleType, wideSampleType>::intfloat_t;


    DNSE_CH(int roomSize, int gain)
        : Filter<sampleType, wideSampleType>({ 32000, 44100, 48000 })
    {
        roomSize_ = std::max(1, std::min(13, roomSize));

        gain = std::max(0, std::min((int)std::size(DNSE_CH_Params::total_gains) - 1, gain));
        totalGain_ = DNSE_CH_Params::total_gains[gain];
    }

    void setSamplerate(int sampleRate) override
    {
        int srSelector = 1;
        if (sampleRate == 48000)
            srSelector = 2;
        else if (sampleRate == 32000)
            srSelector = 0;

        auto presetFilter = &((PresetFilter *)DNSE_CH_Params::preset_filters)[(roomSize_ - 1) * 3 + srSelector];
        presetGain_ = &((PresetGain *)DNSE_CH_Params::preset_gains)[(roomSize_ - 1)];

        toneFilter_ = ToneFilter(presetFilter->d1c785c,
                                    presetFilter->tone,
                                    to_array(presetFilter->gains),
                                    totalGain_);
        dsFilter_ = DelaySplitFilter(4159, to_array(presetFilter->gains), to_array(presetFilter->er_delays));
        er_ap1_ = APFilter(std::min(presetFilter->er_ap1_delay, 631), presetFilter->er_ap1_gain);
        er_ap2_ = APFilter(std::min(presetFilter->er_ap2_delay, 739), presetFilter->er_ap2_gain);

        ch1_ = FilterChain();
        ch1_.add(APFilter(std::min(presetFilter->ap3_delay, 1993), presetFilter->ap3_gain));
        ch1_.add(APFilter(std::min(presetFilter->ap4_delay, 2843), presetFilter->ap4_gain));
        ch1_.add(APFilter(std::min(presetFilter->ap5_delay, 2053), presetFilter->ap5_gain));
        auto vbr1_delay = ((0xa3d * presetFilter->vbr1_delay) >> 12);
        vbr1_ = VbrFilter(std::min(vbr1_delay, 2881), presetFilter->vbr1_gain, 0x14ce, 0x102);

        delay1_ = DelayShiftFilter(std::min(presetFilter->delay1_delay, 2837),
                                    presetFilter->delay1_gain,
                                    presetFilter->absorb,
                                    12);

        ch2_ = FilterChain();
        ch2_.add(APFilter(std::min(presetFilter->ap6_delay, 1637), presetFilter->ap6_gain));
        ch2_.add(APFilter(std::min(presetFilter->ap7_delay, 2749), presetFilter->ap7_gain));
        ch2_.add(APFilter(std::min(presetFilter->ap8_delay, 3079), presetFilter->ap8_gain));
        auto vbr2_delay = ((0xb1b * presetFilter->vbr2_delay) >> 12);
        vbr2_ = VbrFilter(std::min(vbr2_delay, 3038), presetFilter->vbr2_gain, 0x17c6, 0xc0);

        delay2_ = DelayShiftFilter(std::min(presetFilter->delay2_delay, 2833),
                                    presetFilter->delay2_gain,
                                    presetFilter->absorb,
                                    22);
    }

    virtual void filter(sample_t l, sample_t r,
                        sample_t * l_out, sample_t * r_out) override
    {
        //samplew_t sum = (samplew_t(l) + r) >> 2;
        auto sum = sample_t((samplew_t(l) + r) / 4);

        auto tone = toneFilter_.filter(sum);
        auto [ls, rs] = dsFilter_.filter(tone);

        auto ap1 = er_ap1_.filter(ls);
        auto ap2 = er_ap2_.filter(rs);

        auto sec1in = ap1 - delay2_.last();
        auto sec2in = ap2 + delay1_.last();

        auto dr1 = delay1_.filter(vbr1_.filter(ch1_.filter(sec1in)));
        auto dr2 = delay2_.filter(vbr2_.filter(ch2_.filter(sec2in)));

        samplew_t r1 = ((dr1 * presetGain_->r_gain + ap1 * presetGain_->er_gain) / 0x1000);
        samplew_t r2 = ((dr2 * presetGain_->r_gain + ap2 * presetGain_->er_gain) / 0x1000);

        r1 = (r1 + l);
        r2 = (r2 + r);

        normalize(r1, r2);

        *l_out = sample_t(r1);
        *r_out = sample_t(r2);
    }

private:
    // 0.5 of >>12
    static constexpr samplew_t m800 = 0x800 << (sizeof(DNSE_CH::sample_t) * 8 - 16);

    class ToneFilter
    {
    public:
        ToneFilter() = default;
        ToneFilter(int32_t d1c785c, int32_t tone, const std::array<int32_t, 4> & gains, int32_t total_gain)
        {
            if constexpr (std::is_integral_v<sample_t>)
            {
                d1c785c_ = d1c785c;
                tone_ = tone;
                gains_ = gains;
                total_gain_ = total_gain;
            }
            else if constexpr (std::is_floating_point_v<sample_t>)
            {
                d1c785c_ = d1c785c / float(0x1000);
                tone_ = tone / float(0x1000);
                gains_ = gains / float(0x1000);
                total_gain_ = total_gain / float(0x1000);
            }
        }

        template<typename T = sample_t, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        sample_t filter(sample_t in)
        {
            v_tone += (samplew_t(in - v_tone) * tone_ + m800) >> 12;

            auto v1 = delay_[1] * gains_[2] + delay_[0] * gains_[3] + ((int64_t /* not w!*/)v_tone << 12);
            auto v2 = delay_[1] * gains_[0] + delay_[0] * gains_[1];

            delay_.push_back(samplew_t(v1 >> 12));

            return sample_t(((((/*64bit mul for 16bit sample_t*/v1 * d1c785c_ >> 12) + v2) >> 12) * total_gain_) >> 12);
        }

        template<typename T = sample_t, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
        sample_t filter(sample_t in)
        {
            v_tone = sample_t(v_tone + samplew_t(in - v_tone) * tone_);

            auto v1 = delay_[1] * gains_[2] + delay_[0] * gains_[3] + v_tone;
            auto v2 = delay_[1] * gains_[0] + delay_[0] * gains_[1];

            delay_.push_back(v1);

            return sample_t((v1 * d1c785c_ + v2) * total_gain_);
        }

    private:
        intfloat_t total_gain_ = 0;
        intfloat_t tone_ = 0;
        intfloat_t d1c785c_ = 0;
        std::array<intfloat_t, 4> gains_ { 0 };

        sample_t v_tone = 0;
        boost::circular_buffer<samplew_t> delay_ { 2, 0 };
    };

    class DelayFilter
    {
    public:
        DelayFilter() = default;
        DelayFilter(int delay)
            : delay_(delay)
            , delay_buff_(delay, 0)
        {}

        virtual ~DelayFilter() {}

    protected:
        int delay_ = 0;
        boost::circular_buffer<samplew_t> delay_buff_;
    };

    class DelaySplitFilter : public DelayFilter
    {
        using DelayFilter::delay_buff_;
    public:
        DelaySplitFilter() = default;
        DelaySplitFilter(int delay,
                         const std::array<int32_t, 4> & gains,
                         const std::array<int32_t, 7> & er_delays)
            : DelayFilter(delay)
            , gains_ { gains }
            , er_delays_ { er_delays }
        {
            for (auto & d : er_delays_)
            {
                // workaround firmware bug
                assert(d <= delay);
                if (d >= delay)
                {
                    d -= delay;
                }
            }
        }

        std::array<samplew_t, 2> filter(samplew_t in)
        {
            delay_buff_.push_front(in);

            auto l = (delay_buff_[er_delays_[0]] / 2) + (delay_buff_[er_delays_[1]]) + (delay_buff_[er_delays_[3]] / 2) + (delay_buff_[er_delays_[5]] / 2);
            auto r = (delay_buff_[er_delays_[0]] / 2) + (delay_buff_[er_delays_[2]]) + (delay_buff_[er_delays_[4]] / 2) + (delay_buff_[er_delays_[6]] / 2);
            return { l, r };
        }

    private:
        // gains, why ?
        std::array<int32_t, 4> gains_ { 0 };
        std::array<int32_t, 7> er_delays_ { 0 };
    };

    class APFilter : public DelayFilter
    {
        using DelayFilter::delay_buff_;
    public:
        APFilter() = default;
        APFilter(int delay, int gain)
            : DelayFilter(delay)
        {
            if constexpr (std::is_integral_v<sample_t>)
                gain_ = gain;
            else
                gain_ = gain / float(0x1000);
        }

        template<typename T = sample_t, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        samplew_t filter(samplew_t in)
        {
            auto last = delay_buff_.back();

            auto first = in + ((gain_ * last + m800) >> 12);
            delay_buff_.push_front(first);
            return last - (((gain_ * first) + m800) >> 12);
        }

        template<typename T = sample_t, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
        samplew_t filter(samplew_t in)
        {
            auto last = delay_buff_.back();

            auto first = in + gain_ * last;
            delay_buff_.push_front(first);
            return last - gain_ * first;
        }

    protected:
        intfloat_t gain_ = 0;
    };

    class FilterChain
    {
    public:
        void add(APFilter && filter)
        {
            chain_.push_back(std::move(filter));
        }

        samplew_t filter(samplew_t in)
        {
            for (auto & f : chain_)
            {
                in = f.filter(in);
            }
            return in;
        }

    private:
        std::vector<APFilter> chain_;
    };

    class VbrFilter : public DelayFilter
    {
        using DelayFilter::delay_buff_;
    public:
        VbrFilter() = default;
        VbrFilter(int delay, int gain, int shift, int ggain)
            : DelayFilter(delay + ggain + 3)
            , shift_(shift)
            , vbr_delay_(delay)
            , ggain_(ggain)
        {
            if constexpr (std::is_integral_v<sample_t>)
                gain_ = gain;
            else
                gain_ = gain / float(0x1000);
        }

        samplew_t filter(samplew_t in)
        {
            v66 += shift_;
            if (v66 > 0x20000000)
            {
                v66 = -0x20000000;
            }
            auto pre_offset = (vbr_delay_ << 12) + ggain_ * (abs(v66) >> 17);
            auto offset = (pre_offset >> 12);
            /// ...
            // 0x20000000 >> 17 = 0x1000 ... ?
            // notice off-by-one when using deque instead of circular buffer
            auto data_2 = delay_buff_[std::min(size_t(offset + 1), delay_buff_.size() - 1)];
            auto data_1 = delay_buff_[std::min(size_t(offset), delay_buff_.size() - 1)];
            if constexpr (std::is_integral_v<sample_t>)
            {
                auto offset_s = 0x1000 - (pre_offset - (pre_offset >> 12 << 12));
                v66_1 = data_2 + ((offset_s * data_1) >> 12) - ((offset_s * v66_1) >> 12);
                auto y = v66_1 - ((in * gain_) >> 12);
                auto first = in + ((y * gain_) >> 12);
                delay_buff_.push_front(first);
                return y;
            }
            else if constexpr (std::is_floating_point_v<sample_t>)
            {
                auto offset_s = (0x1000 - (pre_offset - (pre_offset >> 12 << 12))) / float(0x1000);
                v66_1 = data_2 + offset_s * data_1 - offset_s * v66_1;
                auto y = v66_1 - in * gain_;
                auto first = in + y * gain_;
                delay_buff_.push_front(first);
                return y;
            }
        }

    private:
        int vbr_delay_ = 0;
        int shift_ = 0;
        int ggain_ = 0;
        int v66 = 0;
        intfloat_t gain_ = 0;
        samplew_t v66_1 = 0;
    };

    class DelayShiftFilter : public DelayFilter
    {
        using DelayFilter::delay_buff_;
    public:
        DelayShiftFilter() = default;
        DelayShiftFilter(int delay,
                         int32_t gain,
                         int32_t absorb,
                         int32_t out_shift)
            : DelayFilter(delay)
            , out_shift_(out_shift)
        {
            assert(out_shift_ > 0 && out_shift <= delay);
            if constexpr (std::is_integral_v<sample_t>)
            {
                gain_ = gain;
                absorb_ = absorb;
            }
            else if constexpr (std::is_floating_point_v<sample_t>)
            {
                gain_ = gain / float(0x1000);
                absorb_ = absorb / float(0x1000);
            }
        }

        template<typename T = sample_t, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        samplew_t last()
        {
            return (delay_buff_.back() * gain_) >> 12;
        }

        template<typename T = sample_t, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        samplew_t filter(samplew_t in)
        {
            flow_ += ((((in - flow_) * absorb_) + m800) >> 12);
            delay_buff_.push_front(flow_);

            return delay_buff_[out_shift_ - 1];
        }


        template<typename T = sample_t, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
        samplew_t last()
        {
            return delay_buff_.back() * gain_;
        }

        template<typename T = sample_t, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
        samplew_t filter(samplew_t in)
        {
            flow_ += (in - flow_) * absorb_;
            delay_buff_.push_front(flow_);

            return delay_buff_[out_shift_ - 1];
        }

    private:
        samplew_t flow_ = 0;
        intfloat_t gain_ = 0;
        intfloat_t absorb_ = 0;
        size_t out_shift_ = 0;
    };


    int roomSize_;
    int totalGain_;
    const PresetGain * presetGain_ = nullptr;

    ToneFilter          toneFilter_;
    DelaySplitFilter    dsFilter_;
    APFilter            er_ap2_;
    APFilter            er_ap1_;
    FilterChain         ch1_;
    FilterChain         ch2_;
    VbrFilter           vbr1_;
    VbrFilter           vbr2_;
    DelayShiftFilter    delay1_;
    DelayShiftFilter    delay2_;
};
