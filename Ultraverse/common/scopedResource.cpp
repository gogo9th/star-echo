#include "pch.h"

#include "scopedResource.h"


template<> void Handle::Cleanup() noexcept
{
    if (resource_)
    {
        CloseHandle(resource_);
    }
}

template<> void HMenu::Cleanup() noexcept
{
    if (resource_ && resource_ != INVALID_HANDLE_VALUE)
    {
        DestroyMenu(resource_);
    }
}

template<> void HKey::Cleanup() noexcept
{
    if (resource_ && resource_ != 0)
    {
        RegCloseKey(resource_);
    }
}

template<> void PSid::Cleanup() noexcept
{
    if (resource_ && resource_ != 0)
    {
        FreeSid(resource_);
    }
}

