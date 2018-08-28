#ifndef AVICAP_RENDERER_H
#define AVICAP_RENDERER_H

#include "avm_stl.h"
#include "image.h"
#include "subtitle.h"
#include "renderer.h"
#include "playerwidget.h"
#include <videodecoder.h>
#include <aviplay.h>
#include <avifile.h>
#include <avm_creators.h>

#include "capproc.h"
#include "v4lwindow.h"

class V4LWindow;

AVM_BEGIN_NAMESPACE;

class AvicapRenderer : public IPlayerWidget
{
    IPlayerWidget* m_pPW;
    void* m_pDpy;
    //int m_iResizeCount;
    int zx, zy, zw, zh;
    Colorspaces recording_csp;
    V4LWindow *v4lw;
    bool my_useyuv;
    int m_iWidth,m_iHeight;
    avm::vector<IVideoRenderer*> m_VideoRenderers; // we could draw image to more places
    int GetWidth() const {return m_iWidth;}
    int GetHeight() const {return m_iHeight;}
    uint_t m_CSP;               // used colorspace by renderer

public:
    AvicapRenderer(void *dpy);
    virtual ~AvicapRenderer();
    virtual void createVideoRenderer();
    virtual int Refresh();
    virtual int Resize(int& new_w, int& new_h);
    virtual int Zoom(int x, int y, int w, int h);
    virtual void PW_resize(int w, int h) { Resize(w,h); } // resize event
    virtual void PW_menu_slot() { v4lw->avicap_renderer_popup(); }
    virtual void PW_quit_func() { v4lw->avicap_renderer_close(); }
    virtual void PW_key_func(int sym, int mod) { v4lw->avicap_renderer_keypress(sym,mod); }

    virtual int ToggleFullscreen(bool maximize=false);
    void updateResize();
    bool checkColorSpace(fourcc_t csp);
    int drawFrame(avm::CImage* im);
    void setSize(int w,int h);
    void setV4LWindow(V4LWindow *v4lwindow) { v4lw=v4lwindow; };
    void setYUVrendering(bool mode) { my_useyuv=mode; };

    //  void PopupMenu();

    void setRecordingColorSpace(Colorspaces colorspace);

    void setCaption(avm::string capstr);
};

AVM_END_NAMESPACE;

#endif  // AVICAP_RENDERER_H
