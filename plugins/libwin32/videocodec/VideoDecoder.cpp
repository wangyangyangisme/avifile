/********************************************************

	Video decoder implementation
	Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)

*********************************************************/

#include "wine/windef.h"

#include "avm_fourcc.h"
#include "avm_cpuinfo.h"
#include "avm_output.h"

#include "VideoDecoder.h"
#include "VideoEncoder.h"
#include "dshow/DS_Filter.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h> // labs
#include <stdio.h>

#define ICDECOMPRESS_HURRYUP		0x80000000	/* don't draw just buffer (hurry up!) */
#define ICDECOMPRESS_UPDATE		0x40000000	/* don't draw just update screen */
#define ICDECOMPRESS_PREROL		0x20000000	/* this frame is before real start */
#define ICDECOMPRESS_NOTKEYFRAME	0x08000000	/* this frame is not a key frame */

AVM_BEGIN_NAMESPACE;

VideoDecoder::VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
    :IVideoDecoder(info, bh), m_pModule(0), m_HIC(0), m_iStatus(0),
    m_pLastImage(0), m_bitrick(0), m_divx_trick(false),
    m_bUseEx((info.kind == CodecInfo::Win32Ex)),
    m_bFlip(flip)
{
}

int VideoDecoder::init()
{
    /* On2 Truemotion VP3.x support */
    ICINFO ici;
//
// MPEG-4 format has changed during development. Package
// binaries.zip includes mpg4c32.dll - old MPEG-4 codec
// with unlocked compression - and divxc32.dll,
// which differs from current MPEG-4 only by FOURCC's.
// The problem is to identify which is needed to us,
// because 'old' codec doesn't work with 'new' clips and
// vice versa. Unfortunately, they may simulate correct
// work until the moment of actual decompression. Then,
// you'll receive garbled picture and lots of warnings about
// hr=-100.
//

// I've got old video with fccHandler mp43 and biCompression MPG4.
// Dirk Vornheider sent me old video with both values MP42.
    //AVM_WRITE("Win32 video decoder", "BI %d  %.4s\n", m_pFormat->biBitCount, (char*)&m_pFormat->biBitCount);
    m_pModule = control.Create(m_Info);
    if (!m_pModule)
    {
	AVM_WRITE("Win32 video decoder", "Can't create module\n");
        return -1;
    }

    m_HIC = m_pModule->CreateHandle(m_pFormat->biCompression, Module::Decompress);
    if (!m_HIC)
    {
	AVM_WRITE("Win32 video decoder", "ICOpen failed\n");
        return -1;
    }

    int hr = ICDecompressGetFormat(m_HIC, m_pFormat, 0);
    if (hr <= 0)
    {
	AVM_WRITE("Win32 video decoder", "ICDecompressGetFormatSize failed\n");
        return -1;
    }
    if (hr < (int)sizeof(BitmapInfo))
	hr = sizeof(BitmapInfo);

    m_Dest.SetBits(24);
    m_bitrick = (BITMAPINFOHEADER*) malloc(hr);
    // ok I guess Xvid people brinks here another interesting
    // aspect - we are asking for some format but we already
    // have to pass one here!!!
    memset(m_bitrick, 0, hr);
    memcpy(m_bitrick, &m_Dest, sizeof(BITMAPINFOHEADER));
    m_bitrick->biHeight = labs(m_Dest.biHeight); // xvid trick
    hr = ICDecompressGetFormat(m_HIC, m_pFormat, m_bitrick);

    if (hr != 0)
    {
	AVM_WRITE("Win32 video decoder", "Declined format dump:\n");
	BitmapInfo(*m_pFormat).Print();
	m_Dest.Print();
	AVM_WRITE("Win32 video decoder", " ICDecompressGetFormat failed Error %d\n", hr);
        return -1;
    }

    switch (m_Info.fourcc)
    {
    case fccCRAM:
	//if (m_pFormat->biCompression != fccMSVC)
        //    break;
    case fccCVID:
    case fccIV31:
    case fccIV32:
    case fccTSCC:
        m_bLastNeeded = true;
	break;
    default:
	m_bLastNeeded = false;
    }

    hr = ICGetInfo(m_HIC, &ici);
    // this might be good to use but many codecs returns false info here
    //if (hr && !(ici.dwFlags & VIDCF_FASTTEMPORALD)) m_bLastNeeded = true;

    switch (m_Info.fourcc)
    {
    case fccIV31:
    case fccIV32:
	m_Dest.SetBits(16); // doesn't support 24bit RGB
	break;
    case fccTSCC:
	m_Dest.SetBits(15); // doesn't support 24bit RGB
	break;
    case fccZLIB:
    case fccMSZH:
    case fccASV1:
    case fccASV2:
    case mmioFOURCC('A', 'V', 'R', 'n'):
	m_bFlip = 1; // these codecs are using always top-down RGB
	/* fall through */
    default:
	m_Dest.SetBits(24);
    }

    if (m_bFlip)
	m_Dest.biHeight = labs(m_Dest.biHeight);
    else
    {
	setDecoder(m_Dest);
	hr = (m_bUseEx) ?
	    ICUniversalEx(m_HIC, ICM_DECOMPRESSEX_QUERY, 0,
			  m_pFormat, 0, m_bitrick, 0) :
	    ICDecompressQuery(m_HIC, m_pFormat, m_bitrick);
	if (hr)
	{
	    m_Dest.biHeight = labs(m_Dest.biHeight);
	    AVM_WRITE("Win32 video decoder", "Decoder does not support upside-down RGB frames\n");
	}
    }
    setDecoder(m_Dest);

    m_Caps = CAP_NONE;
    switch (m_Info.fourcc)
    {
    case fccDIV3:
    case fccDIV4:
    case fccDIV5:
    case fccDIV6:
    case fccMP42:
	// only needed for some older codecs
	m_Caps = (CAPS) (CAP_YUY2 | CAP_UYVY | CAP_YVYU); //planar formats broken, others untested
	m_divx_trick = true;
	break;
    case fccHFYU:
	if (m_pFormat->biSize == 232)
	{
	    m_Caps = CAP_YUY2;
	    //m_Dest.SetSpace(fccYUY2);
	    break;
	}
	// fall through for RGB mode
    default:
	{
	    struct ct {
		fourcc_t fcc;
		CAPS cap;
	    } check[] = {
		{ fccIYUV, CAP_IYUV },
		{ fccUYVY, CAP_UYVY },
		{ fccYV12, CAP_YV12 },
		{ fccYVYU, CAP_YVYU },
		{ fccYUY2, CAP_YUY2 },
		{ fccI420, CAP_I420 },
		{ fccYVU9, CAP_YVU9 },
		// anyone heard about MTX6 ??
		{ 0, CAP_NONE }
	    };

	    BitmapInfo mbi(m_Dest);
	    for (ct* c = check; c->fcc; c++)
	    {
		mbi.SetSpace(c->fcc);
		setDecoder(mbi);
		hr = (m_bUseEx) ?
		    ICUniversalEx(m_HIC, ICM_DECOMPRESSEX_QUERY, 0,
				  m_pFormat, 0, m_bitrick, 0) :
		    ICDecompressQuery(m_HIC, m_pFormat, m_bitrick);
		//printf ("TEST %.4s %d\n", (char*)&(c->fcc), h);
		if (hr == 0)
		    m_Caps = (CAPS)(m_Caps | c->cap);
	    }
	    if (m_Info.fourcc == fccMJPG
		&& strcmp((const char*)m_Info.dll, "m3jpeg32.dll") == 0)
	    {
		// YV12 works but incorrectly - it's I420
		m_Caps = (CAPS) (m_Caps & ~CAP_YV12);
	    }
	    setDecoder(m_Dest);
	}
    }
    if (m_Caps)
	AVM_WRITE("Win32 video decoder", "Decoder is capable of YUV output ( flags 0x%x)\n", (int)m_Caps);
    return 0;
}

