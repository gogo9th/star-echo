#pragma once

#include <wtypes.h>
#undef small

class Icon
{
public:
    explicit Icon(HICON hIcon, bool isSmall)
    {
        if (isSmall)
        {
            smallIcon_ = hIcon;
        }
        else
        {
            icon_ = hIcon;
        }
    }

    Icon(int id);
    ~Icon();

    operator HICON() const
    {
        return icon_;
    }

    HICON small();

private:
    int iconId_ = 0;
    HICON icon_ = 0;
    HICON smallIcon_ = 0;
};
