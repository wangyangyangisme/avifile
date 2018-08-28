/********************************************************

	DirectShow Video decoder implementation
	Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)

*********************************************************/

#include "dshow/guids.h"
#include "dshow/interfaces.h"
#include "avm_output.h"

#include "DS_VideoDecoder.h"
#include "ldt_keeper.h"
#include "configfile.h"
#include "avm_cpuinfo.h"

#include "wine/windef.h"
#include "wine/winreg.h"
#include "wine/vfw.h"
#include "registry.h"

#ifndef NOAVIFILE_HEADERS
#define VFW_E_NOT_RUNNING               0x80040226
#include "avm_fourcc.h"
#endif

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>  // labs

AVM_BEGIN_NAMESPACE;

DS_VideoDecoder::DS_VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& format, int flip)
    :IVideoDecoder(info, format), m_sVhdr2(0), m_pIDivx4(0), m_iStatus(0),
    m_iLastPPMode(0), m_iLastBrightness(0), m_iLastContrast(0),
    m_iLastSaturation(0), m_iLastHue(0), 
    m_CType(CT_NONE), m_bHaveUpsideDownRGB(true),
    m_bSetFlg(false), m_bFlip(flip)
{
    unsigned bihs = (m_pFormat->biSize < (int) sizeof(BITMAPINFOHEADER)) ?
	sizeof(BITMAPINFOHEADER) : m_pFormat->biSize;
    bihs = sizeof(VIDEOINFOHEADER) - sizeof(BITMAPINFOHEADER) + bihs;

    //int ss = 0;for (unsigned i = 0; i < m_pFormat->biSize; i++) { ss += ((unsigned char*) m_pFormat)[i];  printf("  %2x", ((unsigned char*) m_pFormat)[i]); if (!(i%16)) printf("\n"); }
    //printf("\nBITMAPINFOHEADER %d  %d %d\n", m_pFormat->biSize, bihs, ss);
    //m_Dest.biHeight = labs(m_Dest.biHeight);

    // parent:
    //m_Dest = *m_pFormat;
    //m_Dest.SetBits(24);

    m_sVhdr = (VIDEOINFOHEADER*) malloc(bihs);
    memset(m_sVhdr, 0, bihs);
    memcpy(&m_sVhdr->bmiHeader, m_pFormat, m_pFormat->biSize);
    m_sVhdr->rcSource.left = m_sVhdr->rcSource.top = 0;
    m_sVhdr->rcSource.right = m_sVhdr->bmiHeader.biWidth;
    m_sVhdr->rcSource.bottom = m_sVhdr->bmiHeader.biHeight;
    m_sVhdr->rcTarget = m_sVhdr->rcSource;

    m_sOurType.majortype = MEDIATYPE_Video;
    m_sOurType.subtype = MEDIATYPE_Video;
    m_sOurType.subtype.f1 = m_sVhdr->bmiHeader.biCompression;
    m_sOurType.formattype = FORMAT_VideoInfo;
    m_sOurType.bFixedSizeSamples = false;
    m_sOurType.bTemporalCompression = true;
    m_sOurType.pUnk = 0;
    m_sOurType.cbFormat = bihs;
    m_sOurType.pbFormat = (char*)m_sVhdr;

    m_sVhdr2 = (VIDEOINFOHEADER*) malloc(sizeof(VIDEOINFOHEADER) + 12);
    // basicaly copy of m_Dest - but staying friendly to other players..
    memset(m_sVhdr2, 0, sizeof(VIDEOINFOHEADER) + 12);
    m_sVhdr2->bmiHeader.biBitCount = 24;
    m_sVhdr2->bmiHeader.biWidth = m_Dest.biWidth;
    m_sVhdr2->bmiHeader.biHeight = m_Dest.biHeight;
    m_sVhdr2->bmiHeader.biSizeImage = m_sVhdr2->bmiHeader.biWidth
	* labs(m_sVhdr2->bmiHeader.biHeight)
	* m_sVhdr2->bmiHeader.biBitCount / 8;
    m_sVhdr2->rcSource = m_sVhdr->rcSource;
    m_sVhdr2->rcTarget = m_sVhdr->rcTarget;

    memset(&m_sDestType, 0, sizeof(m_sDestType));
    m_sDestType.majortype = MEDIATYPE_Video;
    m_sDestType.formattype = FORMAT_VideoInfo;
    m_sDestType.subtype = MEDIASUBTYPE_RGB24;
    m_sDestType.bFixedSizeSamples = true;
    m_sDestType.bTemporalCompression = false;
    m_sDestType.lSampleSize = m_sVhdr2->bmiHeader.biSizeImage;
    m_sDestType.cbFormat = sizeof(VIDEOINFOHEADER);
    m_sDestType.pbFormat = (char*)m_sVhdr2;
}

