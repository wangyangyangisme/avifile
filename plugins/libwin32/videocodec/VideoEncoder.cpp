/********************************************************

	Video encoder implementation
	Copyright 2000 Eugene Kuznetsov (divx@euro.ru)

*********************************************************/

#include "wine/windef.h"
#include "wine/winreg.h"
#include "image.h"
#include "avm_fourcc.h"
//#define TIMING
#include "avm_output.h"
//#undef TIMING
#ifdef TIMING
#include "cpuinfo.h"
#endif

#include "VideoEncoder.h"
#include "registry.h"

#if 0
struct ICINFO
{
	long	dwSize;		/* 00: */
	long	fccType;	/* 04:compressor type     'vidc' 'audc' */
	long	fccHandler;	/* 08:compressor sub-type 'rle ' 'jpeg' 'pcm '*/
	long	dwFlags;	/* 0c:flags LOshort is type specific */
	long	dwVersion;	/* 10:version of the driver */
	long	dwVersionICM;	/* 14:version of the ICM used */
	/*
	 * under Win32, the driver always returns UNICODE strings.
	 */
	short	szName[16];		/* 18:short name */
	short	szDescription[128];	/* 38:long name */
	short	szDriver[128];		/* 138:driver that contains compressor*/
           					/* 238: */
/*
   Applicable flags. Zero or more of the following flags can be set:
        VIDCF_COMPRESSFRAMES 
              Driver is requesting to compress all frames. For information about compressing all
              frames, see the ICM_COMPRESS_FRAMES_INFO message. 
        VIDCF_CRUNCH 
              Driver supports compressing to a frame size. 
        VIDCF_DRAW 
              Driver supports drawing. 
        VIDCF_FASTTEMPORALC 
              Driver can perform temporal compression and maintains its own copy of the
              current frame. When compressing a stream of frame data, the driver doesn't need
              image data from the previous frame. 
        VIDCF_FASTTEMPORALD 
              Driver can perform temporal decompression and maintains its own copy of the
              current frame. When decompressing a stream of frame data, the driver doesn't
              need image data from the previous frame. 
        VIDCF_QUALITY 
              Driver supports quality values. 
        VIDCF_TEMPORAL 
              Driver supports inter-frame compression
*/
};
#endif

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

AVM_BEGIN_NAMESPACE;

VideoEncoder::VideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& format)
:IVideoEncoder(info), m_pModule(0), m_HIC(0), m_bh(0),
    m_bhorig(0), m_obh(0),
    m_prev(0), m_pConfigData(0), m_iConfigDataSize(0),
    m_iState(0), m_iBitrate(910000), m_fFps(25.0)
{
    unsigned bihs = (format.biSize < (int) sizeof(BITMAPINFO)) ?
	sizeof(BITMAPINFO) : format.biSize;
    m_bh = (BitmapInfo*) malloc(bihs);
    memcpy(m_bh, &format, bihs);
    m_bhorig = (BitmapInfo*) malloc(bihs);
    memcpy(m_bhorig, &format, bihs);
    m_bh->biHeight = labs(m_bh->biHeight);

    if (m_bhorig->biCompression == BI_RGB ||
	m_bhorig->biCompression == BI_BITFIELDS)
        // only bottom-up RGB images
        m_bhorig->biHeight = labs(m_bhorig->biHeight);

    switch (compressor)
    {
    case fccMP41:
    case fccMP43:
	compressor = fccDIV3;
	break;
    }
    m_comp_id = compressor;
}

