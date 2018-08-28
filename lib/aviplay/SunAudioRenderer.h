#ifndef AVIFILE_SUNAUDIORENDERER_H
#define AVIFILE_SUNAUDIORENDERER_H

#include "IAudioRenderer.h"
#include "AudioQueue.h"

#ifdef HAVE_SUNAUDIO

AVM_BEGIN_NAMESPACE;

class SunAudioMix : public IAudioMix
{
    int m_iFd;
public:
    SunAudioMix(int fd) : m_iFd(fd) {}
    virtual int Mix(void* data, const void* src, uint_t n) const;
};


class SunAudioRenderer: public IAudioRenderer
{
public:
    SunAudioRenderer(IReadStream* stream, WAVEFORMATEX& Owf, const char* cname = 0);
    ~SunAudioRenderer();
    virtual int SetVolume(int volume);
				//restart thread if it waits
protected:
    virtual double getRendererBufferTime() const;

    virtual int reset();
    virtual void pause(int state);
    static void* doAudioOut(void* arg);

    int m_iAudioFd;		// /dev/audio file descriptor
    PthreadTask* m_pAudioThread;// writes data to audio_fd
    SunAudioMix* m_pAudioMix;
    uint_t m_iSamplesSent;	// number of samples written to device
    uint_t m_uiWriteSize;
};

AVM_END_NAMESPACE;

#endif // HAVE_SUNAUDIO
#endif // AVIFILE_SUNAUDIORENDERER_H
