#include "stdafx.h"

#include <audioengineextensionapo.h>

#include "DNSE_CH.h"
#include "DNSE_EQ.h"
#include "Q2APO.h"
#include "log.hpp"

#include "common/stringUtils.h"
#include "common/registry.h"
#include "common/settings.h"
#include "common/error.h"


#pragma comment(lib, "legacy_stdio_definitions.lib")    // _vsnwprintf


OBJECT_ENTRY_AUTO(__uuidof(Q2APOMFX), Q2APOMFX)

const CRegAPOProperties<1> Q2APOMFX::regProperties(
    __uuidof(Q2APOMFX),
    L"Q2APO", L"" /* no copyright specified */, 1, 0,
    __uuidof(IAudioProcessingObject),
    (APO_FLAG)(APO_FLAG_FRAMESPERSECOND_MUST_MATCH | APO_FLAG_BITSPERSAMPLE_MUST_MATCH | APO_FLAG_INPLACE)
);

// {6BB23FB8-BE73-4E55-B04C-FB7BF4866AC5}
DEFINE_GUID(Q2EffectCH, 0x6bb23fb8, 0xbe73, 0x4e55, 0xb0, 0x4c, 0xfb, 0x7b, 0xf4, 0x86, 0x6a, 0xc5);

struct CUSTOM_FORMAT_ITEM
{
    UNCOMPRESSEDAUDIOFORMAT format;
    LPCWSTR                 name;
};

static const struct
{
    UNCOMPRESSEDAUDIOFORMAT format;
    LPCWSTR                 name;
}
availableFormatsPcm_[] =
{
    {{KSDATAFORMAT_SUBTYPE_PCM, 2, 2, 16, 32000, KSAUDIO_SPEAKER_STEREO}, L"Wave 32 KHz, 16-bit, stereo"},
    {{KSDATAFORMAT_SUBTYPE_PCM, 2, 2, 16, 44100, KSAUDIO_SPEAKER_STEREO}, L"Wave 44.1 KHz, 16-bit, stereo"},
    {{KSDATAFORMAT_SUBTYPE_PCM, 2, 2, 16, 48000, KSAUDIO_SPEAKER_STEREO}, L"Wave 48 KHz, 16-bit, stereo"},
},
 availableFormatsFloat_[] =
{
    {{KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 2, 4, 32, 32000, KSAUDIO_SPEAKER_STEREO}, L"Wave 32 KHz, float, stereo"},
    {{KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 2, 4, 32, 44100, KSAUDIO_SPEAKER_STEREO}, L"Wave 44.1 KHz, float, stereo"},
    {{KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 2, 4, 32, 48000, KSAUDIO_SPEAKER_STEREO}, L"Wave 48 KHz, float, stereo"},
}
;

static bool isEqual(const UNCOMPRESSEDAUDIOFORMAT & l, const UNCOMPRESSEDAUDIOFORMAT & r)
{
    return l.guidFormatType == r.guidFormatType
        && l.dwSamplesPerFrame == r.dwSamplesPerFrame
        && l.dwBytesPerSampleContainer == r.dwBytesPerSampleContainer
        && l.dwValidBitsPerSample == r.dwValidBitsPerSample
        && l.fFramesPerSecond == r.fFramesPerSecond
        && l.dwChannelMask == r.dwChannelMask;
};

//

template<typename T>
std::basic_ostream<T> & operator<<(std::basic_ostream<T> & os, IAudioMediaType * mediaType)
{
    if (!mediaType)
    {
        os << LS<T>("MediaType { }");
        return os;
    }

    os << LS<T>("MediaType { Compressed: ");

    BOOL isCompressed;
    HRESULT icfr = mediaType->IsCompressedFormat(&isCompressed);
    if (SUCCEEDED(icfr))
    {
        os << isCompressed;
    }
    else
    {
        os << LS<T>(" <Err ") << icfr << LS<T>(">");
    }

    auto format = mediaType->GetAudioFormat();
    os << LS<T>(" tag: ") << format->wFormatTag
        << LS<T>(" ch: ") << format->nChannels
        << LS<T>(" sps: ") << format->nSamplesPerSec
        << LS<T>(" bps: ") << format->nAvgBytesPerSec
        << LS<T>(" block: ") << format->nBlockAlign
        << LS<T>(" bits/s: ") << format->wBitsPerSample;
    if (format->cbSize >= 22)
    {
        auto formatEx = (WAVEFORMATEXTENSIBLE *)format;
        os << LS<T>(" | samples: ") << formatEx->Samples.wSamplesPerBlock
            << LS<T>(" cm: ") << formatEx->dwChannelMask
            << LS<T>(" subformat: ") << formatEx->SubFormat;
    }
    else
    {
        os << LS<T>(" cb: ") << format->cbSize;
    }
    os << LS<T>(" }");

    return os;
}

