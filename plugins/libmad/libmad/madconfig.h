#ifndef LIBMAD_MADCONFIG_H
#define LIBMAD_MADCONFIG_H

/* optimize for speed at this moment */
#define OPT_SPEED
/* #define OPT_ACCURACY */

#ifdef ARCH_X86
#define FPM_INTEL
#elif ARCH_X86_64
#define FPM_64BIT
#elif ARCH_POWERPC
#define FPM_PPC
#elif ARCH_SPARC
#define FPM_SPARC
#elif ARCH_MIPS
#define FPM_MIPS
#endif

#endif /* LIBMAD_MADCONFIG_H */