// this is cruel hack to avoid problems with throws -
// could anyone explain me why GNU people can't build reliable
// C++ compiler
int DS_VideoDecoder::init()
{
    Setup_FS_Segment();

    m_pDS_Filter = DS_FilterCreate((const char*)m_Info.dll, &m_Info.guid, &m_sOurType, &m_sDestType);

    if (!m_pDS_Filter)
    {
	AVM_WRITE("Win32 DS video decoder", "WARNING: format not accepted!\n");
        return -1;
    }

    if (m_Dest.biHeight < 0)
    {
	int r = m_pDS_Filter->m_pOutputPin->vt->QueryAccept(m_pDS_Filter->m_pOutputPin, &m_sDestType);
	if (r)
	{
	    AVM_WRITE("Win32 DS video decoder", "WARNING: decoder does not support upside-down RGB frames!\n");
	    m_Dest.biHeight = -m_Dest.biHeight;
	    m_sVhdr2->bmiHeader.biHeight = m_Dest.biHeight;
	    m_bHaveUpsideDownRGB = false;
	}
    }

    switch (m_Info.fourcc)
    {
	//case fccIV50: works normaly with YV12 (kabi)
	// IV50 doesn't support I420
    case fccTM20:
	m_Caps = CAP_NONE;
	break;
    case fccDIV3:
    case fccDIV4:
    case fccDIV5:
    case fccDIV6:
    case fccMP42:
	//YV12 seems to be broken for DivX :-) codec
	//produces incorrect picture
	//m_Caps = (CAPS) (m_Caps & ~CAP_YV12);
	//m_Caps = CAP_UYVY;//CAP_YUY2; // | CAP_I420;
	//m_Caps = CAP_I420;
	m_Caps = (CAPS) (CAP_YUY2 | CAP_UYVY);
	break;
    default:
	{
	    struct ct {
		unsigned int bits;
		fourcc_t fcc;
		GUID subtype;
		CAPS cap;
	    } check[] = {
		{16, fccYUY2, MEDIASUBTYPE_YUY2, CAP_YUY2},
		{12, fccIYUV, MEDIASUBTYPE_IYUV, CAP_IYUV},
		{16, fccUYVY, MEDIASUBTYPE_UYVY, CAP_UYVY},
		{12, fccYV12, MEDIASUBTYPE_YV12, CAP_YV12},
		{16, fccYV12, MEDIASUBTYPE_YV12, CAP_YV12},
		{16, fccYVYU, MEDIASUBTYPE_YVYU, CAP_YVYU},
		//{12, fccI420, MEDIASUBTYPE_I420, CAP_I420},
		{0},
	    };
	    m_Caps = CAP_NONE;

	    for (ct* c = check; c->bits; c++)
	    {
		m_sVhdr2->bmiHeader.biBitCount = c->bits;
		m_sVhdr2->bmiHeader.biCompression = c->fcc;
		m_sDestType.subtype = c->subtype;
		int r = m_pDS_Filter->m_pOutputPin->vt->QueryAccept(m_pDS_Filter->m_pOutputPin, &m_sDestType);
		if (!r)
		    m_Caps = (CAPS)(m_Caps | c->cap);
	    }
	    m_sVhdr2->bmiHeader.biBitCount = 24;
	    m_sVhdr2->bmiHeader.biCompression = 0;
	    m_sDestType.subtype = MEDIASUBTYPE_RGB24;
	}
    }

    const char* dll = (const char*)m_Info.dll;
    if (strcmp(dll, "divxcvki.ax") == 0
	|| strcmp(dll, "divx_c32.ax") == 0
	|| strcmp(dll, "wmvds32.ax") == 0
	|| strcmp(dll, "wmv8ds32.ax") == 0)
    {
	m_CType = CT_DIVX3;
	m_iMaxAuto = RegReadInt("win32", "maxauto", 4);
    }
    else if (strcmp(dll, "divxdec.ax") == 0)
    {
	m_CType = CT_DIVX4;
	m_iMaxAuto = RegReadInt("win32DivX4", "maxauto", 6);
	if (m_pDS_Filter->m_pFilter->vt->QueryInterface((IUnknown*)m_pDS_Filter->m_pFilter,
							&IID_IDivxFilterInterface, (void**)&m_pIDivx4))
	{
	    AVM_WRITE("Win32 video decoder", 1, "No such interface\n");
	    m_CType = CT_NONE;
	}
    }
    else if (strcmp(dll, "ir50_32.dll") == 0)
	m_CType = CT_IV50;

    getCodecValues();

    if (m_Caps != CAP_NONE)
	AVM_WRITE("Win32 DS video decoder", "Decoder is capable of YUV output ( flags 0x%x )\n", (int)m_Caps);

    SetDirection(m_bFlip);
    //m_Dest.Print();
    return 0;
}

