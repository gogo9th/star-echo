#pragma once

#include <vector>

#include "filter.h"


class FilterFabric
{
public:
    // 'ch,10,9'
    bool addDesc(const std::string & desc);

    std::vector<std::unique_ptr<Filter>> create(int sampleRate) const;

private:
    std::vector<std::function<std::unique_ptr<Filter>(int)>> filterCtors_;
};
