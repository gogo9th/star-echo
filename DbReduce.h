#pragma once

#include "filter.h"

class DbReduce: public Filter
{
public:
    DbReduce(int db);

    void filter(int16_t l, const int16_t r,
                int16_t * l_out, int16_t * r_out) override;

private:
    int gain_;
};
