#include "config.h"

#include "loader.h"
#include "driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#define STORE_ALL
#define REST_ALL

HIC ICOpen(int compressor, unsigned int mode) /// >0 compress  0 == decomp
{
    ICOPEN icopen;
    icopen.fccType    = mmioFOURCC('v', 'i', 'd', 'c');
    icopen.fccHandler = compressor;
    icopen.dwSize     = sizeof(ICOPEN);
    icopen.dwFlags    = mode ? ICMODE_COMPRESS : ICMODE_DECOMPRESS;

    driver.dwDriverID = ++_refcount;
    DRVR* hDriver = new DRVR(driver);
    //printf("Creating new handle for %s _refcount %d, driver %d\n", Name(), _refcount,(int)hDriver);
    //printf("fcc: 0x%lx,  handler: 0x%lx  (%.4s)\n", icopen.fccType, icopen.fccHandler, (char*) &icopen.fccHandler);

    STORE_ALL;
    hDriver->dwDriverID = driver.DriverProc((int)_refcount, (int)hDriver, DRV_OPEN,
					    0, (int) &icopen);
    REST_ALL;

    if (!hDriver->dwDriverID)
    {
	AVM_WRITE("Win32 loader", "WARNING DRV_OPEN failed (0x%lx)\n", icopen.fccHandler);
        return 0;
    }

    return (HIC) hDriver;
}

int ICClose(HIC hic)
{
    WINE_HIC *whic = (WINE_HIC*)hic;
    /* FIXME: correct? */
    //	CloseDriver(whic->hdrv,0,0);
    DrvClose(whic->hdrv);
    //#warning FIXME: DrvClose
    free(whic);
    return 0;
}

int Module::CloseHandle(int handle)
{
    DRVR* hDriver = (DRVR*) handle;

    Setup_FS_Segment();
    //printf("CLOSE HANDLE %s\n", Name());
    STORE_ALL;
    driver.DriverProc(hDriver->dwDriverID, (int)hDriver, DRV_CLOSE, 0, 0);
    REST_ALL;
    //printf("decrefoucount %d\n", _refcount);
    _refcount--;
    if (_refcount==0)
	delete this;
    delete hDriver;

    return 0;
}
#endif


long VFWAPIV ICCompress(HIC hic,long dwFlags,LPBITMAPINFOHEADER lpbiOutput,
			void* lpData,  LPBITMAPINFOHEADER lpbiInput,
			void* lpBits, long* lpckid, long* lpdwFlags,
			long lFrameNum,long dwFrameSize,long dwQuality,
			LPBITMAPINFOHEADER lpbiPrev,void* lpPrev)
{
    //    dwFlags		Equal to ICCOMPRESS_KEYFRAME if a keyframe is
    //    			required, and zero otherwise.
    //    lpckid		Always points to zero.
    //    lpdwFlags		Points to AVIIF_KEYFRAME if a keyframe is required,
    //    			and zero otherwise.
    //    lFrameNum		Ascending from zero.
    //    dwFrameSize	Always set to 7FFFFFFF (Win9x) or 00FFFFFF (WinNT)
    //    			for first frame.  Set to zero for subsequent frames
    //    			if data rate control is not active or not supported,
    //    			and to the desired frame size in bytes if it is.
    //    dwQuality		Set to quality factor from 0-10000 if quality is
    //    			supported.  Otherwise, it is zero.
    //    lpbiPrev		Set to NULL if not required.
    //    lpPrev		Set to NULL if not required.
    ICCOMPRESS iccmp;

    iccmp.dwFlags		= dwFlags;

    iccmp.lpbiOutput		= lpbiOutput;
    iccmp.lpOutput		= lpData;
    iccmp.lpbiInput		= lpbiInput;
    iccmp.lpInput		= lpBits;

    iccmp.lpckid		= lpckid;
    iccmp.lpdwFlags		= lpdwFlags;
    iccmp.lFrameNum		= lFrameNum;
    iccmp.dwFrameSize		= dwFrameSize;
    iccmp.dwQuality		= dwQuality;
    iccmp.lpbiPrev		= lpbiPrev;
    iccmp.lpPrev		= lpPrev;
    return ICSendMessage(hic, ICM_COMPRESS, (long)&iccmp, sizeof(iccmp));
}

long VFWAPIV ICDecompress(HIC hic, long dwFlags, LPBITMAPINFOHEADER lpbiFormat,
			  void* lpData, LPBITMAPINFOHEADER lpbi, void* lpBits)
{
    ICDECOMPRESS icd;

    icd.dwFlags		= dwFlags;
    icd.lpbiInput	= lpbiFormat;
    icd.lpInput		= lpData;

    icd.lpbiOutput	= lpbi;
    icd.lpOutput	= lpBits;
    icd.ckid		= 0;
    return ICSendMessage(hic, ICM_DECOMPRESS, (long)&icd, sizeof(icd));
}

long VFWAPIV ICUniversalEx(HIC hic, int command, long dwFlags,
			   LPBITMAPINFOHEADER lpbiFormat,
                           const void* lpData,
			   LPBITMAPINFOHEADER lpbi,
			   void* lpBits)
{
    ICDECOMPRESSEX icd;

    icd.dwFlags		= dwFlags;

    icd.lpbiSrc		= lpbiFormat;
    icd.lpSrc		= lpData;

    icd.lpbiDst		= lpbi;
    icd.lpDst		= lpBits;

    icd.xSrc = icd.ySrc = 0;
    icd.dxSrc = lpbiFormat->biWidth;
    icd.dySrc = labs(lpbiFormat->biHeight);

    icd.xDst = icd.yDst = 0;
    icd.dxDst = lpbi->biWidth;
    icd.dyDst = labs(lpbi->biHeight);

    return ICSendMessage(hic, command, (long)&icd, sizeof(icd));
}
