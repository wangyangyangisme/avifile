
#include "aviplay_impl.h"
#include "playerwidget.h"
#include "avm_fourcc.h"
#include "avm_cpuinfo.h"
#include "avm_except.h"
#include "configfile.h"
#include "infotypes.h"

#include <stdio.h>
#include <stdlib.h> // free()

AVM_BEGIN_NAMESPACE;

class AviPlayer2: public AviPlayer, public IPlayerWidget
{
    IPlayerWidget* m_pPW;
    void* m_pDpy;
    int m_iResizeCount;
    int zx, zy, zw, zh;
public:
    AviPlayer2(IPlayerWidget* pw, void* _dpy,
	       const char* filename, const char* subname,
               unsigned int flags,
	       const char* vcodec, const char* acodec) :
    AviPlayer(filename, GetPhysicalDepth(_dpy), subname, flags,
	      vcodec, acodec), m_pPW(pw), m_pDpy(_dpy),
	m_iResizeCount(1), zx(0), zy(0), zw(0), zh(0)
    {
	m_pKillhandler = killHandler;
	m_pKillhandlerArg = (void*)this;

	if (!m_pPW)
	    m_pPW = this;
    }
    virtual ~AviPlayer2() { m_pKillhandler = 0; }
    virtual int Refresh()
    {
	for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    m_VideoRenderers[i]->Refresh();
	return 0;
    }
    // should be redesigned - only first one will get resize event for now
    virtual int Resize(int& new_w, int& new_h)
    {
	if (m_VideoRenderers.size() > 0)
	{
	    bool pres;
	    Get(VIDEO_PRESERVE_ASPECT, &pres, 0);
	    if (pres)
	    {
		StreamInfo* si = m_pVideostream->GetStreamInfo();
		float a = si->GetAspectRatio();
                delete si;
		if (a == 0)
                    a = GetWidth() / (float) GetHeight();
		new_w = (int) (new_h * a + 0.5);
		if (zw)
		    new_w = new_h * zw / zh;
	    }
	    return m_VideoRenderers.front()->Resize(new_w, new_h);
	}
	return 0;
    }
    virtual int Zoom(int x, int y, int w, int h)
    {
	zx = zy = zw = zh = 0;
	for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    if (m_VideoRenderers[i]->Zoom(x, y, w, h) == 0)
	    {
		zx = x; zy = y; zw = w; zh = h;
	    }
        return 0;
    }
    virtual int ToggleFullscreen(bool maximize=false)
    {
	for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    if (m_VideoRenderers[i]->ToggleFullscreen(maximize) != 0)
		return -1;
	return 0;
    }

    // FIXME: will be removed - and replaced with better code later
    // this is very impractical
    // we need to support many other callbacks
    // users might want to use any device they have connected
    // to their linux to control player
    virtual void PW_stop_func() { Stop(); }	//'x'
    virtual void PW_middle_button()		//middle-click
    {
	m_iResizeCount++;
	updateResize();
    }
    virtual void PW_pause_func() { Pause(true); }//'c'
    virtual void PW_play_func()			//'v'
    {
	if (!IsPlaying()) Start(); else Pause(false);
    }
    virtual void PW_fullscreen() { ToggleFullscreen(); } //alt+enter, esc
    virtual void PW_resize(int w, int h) { Resize(w,h); } // resize event
    virtual void PW_refresh() { Refresh(); }
    virtual void PW_maximize_func()  { ToggleFullscreen(true); }