VideoDecoder::~VideoDecoder()
{
    Stop();
    if (m_bitrick)
	free(m_bitrick);
    if (m_pModule)
	m_pModule->CloseHandle(m_HIC);
}

int VideoDecoder::Start()
{
    if (m_iStatus == 1)
        return 0;
#if 0
    AVM_WRITE("Win32 video decoder", "Starting decompression, format: \n");
    BitmapInfo(m_Dest).Print();
    //AVM_WRITE("Win32 video decoder", "Dest fmt:\n");
    //BitmapInfo(m_decoder).Print();
#endif

    int tmpcomp = m_bitrick->biCompression;
    if (m_divx_trick)
	m_bitrick->biCompression = 0;
    /* On2 Truemotion VP3.x support */
    int hr = (m_bUseEx) ?
	ICUniversalEx(m_HIC, ICM_DECOMPRESSEX_BEGIN, 0,
		      m_pFormat, 0, m_bitrick, 0) :
	ICDecompressBegin(m_HIC, m_pFormat, m_bitrick);
    m_bitrick->biCompression = tmpcomp;
    //AVM_WRITE("Win32 video decoder", "DECODER COMPRESSSION  %.4s\n", (char*)&m_decoder.biCompression);
    if (hr != 0 && (hr != -2 || m_Info.fourcc != fccMJPG))
    {
	// FIXME  MJPG decoder (m3jpeg32.dll) returns -2
        // if anyone knows why - please fix it....
	//BitmapInfo(*m_pFormat).Print();
	//AVM_WRITE("Win32 video decoder", "Dest fmt:\n");
	//BitmapInfo(m_decoder).Print();
	AVM_WRITE("Win32 video decoder", "WARNING: ICDecompressBegin() failed ( shouldn't happen ), hr=%d (%s)\n",
		  (int)hr, ((hr == ICERR_BADFORMAT) ? "Bad Format)" : "?)"));
        return -1;
    }
    m_iStatus = 1;
    return 0;
}

int VideoDecoder::Stop()
{
    if (m_iStatus)
    {
	int r = ICDecompressEnd(m_HIC);
	if (r != 0)
	    AVM_WRITE("Win32 video decoder", "WARNING: ICDecompressEnd() failed ( shouldn't happen ), hr=%d\n", r);
        m_iStatus = 0;
    }
    return 0;
}

