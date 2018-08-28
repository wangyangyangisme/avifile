/**
 * DMO Video decoder implementation
 * Copyright 2002 Zdenek Kabelac (kabi@users.sf.net)
 *  		  Eugene Kuznetsov  (divx@euro.ru)
 */

#include "dshow/guids.h"
#include "dshow/interfaces.h"
#include "avm_output.h"

#include "DMO_VideoDecoder.h"
#include "ldt_keeper.h"
#include "configfile.h"
#include "avm_cpuinfo.h"

#include "wine/winerror.h"
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

extern "C" void trapbug();

AVM_BEGIN_NAMESPACE;

static const struct fcc2gc_s {
    fourcc_t fcc;
    unsigned int bits;
    GUID subtype;
    DMO_VideoDecoder::CAPS cap;
} fcc2gctab[] = {
    { fccI420, 12, MEDIASUBTYPE_I420, IVideoDecoder::CAP_I420 },
    { fccYV12, 12, MEDIASUBTYPE_YV12, IVideoDecoder::CAP_YV12 },
    { fccYUY2, 16, MEDIASUBTYPE_YUY2, IVideoDecoder::CAP_YUY2 },
    { fccUYVY, 16, MEDIASUBTYPE_UYVY, IVideoDecoder::CAP_UYVY },
    { fccYVYU, 16, MEDIASUBTYPE_YVYU, IVideoDecoder::CAP_YVYU },
    { fccIYUV, 24, MEDIASUBTYPE_IYUV, IVideoDecoder::CAP_IYUV },

    {      8,  8, MEDIASUBTYPE_RGB8, IVideoDecoder::CAP_NONE },
    {     15, 16, MEDIASUBTYPE_RGB555, IVideoDecoder::CAP_NONE },
    {     16, 16, MEDIASUBTYPE_RGB565, IVideoDecoder::CAP_NONE },
    {     24, 24, MEDIASUBTYPE_RGB24, IVideoDecoder::CAP_NONE },
    {     32, 32, MEDIASUBTYPE_RGB32, IVideoDecoder::CAP_NONE },

    { 0 },
};

static const struct fcc2gc_s* fcc2gc(uint_t fcc)
{
    const struct fcc2gc_s* c;
    for (c = fcc2gctab; c->fcc; c++)
	if (c->fcc == fcc)
	    return c;
    return 0;
}

DMO_VideoDecoder::DMO_VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& format, int flip)
    :IVideoDecoder(info, format), m_iStatus(0),
    m_iLastPPMode(0), m_iLastBrightness(0), m_iLastContrast(0),
    m_iLastSaturation(0), m_iLastHue(0),
    m_bHaveUpsideDownRGB(true), m_bFlip(flip)
{
    unsigned bihs = (m_pFormat->biSize < (int) sizeof(BITMAPINFOHEADER)) ?
	sizeof(BITMAPINFOHEADER) : m_pFormat->biSize;
    bihs = sizeof(VIDEOINFOHEADER) - sizeof(BITMAPINFOHEADER) + bihs;
    m_pDMO_Filter = 0;

    //int ss = 0;for (unsigned i = 0; i < m_pFormat->biSize; i++) { ss += ((unsigned char*) m_pFormat)[i];  printf("  %2x", ((unsigned char*) m_pFormat)[i]); if (!(i%16)) printf("\n"); }
    //printf("\nBITMAPINFOHEADER %d  %d %d\n", m_pFormat->biSize, bihs, ss);
    //m_Dest.SetSpace(IMG_FMT_YV12);
    //m_Dest = *m_pFormat;
    //m_Dest.biHeight = labs(m_Dest.biHeight);
    //m_Dest.Print();

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
    memset(m_sVhdr2, 0, sizeof(VIDEOINFOHEADER) + 12);
    m_sVhdr2->rcTarget = m_sVhdr->rcTarget;
    m_sVhdr2->rcSource = m_sVhdr->rcSource;
    memset(&m_sDestType, 0, sizeof(m_sDestType));
    m_sDestType.majortype = MEDIATYPE_Video;
    m_sDestType.formattype = FORMAT_VideoInfo;
    m_sDestType.bFixedSizeSamples = true;
    m_sDestType.bTemporalCompression = false;
    m_sDestType.cbFormat = sizeof(VIDEOINFOHEADER);
    m_sDestType.pbFormat = (char*)m_sVhdr2;

    SetDestFmt(24, 0);
    //SetDestFmt(0, IMG_FMT_YV12);
}

