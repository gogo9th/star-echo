#pragma once

#include <string>


class MMDeviceInfo
{
public:
    MMDeviceInfo(const std::wstring & deviceGuid);

    bool isPresent() const
    {
        return !guid.empty();
    }

    std::wstring    guid;
    std::wstring    deviceName;
    std::wstring    endpointName;
    unsigned long   deviceState;
    bool            enhancementsDisabled = false;
    bool            mfxInstalled = false;
};