#pragma once

#include "filter.h"

class DbReduce: public Filter
{
public:
    DbReduce(int db);

    void setSamplerate(int sampleRate) override;

    void filter(sample_t l, const sample_t r,
                sample_t * l_out, sample_t * r_out) override;

private:
    int gain_;
};
