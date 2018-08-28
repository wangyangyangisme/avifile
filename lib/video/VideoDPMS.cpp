#include "VideoDPMS.h"
#include "avm_output.h"

#ifdef HAVE_DPMS
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#endif

#ifdef HAVE_OK_DPMS_H
extern "C"
{
#include <X11/extensions/dpms.h>
}

//#include <stdio.h>

#ifndef sun
// on linux its just int
#undef CARD32
#define CARD32 int
#endif // sun

#else // !HAVE_OK_DPMS_H

#ifndef sun
#undef CARD32
#define CARD32 int
#endif // sun
/* there are several prototypes missing on solaris 8: */
#ifdef HAVE_DPMS
extern "C"
{
Bool DPMSQueryExtension(Display*, CARD32*, CARD32*);
Status DPMSEnable(Display*);
Status DPMSDisable(Display*);
Status DPMSInfo(Display*, CARD16*, BOOL*);
}
#endif

#endif // !HAVE_OK_DPMS_H

AVM_BEGIN_NAMESPACE;

// copy from mplayer
// FIXME - create class and add it as a member later..
// with construtor Off  destructor On
VideoDPMS::VideoDPMS(Display *dpy)
    :m_pDisplay(dpy), m_bDisabled(false)
{
#ifdef HAVE_DPMS
    int interval, prefer_blank, allow_exp;
    CARD32 nothing;

    if (DPMSQueryExtension(m_pDisplay, &nothing, &nothing))
    {
	BOOL onoff;
	CARD16 state;
	DPMSInfo(m_pDisplay, &state, &onoff);
	if (onoff)
	{
	    AVM_WRITE("DPMS module", "Disabling DPMS\n");
	    m_bDisabled = true;
	    DPMSDisable(m_pDisplay);  // monitor powersave off
	}
    }
    XGetScreenSaver(m_pDisplay, &m_iTimeoutSave, &interval, &prefer_blank, &allow_exp);
    if (m_iTimeoutSave)
	// turning off screensaver
	XSetScreenSaver(m_pDisplay, 0, interval, prefer_blank, allow_exp);
#else
    AVM_WRITE("DPMS module", "DPMS suppport not compiled\n");

#endif
}

VideoDPMS::~VideoDPMS()
{
#ifdef HAVE_DPMS
    CARD32 nothing;

    if (m_bDisabled)
    {
	if (DPMSQueryExtension(m_pDisplay, &nothing, &nothing))
	{
	    AVM_WRITE("DPMS module", "Enabling DPMS\n");
	    DPMSEnable(m_pDisplay);  // restoring power saving settings
	    DPMSQueryExtension(m_pDisplay, &nothing, &nothing);
	}
    }

    if (m_iTimeoutSave)
    {
	int dummy, interval, prefer_blank, allow_exp;
	XGetScreenSaver(m_pDisplay, &dummy, &interval, &prefer_blank, &allow_exp);
	XSetScreenSaver(m_pDisplay, m_iTimeoutSave, interval, prefer_blank, allow_exp);
	XGetScreenSaver(m_pDisplay, &m_iTimeoutSave, &interval, &prefer_blank, &allow_exp);
    }
#endif
}

AVM_END_NAMESPACE;
