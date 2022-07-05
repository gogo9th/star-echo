#pragma once

#include <vector>

#include "filter.h"


class FilterFabric
{
public:
    FilterFabric(bool doDbReduce = true)
        : doDbReduce_(doDbReduce)
    {}

    bool addDesc(std::wstring desc);

    std::vector<std::unique_ptr<Filter>> create() const;

private:
    std::vector<std::unique_ptr<Filter>> createFilter(const std::wstring & desc) const;

    bool doDbReduce_;
    std::vector<std::wstring> descs_;
};
