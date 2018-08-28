#ifndef AUDIOFPHQRESAMPLER_H
#define AUDIOFPHQRESAMPLER_H

#include "IAudioResampler.h"


AVM_BEGIN_NAMESPACE;

IAudioResampler* CreateHQResampler(uint_t channels, uint_t bitsPerSample);

AVM_END_NAMESPACE;

#endif // AUDIOFPHQRESAMPLER_H
