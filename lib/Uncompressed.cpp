#include "Uncompressed.h"
#include "avm_except.h"
#include <string.h>
#include <stdlib.h> // sprintf
#include <stdio.h> // sprintf

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "Uncompress"

Unc_Decoder::Unc_Decoder(const CodecInfo& info, const BITMAPINFOHEADER& h, int flip)
    :IVideoDecoder(info, h)
{
    m_Dest = *m_pFormat;

    switch (m_pFormat->biCompression)
    {
    case IMG_FMT_YUY2:
	m_Dest.SetSpace(IMG_FMT_YUY2);
	m_Cap = CAP_YUY2;
        break;
    case IMG_FMT_YV12:
        m_Dest.SetSpace(IMG_FMT_YV12);
	m_Cap = CAP_YV12;
	break;
    case IMG_FMT_I420:
        m_Dest.SetSpace(IMG_FMT_I420);
	m_Cap = CAP_I420;
	break;
    case IMG_FMT_UYVY:
	m_Dest.SetSpace(IMG_FMT_UYVY);
	m_Cap = CAP_UYVY;
        break;
    case IMG_FMT_Y800:
	m_Dest.SetSpace(IMG_FMT_Y800);
        break;
    case 0:
    case mmioFOURCC('D', 'I', 'B', ' '):
    case mmioFOURCC('R', 'G', 'B', ' '):
    case mmioFOURCC('r', 'a', 'w', ' '):
	if (m_Dest.biBitCount == 16)
	    m_Dest.SetBits(15);
        else
	    m_Dest.SetBits(m_Dest.biBitCount);
        /* fall through */
    case 3:
	// image is always stored in bottom-up form
	if (!flip)
	    m_Dest.biHeight *= -1;
	m_Cap = CAP_NONE;
	break;
    default:
        char msg[100];
	sprintf(msg, "unsupported format: 0x%x  (%.4s)!",
		m_pFormat->biCompression, (char*)&m_pFormat->biCompression);
        throw FATAL(msg);
    }
#undef __MODULE__


    // 32bit 8bit B  8bit G  8bit R   8bit reserved
    // 24bit 8bit B  8bit G  8bit R
    // 16bit Windows BI_RGB format always means
    //       (11 - 15) B   (6 - 10) G   (1 - 5) R   0 reserved

    // Image data is always padded to ensure that each scan line
    // begins on an even byte boundary
    // e.g. - 75 pixels width image has one empty column

    /*
    printf("Uncompressed video  %dx%d %d  %d  %d\n",
	   m_pFormat->biWidth, m_pFormat->biHeight,
	   m_pFormat->biSizeImage, m_pFormat->biWidth * m_pFormat->biHeight,
	   m_pFormat->biBitCount);
           */
    dest = m_Dest;
    dest.biWidth = (dest.biWidth + 1) & ~1;
}

Unc_Decoder::~Unc_Decoder()
{
    Stop();
}

int Unc_Decoder::DecodeFrame(CImage* pImage, const void* src, uint_t size,
			     int is_keyframe, bool render, CImage** pOut)
{
    CImage ci(&dest, (const uint8_t*)src, false);
    //printf("DECODER RGB\n"); dest.Print(); pImage->GetFmt()->Print();
    pImage->Convert(&ci);
    pImage->SetQuality(1.0);
    return 0;
}

int Unc_Decoder::SetDestFmt(int bits, fourcc_t csp)
{
    if (!CImage::Supported(csp, bits))
	return -1;

    // we allow to set any dest format 
    if (bits == 0)
        bits = csp;

    switch (bits)
    {
    case 15:
    case 16:
    case 24:
    case 32:
	m_Dest.SetBits(bits);
	return 0;
    }

    m_Dest.SetSpace(bits);
    //printf("SETDESTFMT %d   %.4s\n", bits, (char*) &csp);
    return 0;
}



/**
 *
 * E N C O D E R
 *
 */
Unc_Encoder::Unc_Encoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& header)
    :IVideoEncoder(info), head(header)
{
    switch (info.fourcc)
    {
    case fccYUY2:
    case fccYV12:
    case fccI420:
    case fccUYVY:
	head.SetSpace(info.fourcc);
	break;
    default:
        head.SetBits(info.fourcc);
	head.SetDirection(1);
    }
    chead = head;
    // for YUV - topdown is used - but biHeight in header is positive
    head.biHeight = labs(head.biHeight);
}

Unc_Encoder::~Unc_Encoder()
{
}

int Unc_Encoder::GetOutputSize() const
{
    return head.biSizeImage;
}

const BITMAPINFOHEADER& Unc_Encoder::GetOutputFormat() const
{
    return head;
}

int Unc_Encoder::EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid)
{
    // FIXME - add check for proper orded!!!
    //printf("FMT from -> to\n"); src->GetFmt()->Print(); chead.Print();
    CImage ci(&chead, (uint8_t*)dest, false);
    ci.Convert(src);
    if (size)
	*size = head.biSizeImage;
    if (is_keyframe)
	*is_keyframe = 16;//AVIIF_KEYFRAME
    return 0;
}

AVM_END_NAMESPACE;
