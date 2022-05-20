#include "stdafx.h"

#include "Q2APO.h"
#include "log.hpp"


class LogInit init_;


class Q2APODll : public CAtlDllModuleT<Q2APODll>
{
public:
    DECLARE_LIBID(LIBID_Q2APOLib)
    //DECLARE_REGISTRY_APPID_RESOURCEID(IDR_Q2APODLL, "{F959DBE4-EBF7-4A7A-B3F4-DEC4EB102E6B}")

    HRESULT DllRegisterServer(BOOL bRegTypeLib = TRUE) throw()
    {
        HRESULT hResult;

        // Register all APOs implemented in this module.
        unsigned regIndex = 0;
        for (; regIndex < std::size(gCoreAPOs); regIndex++)
        {
            hResult = RegisterAPO(gCoreAPOs[regIndex]);
            if (FAILED(hResult))
            {
                goto ExitFailed;
            }
        }

        // Register the module.
        hResult = CAtlDllModuleT<Q2APODll>::DllRegisterServer(bRegTypeLib);
        if (FAILED(hResult))
        {
            goto ExitFailed;
        }

        return hResult;

    ExitFailed:
        // Unregister all registered APOs if something failed.
        for (unsigned unregIndex = 0; unregIndex < regIndex; unregIndex++)
        {
            ATLVERIFY(SUCCEEDED(UnregisterAPO(gCoreAPOs[unregIndex]->clsid)));
        }

        return hResult;
    }

    HRESULT DllUnregisterServer(BOOL bUnRegTypeLib = TRUE) throw()
    {
        HRESULT hResult;

        // Unregister all the APOs that are implemented in this module.
        for (unsigned u32APOUnregIndex = 0; u32APOUnregIndex < std::size(gCoreAPOs); u32APOUnregIndex++)
        {
            hResult = UnregisterAPO(gCoreAPOs[u32APOUnregIndex]->clsid);
            ATLASSERT(SUCCEEDED(hResult));
            if (FAILED(hResult))
            {
                goto Exit;
            }
        }

        // Unregister the module.
        hResult = CAtlDllModuleT<Q2APODll>::UnregisterServer(bUnRegTypeLib);
        if (FAILED(hResult))
        {
            goto Exit;
        }


    Exit:
        return hResult;
    }

private:
    inline static const APO_REG_PROPERTIES * gCoreAPOs[] =
    {
        &Q2APOMFX::regProperties.m_Properties,
    };
};

Q2APODll _module;


extern "C" BOOL WINAPI DllMain(HINSTANCE /* hInstance */, DWORD dwReason, LPVOID lpReserved)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
    }
    // do necessary cleanup only if the DLL is being unloaded dynamically
    else if ((DLL_PROCESS_DETACH == dwReason) && (NULL == lpReserved))
    {
    }

    return _module.DllMain(dwReason, lpReserved);
}

__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow(void)
{
    return _module.DllCanUnloadNow();
}

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR * ppv)
{
    return _module.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    return _module.DllRegisterServer();
}

STDAPI DllUnregisterServer()
{
    return _module.DllUnregisterServer();
}