    void updateResize()
    {
	int w, h;
	switch(m_iResizeCount % 3)
	{
	case 0:
	    w=GetWidth()/2; h=GetHeight()/2;
	    break;
	case 1:
	    w=GetWidth(); h=GetHeight();
	    break;
	case 2:
	    w=GetWidth()*2; h=GetHeight()*2;
	    break;
	}
	Resize(w, h);
    }

protected:
    virtual void createVideoRenderer()
    {
        int pos = -1, wx, wy, ww, wh;
	while (m_VideoRenderers.size() > 0)
	{
            if (pos < 0)
		pos = m_VideoRenderers.back()->GetPosition(wx, wy);
            if (pos == 0)
		pos = m_VideoRenderers.back()->GetSize(ww, wh);
	    delete m_VideoRenderers.back();
	    m_VideoRenderers.pop_back();
	}

	//printf("WX %d   WY %d  WW %d  WH %d\n", wx, wy, ww, wh);
#ifndef X_DISPLAY_MISSING
	if (m_pDpy && GetWidth() > 0 && GetHeight() > 0)
	{
	    bool sub = HasSubtitles();
	    IVideoRenderer* renderer = 0;

	    bool useyuv;
	    Get(USE_YUV, &useyuv, 0);

	    if (useyuv && !renderer)
	    {
		// checking for YUY2 first
		static const fourcc_t tryFcc[] = {
		    //fccYUY2, // looks like YV12 is not supported for Color changes
		    fccYUY2,
		    fccYV12, // fastest
		    fccUYVY,
		    fccI420,
                    // forced
		    //fccYUY2,
		    fccYV12,
		    //fccUYVY,
                    0
		};
		int i = 0;

		while (!renderer && tryFcc[i])
		{
		    try
		    {
			// we always allow to use last entry when HW acceleration
			// is requested - it is giving hw scalable picture
			m_CSP = tryFcc[i];
			if (!tryFcc[i + 1] ||  SetColorSpace(m_CSP, true) == 0)
			{
			    renderer = CreateYUVRenderer(m_pPW, m_pDpy,
							 GetWidth(), GetHeight(),
							 m_CSP, sub);
			    SetColorSpace(m_CSP, false);
			}
		    }
		    catch (FatalError& e)
		    {
			e.PrintAll();
			delete renderer;
			renderer=0;
		    }
		    i++;
		}
	    }

	    if (!renderer)
	    {
		renderer = CreateFullscreenRenderer(m_pPW, m_pDpy, GetWidth(), GetHeight(), sub);
		m_CSP = m_iDepth;
		int result = m_pVideostream->GetVideoDecoder()->SetDestFmt(m_iDepth);
	    }

	    if (renderer)
	    {
		StreamInfo* si = m_pVideostream->GetStreamInfo();
		float a = si->GetAspectRatio();
		if (a > 1.)
		{
		    int w = (int) (GetHeight() * a + 0.5);
                    int h = GetHeight();
		    renderer->Resize(w, h);
		}
                delete si;
		if (pos == 0)
		{
		    renderer->Resize(ww, wh);
                    renderer->SetPosition(wx, wy);
		}
		m_VideoRenderers.push_back(renderer);
		char* fs;
		Get(SUBTITLE_FONT, &fs, 0);
		renderer->SetFont(sub ? fs : 0);
		if (fs) free(fs);
	    }
	    setVideoBuffering();
	    //SetColorSpace(IMG_FMT_I420, false);
	    //m_pVideostream->GetVideoDecoder()->SetDestFmt(IMG_FMT_I420);
	}
#endif
    }
    virtual int setFont(const char* fn)
    {
	if (m_pVideostream)
	{
	    lockThreads("setFont");

	    for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    {
		m_VideoRenderers[i]->SetFont(fn);
	    }

	    // direct renderer for X11 needs to reinitilize surface!
	    if (m_pVideostream->GetVideoDecoder())
		m_pVideostream->GetVideoDecoder()->Restart();

	    unlockThreads();
	}
	return 0;
    }
    static void drawFunc(const CImage*, void* arg=0) {}
    static void killHandler(int, void* arg=0) {}
};


IAviPlayer2* CreateAviPlayer2(IPlayerWidget* parent, void* dpy,
			      const char* filename, const char* subname,
                              unsigned int flags,
			      const char* vcodec, const char* acodec)
{
    AviPlayer2* p = new AviPlayer2(parent, dpy, filename, subname,
                                   flags, vcodec, acodec);
    if (p)
	p->updateResize();
    return p;
}

AVM_END_NAMESPACE;
