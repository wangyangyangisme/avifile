#include "avm_fourcc.h"
#include "avm_cpuinfo.h"
#include "avm_except.h"
#include "avm_creators.h"
#include "avm_output.h"
#include "utils.h"

#include <unistd.h> // geteuid
#include <string.h> // memcpy
#include <stdlib.h> // getenv, free()
#include <stdio.h>

#include "fullscreen_renderer.h"


AVM_BEGIN_NAMESPACE;

#define __MODULE__ "AvicapRenderer"

AvicapRenderer::AvicapRenderer(void *dpy){
  m_pPW=this;
  m_pDpy=dpy;
  //  m_bQuit=false;

  zx = zy = zw = zh = 0;
  
}

void AvicapRenderer::setSize(int w,int h){
  m_iWidth=w;
  m_iHeight=h;
}

AvicapRenderer::~AvicapRenderer(){

   while (m_VideoRenderers.size() > 0)
    {
	delete m_VideoRenderers.back();
	m_VideoRenderers.pop_back();
    }


}

void AvicapRenderer::setCaption(avm::string capstr){
  for(uint i=0;i<m_VideoRenderers.size();i++){
    m_VideoRenderers[i]->SetCaption(capstr.c_str(),NULL);
  }
}

void AvicapRenderer::setRecordingColorSpace(Colorspaces colorspace)
{
  recording_csp=colorspace;
}

bool AvicapRenderer::checkColorSpace(fourcc_t csp)
{
  if(csp==fccYUY2 && recording_csp==cspYUY2){
    return true;
  }
  else if(csp==fccYV12 && recording_csp==cspYV12){
    return true;
  }

  return false;
}

// used when we are in pause mode or the player is not playing
int AvicapRenderer::drawFrame(CImage *im)
{
#if 1
  //  printf("c1\n");
    if (true)
    {
      //printf("c2\n");
      //CImage* im = m_pVideostream->GetFrame(true); // ReadFrame
      //	m_fLastDiff = 0.0;
	//	setQuality();
	//	  printf("c2.1\n");
	if (im)
	{
	  //  printf("c3\n");
	  //const subtitle_line_t* sl = GetCurrentSubtitles();
	    for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    {
	      //	       printf("drawing on vr %d\n",i);
		m_VideoRenderers[i]->Draw(im);
		//if (HasSubtitles())
		//    m_VideoRenderers[i]->DrawSubtitles(sl);

		m_VideoRenderers[i]->Sync();
	    }
	    //printf("c4\n");
	  //    m_Quality.insert(im->GetQuality() * 100.0);
	    im->Release();
	    //	    m_iFramesVideo++;
	}
	return 0;
    }
#endif
    return -1;
}


//from aviplay2

    int AvicapRenderer::Refresh()
    {
      printf("avicaprenderer-refresh\n");
      for (unsigned i = 0; i < m_VideoRenderers.size(); i++){
	  printf("avicaprenderer-refresh %d\n",i);
	    m_VideoRenderers[i]->Refresh();
      }
	return 0;
    }

    // should be redesigned - only first one will get resize event for now
    int AvicapRenderer::Resize(int& new_w, int& new_h)
    {
      printf("resize %d %d\n",new_w,new_h);
	if (m_VideoRenderers.size() > 0)
	{
	    bool pres;
	    //Get(VIDEO_PRESERVE_ASPECT, &pres, 0);
	    pres=false;
	    if (pres)
	    {
	      //StreamInfo* si = m_pVideostream->GetStreamInfo();
	      //float a = si->GetAspectRatio();
	      float a=0.0;
	      //delete si;
		if (a == 0)
                    a = GetWidth() / (float) GetHeight();
		new_w = (int) (new_h * a + 0.5);
		if (zw)
		    new_w = new_h * zw / zh;
	    }
	    printf("new w/h %d %d\n",new_w,new_h);
	    return m_VideoRenderers.front()->Resize(new_w, new_h);
	}
	return 0;
    }

    int AvicapRenderer::Zoom(int x, int y, int w, int h)
    {
	zx = zy = zw = zh = 0;
	for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    if (m_VideoRenderers[i]->Zoom(x, y, w, h) == 0)
	    {
		zx = x; zy = y; zw = w; zh = h;
	    }
        return 0;
    }

   int AvicapRenderer::ToggleFullscreen(bool maximize)
     //int AvicapRenderer::ToggleFullscreen(bool maximize=false)
    {
	for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    if (m_VideoRenderers[i]->ToggleFullscreen(maximize) != 0)
		return -1;
	return 0;
    }

#if 0
    void AvicapRenderer::updateResize()
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
#endif
void AvicapRenderer::createVideoRenderer()
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

	printf("WX %d   WY %d  WW %d  WH %d\n", wx, wy, ww, wh);
#ifndef X_DISPLAY_MISSING
	if (m_pDpy && GetWidth() > 0 && GetHeight() > 0)
	{
	  bool sub = false;
	    IVideoRenderer* renderer = 0;

	    bool useyuv;
	    //  Get(USE_YUV, &useyuv, 0);

	    useyuv=my_useyuv;

	    if (useyuv && !renderer)
	    {
		// checking for YUY2 first
		static const fourcc_t tryFcc[] = {
		    fccYUY2, // looks like YV12 is not supported for Color changes
		    //fccYUY2,
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
			if (!tryFcc[i + 1] ||  checkColorSpace(m_CSP) == true)
			{
			  printf("yuvrenderer %d\n",m_CSP);
			    renderer = CreateYUVRenderer(m_pPW, m_pDpy,
							 GetWidth(), GetHeight(),
							 m_CSP, sub);
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
	      printf("creating rgb renderer\n");
		renderer = CreateFullscreenRenderer(m_pPW, m_pDpy, GetWidth(), GetHeight(), sub);
		//m_CSP = m_iDepth;
		//int result = m_pVideostream->GetVideoDecoder()->SetDestFmt(m_iDepth);
	    }

	    if (renderer)
	    {
	      //StreamInfo* si = m_pVideostream->GetStreamInfo();
	      //float a = si->GetAspectRatio();
	      float a = 0.1;
		if (a > 1.)
		{
		    int w = (int) (GetHeight() * a + 0.5);
                    int h = GetHeight();
		    renderer->Resize(w, h);
		}
                //delete si;
#if 0
		if (pos == 0)
		{
		    renderer->Resize(ww, wh);
                    renderer->SetPosition(wx, wy);
		}
#endif
		renderer->SetCaption("avicap: unknown",NULL);

		m_VideoRenderers.push_back(renderer);
		printf("pushback renderer\n");
		//char* fs;
		//Get(SUBTITLE_FONT, &fs, 0);
		//renderer->SetFont(sub ? fs : 0);
		//if (fs) free(fs);
	    }
	    //setVideoBuffering();
	    //SetColorSpace(IMG_FMT_I420, false);
	    //m_pVideostream->GetVideoDecoder()->SetDestFmt(IMG_FMT_I420);
	}
#endif
    }

AVM_END_NAMESPACE;


