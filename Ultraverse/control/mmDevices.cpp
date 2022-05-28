
#define INITGUID
#include <mmdeviceapi.h>

#include "mmDevices.h"


std::wstring getDefaultMMDevice(EDataFlow flow)
{
    static PROPERTYKEY guidPropertyKey = { {0x1da5d803, 0xd492, 0x4edd, 0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}, 4 };

    std::wstring result;

    IMMDeviceEnumerator * enumerator = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&enumerator);
    if (SUCCEEDED(hr))
    {
        IMMDevice * endPoint = NULL;
        hr = enumerator->GetDefaultAudioEndpoint(flow, eMultimedia, &endPoint);
        if (SUCCEEDED(hr))
        {
            IPropertyStore * propertyStore = NULL;
            hr = endPoint->OpenPropertyStore(STGM_READ, &propertyStore);
            if (SUCCEEDED(hr))
            {
                PROPVARIANT variant;
                PropVariantInit(&variant);
                hr = propertyStore->GetValue(PKEY_AudioEndpoint_GUID, &variant);
                if (SUCCEEDED(hr))
                {
                    result = variant.pwszVal;
                    PropVariantClear(&variant);
                }
                propertyStore->Release();
            }
            endPoint->Release();
        }
        enumerator->Release();
    }

    return result;
}