template<typename T>
std::basic_ostream<T> & operator<<(std::basic_ostream<T> & os, const UNCOMPRESSEDAUDIOFORMAT & format)
{
    os << LS<T>("Format { ")
        << format.guidFormatType
        << LS<T>(" s/f: ") << format.dwSamplesPerFrame
        << LS<T>(" b/s: ") << format.dwBytesPerSampleContainer
        << LS<T>(" bits/s: ") << format.dwValidBitsPerSample
        << LS<T>(" fps: ") << format.fFramesPerSecond
        << LS<T>(" cm: ") << format.dwChannelMask
        << LS<T>(" }");

    return os;
}

//

Q2APOMFX::Q2APOMFX()
    : CBaseAudioProcessingObject(regProperties)
    , settingMonitorThread_(&Q2APOMFX::settingsMonitor, this)
    , stopSettingMonitorEvent_(CreateEventW(NULL, TRUE, FALSE, NULL))
{
    dbg() << "Creating";

    filterSettings_ = Settings::currentEffect();
}

Q2APOMFX::~Q2APOMFX()
{
    dbg() << "Deleting";

    SetEvent(stopSettingMonitorEvent_);
    if (settingMonitorThread_.joinable())
    {
        settingMonitorThread_.join();
    }
}

STDMETHODIMP Q2APOMFX::GetLatency(HNSTIME * pTime)
{
    if (!pTime) return E_POINTER;

    if (!m_bIsLocked)
        return APOERR_ALREADY_UNLOCKED;

    *pTime = 0;

    return S_OK;
}


STDMETHODIMP Q2APOMFX::Initialize(UINT32 cbDataSize, BYTE * pbyData)
{
    // init is called multiple times
    // init may be called while locked o_O

    if ((NULL == pbyData) && (0 != cbDataSize)) return E_INVALIDARG;
    if ((NULL != pbyData) && (0 == cbDataSize)) return E_POINTER;

    if (cbDataSize == sizeof(APOInitSystemEffects))
    {
        dbg() << "Init APOInitSystemEffects L " << m_bIsLocked;
        //APOInitSystemEffects * eff = (APOInitSystemEffects *)pbyData;
    }
    else if (cbDataSize == sizeof(APOInitSystemEffects2))
    {
        auto eff = (APOInitSystemEffects2 *)pbyData;
        // AUDIO_SIGNALPROCESSINGMODE_DEFAULT
        dbg() << "Init APOInitSystemEffects 2" << " mode " << eff->AudioProcessingMode << " L " << m_bIsLocked;
    }

    return CBaseAudioProcessingObject::Initialize(cbDataSize, pbyData);
}