// this is cruel hack to avoid problems with throws -
// could anyone explain me why GNU people can't build reliable
// C++ compiler
int DMO_VideoDecoder::init()
{
    const struct fcc2gc_s* c;
    int r;
    int bbc;
    int bic;
    GUID sbt;

    Setup_FS_Segment();

    m_pDMO_Filter = DMO_FilterCreate((const char*)m_Info.dll, &m_Info.guid, &m_sOurType, &m_sDestType);
    if (!m_pDMO_Filter)
    {
	AVM_WRITE("Win32 DMO video decoder", "WARNING: format not accepted!\n");
        return -1;
    }

    AVM_WRITE("Win32 DMO video decoder", "opened dll: %s\n", (const char*)m_Info.dll);

    if (m_Dest.biHeight < 0)
    {
	r = m_pDMO_Filter->m_pMedia->vt->SetOutputType(m_pDMO_Filter->m_pMedia, 0, &m_sDestType, DMO_SET_TYPEF_TEST_ONLY);
	if (r)
	{
	    AVM_WRITE("Win32 DMO video decoder", "WARNING: decoder does not support upside-down RGB frames!\n");
	    m_Dest.biHeight = -m_Dest.biHeight;
	    m_sVhdr2->bmiHeader.biHeight = m_Dest.biHeight;
	    m_bHaveUpsideDownRGB = false;
	}
    }

    m_Caps = CAP_NONE;
    bbc = m_sVhdr2->bmiHeader.biBitCount;
    bic = m_sVhdr2->bmiHeader.biCompression;
    sbt = m_sDestType.subtype;
    for (c = fcc2gctab; c->bits && (c->cap != CAP_NONE); c++)
    {
        // check only for YUV
	m_sVhdr2->bmiHeader.biBitCount = c->bits;
	m_sVhdr2->bmiHeader.biCompression = c->fcc;
	m_sDestType.subtype = c->subtype;
	r = m_pDMO_Filter->m_pMedia->vt->SetOutputType(m_pDMO_Filter->m_pMedia, 0, &m_sDestType, DMO_SET_TYPEF_TEST_ONLY);
	if (!r)
	    m_Caps = (CAPS)(m_Caps | c->cap);
	//printf("Testing CSP err: %x  %x   %.4s\n", r, c->fcc, (char*)&c->fcc);
    }
    //m_Caps = CAP_YV12;

    m_sVhdr2->bmiHeader.biBitCount = bbc;
    m_sVhdr2->bmiHeader.biCompression = bic;
    m_sDestType.subtype = sbt;

    SetDirection(m_bFlip);

    //m_Dest.Print();
    //r = m_pDMO_Filter->m_pMedia->vt->Flush(m_pDMO_Filter->m_pMedia);
    return 0;
}

DMO_VideoDecoder::~DMO_VideoDecoder()
{
    Stop();

    if (m_sVhdr)
	free(m_sVhdr);
    if (m_sVhdr2)
	free(m_sVhdr2);
    if (m_pDMO_Filter)
	DMO_Filter_Destroy(m_pDMO_Filter);
}

int DMO_VideoDecoder::Start()
{
    if (!m_iStatus)
    {
	//AVM_WRITE("Win32 video decoder", "DSSTART  %p  %d\n", this, m_sVhdr2->bmiHeader.biHeight);
	m_iStatus = 1;
        Flush();
    }
    return 0;
}

int DMO_VideoDecoder::Stop()
{
    if (m_iStatus)
    {
        m_iStatus = 0;
	//AVM_WRITE("Win32 video decoder", "DSSTOP  %p\n", this);
    }
    return 0;
}

int DMO_VideoDecoder::DecodeFrame(CImage* dest, const void* src, uint_t size,
				 int is_keyframe, bool render, CImage** pOut)
{
    unsigned long status; // to be ignored by M$ specs
    DMO_OUTPUT_DATA_BUFFER db;
    CMediaBuffer* bufferin;
    uint8_t* imdata = dest ? dest->Data() : 0;

    if (!m_iStatus)
    {
        AVM_WRITE("Win32 DMO video decoder", "not started!\n");
	return -1;
    }

    Setup_FS_Segment();

    //if (!dest->IsFmt(&m_Dest)) { dest->GetFmt()->Print(); m_Dest.Print(); printf("\n\nERRRRRRRRR\n\n"); }
    //unsigned long dwf = 0;
    //int rr1 = m_pDMO_Filter->m_pMedia->vt->GetInputStatus(m_pDMO_Filter->m_pMedia, 0, &dwf);
    //printf("PROCESSTESTINPUT %d %x\n", rr1, dwf);
    //rr1 = m_pDMO_Filter->m_pMedia->vt->GetInputStreamInfo(m_pDMO_Filter->m_pMedia, 0, &dwf);
    //printf("PROCESSTESTINPUTSTREAM %d %x\n", rr1, dwf);
    //trapbug();
    bufferin = CMediaBufferCreate(size, (void*)src, size, 0);
    int r = m_pDMO_Filter->m_pMedia->vt->ProcessInput(m_pDMO_Filter->m_pMedia, 0,
						      (IMediaBuffer*)bufferin,
						      (is_keyframe) ? DMO_INPUT_DATA_BUFFERF_SYNCPOINT : 0,
						      0, 0);
    ((IMediaBuffer*)bufferin)->vt->Release((IUnknown*)bufferin);

    if (r != S_OK)
    {
        /* something for process */
	if (r != S_FALSE)
	    printf("ProcessInputError  r:0x%x=%d (keyframe: %d)\n", r, r, is_keyframe);
	else
	    printf("ProcessInputError  FALSE ?? (keyframe: %d)\n", is_keyframe);
	return size;
    }

    db.rtTimestamp = 0;
    db.rtTimelength = 0;
    db.dwStatus = 0;
    db.pBuffer = (IMediaBuffer*) CMediaBufferCreate(m_sDestType.lSampleSize,
						    imdata, 0, 0);
    r = m_pDMO_Filter->m_pMedia->vt->ProcessOutput(m_pDMO_Filter->m_pMedia,
						   (imdata) ? 0 : DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER,
						   1, &db, &status);
    //m_pDMO_Filter->m_pMedia->vt->Lock(m_pDMO_Filter->m_pMedia, 0);
    if ((unsigned)r == DMO_E_NOTACCEPTING)
	printf("ProcessOutputError: Not accepting\n");
    else if (r)
	printf("ProcessOutputError: r:0x%x=%d  %ld  stat:%ld\n", r, r, status, db.dwStatus);

    ((IMediaBuffer*)db.pBuffer)->vt->Release((IUnknown*)db.pBuffer);


    //int r = m_pDMO_Filter->m_pMedia->vt->Flush(m_pDMO_Filter->m_pMedia);
    //printf("FLUSH %d\n", r);
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

    if (check)
	getCodecValues();

    return size;//hr;
}