int VideoDecoder::DecodeFrame(CImage* dest, const void* src, uint_t size,
			      int is_keyframe, bool render, CImage** pOut)
{
    CImage* tmp = 0;
    int hr;

    //printf("DECODERNOMAR\n"); m_Dest.Print(); dest->GetFmt()->Print();
    uint8_t* d = (dest) ? dest->Data() : 0;
    int flg = ((is_keyframe) ? 0 : ICDECOMPRESS_NOTKEYFRAME)
	| ((dest && render) ? 0 : ICDECOMPRESS_HURRYUP/*|ICDECOMPRESS_PREROL*/);

    if (!m_iStatus)
        return -1;

    if (m_bLastNeeded)
    {
	if (!dest || !dest->IsFmt(&m_Dest))
	{
	    if (!m_pLastImage)
		m_pLastImage = new CImage(&m_Dest);
	    if (dest)
		d = m_pLastImage->Data();
	}
	else
	{
	    if (m_pLastImage)
	    {
		if (1 || !is_keyframe)
		{
		    // unfortunately for reconstructed files
                    // we can't recognize keyframes for them
		    //printf("Copy prev last %p  %p\n", dest, m_pLastImage);
		    dest->Convert(m_pLastImage);
		}
		m_pLastImage->Release();
	    }
	    dest->AddRef();
	    m_pLastImage = dest;
	}
    }

    //AVM_WRITE("Win32 video decoder", "PTR %p   src: %p   size: %d  kf: %d\n", dest->Data(), src, size, is_keyframe);
    setDecoder(m_Dest);
    m_pFormat->biSizeImage = size;

    hr = (m_bUseEx) ?
	ICUniversalEx(m_HIC, ICM_DECOMPRESSEX, flg, m_pFormat, (void*) src, m_bitrick, d) :
	ICDecompress(m_HIC, flg, m_pFormat, (void*) src, m_bitrick, d);

    if (dest)
    {
	if (hr)
	    AVM_WRITE("Win32 video decoder", "VideoDecoder: warning: hr=%d\n", hr);
	else if (m_bLastNeeded && dest && d != dest->Data())
	    dest->Convert(m_pLastImage);
    }

    return hr;
}

int VideoDecoder::SetDestFmt(int bits, fourcc_t csp)
{
    if (!CImage::Supported(csp, bits))
	return -1;

    switch (m_Info.fourcc)
    {
    case fccIV31:
    case fccIV32:
        return -1;
    }

    BitmapInfo tmpbi(m_Dest);
    AVM_WRITE("Win32 video decoder", 1, "SetDestFmt  bits: %d  csp: %.4s\n", bits, (char*)&csp);

    if (bits)
    {
	switch (bits)
	{
	case 15:
	case 16:
	case 24:
	case 32:
	    // 16bit ??? if (m_Info.fourcc_array[0] == fccMJPG)
	    //     m_Dest.biSize=0x28;

            // FIXME - use one routine to setup biHeight!!!
	    m_Dest.SetBits(bits);
	    switch (m_Info.fourcc)
	    {
	    case fccASV1:
	    case fccASV2:
		m_Dest.biHeight = labs(m_Dest.biHeight);
		// these codecs produces upside down RGB images
		break;
	    }
	    break;
	default:
            return -1;
        }
    } else
	m_Dest.SetSpace(csp);

    Stop();
    // note i263 is able to produce downside-up down image for YUY2
    setDecoder(m_Dest);
    int tmpcomp = m_bitrick->biCompression;
    if (m_divx_trick)
	m_bitrick->biCompression = 0;

    int hr = (m_bUseEx) ?
	ICUniversalEx(m_HIC, ICM_DECOMPRESSEX_QUERY, 0,
		      m_pFormat, 0, m_bitrick, 0) :
	ICDecompressQuery(m_HIC, m_pFormat, m_bitrick);

    //m_bitrick.Print();
    //BitmapInfo tmx((BitmapInfo*)m_pFormat);

    m_bitrick->biCompression = tmpcomp;

    if (hr != 0)
    {
	if (csp)
	{
	    AVM_WRITE("Win32 video decoder", "WARNING: Unsupported color space 0x%x  (%.4s)\n",
		      csp, (char*)&csp);
	}
	else
	    AVM_WRITE("Win32 video decoder", "WARNING: Unsupported bit depth: %d\n", bits);

	m_Dest = tmpbi;
        m_Dest.Print();
	setDecoder(m_Dest);
    }

    Start();

    return (hr == 0) ? 0 : -1;
}

void VideoDecoder::setDecoder(BitmapInfo& bi)
{
    // persistent biSize
    memcpy((char*)m_bitrick + 4, (char*)&bi + 4, sizeof(BitmapInfo) - 4);
    m_bitrick->biSize = 40;
    switch (m_Info.fourcc)
    {
    case fccXVID:
	//if (m_bitrick->biCompression == 0 || m_bitrick->biCompression == 3)
	//    break;
        // XviD doesn't like biHeight < 0 even for YUV surfaces
	break;
    }
}

AVM_END_NAMESPACE;
