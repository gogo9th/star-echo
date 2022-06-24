
#include <array>

#include <boost/algorithm/string.hpp>

#include "utils.h"
#include "log.hpp"
#include "FilterFabric.h"
#include "DNSE_CH.h"
#include "DNSE_EQ.h"
#include "DbReduce.h"


bool FilterFabric::addDesc(std::string desc, bool doDbReduce)
{
    try
    {
        if (boost::iequals(desc, "ballad"))
        {
            desc = "eq,12,10,16,12,14,12,10";
        }
        else if (boost::iequals(desc, "club"))
        {
            desc = "eq,19,17,9,7,15,19,18";
        }
        else if (boost::iequals(desc, "rnb"))
        {
            desc = "eq,13,19,15,13,13,15,11";
        }

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
                filterCtors_.push_back(std::bind([] (int /*sr*/)
                                                 {
                                                     return std::make_unique<DbReduce>(9);
                                                 }, std::placeholders::_1));
            }

            int a1 = params.size() > 0 ? std::stoi(params[0]) : 10;
            int a2 = params.size() > 1 ? std::stoi(params[1]) : 9;
            filterCtors_.push_back(std::bind([] (int a1, int a2, int sr)
                                             {
                                                 return std::make_unique<DNSE_CH>(a1, a2, sr);
                                             }, a1, a2, std::placeholders::_1));
            return true;
        }
        else if (boost::iequals(filterName, "eq"))
        {
            if (params.size() < 7)
            {
                err() << "too few parameters for EQ filter";
                return false;
            }

            std::array<int16_t, 7> gains;
            int i = 0;
            for (auto & param : params)
            {
                gains[i++] = std::stoi(param);
            }

            if (doDbReduce)
            {
                filterCtors_.push_back(std::bind([] (int /*sr*/)
                                                 {
                                                     return std::make_unique<DbReduce>(6);
                                                 }, std::placeholders::_1));
            }
            filterCtors_.push_back(std::bind([] (auto a1, int sr)
                                             {
                                                 return std::make_unique<DNSE_EQ>(a1, sr);
                                             }, gains, std::placeholders::_1));
            return true;
        }
        else
        {
            err() << "unsupported filter " << filterName;
        }
    }
    catch (const std::exception & e)
    {
        err() << "invalid filter: " << desc << " (" << e.what() << ')';
    }
    return false;
}

std::vector<std::unique_ptr<Filter>> FilterFabric::create(int sampleRate) const
{
    std::vector<std::unique_ptr<Filter>> r;
    for (auto & ctor : filterCtors_)
    {
        r.emplace_back(std::move(ctor(sampleRate)));
    }
    return r;
}