DS_VideoDecoder::~DS_VideoDecoder()
{
    Stop();
    if (m_pIDivx4)
	m_pIDivx4->vt->Release((IUnknown*)m_pIDivx4);

    if (m_sVhdr)
	free(m_sVhdr);
    if (m_sVhdr2)
	free(m_sVhdr2);
    if (m_pDS_Filter)
	DS_Filter_Destroy(m_pDS_Filter);
}

int DS_VideoDecoder::Start()
{
    if (!m_iStatus)
    {
	Setup_FS_Segment();
	//AVM_WRITE("Win32 video decoder", "DSSTART  %p  %d\n", this, m_sVhdr2->bmiHeader.biHeight);
        m_iStatus = 1;
	m_pDS_Filter->Start(m_pDS_Filter);
	ALLOCATOR_PROPERTIES props, props1;
	props.cBuffers = 1;
	props.cbBuffer = m_sDestType.lSampleSize;

	//don't know how to do this correctly
	props.cbAlign = 1; // Sascha Sommer thinks this is the right value
	props.cbPrefix = 0;
	m_pDS_Filter->m_pAll->vt->SetProperties(m_pDS_Filter->m_pAll, &props, &props1);
	m_pDS_Filter->m_pAll->vt->Commit(m_pDS_Filter->m_pAll);
    }
    return 0;
}

int DS_VideoDecoder::Stop()
{
    if (m_iStatus)
    {
	Setup_FS_Segment();
	m_pDS_Filter->Stop(m_pDS_Filter);
        m_iStatus = 0;
	//AVM_WRITE("Win32 video decoder", "DSSTOP  %p\n", this);
    }
    return 0;
}

