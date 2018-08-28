#ifndef AVIFILE_AVM_CPUINFO_H
#define AVIFILE_AVM_CPUINFO_H

#include "avm_default.h"

AVM_BEGIN_NAMESPACE;

class CPU_Info
{
    double freq;
    bool have_tsc;
    bool have_mmx;
    bool have_mmxext;
    bool have_sse;
public:
    CPU_Info() {}
    void Init();
    /**
     * Returns nonzero if the processor supports MMX instruction set.
     */
    bool HaveMMX() const {return have_mmx;}
    /**
     * Returns nonzero if the processor supports extended integer MMX instruction set
     ( Pentium-III, AMD Athlon and compatibles )
     */
    bool HaveMMXEXT() const {return have_mmxext;}
    /**
     * Returns nonzero if the processor supports 'SSE' floating-point SIMD instruction set
     ( Pentium-III and compatibles )
     */
    bool HaveSSE() const {return have_sse;}
    /**
     * Returns nonzero if the processor has time-stamp counter feature.
     */
    bool HaveTSC() const {return have_tsc;}
    /**
     * Returns processor frequency in kHz.
     */
    operator double() const {return freq;}
};

AVM_END_NAMESPACE;

extern avm::CPU_Info freq;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline int avm_is_mmx_state()
{
#ifdef ARCH_X86
    if (freq.HaveMMX())
    {
	unsigned short tagword = 0;
	char b[28];
	__asm__ __volatile__ ("fnstenv (%0)\n\t" : : "r"(&b));
	tagword = *(unsigned short*) (b + 8);
	return (tagword != 0xffff);
    }
#endif
    return 0;
}

/**
 *  Returns duration of time interval between two timestamps, received
 *  with longcount().
 */
static inline float to_float(int64_t tend, int64_t tbegin)
{
    return float((tend - tbegin) / (double)freq / 1000.);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // AVIFILE_AVM_CPUINFO_H
