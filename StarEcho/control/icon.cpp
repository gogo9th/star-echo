#include "framework.h"

#include "icon.h"

Icon::Icon(int id)
    : iconId_(id)
{
    assert(hInstance() != 0);

    icon_ = (HICON)LoadImage(hInstance(),
                             MAKEINTRESOURCE(iconId_),
                             IMAGE_ICON,
                             GetSystemMetrics(SM_CXICON),
                             GetSystemMetrics(SM_CYICON),
                             LR_DEFAULTCOLOR);
    if (icon_ == NULL) throw Error::gle("Failed to load icon");
}

Icon::~Icon()
{
    DestroyIcon(icon_);
}

HICON Icon::small()
{
    if (smallIcon_ == 0)
    {
        smallIcon_ = (HICON)LoadImage(hInstance(),
                                      MAKEINTRESOURCE(iconId_),
                                      IMAGE_ICON,
                                      GetSystemMetrics(SM_CXSMICON),
                                      GetSystemMetrics(SM_CYSMICON),
                                      LR_DEFAULTCOLOR);
        if (smallIcon_ == NULL) throw Error::gle("Failed to load icon (B)");
    }
    return smallIcon_;
}