int DS_VideoDecoder::DecodeFrame(CImage* dest, const void* src, uint_t size,
				 int is_keyframe, bool render, CImage** pOut)
{
    IMediaSample* sample = 0;

    if (!m_iStatus)
    {
        AVM_WRITE("Win32 DS video decoder", "not started!\n");
	return -1;
    }
    Setup_FS_Segment();

    if (!dest->IsFmt(&m_Dest)) printf("\n\nERRRRRRRRR\n\n");

    m_pDS_Filter->m_pAll->vt->GetBuffer(m_pDS_Filter->m_pAll, &sample, 0, 0, 0);

    if (!sample)
    {
	AVM_WRITE("Win32 video decoder", 1, "ERROR: null sample\n");
	return -1;
    }

    //printf("DECODE this:%p  img: %p\n", this, dest);
    if (dest)
    {
	if (!(dest->Data()))
	{
	    AVM_WRITE("Win32 DS video decoder", 1, "no m_outFrame??\n");
	}
	else
	    m_pDS_Filter->m_pOurOutput->SetPointer2(m_pDS_Filter->m_pOurOutput, (char*)dest->Data());
    }
    //printf("W32 from -> to\n"); m_Dest.Print(); dest->GetFmt()->Print();
    char* ptr;
    sample->vt->SetActualDataLength(sample, size);
    sample->vt->GetPointer(sample, (BYTE **)&ptr);
    memcpy(ptr, src, size);
    sample->vt->SetSyncPoint(sample, is_keyframe);
    sample->vt->SetPreroll(sample, dest ? 0 : 1);
    // sample->vt->SetMediaType(sample, &m_sOurType);

    int check = 0;
    if (m_bSetFlg)
    {
	//printf("SETFLG  %d \n", m_iLastPPMode);
	if (m_iLastPPMode >= 0 && m_iLastHue != -1)
	{
	    m_bSetFlg = false;
	    setCodecValues();
	}
	check++;
    }

    int hr = m_pDS_Filter->m_pImp->vt->Receive(m_pDS_Filter->m_pImp, sample);
    if (hr)
    {
	AVM_WRITE("Win32 DS video encoder", 1, "DS_VideoDecoder::DecodeInternal() error putting data into input pin %x\n", hr);
        hr = -1;
    }

    sample->vt->Release((IUnknown*)sample);

    if (check)
	getCodecValues();

    switch (m_CType)
    {
    case CT_DIVX4:
	dest->SetQuality(m_iLastPPMode / 6.0);
	break;
    case CT_DIVX3:
	dest->SetQuality(m_iLastPPMode / 4.0);
	break;
    default:
	;
    }

    return hr;
}

/*
 * bits == 0   - leave unchanged
 */
