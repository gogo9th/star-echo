
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>

#include "mmDeviceInfo.h"
#include "mmDevices.h"

#include "common/registry.h"
#include "common/stringUtils.h"
#include <apo/Q2APO.h>



MMDeviceInfo::MMDeviceInfo(const std::wstring & deviceGuid)
{
    auto devRoot = Registry::openKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Render\\" + deviceGuid);

    Registry::getValue(devRoot, L"DeviceState", deviceState);

    if (deviceState & DEVICE_STATE_NOTPRESENT
        || deviceState & DEVICE_STATE_DISABLED)
        return;

    guid = deviceGuid;
    toUpperCase(guid);

    auto hKeyProps = Registry::openKey(devRoot, L"Properties");
    Registry::getValue(hKeyProps, toString(PKEY_Endpoint_Name), endpointName);
    Registry::getValue(hKeyProps, toString(PKEY_Device_DeviceDesc), deviceName);

    auto KEY_AudioEndpoint_Disable_SysFx = toString(PKEY_AudioEndpoint_Disable_SysFx);
    if (Registry::valueExists(hKeyProps, KEY_AudioEndpoint_Disable_SysFx))
    {
        DWORD ed;
        Registry::getValue(hKeyProps, KEY_AudioEndpoint_Disable_SysFx, ed);
        enhancementsDisabled = ed != ENDPOINT_SYSFX_ENABLED;
    }

    if (Registry::keyExists(devRoot, L"FxProperties"))
    {
        auto hKeyFXProps = Registry::openKey(devRoot, L"FxProperties");

        auto KEY_FX_ModeEffectClsid = toString(PKEY_FX_ModeEffectClsid);
        if (Registry::valueExists(hKeyFXProps, KEY_FX_ModeEffectClsid))
        {
            std::wstring apoGuid;
            Registry::getValue(hKeyFXProps, KEY_FX_ModeEffectClsid, apoGuid);
            if (apoGuid == toString(__uuidof(Q2APOMFX)))
            {
                mfxInstalled = true;
            }
        }
    }

}
