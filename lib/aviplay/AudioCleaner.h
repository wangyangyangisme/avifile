#ifndef AVIFILE_AUDIOCLEANER_H
#define AVIFILE_AUDIOCLEANER_H

#include "avm_default.h"
#include "formats.h"
#include <string.h>

AVM_BEGIN_NAMESPACE;

class IAudioCleaner
{
protected:
    static const uint_t CLEARED_SAMPLES = 32;
    uint_t m_uiClearSize;
    uint_t m_uiRemains;
public:
    IAudioCleaner(uint_t clear) : m_uiClearSize(clear), m_uiRemains(0) {}
    virtual ~IAudioCleaner() {}
    virtual uint_t soundOff(void* out, const void* in) = 0;
    virtual uint_t soundOn(void* out, uint_t n) = 0;
};

template <class T> class AudioCleanerStereo : public IAudioCleaner
{
public:
    AudioCleanerStereo(uint_t clear) : IAudioCleaner(clear) {}
    uint_t soundOff(void* out, const void* in);
    uint_t soundOn(void* out, uint_t n);
};

template <class T> class AudioCleanerMono : public IAudioCleaner
{
public:
    AudioCleanerMono(uint_t clear) : IAudioCleaner(clear) {}
    uint_t soundOff(void* out, const void* in);
    uint_t soundOn(void* out, uint_t n);
};

IAudioCleaner* CreateAudioCleaner(uint_t channels,
				  uint_t bitsPerSample,
				  uint_t clearsz);

AVM_END_NAMESPACE;

#endif // AVIFILE_AUDIOCLEANER_H