int DS_VideoDecoder::SetDestFmt(int bits, fourcc_t csp)
{
    if ((bits || csp) && !CImage::Supported(csp, bits))
	return -1;

    AVM_WRITE("Win32 DS video decoder", 1, "SetDestFmt %d   %.4s\n", bits, (char*)&csp);
    BitmapInfo temp = m_Dest;
    if (bits != 0)
    {
	bool ok = true;

	switch (bits)
        {
	case 15:
	    m_sDestType.subtype = MEDIASUBTYPE_RGB555;
    	    break;
	case 16:
	    m_sDestType.subtype = MEDIASUBTYPE_RGB565;
	    break;
	case 24:
	    m_sDestType.subtype = MEDIASUBTYPE_RGB24;
	    break;
	case 32:
	    m_sDestType.subtype = MEDIASUBTYPE_RGB32;
	    break;
	default:
            ok = false;
	    break;
	}
	if (ok)
	{
	    m_Dest.SetBits(bits);
	    if (!m_bHaveUpsideDownRGB)
		m_Dest.biHeight = labs(m_Dest.biHeight);

	    //m_Dest.biHeight = labs(m_Dest.biHeight);
	}
    }
    else if (csp != 0)
    {
        bool ok = true;
	switch (csp)
	{
	case fccYUY2:
	    m_sDestType.subtype = MEDIASUBTYPE_YUY2;
	    break;
	case fccYV12:
	    m_sDestType.subtype = MEDIASUBTYPE_YV12;
	    break;
	case fccIYUV:
	    m_sDestType.subtype = MEDIASUBTYPE_IYUV;
	    break;
	case fccUYVY:
	    m_sDestType.subtype = MEDIASUBTYPE_UYVY;
	    break;
	case fccYVYU:
	    m_sDestType.subtype = MEDIASUBTYPE_YVYU;
	    break;
	default:
	    ok = false;
            break;
	}

	if (ok)
	    m_Dest.SetSpace(csp);
    }

    Setup_FS_Segment();
    m_sDestType.lSampleSize = m_Dest.biSizeImage;
    memcpy(&(m_sVhdr2->bmiHeader), &m_Dest, sizeof(m_Dest));
    m_sVhdr2->bmiHeader.biHeight = m_Dest.biHeight;
    m_sVhdr2->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_sDestType.cbFormat = sizeof(VIDEOINFOHEADER);
    if (m_sVhdr2->bmiHeader.biCompression == 3)
        m_sDestType.cbFormat += 12;

    struct ct {
	fourcc_t fcc;
        CAPS cap;
    } map[] =
    {
	{ fccYUY2, CAP_YUY2 },
	{ fccYV12, CAP_YV12 },
	{ fccIYUV, CAP_IYUV },
	{ fccUYVY, CAP_UYVY },
	{ fccYVYU, CAP_YVYU },
	{ 0 }
    };

    int result = 0;
    bool should_test = true;
    for (ct* c = map; c->fcc; c++)
    {
	if (c->fcc == csp)
	{
            if (!(m_Caps & c->cap))
		should_test = false;
            break;
	}
    }
    if (should_test)
	result = m_pDS_Filter->m_pOutputPin->vt->QueryAccept(m_pDS_Filter->m_pOutputPin, &m_sDestType);

    if (result != 0)
    {
	if (csp)
	    AVM_WRITE("Win32 video decoder", "Warning: unsupported color space\n");
	else
	    AVM_WRITE("Win32 video decoder", "Warning: unsupported bit depth\n");
        m_Dest = temp;
	m_sDestType.lSampleSize = m_Dest.biSizeImage;
	memcpy(&(m_sVhdr2->bmiHeader), &m_Dest, sizeof(temp));
	m_sVhdr2->bmiHeader.biHeight = m_Dest.biHeight;
	m_sVhdr2->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_sDestType.cbFormat = sizeof(VIDEOINFOHEADER);
	if (m_sVhdr2->bmiHeader.biCompression == 3)
	    m_sDestType.cbFormat += 12;

	return -1;
    }

    m_pFormat->biBitCount = bits;

    //Restart();
    bool stoped = false;
    if (m_iStatus)
    {
	Stop();
        stoped = true;
    }

    m_pDS_Filter->m_pInputPin->vt->Disconnect(m_pDS_Filter->m_pInputPin);
    m_pDS_Filter->m_pOutputPin->vt->Disconnect(m_pDS_Filter->m_pOutputPin);
    m_pDS_Filter->m_pOurOutput->SetNewFormat(m_pDS_Filter->m_pOurOutput, &m_sDestType);
    result = m_pDS_Filter->m_pInputPin->vt->ReceiveConnection(m_pDS_Filter->m_pInputPin,
							      m_pDS_Filter->m_pOurInput,
							      &m_sOurType);
    if (result)
    {
	AVM_WRITE("Win32 video decoder", "Error reconnecting input pin 0x%x\n", (int)result);
	return -1;
    }
    result = m_pDS_Filter->m_pOutputPin->vt->ReceiveConnection(m_pDS_Filter->m_pOutputPin,
							       (IPin*)m_pDS_Filter->m_pOurOutput,
							       &m_sDestType);
    if (result)
    {
	AVM_WRITE("Win32 video decoder", "Error reconnecting output pin 0x%x\n", (int)result);
	return -1;
    }

    if (stoped)
	Start();

    return 0;
}
int DS_VideoDecoder::SetDirection(int d)
{
    if (m_Dest.biHeight < 0)
	m_Dest.biHeight = -m_Dest.biHeight;

    if (!d && m_bHaveUpsideDownRGB)
	m_Dest.biHeight = -m_Dest.biHeight;

    m_sVhdr2->bmiHeader.biHeight = m_Dest.biHeight;

    //printf("SETDIRECTION  %d\n", m_sVhdr2->bmiHeader.biHeight); m_Dest.Print();
    if (m_pDS_Filter)
	SetDestFmt(0, 0);
    return 0;
}