/*
 * bits == 0   - leave unchanged
 */
int DMO_VideoDecoder::SetDestFmt(int bits, fourcc_t csp)
{

    const struct fcc2gc_s* t;
    int r;

    if ((bits || csp) && !CImage::Supported(csp, bits))
	return -1;

    AVM_WRITE("Win32 DMO video decoder", 1, "SetDestFmt %d   %.4s\n", bits, (char*)&csp);

    if (bits == 0)
        bits = csp;

    BitmapInfo temp = m_Dest; // rememeber current setting
    Setup_FS_Segment();

    t = fcc2gc(bits);
    if (t)
    {
	m_sDestType.subtype = t->subtype;
	if (!t->cap)
	{
	    m_Dest.SetBits(bits);
	    if (!m_bHaveUpsideDownRGB)
		m_Dest.biHeight = labs(m_Dest.biHeight);
	}
	else
	    m_Dest.SetSpace(bits);
    } // else reset what we already have (dir change)

    //m_Dest.Print();
    m_sDestType.lSampleSize = m_Dest.biSizeImage;
    memcpy(&m_sVhdr2->bmiHeader, &m_Dest, sizeof(m_sVhdr2->bmiHeader));
    m_sDestType.cbFormat = sizeof(VIDEOINFOHEADER);
    if (m_sVhdr2->bmiHeader.biCompression == 3)
        m_sDestType.cbFormat += 12;

    if (!m_pDMO_Filter)
	return 0;
    // test accept
    r = m_pDMO_Filter->m_pMedia->vt->SetOutputType(m_pDMO_Filter->m_pMedia, 0, &m_sDestType, DMO_SET_TYPEF_TEST_ONLY);
    if (r != 0)
    {
	if (csp)
	    AVM_WRITE("Win32 video decoder", "Warning: unsupported color space\n");
	else
	    AVM_WRITE("Win32 video decoder", "Warning: unsupported bit depth\n");
        m_Dest = temp;
	m_sDestType.lSampleSize = m_Dest.biSizeImage;
	memcpy(&(m_sVhdr2->bmiHeader), &m_Dest, sizeof(temp));
	m_sDestType.cbFormat = sizeof(VIDEOINFOHEADER);
	if (m_sVhdr2->bmiHeader.biCompression == 3)
	    m_sDestType.cbFormat += 12;

	return -1;
    }
    m_pDMO_Filter->m_pMedia->vt->SetOutputType(m_pDMO_Filter->m_pMedia, 0, &m_sDestType, 0);
    return 0;
}

int DMO_VideoDecoder::SetDirection(int d)
{
    if (m_Dest.biHeight < 0)
	m_Dest.biHeight = -m_Dest.biHeight;

    if (!d && m_bHaveUpsideDownRGB)
	m_Dest.biHeight = -m_Dest.biHeight;

    m_sVhdr2->bmiHeader.biHeight = m_Dest.biHeight;

    //printf("SETDIRECTION  %d\n", m_sVhdr2->bmiHeader.biHeight); m_Dest.Print();
    if (m_pDMO_Filter)
	SetDestFmt(0, 0);
    return 0;
}

int DMO_VideoDecoder::GetValue(const char* name, int* value) const
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

int DMO_VideoDecoder::getCodecValues()
{
    return 0;
}

int DMO_VideoDecoder::SetValue(const char* name, int value)
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

int DMO_VideoDecoder::setCodecValues()
{
    if (!m_iStatus)
	return -1;
    return 0;
}

AVM_END_NAMESPACE;
