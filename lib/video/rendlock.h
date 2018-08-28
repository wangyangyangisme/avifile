#ifndef RENDLOCK_H
#define RENDLOCK_H

#include "avm_locker.h"
#include "avm_cpuinfo.h"
#include "renderer.h"
#include "playerwidget.h"
#include "utils.h"

AVM_BEGIN_NAMESPACE;

// use our internal class for our renderers;
class VideoRendererWithLock : public IVideoRenderer
{
public:
    VideoRendererWithLock(int width, int height, bool _subtitles = false)
	:m_w(width), m_h(height), pic_w(width), pic_h(height),
	m_sub(0), m_iFontHeight(0), m_lLastDrawStamp(0), m_pPw(0),
	m_iAuto(1), m_bQuit(false), m_bSubtitles(_subtitles)
    {}
    virtual int Lock() const { return m_Mutex.Lock(); }
    virtual int TryLock() const { return m_Mutex.TryLock(); }
    virtual int Unlock() const { return m_Mutex.Unlock(); }
    virtual const avm::vector<VideoMode>& GetVideoModes() const { return modes; }
    virtual int GetSize(int& width, int& height) const
    {
	width = pic_w;
	height = pic_h;
	return 0;
    }
    virtual int processEvent() = 0;
protected:
    static void* eventThread(void *arg);

    mutable PthreadMutex m_Mutex;
    int m_w, m_h;
    int pic_w, pic_h;
    int m_sub;
    int m_iFontHeight;
    PthreadMutex emutex;
    PthreadCond econd;
    int64_t m_lLastDrawStamp;
    avm::vector<VideoMode> modes;
    PlayerWidget* m_pPw;
    int m_iAuto; // keyboard  autorepeat
    bool m_bQuit;
    bool m_bSubtitles;
};

AVM_END_NAMESPACE;

#endif
