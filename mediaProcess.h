#pragma once

#include <memory>
#include <vector>
#include <filesystem>
#include <functional>

#include "filter.h"
#include "FilterFabric.hpp"


struct FileItem
{
    std::filesystem::path input;
    std::filesystem::path output;
    bool normalize;
};


class MediaProcess
{
    MediaProcess(const MediaProcess &) = delete;
    MediaProcess operator=(const MediaProcess &) = delete;
public:
    //using FilterFab = FilterFabI16;
    //using FilterFab = FilterFabI32;
    using FilterFab = FilterFabD;

    MediaProcess(const FilterFab & fab)
        : filterFab_(fab)
    {}

    std::string operator()(const FileItem & item) const;

private:
    void process(const FileItem & item) const;
    bool do_process(const FileItem & item, std::vector<float>& normalizers) const;

    FilterFab filterFab_;
};
