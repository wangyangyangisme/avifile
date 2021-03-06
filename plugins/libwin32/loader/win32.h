#ifndef loader_win32_h
#define loader_win32_h

#include <time.h>

#include <wine/windef.h>
#include <wine/winbase.h>
#include <com.h>

#ifdef AVIFILE
#ifdef __GNUC__
#include "avm_output.h"
#ifndef __cplusplus
#define printf(args...)  avm_printf("Win32 plugin", args )
#endif
#endif
#endif

extern void my_garbagecollection(void);

typedef struct {
    UINT             uDriverSignature;
    HINSTANCE        hDriverModule;
    DRIVERPROC       DriverProc;
    DWORD            dwDriverID;
} DRVR;

typedef DRVR  *PDRVR;
typedef DRVR  *NPDRVR;
typedef DRVR  *LPDRVR;

typedef struct tls_s tls_t;


extern const void* LookupExternal(const char* library, int ordinal);
extern const void* LookupExternalByName(const char* library, const char* name);

#endif
