#pragma once

#include <string>
#include <vector>
#include <memory>


class MMDeviceInfo
{
public:
    MMDeviceInfo(const std::wstring & deviceGuid);

    bool isPresent() const
    {
        return !guid.empty();
    }

    void installApo();
    void removeApo();

    void applyTodo();
    void enableEnhancements();


    std::wstring    guid;
    std::wstring    deviceName;
    std::wstring    endpointName;
    unsigned long   deviceState;
    bool            enhancementsDisabled = false;

    bool            apoMfxInstalled = false;
    enum ToDoState
    {
        ToDoNothing,
        ApoToBeInstalled,
        ApoToBeRemoved,
    }               toDoState = ToDoNothing;
};

std::vector<std::shared_ptr<MMDeviceInfo>> getPlaybackDevices();
