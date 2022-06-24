#pragma once

#include <vector>

#include "filter.h"


class FilterFabric
{
public:
    // 'ch,10,9'
    // 'eq,12,12,12,12,12,12,12
    bool addDesc(std::string desc, bool doDbReduce = true);

    std::vector<std::unique_ptr<Filter>> create(int sampleRate) const;

private:
    std::vector<std::function<std::unique_ptr<Filter>(int)>> filterCtors_;
};
