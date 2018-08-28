#ifndef AVIFILE_COPYAUDIORENDERER_H
#define AVIFILE_COPYAUDIORENDERER_H

#include "IAudioRenderer.h"
#include "AudioQueue.h"

AVM_BEGIN_NAMESPACE;

class CopyAudioMix : public IAudioMix
{
    AUDIOFUNC m_pAf;
    void* m_pArg;
public:
    CopyAudioMix(AUDIOFUNC func, void* arg) : m_pAf(func), m_pArg(arg) {}
    virtual int Mix(void* data, const void* src, uint_t n) const;
};

class CopyAudioRenderer: public IAudioRenderer
{
public:
    CopyAudioRenderer(IReadStream* stream, WAVEFORMATEX& Owf, AUDIOFUNC func, void* arg);
    ~CopyAudioRenderer() { delete m_pAudioMix; }
    virtual int Extract();
protected:
    CopyAudioMix* m_pAudioMix;
};

AVM_END_NAMESPACE;

#endif //AVIFILE_COPYAUDIORENDERER_H
