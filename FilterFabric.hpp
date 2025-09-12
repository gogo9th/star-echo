#pragma once

#include <vector>
#include <map>

#include <boost/algorithm/string.hpp>
#include "filter.h"
#include "utils.h"
#include "DNSE_CH.hpp"
#include "DNSE_EQ.hpp"
#include "DNSE_3D.hpp"
#include "DNSE_BE.hpp"
#include "DNSE_AuUp.hpp"
#include "DbReduce.hpp"


class FilterFabric
{
    template<typename valueType, size_t size, typename charType>
    inline static auto getIntParams(const std::vector<std::basic_string<charType>> & params)
    {
        std::array<valueType, size> values { 0 };
        for (int i = 0; i < std::min(size, params.size()); ++i)
        {
            values[i] = std::stoi(params[i]);
        }
        return values;
    }

public:
    FilterFabric(bool doDbReduce = true, int silence = 0)
        : doDbReduce_(doDbReduce), silence_(silence)
    {}
    int getSilence() const noexcept { return silence_; }  // inline + const 
    bool addDesc(std::wstring desc)
    {
        static const std::map<std::wstring, std::wstring> aliases = {
            { L"studio", L"eq,12,14,12,14,12,12,14;3d,6,1,16;be,4,0" },
            { L"rock", L"eq,15,14,7,5,15,17,15;3d,6,1,16;be,8,2" },
            { L"classical", L"eq,11,11,11,13,15,15,16;3d,8,1,16;be,4,0" },
            { L"jazz", L"eq,12,10,16,12,14,12,10;3d,8,1,16;be,4,0" },
            { L"dance", L"eq,13,16,7,5,14,13,9;3d,8,2,16;be,10,0" },
            { L"ballad", L"eq,12,10,16,12,14,12,10" },
            { L"club", L"eq,19,17,9,7,15,19,18" },
            { L"rnb", L"eq,13,19,15,13,13,15,11" },
            { L"cafe", L"ch,1,7" },
            { L"concert", L"ch,5,7" },
            { L"livecafe", L"ch,13,10" },
            { L"cathedral", L"ch,10,9" },
        };

        auto filters = stringSplit(desc, L";");
        for (auto & filter : filters)
        {
            auto alias = aliases.find(filter);
            if (alias != aliases.end())
            {
                if (!addDesc(alias->second))
                {
                    return false;
                }
            }
            else
            {
                // validation, try to create filter with some typing
                auto f = createFilter<int16_t, int32_t>(filter);
                if (f.empty())
                {
                    return false;
                }

                descs_.push_back(std::move(filter));
            }
        }

        return true;
    }

    template<typename sampleType, typename wideSampleType>
    std::vector<std::unique_ptr<Filter<sampleType, wideSampleType>>> create() const
    {
        std::vector<std::unique_ptr<Filter<sampleType, wideSampleType>>> r;

        for (auto & desc : descs_)
        {
            auto f = createFilter<sampleType, wideSampleType>(desc);
            if (f.empty())
            {
                return {};
            }

            for (auto & pf : f)
            {
                r.emplace_back(std::move(pf));
            }
        }
        return r;
    }

private:
    template<typename sampleType, typename wideSampleType>
    std::vector<std::unique_ptr<Filter<sampleType, wideSampleType>>> createFilter(const std::wstring & desc) const
    {
        try
        {
            auto params = stringSplit(desc, L",");
            if (params.empty())
            {
                return {};
            }

            std::vector<std::unique_ptr<Filter<sampleType, wideSampleType>>> filters;

            auto filterName = params.front();
            params.erase(params.begin());

            if (boost::iequals(filterName, "ch"))
            {
                if (doDbReduce_)
                {
                    filters.push_back(std::make_unique<DbReduce<sampleType, wideSampleType>>(9));
                }

                int a1 = params.size() > 0 ? std::stoi(params[0]) : 10;
                int a2 = params.size() > 1 ? std::stoi(params[1]) : 9;
                filters.push_back(std::make_unique<DNSE_CH<sampleType, wideSampleType>>(a1, a2));
            }
            else if (boost::iequals(filterName, "eq"))
            {
                if (params.size() < 7)
                {
                    err() << "too few parameters for EQ filter";
                    return {};
                }

                if (doDbReduce_)
                {
                    filters.push_back(std::make_unique<DbReduce<sampleType, wideSampleType>>(6));
                }

                auto gains = getIntParams<int16_t, 7>(params);
                filters.push_back(std::make_unique<DNSE_EQ<sampleType, wideSampleType>>(gains));
            }
            else if (boost::iequals(filterName, "3d"))
            {
                if (params.size() < 3)
                {
                    err() << "too few parameters for 3D filter";
                    return {};
                }

                auto intParams = getIntParams<int, 3>(params);
                filters.push_back(std::make_unique<DNSE_3D<sampleType, wideSampleType>>(intParams[0], intParams[1], intParams[2]));
            }
            else if (boost::iequals(filterName, "be"))
            {
                if (params.size() < 2)
                {
                    err() << "too few parameters for 3D filter";
                    return {};
                }

                auto intParams = getIntParams<int, 2>(params);
                filters.push_back(std::make_unique<DNSE_BE<sampleType, wideSampleType>>(intParams[0], intParams[1]));
            }
            else if (boost::iequals(filterName, "upscaling"))
            {
                int level = params.size() > 0 ? std::stoi(params[0]) : 10;
                filters.push_back(std::make_unique<DNSE_AuUp<sampleType, wideSampleType>>(5, 0, level));
            }
            else
            {
                err() << "unsupported filter " << wstringToString(filterName);
                return {};
            }

            return filters;
        }
        catch (const std::exception & e)
        {
            err() << "invalid filter: " << wstringToString(desc) << " (" << e.what() << ')';
        }
        return {};
    }

    bool doDbReduce_;
    int silence_;
    std::vector<std::wstring> descs_;
};