int VideoEncoder::init()
{
    m_pModule = control.Create(m_Info);
    if (!m_pModule)
	return -1;

    m_HIC = m_pModule->CreateHandle(m_comp_id, Module::Compress);
    if (!m_HIC)
        return -1;

    int r = ICCompressGetFormat(m_HIC, m_bh, 0);
    if (r < 0)
    {
	AVM_WRITE("VideoEncoder", "Can't handle this format\n");
        return -1;
    }
    //printf("Format size %d\n", r);
    m_obh = (BITMAPINFOHEADER*) malloc(r);
    memset(m_obh, 0, r);
    m_obh->biSize = r;
    r = ICCompressGetFormat(m_HIC, m_bh, m_obh);
    if (r != 0)
    {
	AVM_WRITE("VideoEncoder", "Can't handle this format\n");
        return -1;
    }

    m_obh->biHeight = labs(m_obh->biHeight);
    m_obh->biBitCount = m_bh->biBitCount;

    m_iState = 1;

    // test if it really works
    if (Start() != 0)
    {
	AVM_WRITE("VideoEncoder", "WARNING: CompressBegin failed ( probably unsupported input format  %d )\n", r);
        return -1;
    }
    Stop();

    // Save configuration state.
    //
    // Ordinarily, we wouldn't do this, but there seems to be a bug in
    // the Microsoft MPEG-4 compressor that causes it to reset its
    // configuration data after a compression session.  This occurs
    // in all versions from V1 through V3.
    //
    // Stupid fscking Matrox driver returns -1!!!

    m_iConfigDataSize = ICGetState(m_HIC, 0, 0);

    if (m_iConfigDataSize > 0)
    {
	m_pConfigData = (char*) malloc(m_iConfigDataSize);
	//printf("****************** %d\n", m_iConfigDataSize);
	m_iConfigDataSize = ICGetState(m_HIC, m_pConfigData, m_iConfigDataSize);

	// As odd as this may seem, if this isn't done, then the Indeo5
	// compressor won't allow data rate control until the next
	// compression operation!

	if (m_iConfigDataSize)
	    ICSetState(m_HIC, m_pConfigData, m_iConfigDataSize);
    }
    return 0;
}

VideoEncoder::~VideoEncoder()
{
    if (m_iState != 1)
	Stop();

    if (m_pModule)
	m_pModule->CloseHandle(m_HIC);
    if (m_bh) free(m_bh);
    if (m_bhorig) free(m_bhorig);
    if (m_obh) free(m_obh);
    if (m_prev) free(m_prev);
    if (m_pConfigData) free(m_pConfigData);
}

int VideoEncoder::EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid)
{
    if (m_iState != 2)
	return -1;

#ifdef TIMING
    st1=localcount();
#endif
    CImage* temp = (src->IsFmt(m_bhorig)) ? 0 : new CImage(src, m_bhorig);

    *is_keyframe = 0;
    if (m_iKeyRate && (m_iFrameNum - m_iLastKF) > m_iKeyRate)
    {
	*is_keyframe =  ICCOMPRESS_KEYFRAME; // force KF if too long there wasn't one
	//printf("FORCING KEFRAME  %d  %d\n", m_iFrameNum, m_iLastKF);
    }
    int r = ICCompress(m_HIC, *is_keyframe,
		       m_obh, dest,
		       m_bh, temp ? (void*)temp->Data() : (void*)src->Data(),
		       (long*)lpckid,
		       (long*)is_keyframe,
		       m_iFrameNum, m_iFrameNum ? 0 : 0x7fffffff,
		       m_iQuality,
		       (is_keyframe) ? 0 : m_bh,
		       (is_keyframe) ? 0 : m_prev);
    printf("==> hr:%d  rest:%d  fnum:%d 0x%x\n", r, m_iFrameNum%m_iKeyRate, m_iFrameNum, *is_keyframe);
    if (temp)
	temp->Release();
    *is_keyframe &= 0x10;
    if (*is_keyframe)
	m_iLastKF = m_iFrameNum;

    if (r == 0)
    {
	//if(m_prev==0)m_prev=new char[m_bh->biSizeImage];
	//memcpy(m_prev, src, m_bh->biSizeImage);
	if (!m_prev)
	    m_prev = (char*) malloc(ICCompressGetSize(m_HIC, m_bh, m_obh));
	memcpy(m_prev, dest, m_obh->biSizeImage);

#if 0
	// If we're using a compressor with a stupid algorithm (Microsoft Video 1),
	// we have to decompress the frame again to compress the next one....

	if (res==ICERR_OK && pPrevBuffer && (!lKeyRate || lKeyRateCounter>1)) {
	    res = ICDecompress(hic, dwFlags & AVIIF_KEYFRAME ? 0 : ICDECOMPRESS_NOTKEYFRAME,
			       (LPBITMAPINFOHEADER)pbiOutput,
			       pOutputBuffer,
			       (LPBITMAPINFOHEADER)pbiInput,
			       pPrevBuffer);
	}
#endif
    }
    m_iFrameNum++;
    if (size)
	*size = m_obh->biSizeImage;
    return r;
}

