#ifndef AUDIOINTRESAMPLER_H
#define AUDIOINTRESAMPLER_H

#include "IAudioResampler.h"

// implementation of the AudioResampler Interface

AVM_BEGIN_NAMESPACE;

template <class T> class AudioIntResamplerStereo : public IAudioResampler
{
public:
    virtual uint_t getBitsPerSample() const;
    virtual void resample(void* out, const void* in, uint_t out_count, uint_t in_count);
};

template <class T> class AudioIntResamplerMono : public IAudioResampler
{
public:
    virtual uint_t getBitsPerSample() const;
    virtual void resample(void* out, const void* in, uint_t out_count, uint_t in_count);
};

AVM_END_NAMESPACE;

#endif