int DS_VideoDecoder::GetValue(const char* name, int* value) const
{
    if (strcmp(name, "postprocessing") == 0) // Pp
	*value = m_iLastPPMode;
    else if (strcmp(name, "Brightness") == 0)
	*value = m_iLastBrightness;
    else if (strcmp(name, "Contrast") == 0)
	*value = m_iLastContrast;
    else if (strcmp(name, "Saturation") == 0)
	*value = m_iLastSaturation;
    else if (strcmp(name, "Hue") == 0)
	*value = m_iLastHue;
    else if (strcmp(name, "maxauto") == 0)
	*value = m_iMaxAuto;
    else
        return -1;

    //printf("BR %d   C %d   S %d   H %d   PP: %d   (%s)\n", m_iLastBrightness,
    //       m_iLastContrast, m_iLastSaturation, m_iLastHue, m_iLastPPMode, name);

    return 0;
}

int DS_VideoDecoder::getCodecValues()
{
    switch (m_CType)
    {
    case CT_DIVX4:
	m_pIDivx4->vt->get_PPLevel(m_pIDivx4, &m_iLastPPMode);
	m_iLastPPMode /= 10;
	m_pIDivx4->vt->get_Brightness(m_pIDivx4, &m_iLastBrightness);
	m_pIDivx4->vt->get_Contrast(m_pIDivx4, &m_iLastContrast);
	m_pIDivx4->vt->get_Saturation(m_pIDivx4, &m_iLastSaturation);
	break;
    case CT_DIVX3:
	{
	    // brightness 87
	    // contrast 74
	    // hue 23
	    // saturation 20
	    // post process mode 0
	    // get1 0x01
	    // get2 10
	    // get3=set2 86
	    // get4=set3 73
	    // get5=set4 19
	    // get6=set5 23
	    IHidden* hidden = (IHidden*)((int)m_pDS_Filter->m_pFilter + 0xb8);
	    //#warning NOT SURE
	    hidden->vt->GetSmth2(hidden, &m_iLastPPMode);
	    if (m_iLastPPMode >= 10)
		m_iLastPPMode -= 10;
	    if (m_iLastPPMode < 0 || m_iLastHue < 0)
	    {
		hidden->vt->GetSmth3(hidden, &m_iLastBrightness);
		//printf("BRGET %d\n", m_iLastBrightness);
		hidden->vt->GetSmth4(hidden, &m_iLastContrast);
		hidden->vt->GetSmth5(hidden, &m_iLastSaturation);
		hidden->vt->GetSmth6(hidden, &m_iLastHue);
	    }
	    break;
	}
    case CT_IV50:
	{
	    IHidden2* hidden = 0;
	    if (m_pDS_Filter->m_pFilter->vt->QueryInterface((IUnknown*)m_pDS_Filter->m_pFilter, &IID_Iv50Hidden, (void**)&hidden))
	    {
		AVM_WRITE("Win32 video decoder", 0, "No such interface\n");
		return -1;
	    }
	    else
	    {
		int recordpar[30];
		memset(&recordpar, 0, sizeof(recordpar));
		recordpar[0]=0x7c;
		recordpar[1]=fccIV50;
		recordpar[2]=0x10005;
		recordpar[3]=2;
		recordpar[4]=1;
		recordpar[5]=0x80000000 | 0x20 | 0x40 | 0x80;
		hidden->vt->DecodeGet(hidden, recordpar);
		m_iLastBrightness = recordpar[18];
		m_iLastSaturation = recordpar[19];
		m_iLastContrast = recordpar[20];
		hidden->vt->Release((IUnknown*)hidden);
	    }
	}
    default:
        ;
    }
    return 0;
}