int VideoEncoder::Start()
{
    if (m_iState != 1)
	return -1;//wrong state
    //HRESULT hr=ICCompressBegin(hic, m_bh, m_obh);

    ICINFO ici;

    int r = ICGetInfo(m_HIC, &ici);
    if (!r)
    {
        AVM_WRITE("Win32 video encoder", "Unable to retrieve video compressor info!");
    }

    if (!(ici.dwFlags & VIDCF_QUALITY))
	m_iQuality = 0;
    else
	ICGetDefaultQuality(m_HIC, &m_iQuality);
    if (ICGetDefaultKeyFrameRate(m_HIC, &m_iKeyRate) != 0)
	m_iKeyRate = 0xFFFF;
    printf("KEYRATE %d\n", m_iKeyRate);
    m_iKeyRate = 100;

#if 0
    AVM_WRITE("Win32 video encoder", "ICINFO  dwSize:%ld  fccType:%.4s  fccHandler:%.4s\n"
	   "  dwFlags:0x%lx  version:%d versionICM:%d\n",
	   //"  name:%s  descrition:%s  driver:%s\n",
	   ici.dwSize, (char*)&ici.fccType, (char*)&ici.fccHandler,
	   ici.dwFlags, ici.dwVersion, ici.dwVersionICM
	   //,ici.szName, ici.szDescription, ici.szDriver
	  );
#endif
    if (ici.dwFlags & VIDCF_TEMPORAL) {
	if (!(ici.dwFlags & VIDCF_FASTTEMPORALC)) {
	    // Allocate backbuffer
	    //throw FATAL("BACKBUFFER needed - implement me!!");
	    //if (!(pPrevBuffer = new char[pbiInput->bmiHeader.biSizeImage]))
	    //    throw MyMemoryError();
	}
    }

    if (m_bh->biSizeImage == 0)
	m_bh->biSizeImage = m_bh->biWidth*labs(m_bh->biHeight)*((m_bh->biBitCount+7)/8);
    // Make sure that we can work with this format!

    AVM_WRITE("Win32 video encoder", "W32 Quality %d  KeyFrames: %d   BitRate: %d\n", m_iQuality, m_iKeyRate, m_iBitrate);

    setDivXRegs();

    ICCOMPRESSFRAMES icf;
    memset(&icf, 0, sizeof icf);
    //	ICM_COMPRESS_FRAMES_INFO:
    //
    //		dwFlags	      Trashed with address of lKeyRate in tests. Something
    //			      	might be looking for a non-zero value here, so better
    //			      	set it.
    //		lpbiOutput    NULL.
    //		lOutput	      0.
    //		lpbiInput     NULL.
    //		lInput	      0. (reserved)
    //		lStartFrame   0.
    //		lFrameCount   Number of frames.
    //		lQuality      Set to quality factor, or zero if not supported.
    //		lDataRate     Set to data rate in 1024*kilobytes, or zero if not
    //			      	supported.
    //		lKeyRate      Set to the desired maximum keyframe interval.  For
    //			      	all keyframes, set to 1.

    icf.dwFlags		= (long)&icf.lKeyRate;
    icf.lStartFrame	= 0;
    icf.lFrameCount	= 0x0FFFFFFF;
    icf.lQuality	= m_iQuality;
    icf.lDataRate	= m_iBitrate * 1000 / 1024;
    icf.lKeyRate	= (m_iKeyRate > 0) ? 0 * m_iKeyRate : 0;
    icf.dwRate		= 1000000;
    icf.dwScale		= (int) (icf.dwRate / m_fFps);
    //printf("SETTING BITRATE  B:%ld   K:%ld   Q:%d   %ld / %f    s:%ld\n", icf.lDataRate, m_iKeyRate, m_iQuality, icf.dwRate, m_fFps, icf.dwScale);

    ICCompressFramesInfo(m_HIC, &icf);

    r = ICCompressBegin(m_HIC, m_bh, m_obh);
    if (r != 0)
    {
	AVM_WRITE("Win32 video encoder", "ICCompressBegin() failed ( shouldn't happen ), error code %d\n", (int)r);
        return -1;
    }

    m_iLastKF = m_iFrameNum = 0;
    m_iState = 2;
    //AVM_WRITE("Win32 video encoder", "START done\n");
    return 0;
}

