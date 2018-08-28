#ifndef AVIFILE_OSSAUDIORENDERER_H
#define AVIFILE_OSSAUDIORENDERER_H

#include "IAudioRenderer.h"
#include "AudioQueue.h"

AVM_BEGIN_NAMESPACE;

class OssAudioMix : public IAudioMix
{
public:
    int m_iFd;
    OssAudioMix() : m_iFd(-1) {}
    virtual int Mix(void* data, const void* src, uint_t n) const;
};

class OssAudioRenderer: public IAudioRenderer
{
    static const int OSS_VOL_MAX = 100;
    enum Act { GET, SET };
public:
    OssAudioRenderer(IReadStream* as, WAVEFORMATEX& Owf, uint_t useFreq = 0)
	:IAudioRenderer(as, Owf), m_pAudioThread(0),
	m_iAudioFd(-1), m_iMixFd(-1), m_uiUseFreq(useFreq), m_iDelayMethod(2) {}
    ~OssAudioRenderer();
    virtual int Init();
    virtual int GetBalance();
    virtual int GetVolume();
    virtual int SetBalance(int balance);
    virtual int SetVolume(int volume);
protected:
    // length of sound in OSS buffer
    virtual double getRendererBufferTime() const;
    virtual int reset();

    int mixer(Act a);
    static void* doAudioOut(void* arg);

    PthreadTask* m_pAudioThread; // Writes data to audio_fd
    OssAudioMix m_AudioMix;
    int m_iAudioFd;		// /dev/dsp or ESD socket ( syntactically the same behaviour )
    int m_iMixFd;		// /dev/mixer
    uint_t m_iSndLimit;		// DMA buffer size
    uint_t m_uiWriteSize;
    uint_t m_uiUseFreq;
    mutable int m_iDelayMethod;
};

AVM_END_NAMESPACE;

#endif // AVIFILE_OSSAUDIORENDERER_H
