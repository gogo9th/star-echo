#pragma once

#include "stdafx.h"

#include <Unknwn.h>
#include <audioenginebaseapo.h>
#include <BaseAudioProcessingObject.h>

#include "Q2APO_h.h"
#include "resource.h"
#include "common/scopedResource.h"


class Filter;

#pragma AVRT_CONST_BEGIN

class Q2APOMFX :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<Q2APOMFX, &CLSID_Q2APOMFX>,
    public CBaseAudioProcessingObject,
    //public IMMNotificationClient,
    //public IAudioProcessingObjectNotifications,   // this adds extra formats to device format list
    //public IAudioSystemEffectsCustomFormats,
    //public IAudioSystemEffects3,  // Windows 11
    public IAudioSystemEffects2,    // 8.1
    //public IAudioSystemEffects,
    public IQ2APOMFX
{
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_Q2APOMFX)

    BEGIN_COM_MAP(Q2APOMFX)
        COM_INTERFACE_ENTRY(IQ2APOMFX)
        COM_INTERFACE_ENTRY(IAudioSystemEffects)
        COM_INTERFACE_ENTRY(IAudioSystemEffects2)
        //COM_INTERFACE_ENTRY(IAudioSystemEffects3)
        //COM_INTERFACE_ENTRY(IAudioSystemEffectsCustomFormats)
        //COM_INTERFACE_ENTRY(IMMNotificationClient)
        //COM_INTERFACE_ENTRY(IAudioProcessingObjectNotifications)
        COM_INTERFACE_ENTRY(IAudioProcessingObjectRT)
        COM_INTERFACE_ENTRY(IAudioProcessingObject)
        COM_INTERFACE_ENTRY(IAudioProcessingObjectConfiguration)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    Q2APOMFX();
    ~Q2APOMFX();

    // IAudioProcessingObject
    STDMETHOD(GetLatency)(HNSTIME * pTime);
    STDMETHOD(Initialize)(UINT32 cbDataSize, BYTE * pbyData);
    STDMETHOD(IsInputFormatSupported)(IAudioMediaType * pOutputFormat,
                                      IAudioMediaType * pRequestedInputFormat,
                                      IAudioMediaType ** ppSupportedInputFormat);
    STDMETHOD(IsOutputFormatSupported)(IAudioMediaType * pInputFormat,
                                       IAudioMediaType * pRequestedOutputFormat,
                                       IAudioMediaType ** ppSupportedOutputFormat);
    STDMETHOD(GetInputChannelCount)(UINT32 * pu32ChannelCount);

    // IAudioSystemEffects2
    STDMETHOD(GetEffectsList)(LPGUID * ppEffectsIds, UINT * pcEffects, HANDLE Event);

    // IAudioProcessingObjectConfiguration
    STDMETHOD(LockForProcess)(UINT32 u32NumInputConnections, APO_CONNECTION_DESCRIPTOR ** ppInputConnections,
                              UINT32 u32NumOutputConnections, APO_CONNECTION_DESCRIPTOR ** ppOutputConnections);
    STDMETHOD(UnlockForProcess)(void);

    // IAudioProcessingObjectRT
    STDMETHOD_(void, APOProcess)(UINT32 u32NumInputConnections, APO_CONNECTION_PROPERTY ** ppInputConnections,
                                 UINT32 u32NumOutputConnections, APO_CONNECTION_PROPERTY ** ppOutputConnections);

    // IAudioSystemEffectsCustomFormats
    // This interface may be optionally supported by APOs that attach directly to the connector in the DEFAULT mode streaming graph
    STDMETHOD(GetFormatCount)(UINT * pcFormats);
    STDMETHOD(GetFormat)(UINT nFormat, IAudioMediaType ** ppFormat);
    STDMETHOD(GetFormatRepresentation)(UINT nFormat, _Outptr_ LPWSTR * ppwstrFormatRep);


    static const CRegAPOProperties<1> regProperties;

private:
    bool checkFormat(const UNCOMPRESSEDAUDIOFORMAT & pRequestedFormat);
    void settingsMonitor();
    void parseSettings(const std::wstring & settings);
    void createFilters(const std::wstring & settings);

    bool    errorOnce_ = false;

    std::recursive_mutex    filterSettingMtx_;
    std::wstring            filterSettings_;

    std::mutex                              filtersMtx_;
    std::vector<std::unique_ptr<Filter>>    filters_;
    float   adjustGain_ = 1;

    bool lockedIsPCM_ = false;
    bool lockedSampleRate_ = 0;

    std::thread settingMonitorThread_;
    Handle      stopSettingMonitorEvent_;
};

#pragma AVRT_CONST_END
