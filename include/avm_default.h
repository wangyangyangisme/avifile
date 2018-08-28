#ifndef AVM_DEFAULT_H
#define AVM_DEFAULT_H

#ifndef	WIN32

#ifdef HAVE_CONFIG_H
#include "config.h"	/* to get the HAVE_xxx_H defines */
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>	/* for int64_t */
#endif

#if WITH_DMALLOCTH
/* #define DMALLOC_DISABLE stdlib is not compilable without this */
#include <stdlib.h>
#include <dmalloc.h>
#endif /* WITH_DMALLOCTH */

#ifndef __WINE_WINDEF_H
typedef long HRESULT;
#endif /* __WINE_WINDEF_H */

#else /* WIN32 */

#define __attribute__()

/* '<Unknown'> has C-linkage specified, but returns UDT 'basic_string<char, struct ...' */
 #pragma warning (disable: 4190)

/* 'std::_Tree(std::basic_string<... ': identifier was truncated to '255' characters in the debug information */
 #pragma warning (disable: 4786)

/* 'unsigned char': forcing value to bool 'true' or 'false' */
 #pragma warning (disable: 4800)
typedef __int64  int64_t;
typedef __int32  int32_t;
typedef __int16  int16_t;
typedef __int8   int8_t;

typedef __uint64 uint64_t;
typedef __uint32 uint32_t;
typedef __uint16 uint16_t;
typedef __uint8  uint8_t;
#endif /* WIN32 */

#define E_ERROR -2

typedef uint32_t fourcc_t;
typedef unsigned int uint_t;    /* use as generic type */
typedef uint_t framepos_t;
typedef uint_t streamid_t;	/* \obsolete use uint_t */
typedef uint_t Unsigned; 	/* \obsolete use uint_t */

#ifdef X_DISPLAY_MISSING
typedef int Display;
#endif

#define AVM_BEGIN_NAMESPACE namespace avm {
#define AVM_END_NAMESPACE   }

/**
 * \namespace avm
 *
 * encupsulates functions & classes from the avifile library
 *
 * \author Zdenek Kabelac (kabi@users.sourceforge.net)
 * \author Eugene Kuznetsov (divx@euro.ru)
 */

#define AVM_COMPATIBLE  /* define & build backward compatible code */

#endif /* AVIFILE_DEFAULT_H */
