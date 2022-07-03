
#include <array>

#include <boost/algorithm/string.hpp>

#include "utils.h"
#include "log.hpp"
#include "FilterFabric.h"
#include "DNSE_CH.h"
#include "DNSE_EQ.h"
#include "DNSE_3D.h"
#include "DNSE_BE.h"
#include "DbReduce.h"


template<size_t size, typename charType>
inline static std::array<int16_t, size> getInts(const std::vector<std::basic_string<charType>> & params)
{
    std::array<int16_t, size> values;
    int i = 0;
    for (auto & param : params)
    {
        values[i++] = std::stoi(param);
    }
    return values;
}


bool FilterFabric::addDesc(std::string desc, bool doDbReduce)
{
    try
    {
        auto params = stringSplit(desc, ",");
        if (params.empty())
        {
            return false;
        }

        auto filterName = params.front();
        params.erase(params.begin());

        if (boost::iequals(filterName, "ch"))
        {
            if (doDbReduce)
            {
                filterCtors_.push_back(std::bind([] () { return std::make_unique<DbReduce>(9); }));
            }

            int a1 = params.size() > 0 ? std::stoi(params[0]) : 10;
            int a2 = params.size() > 1 ? std::stoi(params[1]) : 9;
            filterCtors_.push_back(std::bind([] (int a1, int a2)
                                             {
                                                 return std::make_unique<DNSE_CH>(a1, a2);
                                             }, a1, a2));
        }
        else if (boost::iequals(filterName, "eq"))
        {
            if (params.size() < 7)
            {
                err() << "too few parameters for EQ filter";
                return false;
            }

            if (doDbReduce)
            {
                filterCtors_.push_back(std::bind([] () { return std::make_unique<DbReduce>(6); }));
            }

            auto gains = getInts<7>(params);
            filterCtors_.push_back(std::bind([] (auto a1)
                                             {
                                                 return std::make_unique<DNSE_EQ>(a1);
                                             }, gains));
        }
        else if (boost::iequals(filterName, "3d"))
        {
            if (params.size() < 3)
            {
                err() << "too few parameters for 3D filter";
                return false;
            }

            auto intParams = getInts<3>(params);
            filterCtors_.push_back(std::bind([] (auto a1, auto a2, auto a3)
                                             {
                                                 return std::make_unique<DNSE_3D>(a1, a2, a3);
                                             }, intParams[0], intParams[1], intParams[2]));
        }
        else if (boost::iequals(filterName, "be"))
        {
            if (params.size() < 2)
            {
                err() << "too few parameters for 3D filter";
                return false;
            }

            auto intParams = getInts<2>(params);
            filterCtors_.push_back(std::bind([] (auto a1, auto a2)
                                             {
                                                 return std::make_unique<DNSE_BE>(a1, a2);
                                             }, intParams[0], intParams[1]));
        }
        else
        {
            err() << "unsupported filter " << filterName;
            return false;
        }

        return true;
    }
    catch (const std::exception & e)
    {
        err() << "invalid filter: " << desc << " (" << e.what() << ')';
    }
    return false;
}

std::vector<std::unique_ptr<Filter>> FilterFabric::create() const
{
    std::vector<std::unique_ptr<Filter>> r;
    for (auto & ctor : filterCtors_)
    {
        r.emplace_back(std::move(ctor()));
    }
    return r;
}

