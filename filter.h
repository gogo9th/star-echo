#pragma once

#include <cstdint>

class Filter
{
public:
    virtual ~Filter() = default;

    virtual void filter(const int16_t * lb, const int16_t * rb,
                        int16_t * lb_out, int16_t * rb_out,
                        int nSamples) = 0;
};
