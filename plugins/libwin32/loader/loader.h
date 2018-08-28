/********************************************************

	Win32 binary loader interface
	Copyright 2000 Eugene Kuznetsov (divx@euro.ru)
	Shamelessly stolen from Wine project

*********************************************************/

#ifndef _LOADER_H
#define _LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wine/windef.h>
#include <wine/driver.h>
#include <wine/mmreg.h>
#include <wine/vfw.h>
#include <wine/msacm.h>

#define ICSendMessage SendDriverMessage

unsigned int _GetPrivateProfileIntA(const char* appname, const char* keyname, int default_value, const char* filename);
int _GetPrivateProfileStringA(const char* appname, const char* keyname,
	const char* def_val, char* dest, unsigned int len, const char* filename);
int _WritePrivateProfileStringA(const char* appname, const char* keyname,
	const char* string, const char* filename);

/**********************************************
  extra  MS VFW ( Video For Windows ) interface
**********************************************/
long VFWAPIV ICUniversalEx(HIC hic, int command, long dwFlags,
			   LPBITMAPINFOHEADER lpbiFormat,
                           const void* lpData,
			   LPBITMAPINFOHEADER lpbi,
			   void* lpBits);

WIN_BOOL VFWAPI	ICInfo(long fccType, long fccHandler, ICINFO * lpicinfo);

HIC	VFWAPI	ICOpen(long fccType, long fccHandler, UINT wMode);
HIC	VFWAPI	ICOpenFunction(long fccType, long fccHandler, unsigned int wMode, void* lpfnHandler);
HIC	VFWAPI ICLocate(long fccType, long fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, short wFlags);

INT WINAPI LoadStringA( HINSTANCE instance, UINT resource_id,
                            LPSTR buffer, INT buflen );

#define ICGetInfo(hic, picinfo) \
    ICSendMessage(hic, ICM_GETINFO, (long)picinfo, sizeof(*picinfo))

#define ICGetState(hic, pcstate, cb) \
    ICSendMessage(hic, ICM_GETSTATE, (long)pcstate, cb)

#define ICSetState(hic, pcstate, cb) \
    ICSendMessage(hic, ICM_SETSTATE, (long)pcstate, cb)

#define ICCompressFramesInfo(hic, picf) \
    ICSendMessage(hic, ICM_COMPRESS_FRAMES_INFO, (long)picf, sizeof(*picf))


#ifdef __cplusplus
}
#endif
#endif /* __LOADER_H */