int VideoEncoder::Stop()
{
    if (m_iState == 2)
    {
	//AVM_WRITE("Win32 video encoder", "STOP\n");
	int r = ICCompressEnd(m_HIC);
	if (r != 0)
	    AVM_WRITE("Win32 video encoder", "ICCompressEnd() failed ( shouldn't happen ), error code %d\n", (int)r);
	m_iState = 1;

	// Reset MPEG-4 compressor

	if (m_pConfigData && m_iConfigDataSize)
	    ICSetState(m_HIC, m_pConfigData, m_iConfigDataSize);
    }
    return 0;
}

const BITMAPINFOHEADER& VideoEncoder::GetOutputFormat() const
{
    return *m_obh; // maybe this is correct
}

int VideoEncoder::GetOutputSize() const
{
    int lMaxPackedSize = ICCompressGetSize(m_HIC, m_bh, m_obh);
    // Work around a bug in Huffyuv.  Ben tried to save some memory
    // and specified a "near-worst-case" bound in the codec instead
    // of the actual worst case bound.  Unfortunately, it's actually
    // not that hard to exceed the m_HIC's estimate with noisy
    // captures -- the most common way is accidentally capturing
    // static from a non-existent channel.
    //
    // According to the 2.1.1 comments, Huffyuv uses worst-case
    // values of 24-bpp for YUY2/UYVY and 40-bpp for RGB, while the
    // actual worst case values are 43 and 51.  We'll compute the
    // 43/51 value, and use the higher of the two.

    if (m_Info.fourcc == fccHFYU) {
	int lRealMaxPackedSize = m_obh->biWidth * m_obh->biHeight;

	if (m_bh->biCompression == 0)//BI_RGB
	    lRealMaxPackedSize = (lRealMaxPackedSize * 51) >> 3;
	else
	    lRealMaxPackedSize = (lRealMaxPackedSize * 43) >> 3;

	if (lRealMaxPackedSize > lMaxPackedSize)
	    lMaxPackedSize = lRealMaxPackedSize;

    }
    //printf("VideoEncoder::GetOutputSize() %d\n", lMaxPackedSize);
    return lMaxPackedSize;
}


int VideoEncoder::SetQuality(int quality)
{
    int newkey;
    if (quality < 0 || quality >10000)
	return -1;

    m_iQuality = quality;
    //printf("SETQUALITY %d\n", m_iQuality);
#if 0
    if (RegCreateKeyExA(HKEY_CURRENT_USER,
			"SOFTWARE\\Microsoft\\Scrunch\\Video",
			0, 0, 0, 0, 0,
			&newkey, 0) == 0)
    {
	RegSetValueExA(newkey, "Quality", 0, REG_DWORD, &quality, 4);
    }
#endif
    return 0;
}