STDMETHODIMP Q2APOMFX::IsInputFormatSupported(IAudioMediaType * pOutputFormat,
                                              IAudioMediaType * pRequestedInputFormat,
                                              IAudioMediaType ** ppSupportedInputFormat)
{
    if (!pRequestedInputFormat || !ppSupportedInputFormat) return E_POINTER;

    //auto hr = IsFormatTypeSupported(pOutputFormat, pRequestedInputFormat, &ppSupportedInputFormat, false);

    UNCOMPRESSEDAUDIOFORMAT ucReq;
    auto uc = pRequestedInputFormat->GetUncompressedAudioFormat(&ucReq);
    if (FAILED(uc)) return uc;

    msg() << "IIFS Req " << ucReq;
    if (checkFormat(ucReq))
    {
        dbg() << " supported";
        *ppSupportedInputFormat = pRequestedInputFormat;
        (*ppSupportedInputFormat)->AddRef();
        return S_OK;
    }
    else
    {
        // propose most suitable
        const UNCOMPRESSEDAUDIOFORMAT * proposed;

        if (ucReq.guidFormatType == KSDATAFORMAT_SUBTYPE_PCM)
        {
            if (ucReq.fFramesPerSecond <= availableFormatsPcm_[0].format.fFramesPerSecond)
                proposed = &availableFormatsPcm_[0].format;
            else if (ucReq.fFramesPerSecond >= availableFormatsPcm_[std::size(availableFormatsPcm_) - 1].format.fFramesPerSecond)
                proposed = &availableFormatsPcm_[std::size(availableFormatsPcm_) - 1].format;
            else
                proposed = &availableFormatsPcm_[1].format;
        }
        else if (ucReq.guidFormatType == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        {
            if (ucReq.fFramesPerSecond <= availableFormatsFloat_[0].format.fFramesPerSecond)
                proposed = &availableFormatsFloat_[0].format;
            else if (ucReq.fFramesPerSecond >= availableFormatsFloat_[std::size(availableFormatsFloat_) - 1].format.fFramesPerSecond)
                proposed = &availableFormatsFloat_[std::size(availableFormatsFloat_) - 1].format;
            else
                proposed = &availableFormatsFloat_[1].format;
        }
        else
        {
            err() << "Unsupported format " << ucReq.guidFormatType;
            return APOERR_FORMAT_NOT_SUPPORTED;
        }

        //auto hr = CreateAudioMediaType(proposed, sizeof(*proposed), ppSupportedInputFormat);
        auto hr = CreateAudioMediaTypeFromUncompressedAudioFormat(proposed, ppSupportedInputFormat);
        if (SUCCEEDED(hr))
        {
            msg() << "IIFS Out proposing " << *ppSupportedInputFormat;
        }
        else
        {
            err() << "Failed to create audio format " << *proposed;
        }

        return SUCCEEDED(hr) ? S_FALSE : APOERR_FORMAT_NOT_SUPPORTED;
    }
}


STDMETHODIMP_(HRESULT __stdcall) Q2APOMFX::IsOutputFormatSupported(IAudioMediaType * pInputFormat,
                                                                   IAudioMediaType * pRequestedOutputFormat,
                                                                   IAudioMediaType ** ppSupportedOutputFormat)
{
    if (!pRequestedOutputFormat || !ppSupportedOutputFormat) return E_POINTER;

    dbg() << "IOFS Req " << pRequestedOutputFormat;

    return IsFormatTypeSupported(pInputFormat, pRequestedOutputFormat, ppSupportedOutputFormat, false);
}


STDMETHODIMP_(HRESULT __stdcall) Q2APOMFX::GetInputChannelCount(UINT32 * pu32ChannelCount)
{
    if (!pu32ChannelCount) return E_POINTER;

    dbg() << "GetInputChannelCount";

    *pu32ChannelCount = 2;

    return S_OK;
}


STDMETHODIMP Q2APOMFX::LockForProcess(UINT32 u32NumInputConnections, APO_CONNECTION_DESCRIPTOR ** ppInputConnections,
                                      UINT32 u32NumOutputConnections, APO_CONNECTION_DESCRIPTOR ** ppOutputConnections)
{
    dbg() << "LockForProcess:  input " << u32NumInputConnections
        << " output " << u32NumOutputConnections;

    if (u32NumInputConnections == 0 || !ppInputConnections
        || u32NumOutputConnections == 0 || !ppOutputConnections)
    {
        return E_INVALIDARG;
    }

    // this apparently calls IOFS/IIFS internally
    auto hr = CBaseAudioProcessingObject::LockForProcess(u32NumInputConnections, ppInputConnections,
                                                         u32NumOutputConnections, ppOutputConnections);
    if (FAILED(hr)) return hr;

    if (u32NumInputConnections > 1 || u32NumOutputConnections > 1)
    {
        err() << "Extra connections are not supported: " << u32NumInputConnections << ' ' << u32NumOutputConnections;
    }

    UNCOMPRESSEDAUDIOFORMAT inFormat;
    hr = ppOutputConnections[0]->pFormat->GetUncompressedAudioFormat(&inFormat);
    if (FAILED(hr)) return hr;

    UNCOMPRESSEDAUDIOFORMAT outFormat;
    hr = ppOutputConnections[0]->pFormat->GetUncompressedAudioFormat(&outFormat);
    if (FAILED(hr)) return hr;

    dbg() << "LockForProcess: <= " << inFormat;
    dbg() << "LockForProcess: => " << outFormat;

    if (!isEqual(inFormat, outFormat))
    {
        err() << "Input and output format mismatch:";
        err() << inFormat;
        err() << outFormat;
        return CO_E_NOT_SUPPORTED;
    }

    lockedIsPCM_ = inFormat.guidFormatType == KSDATAFORMAT_SUBTYPE_PCM;
    lockedSampleRate_ = (int)inFormat.fFramesPerSecond;

    {
        std::lock_guard lk(filterSettingMtx_);
        createFilters(filterSettings_);
    }

    return S_OK;
}


STDMETHODIMP Q2APOMFX::UnlockForProcess()
{
    dbg() << "UnlockForProcess";
    
    errorOnce_ = false;
    lockedSampleRate_ = 0;

    {
        std::lock_guard lk(filtersMtx_);
        filters_.clear();
    }

    return CBaseAudioProcessingObject::UnlockForProcess();
}


STDMETHODIMP Q2APOMFX::GetEffectsList(_Outptr_result_buffer_maybenull_(*pcEffects) LPGUID * ppEffectsIds,
                                      _Out_ UINT * pcEffects,
                                      _In_ HANDLE Event)
{
    if (ppEffectsIds == NULL || pcEffects == NULL)
        return E_POINTER;

    HRESULT hr;
    BOOL effectsLocked = FALSE;
    UINT cEffects = 0;

    dbg() << "GetEffectsList";

    {
        struct EffectControl
        {
            GUID effect;
            //BOOL control;
        };

        EffectControl list[] =
        {
            { Q2EffectCH,  /*m_fEnableSwapMFX*/  },
        };

        //if (!IsEqualGUID(m_AudioProcessingMode, AUDIO_SIGNALPROCESSINGMODE_RAW))
        //{
        //    // count the active effects
        //    for (UINT i = 0; i < ARRAYSIZE(list); i++)
        //    {
        //        if (list[i].control)
        //        {
        //            cEffects++;
        //        }
        //    }
        //}

        cEffects = 1;

        //if (0 == cEffects)
        //{
        //    *ppEffectsIds = NULL;
        //    *pcEffects = 0;
        //}
        //else
        {
            GUID * pEffectsIds = (LPGUID)CoTaskMemAlloc(sizeof(GUID) * cEffects);
            if (pEffectsIds == nullptr)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            // pick up the active effects
            UINT j = 0;
            for (UINT i = 0; i < std::size(list); i++)
            {
                //if (list[i].control)
                {
                    pEffectsIds[j++] = list[i].effect;
                }
            }

            *ppEffectsIds = pEffectsIds;
            *pcEffects = cEffects;
        }

        hr = S_OK;
    }

Exit:
    return hr;
}

#pragma AVRT_CODE_BEGIN
STDMETHODIMP_(void) Q2APOMFX::APOProcess(UINT32 u32NumInputConnections, APO_CONNECTION_PROPERTY ** ppInputConnections,
                                         UINT32 u32NumOutputConnections, APO_CONNECTION_PROPERTY ** ppOutputConnections)
{
    if (u32NumInputConnections > 1 || u32NumOutputConnections > 1)
    {
        if (!errorOnce_)
        {
            err() << "Extra connections are not supported: " << u32NumInputConnections << ' ' << u32NumOutputConnections;
            errorOnce_ = true;
        }
    }

    auto & iConn = ppInputConnections[0];
    auto & oConn = ppOutputConnections[0];

    std::lock_guard lk(filtersMtx_);

    switch (iConn->u32BufferFlags)
    {
        case BUFFER_VALID:
        {
            if (lockedIsPCM_)
            {
                // not tested
                auto inputFrames = reinterpret_cast<int16_t *>(iConn->pBuffer);
                auto outputFrames = reinterpret_cast<int16_t *>(oConn->pBuffer);

                if (!filters_.empty())
                {
                    for (unsigned i = 0; i < iConn->u32ValidFrameCount; i++)
                    {
                        auto l = inputFrames[2 * i];
                        auto r = inputFrames[2 * i + 1];

                        int16_t ol, or ;
                        for (auto & filter : filters_)
                        {
                            filter->filter(l, r, &ol, &or);
                            l = ol;
                            r = or;
                        }
                        outputFrames[2 * i] = int16_t(std::round(float(ol) * adjustGain_));
                        outputFrames[2 * i + 1] = int16_t(std::round(float(or ) * adjustGain_));
                    }
                }
                else
                {
                    std::copy(inputFrames, inputFrames + iConn->u32ValidFrameCount * 2, outputFrames);
                }
            }
            else
            {
                auto inputFrames = reinterpret_cast<float *>(iConn->pBuffer);
                auto outputFrames = reinterpret_cast<float *>(oConn->pBuffer);

                if (!filters_.empty())
                {
                    for (unsigned i = 0; i < iConn->u32ValidFrameCount; i++)
                    {
                        // decrease volume before filter if adjustGain should decrease volume
                        auto l = int((adjustGain_ < 1 ? inputFrames[2 * i] * adjustGain_ : inputFrames[2 * i]) * 0x7FFF);
                        auto r = int((adjustGain_ < 1 ? inputFrames[2 * i + 1] * adjustGain_ : inputFrames[2 * i + 1]) * 0x7FFF);

                        int16_t ol, or;
                        for (auto & filter : filters_)
                        {
                            filter->filter(l, r, &ol, &or);
                            l = ol;
                            r = or ;
                        }

                        // decrease volume after filter if adjustGain should increase volume
                        outputFrames[2 * i] = adjustGain_ > 1 ? (float(ol) / 0x7FFF) * adjustGain_ : (float(ol) / 0x7FFF);
                        outputFrames[2 * i + 1] = adjustGain_ > 1 ? (float(or) / 0x7FFF) * adjustGain_ : (float(or) / 0x7FFF);
                    }
                }
                else
                {
                    std::copy(inputFrames, inputFrames + iConn->u32ValidFrameCount * 2, outputFrames);
                }
            }

            //auto os = std::ofstream("f:\\Temp\\a.pcm", std::ios::binary | std::ios::app);
            //os.write((const char *)iConn->pBuffer, 2 * 4 * iConn->u32ValidFrameCount);

            oConn->u32ValidFrameCount = iConn->u32ValidFrameCount;
            oConn->u32BufferFlags = iConn->u32BufferFlags;

            break;
        }

        case BUFFER_SILENT:
        {
            oConn->u32ValidFrameCount = iConn->u32ValidFrameCount;
            oConn->u32BufferFlags = iConn->u32BufferFlags;

            break;
        }
    }
}
#pragma AVRT_CODE_END


STDMETHODIMP Q2APOMFX::GetFormatCount(UINT * pcFormats)
{
    dbg() << "GetFormatCount";

    if (pcFormats == NULL) return E_POINTER;

    *pcFormats = UINT(std::size(availableFormatsPcm_) + std::size(availableFormatsFloat_));
    return S_OK;
}


STDMETHODIMP Q2APOMFX::GetFormat(UINT nFormat, IAudioMediaType ** ppFormat)
{
    dbg() << "GetFormat " << nFormat;

    if (ppFormat == nullptr) return E_POINTER;
    if (nFormat >= std::size(availableFormatsPcm_) + std::size(availableFormatsFloat_)) return E_INVALIDARG;

    *ppFormat = NULL;

    auto hr = CreateAudioMediaTypeFromUncompressedAudioFormat(nFormat < std::size(availableFormatsPcm_) ? &availableFormatsPcm_[nFormat].format
                                                              : &availableFormatsFloat_[nFormat - std::size(availableFormatsPcm_)].format,
                                                              ppFormat);
    return hr;
}


STDMETHODIMP Q2APOMFX::GetFormatRepresentation(UINT nFormat, LPWSTR * ppwstrFormatRep)
{
    dbg() << "GetFormatRepresentation " << nFormat;

    if (ppwstrFormatRep == nullptr) return E_POINTER;
    if (nFormat >= std::size(availableFormatsPcm_) + std::size(availableFormatsFloat_)) return E_INVALIDARG;

    auto & fmt = nFormat < std::size(availableFormatsPcm_) ?
        availableFormatsPcm_[nFormat]
        : availableFormatsFloat_[nFormat - std::size(availableFormatsPcm_)];

    auto nameLenZ = wcslen(fmt.name) + 1;
    auto nameStr = (LPWSTR)CoTaskMemAlloc(nameLenZ * sizeof(WCHAR));
    if (nameStr == NULL) return E_OUTOFMEMORY;

    std::copy(fmt.name, fmt.name + nameLenZ, nameStr);
    *ppwstrFormatRep = nameStr;
    return S_OK;
}

void Q2APOMFX::createFilters(const std::wstring & settings)
{
    msg() << "Setup: " << settings;

    if (settings == Settings::effectDisabledString()
        || settings.empty())
    {
        std::lock_guard lk(filtersMtx_);
        filters_.clear();
        return;
    }

    std::vector<std::unique_ptr<Filter>> newFilters;
    float adjustGain = 1;

    auto filters = stringSplit(settings, L";");
    for (auto & setting : filters)
    {
        auto params = stringSplit(setting, L",");

        if (params[0] == L"ch")
        {
            auto chRoomSize = params.size() > 1 ? std::stoi(params[1]) : 0;
            auto chGain = params.size() > 2 ? std::stoi(params[2]) : 0;

            if (chRoomSize >= 1 && chRoomSize <= 13
                && chGain >= 0 && chGain <= 11
                && lockedSampleRate_ != 0)
            {
                adjustGain *= 0.75;
                newFilters.push_back(std::make_unique<DNSE_CH>(chRoomSize, chGain, lockedSampleRate_));
            }
            else if (chRoomSize != 0 || chGain != 0)
            {
                err() << "Invalig CH filter settings: " << chRoomSize << ", " << chGain;
            }
        }
        else if (params[0] == L"eq")
        {
            params.erase(params.begin());

            if (params.size() >= 7)
            {
                std::array<int16_t, 7> gains;

                int i = 0;
                for (auto & param : params)
                {
                    gains[i++] = std::stoi(param);
                }

                adjustGain *= 0.75;
                newFilters.push_back(std::make_unique<DNSE_EQ>(gains, lockedSampleRate_));
            }
            else
            {
                err() << "Too few parameters for EQ filter";
            }
        }
        else
        {
            err() << "Unsupported filter setting " << settings;
        }
    }

    {
        std::lock_guard lk(filtersMtx_);
        filters_.swap(newFilters);
        adjustGain_ = adjustGain;
        dbg() << "Total adjust gain: " << adjustGain_;
    }
}

//

bool Q2APOMFX::checkFormat(const UNCOMPRESSEDAUDIOFORMAT & format)
{
    if (format.guidFormatType == KSDATAFORMAT_SUBTYPE_PCM)
    {
        for (auto & fmt : availableFormatsPcm_)
        {
            if (isEqual(fmt.format, format))
            {
                return true;
            }
        }
    }
    else if (format.guidFormatType == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
    {
        for (auto & fmt : availableFormatsFloat_)
        {
            if (isEqual(fmt.format, format))
            {
                return true;
            }
        }
    }

    return false;
}

void Q2APOMFX::settingsMonitor()
{
    try
    {
        Handle hEvent(CreateEventW(NULL, FALSE, FALSE, NULL));
        auto hKey = Registry::openKey(Settings::appPath, KEY_NOTIFY | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
        HANDLE waitObjs[] = { hEvent, stopSettingMonitorEvent_ };

        auto settingCache_ = Settings::currentEffect();

        while (true)
        {
            auto s = RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
            if (s != ERROR_SUCCESS) throw ::Error("RegNotifyChangeKeyValue failed", s);

            auto wr = WaitForMultipleObjects((int)std::size(waitObjs), waitObjs, FALSE, INFINITE);
            switch (wr)
            {
                case WAIT_OBJECT_0:
                {
                    auto newSetting = Settings::currentEffect();
                    if (newSetting == settingCache_)
                    {
                        break;
                    }

                    settingCache_ = newSetting;

                    {
                        std::lock_guard lk(filterSettingMtx_);
                        filterSettings_ = newSetting;
                        createFilters(filterSettings_);
                    }

                    break;
                }

                case WAIT_OBJECT_0 + 1:
                    dbg() << "Settings monitor stopped";
                    return;

                default:
                    err() << "Unexpected wait result " << wr;
                    return;
            }
        }
    }
    catch (const ::Error & e)
    {
        err() << e.what();
    }
    catch (const WError & e)
    {
        err() << e.what();
    }
    catch (const std::exception & e)
    {
        err() << "Exception : " << e.what();
    }
}

