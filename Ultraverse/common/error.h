#pragma once


class WError
{
public:
    WError(const std::wstring & message)
        : message_(message)
    {}

    std::wstring what() const
    {
        return message_;
    }

    static WError gle(const wchar_t * msg)
    {
        return WError(std::wstring(msg) + L" (" + std::to_wstring(GetLastError()) + L")");
    }

private:
    std::wstring message_;
};


class Error : public std::runtime_error
{
public:
    Error(const char * msg)
        : runtime_error(msg)
    {}

    Error(const char * msg, int errorCode)
        : runtime_error(std::string(msg) + " (" + std::to_string(errorCode) + ")")
    {}

    static Error gle(const char * msg)
    {
        return Error(msg, GetLastError());
    }
};