int VideoEncoder::SetKeyFrame(int freq)
{
    if (freq <= 0)
        return -1;

    m_iKeyRate = freq;
    //printf("SETKeyFrame %d\n", m_iKeyRate);
    return 0;
}

int VideoEncoder::SetFps(float fps)
{
    if (fps <= 0. || fps > 100.)
        return -1;
    m_fFps = fps;

    return 0;
}

void VideoEncoder::setDivXRegs(void)
{
    const char* keyname;
    const char* name = (const char*) m_Info.dll;
    int newkey;
    int bitrate;
    int keyframes = 100;
    int crispness = 100;
    int count = 4;

    if (strcmp(name, "divxc32.dll") == 0)
	keyname = "Software\\LinuxLoader\\div3";
    else if (strcmp(name, "divxc32f.dll") == 0)
	keyname = "Software\\LinuxLoader\\div4";
    else if (strcmp(name, "divxcvki.dll") == 0)
	keyname = "Software\\LinuxLoader\\div5";
    else if (strcmp(name, "divxcfvk.dll") == 0)
	keyname = "Software\\LinuxLoader\\div6";
    else
        return;

    printf("KEYNAME %s  %s\n", name, keyname);

    if (RegOpenKeyExA(HKEY_CURRENT_USER, keyname, 0, 0, &newkey) == 0)
    {
	// location in original win32 codec
	//The Bit Rate at AC6
	//The Quality/Cripness at 1EA8
	//The Keyframes at 1EAF
	//printf("sethandle %s\n", keyname);
        char* handle = (char*) m_pModule->GetLibHandle();
	if (RegQueryValueExA(newkey, "BitRate", 0, 0, &m_iBitrate, &count) == 0)
	{
	    if (handle)
	    {
		double d = *(double*)(handle + 0x14c0);
		*(double*)(handle + 0x14c0) = m_iBitrate;
		AVM_WRITE("Win32 video encoder", "BitRate %d  (old: %d)\n", m_iBitrate, (int) d);
	    }
	    bitrate = m_iBitrate;
            m_iBitrate *= 1000;
	}
	else
	    AVM_WRITE("Win32 video encoder", "No 'BitRate' value present\n");
	if (RegQueryValueExA(newkey, "Crispness", 0, 0, &crispness, &count) == 0)
	{
	    if (handle)
	    {
		int a = *(char*)(handle + 0x28a8); //
		*(long*)(handle + 0x28a8) = crispness;
	    }
	    //printf("Crispness %d  (%d)\n", crispness, a);
	}
	if (RegQueryValueExA(newkey, "KeyFrames", 0, 0, &keyframes, &count) == 0)
	{
	    if (handle)
	    {
		int a = *(char*)(handle + 0x28af); //0x28af
		*(long*)(handle + 0x28af) = keyframes;
		printf("KeyFrames %d   (%d)\n", keyframes, a);
	    }
	    m_iKeyRate = keyframes;
	}
	RegCloseKey(newkey);
    }
    else
	AVM_WRITE("Win32 video encoder", "Could not open key %s\n", keyname);

#if 0
    if (RegCreateKeyExA(HKEY_CURRENT_USER,
			"SOFTWARE\\Microsoft\\Scrunch\\Video",
			0, 0, 0, 0, 0,
			&newkey, 0) == 0)
    {
	RegSetValueExA(newkey, "BitRate", 0, REG_DWORD, &bitrate, 4);
	RegSetValueExA(newkey, "KeyFrames", 0, REG_DWORD, &keyframes, 4);
	//RegSetValueExA(newkey, "Quality", 0, REG_DWORD, &crispness, 4);
    }
    RegCloseKey(newkey);
#endif
}

AVM_END_NAMESPACE;
