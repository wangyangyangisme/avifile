#ifndef AVIFILE_VIDEODPMS_H
#define AVIFILE_VIDEODPMS_H

#include "avm_default.h"

#ifndef X_DISPLAY_MISSING
#include <X11/Xlib.h>
#endif

AVM_BEGIN_NAMESPACE;

class VideoDPMS
{
    Display* m_pDisplay;
    bool m_bDisabled;
    int m_iTimeoutSave;
public:
    VideoDPMS(Display* dpy);
    ~VideoDPMS();
};

AVM_END_NAMESPACE;

#endif	// AVIFILE_VIDEODPMS_H
