
#include <algorithm>

#include "DbReduce.h"

static const int dbReduceCoeff[] = { 0x4000, 0x390A, 0x32D6, 0x2D4F, 0x2862, 0x23FD, 0x2013, 0x1C96, 0x197B, 0x16B5, 0x143D, 0x120A, 0x1013, 0xE54, 0xCC5, 0xB62, 0xA25, 0x90A, 0x80F, 0x72E };


DbReduce::DbReduce(int db)
    : Filter({})
{
    db = std::max(0, std::min((int)std::size(dbReduceCoeff) - 1, db));
    gain_ = 4 * dbReduceCoeff[db];
}

void DbReduce::setSamplerate(int sampleRate)
{}

void DbReduce::filter(int16_t l, const int16_t r, int16_t * l_out, int16_t * r_out)
{
    *l_out = (l * gain_) >> 16;
    *r_out = (r * gain_) >> 16;
}
