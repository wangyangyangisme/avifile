#ifndef IAUDIORESAMPLER_H
#define IAUDIORESAMPLER_H

//
// Interface class for the resampling routine
//

#include "avm_default.h"

AVM_BEGIN_NAMESPACE;

// using templates for  char, short, int sized samples
class IAudioResampler
{
public:
    virtual ~IAudioResampler() {}
    virtual uint_t getBitsPerSample() const = 0;
    virtual void resample(void* out, const void* in, uint_t out_count, uint_t in_count) = 0;
};

AVM_END_NAMESPACE;

#endif /* IAUDIORESAMPLER_H */
