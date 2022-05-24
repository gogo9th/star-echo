#pragma once

#include <wtypes.h>
#undef small

class Icon
{
public:
    Icon(int id);
    ~Icon();

    operator HICON() const
    {
        return icon_;
    }

    HICON small();

private:
    int iconId_;
    HICON icon_;
    HICON smallIcon_ = 0;
};
