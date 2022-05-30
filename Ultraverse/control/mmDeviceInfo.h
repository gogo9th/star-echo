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