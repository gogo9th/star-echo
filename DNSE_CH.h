#pragma once

#include <memory>
#include "filter.h"


class DNSE_CH : public Filter
{
public:
    DNSE_CH(int roomSize, int gain, int sampleRate);
    ~DNSE_CH();

    virtual void filter(const int16_t * lb, const int16_t * rb,
                        int16_t * lb_out, int16_t * rb_out,
                        int nSamples) override;
private:
    class PresetGain;
    class FilterChain;
    class APFilter;
    class DelayFilter;
    class DelaySplitFilter;
    class DelayShiftFilter;
    class VbrFilter;
    class ToneFilter;

    const PresetGain * presetGain_;

    std::unique_ptr<ToneFilter>         toneFilter_;
    std::unique_ptr<DelaySplitFilter>   dsFilter_;
    std::unique_ptr<APFilter>           er_ap1_;
    std::unique_ptr<APFilter>           er_ap2_;
    std::unique_ptr<FilterChain>        ch1_;
    std::unique_ptr<FilterChain>        ch2_;
    std::unique_ptr<DelayShiftFilter>   delay1_;
    std::unique_ptr<DelayShiftFilter>   delay2_;
};
