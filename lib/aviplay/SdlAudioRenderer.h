#ifndef AVIFILE_SDLAUDIORENDERER_H
#define AVIFILE_SDLAUDIORENDERER_H

#include "IAudioRenderer.h"
#include "AudioQueue.h"

AVM_BEGIN_NAMESPACE;

class SdlAudioMix;
class SdlAudioRenderer: public IAudioRenderer
{
public:
    SdlAudioRenderer(IReadStream* as, WAVEFORMATEX& owf, uint_t useFreq = 0)
	:IAudioRenderer(as, owf), m_pAudiomix(0), m_uiUseFreq(useFreq) {}
    virtual ~SdlAudioRenderer();
    virtual int Init();
    virtual int SetVolume(int volume);
protected:
    virtual void pause(int v);
    // C binding - used for SDL callback
    static void fillAudio(void* userdata, unsigned char* stream, int len);

    SdlAudioMix* m_pAudiomix;
    uint_t sdl_systems;
    double m_dSpecTime;
    uint_t m_uiUseFreq;
};

AVM_END_NAMESPACE;

#endif // AVIFILE_SDLAUDIORENDERER_H