int DS_VideoDecoder::SetValue(const char* name, int value)
{
    m_bSetFlg = true;
    if (strcmp(name, "postprocessing") == 0) // Pp
	m_iLastPPMode = value;
    else if (strcmp(name, "Brightness") == 0)
	m_iLastBrightness = value;
    else if (strcmp(name, "Contrast") == 0)
	m_iLastContrast = value;
    else if (strcmp(name, "Saturation") == 0)
	m_iLastSaturation = value;
    else if (strcmp(name, "Hue") == 0)
	m_iLastHue = value;
    else if (strcmp(name, "maxauto") == 0)
	m_iMaxAuto = value;
    //printf("SETVAL  B %d   C %d   S %d   H %d   \n", m_iLastBrightness,
    //       m_iLastContrast, m_iLastSaturation, m_iLastHue);
    //else AVM_WRITE("Win32 video decoder", "Set %s  %d\n", name, value);
    return 0;
}

int DS_VideoDecoder::setCodecValues()
{
    if (!m_iStatus)
	return -1;

    switch (m_CType)
    {
    case CT_DIVX4:
	m_pIDivx4->vt->put_PPLevel(m_pIDivx4, m_iLastPPMode * 10);
	m_pIDivx4->vt->put_Brightness(m_pIDivx4, m_iLastBrightness);
	m_pIDivx4->vt->put_Contrast(m_pIDivx4, m_iLastContrast);
	m_pIDivx4->vt->put_Saturation(m_pIDivx4, m_iLastSaturation);
	break;
    case CT_DIVX3:
	{
	    // brightness 87
	    // contrast 74
	    // hue 23
	    // saturation 20
	    // post process mode 0
	    // get1 0x01
	    // get2 10
	    // get3=set2 86
	    // get4=set3 73
	    // get5=set4 19
	    // get6=set5 23
	    IHidden* hidden = (IHidden*)((int)m_pDS_Filter->m_pFilter + 0xb8);
	    hidden->vt->SetSmth(hidden, m_iLastPPMode, 0);
	    hidden->vt->SetSmth2(hidden, m_iLastBrightness, 0);
	    hidden->vt->SetSmth3(hidden, m_iLastContrast, 0);
	    hidden->vt->SetSmth4(hidden, m_iLastSaturation, 0);
	    hidden->vt->SetSmth5(hidden, m_iLastHue, 0);
	}
        break;
    case CT_IV50:
	{
	    IHidden2* hidden = 0;
	    if (m_pDS_Filter->m_pFilter->vt->QueryInterface((IUnknown*)m_pDS_Filter->m_pFilter, &IID_Iv50Hidden, (void**)&hidden))
	    {
		AVM_WRITE("Win32 video decoder", 1, "No such interface\n");
		return -1;
	    }
	    int recordpar[30];
	    memset(&recordpar, 0, sizeof(recordpar));
	    recordpar[0]=0x7c;
	    recordpar[1]=fccIV50;
	    recordpar[2]=0x10005;
	    recordpar[3]=2;
	    recordpar[4]=1;
	    recordpar[5]=0x80000000 | 0x20 | 0x40 | 0x80;

	    recordpar[18] = m_iLastBrightness;
	    recordpar[19] = m_iLastSaturation;
	    recordpar[20] = m_iLastContrast;
	    int hr = hidden->vt->DecodeSet(hidden, recordpar);
	    hidden->vt->Release((IUnknown*)hidden);
	    return hr;
	}
    default:
        ;
    }
    return 0;
}

AVM_END_NAMESPACE;
