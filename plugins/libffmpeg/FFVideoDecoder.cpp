#include "FFVideoDecoder.h"
#include "fillplugins.h"
#include "plugin.h"
#include "avm_output.h"

#include <string.h>
#include <stdlib.h> // free
#include <stdio.h>

#if 0
#define FFTIMING(a) a
#include "avm_cpuinfo.h"
#include "utils.h"
static float ftm = 0;
#else
#define FFTIMING(a)
#endif

#define Debug if (0)
AVM_BEGIN_NAMESPACE;

FFVideoDecoder::FFVideoDecoder(AVCodec* av, const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
    :IVideoDecoder(info, bh), m_pAvCodec(av), m_pAvContext(0),
    m_Caps((CAPS)(CAP_YV12 | CAP_ALIGN16)), m_uiBuffers(0), m_bRestart(true),
    m_Order(20), m_pImg(0), m_bUsed(false)
{
    m_Dest.SetSpace(fccYV12);
    // not supported if (flip) m_Dest.biHeight *= -1;
    Flush();
    m_pAvStream = (struct AVStream*)bh.biXPelsPerMeter; Debug printf("Context %p\n", m_pAvStream);
    if (1 && m_pFormat->biCompression == fccHFYU)
    {
	// for now disabled
	m_pAvCodec->capabilities &= ~(CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1);
	AVM_WRITE(m_Info.GetPrivateName(), "if you have troubles - use Win32 codec instead\n");
	m_Caps = (CAPS) (m_Caps | CAP_YUY2);
    }
}

FFVideoDecoder::~FFVideoDecoder()
{
    Stop();
    FFTIMING(printf("ftm %f\n", ftm));
}

void FFVideoDecoder::Flush()
{
    Debug printf("\nFF: Flush\n\n");
    if (m_pAvContext)
	avcodec_flush_buffers(m_pAvContext);
    m_bFlushed = true;
    m_Order.clear();
    m_iAgeB = m_iAgeIP[0] = m_iAgeIP[1] = 256*256*256*64;
}

// callback to draw horizontal slice
static void draw_slice(struct AVCodecContext *avctx,
		       const AVFrame *src, int offset[4],
		       int y, int type, int height)
{
    FFVideoDecoder* d = (FFVideoDecoder*) avctx->opaque;
    ci_surface_t ci;
    ci.m_pPlane[0] = src->data[0] + offset[0];
    ci.m_pPlane[1] = src->data[2] + offset[2];
    ci.m_pPlane[2] = src->data[1] + offset[1];
    ci.m_iStride[0] = src->linesize[0];
    ci.m_iStride[1] = src->linesize[2];
    ci.m_iStride[2] = src->linesize[1];
    ci.m_iWidth = ci.m_Window.w = avctx->width;
    ci.m_iHeight = ci.m_Window.h = height;
    ci.m_Window.x = 0;
    ci.m_Window.y = y;
    ci.m_iFormat = IMG_FMT_YV12;
    //printf("DrawSlice %p   %p    y:%d w:%d h:%d\n", d->m_pImg, src, ci.m_Window.y, ci.m_Window.w, height);
    d->m_pImg->Slice(&ci);
}

// callback to supply rendering buffer to ffmpeg
static int get_buffer(AVCodecContext* avctx, AVFrame* pic)
{
    FFVideoDecoder* d = (FFVideoDecoder*) avctx->opaque;
    CImage* pImage = d->m_pImg;
    d->m_bUsed = true;
    if (avctx->pix_fmt != PIX_FMT_YUV420P || !pImage || !d->m_bDirect)
    {
	Debug printf("FF: Unsupported pixel format for Dr1 %d\n", avctx->pix_fmt); //abort();
        return avcodec_default_get_buffer(avctx, pic);
    }

    Debug printf("FF: GetBuffer %p  %dx%d  %d  %p:%p:%p  %f\n", pImage, avctx->width, avctx->height, pic->pict_type,
		 pImage->Data(0), pImage->Data(2), pImage->Data(1), pImage->m_lTimestamp / 1000000.0);

    pic->opaque = pImage;
    pic->data[0] = pImage->Data(0);
    pic->data[1] = pImage->Data(2);
    pic->data[2] = pImage->Data(1);
    pic->linesize[0] = pImage->Stride(0);
    // Note: most ffmpeg codecs linsize[1] == linesize[2] !
    pic->linesize[1] = pImage->Stride(2);
    pic->linesize[2] = pImage->Stride(1);
    pic->pts = pImage->m_lTimestamp;
    pic->type = FF_BUFFER_TYPE_USER;
    pImage->m_iType = pic->pict_type;
    //pic->age = pic->coded_picture_number - pImage->m_iAge;
    //pImage->m_iAge = (pic->pict_type == FF_B_TYPE) ?
    //pImage->m_iAge = (pic->reference) ?
    //    -256*256*256*64 : pic->coded_picture_number;

    d->m_iAgeIP[0]++;
    pic->age = d->m_iAgeIP[0] - pImage->m_iAge;
    pImage->m_iAge = (pic->pict_type == FF_B_TYPE) ?
	256*256*256*64 : d->m_iAgeIP[0];
    if (pic->age < 1)
	pic->age = 256*256*256*64;

#if 0
    // mplayer code
    if (pic->reference)
    {
        pic->age = d->m_iAgeIP[0];
	d->m_iAgeIP[0] = d->m_iAgeIP[1] + 1;
        d->m_iAgeIP[1] = 1;
        d->m_iAgeB++;
    }
    else
    {
	pic->age = d->m_iAgeB;
        d->m_iAgeIP[0]++;
        d->m_iAgeIP[1]++;
        d->m_iAgeB = 1;
    }
#endif
    //printf("Age %d  %d   cp:%d   %p\n", pic->age, pImage->m_iAge, pic->coded_picture_number, pImage);
    //printf("PictType %d   %d\n", pic->pict_type, pic->reference);
    //printf("%p %p %p  %d  %d\n", avctx->dr_buffer[0], avctx->dr_buffer[1], avctx->dr_buffer[2], avctx->dr_stride, avctx->dr_uvstride);
    return 0;
}

static void release_buffer(struct AVCodecContext* avctx, AVFrame* pic)
{
    if (pic->type == FF_BUFFER_TYPE_USER)
    {
	FFVideoDecoder* d = (FFVideoDecoder*) avctx->opaque;
	d->m_pReleased = (CImage*) pic->opaque;
	Debug printf("FF: Released buffer %p  %p\n", pic->opaque, pic);
	for (int i = 4; i >= 0; i--)
	    pic->data[i]= NULL;
	pic->opaque = NULL;
    }
    else
    {
	Debug printf("******************************\n");
	avcodec_default_release_buffer(avctx, pic);
    }
}

int FFVideoDecoder::DecodeFrame(CImage* pImage,	const void* src, uint_t size,
				int is_keyframe, bool render, CImage** pOut)
{
    if (m_bFlushed && !is_keyframe)
	Stop();
    m_bFlushed = false;
    FFTIMING(int64_t ts = longcount());
    if ((pImage && pImage->GetAllocator()
	 && m_uiBuffers != pImage->GetAllocator()->GetImages())
	|| m_bRestart)
    {
	//printf("RES %d  %d  %d\n", m_bRestart, m_uiBuffers, pImage->GetAllocator()->GetImages());
	Stop();
    }

    //printf("FFMPEG space  \n"); m_Dest.Print(); pImage->GetFmt()->Print();
    if (!m_pAvContext)
    {
	m_pAvContext = avcodec_alloc_context();
        // for autodetection errors
	m_pAvContext->codec_tag = m_pFormat->biCompression;
	m_pAvContext->bits_per_sample = m_pFormat->biBitCount;
        m_pAvContext->width = m_Dest.biWidth;
	m_pAvContext->height = (m_Dest.biHeight < 0) ? -m_Dest.biHeight : m_Dest.biHeight;
	m_pAvContext->get_buffer = avcodec_default_get_buffer;
	m_pAvContext->release_buffer = avcodec_default_release_buffer;

	if (m_pFormat->biSize > sizeof(BITMAPINFOHEADER)
#if 0
	    // use then for all FOURCC - fix ffmpeg
	    && (m_pFormat->biCompression == mmioFOURCC('A', 'V', 'R', 'n')
		|| m_pFormat->biCompression == fccMJPG
		|| m_pFormat->biCompression == mmioFOURCC('A', 'V', 'S', '1')
		|| m_pFormat->biCompression == mmioFOURCC('A', 'V', 'S', '2')
		|| m_pFormat->biCompression == mmioFOURCC('M', '4', 'S', '2')
		|| m_pFormat->biCompression == mmioFOURCC('M', 'P', '4', 'S')
		|| m_pFormat->biCompression == mmioFOURCC('S', 'V', 'Q', '3')
		|| m_pFormat->biCompression == fccHFYU
		|| m_pFormat->biCompression == mmioFOURCC('W', 'M', 'V', '2'))
#endif
	   )
	{
            m_pAvContext->extradata_size = m_pFormat->biSize - sizeof(BITMAPINFOHEADER);
	    m_pAvContext->extradata = (char*) m_pFormat + sizeof(BITMAPINFOHEADER);
	    if (m_pAvContext->extradata_size > 40)
		m_pAvContext->flags |= CODEC_FLAG_EXTERN_HUFF; // somewhat useless
	}

	m_uiBuffers = (pImage && pImage->GetAllocator()) ? pImage->GetAllocator()->GetImages() : 0;
	//printf("IMAGES %d\n", m_uiBuffers);
	// for SVQ1 - align 63 will be needed FIXME
	int v = 0;
	const char* drtxt = "doesn't support DR1\n";

	m_bDirect = false;
	if (m_pAvCodec->capabilities & CODEC_CAP_DR1)
	{
	    drtxt = "not using DR1\n";
	    if (pImage)
	    {
		unsigned require = 2;
		switch (m_Info.fourcc)
		{
		case fccDVSD:
		    require = 1;
		    break;
		}
		if ((m_Info.FindAttribute(ffstr_dr1)
		     && PluginGetAttrInt(m_Info, ffstr_dr1, &v) == 0 && v)
		    && m_uiBuffers >= require
		    && pImage->Format() == IMG_FMT_YV12
		    && ((pImage->Width() | pImage->Height()) & 0xf) == 0)
		{
		    // for DR we needs some special width aligment
		    // also there are some more limitation
		    m_pAvContext->flags |= CODEC_FLAG_EMU_EDGE;
		    drtxt = "using DR1\n";
                    m_bDirect = true;

		    m_pAvContext->get_buffer = get_buffer;
		    m_pAvContext->release_buffer = release_buffer;
		}
	    }
	}
        if (m_bRestart)
	    AVM_WRITE(m_Info.GetPrivateName(), drtxt);
	m_bRestart = false;

	if (m_Info.fourcc == RIFFINFO_MPG1
	    && m_pAvCodec->capabilities & CODEC_CAP_TRUNCATED)
	    m_pAvContext->flags |= CODEC_FLAG_TRUNCATED;

	//m_pAvContext->error_resilience = 2;
        //m_pAvContext->error_concealment = 3;
	//m_pAvContext->workaround_bugs = FF_BUG_AUTODETECT;
	AVCodec* codec = avcodec_find_decoder_by_name(m_Info.dll);
#if 0
	if (codec->options) {
	    avm::vector<AttributeInfo>::const_iterator it;
	    for (it = m_Info.decoder_info.begin();
		 it != m_Info.decoder_info.end(); it++)
	    {
		char b[100];
                float f;
		b[0] = 0;
		switch (it->GetKind())
		{
		case AttributeInfo::Integer:
		    PluginGetAttrInt(m_Info, it->GetName(), &v);
		    sprintf(b, "%s=%d", it->GetName(), v);
		    break;
		case AttributeInfo::Float:
		    PluginGetAttrFloat(m_Info, it->GetName(), &f);
		    sprintf(b, "%s=%f", it->GetName(), f);
		    break;
		default:
                    ;
		}
		Debug printf("pass options '%s':   %s\n", it->GetName(), b);
		avoption_parse(m_pAvContext, codec->options, b);
	    }
	}
#endif
/*
	static const struct fftable {
	    const char* attr;
	    int flag;
	} fftab[] = {
	    { ffstr_bug_old_msmpeg4, FF_BUG_OLD_MSMPEG4 },
	    { ffstr_bug_xvid_ilace, FF_BUG_XVID_ILACE },
	    { ffstr_bug_ump4, FF_BUG_UMP4 },
	    { ffstr_bug_no_padding, FF_BUG_NO_PADDING },
	    { ffstr_bug_ac_vlc, FF_BUG_AC_VLC },
	    { ffstr_bug_qpel_chroma, FF_BUG_QPEL_CHROMA },
	    { 0, 0 }
	};
	for (const struct fftable* p = fftab; p->attr; p++)
	{
	    if (m_Info.FindAttribute(p->attr)
		&& PluginGetAttrInt(m_Info, p->attr, &v) == 0 && v)
		m_pAvContext->workaround_bugs |= p->flag;
	}
*/
	if (avcodec_open(m_pAvContext, m_pAvCodec) < 0)
	{
	    AVM_WRITE(m_Info.GetPrivateName(), "WARNING: FFVideoDecoder::DecodeFrame() can't open avcodec\n");
            Stop();
	    return -1;
	}
    }

    // try using draw_horiz_band if DR1 is unsupported
    m_pAvContext->draw_horiz_band =
	(!m_bDirect && pImage && pImage->Format() == IMG_FMT_YV12
	 && (m_pAvCodec->capabilities & CODEC_CAP_DRAW_HORIZ_BAND)
	 && !pImage->Direction() && render) ? draw_slice : 0;
    m_pAvContext->opaque = this;

    Debug printf("FF: Decode start %p\n", pImage);
    m_pImg = pImage;
    m_bUsed = false;
    m_pReleased = 0;
    AVFrame pic;
    int got_picture = 0;
    int hr = avcodec_decode_video(m_pAvContext, &pic, &got_picture,
				  (unsigned char*) src, size);
    //printf("DECFF got_picture  %d  %p   del:%d  hr:%d size:%d\n", got_picture, src, m_pAvContext->delay, hr, size);
    //printf("PictType  %d\n", m_pAvContext->pict_type);
    //static int ctr=0; printf("WIDTH %dx%d  %d  r:%d\n", m_pAvContext->width, m_pAvContext->height, ctr++, m_pAvContext->pict_type);
    Debug printf("FF: Decoded used=%d  got_pic=%d   res=%d\n", m_bUsed, got_picture, hr);
    if (hr < 0)
    {
	AVM_WRITE(m_Info.GetPrivateName(), "WARNING: FFVideoDecoder::DecodeFrame() hr=%d\n", hr);
	return hr;
    }
    if (!(m_pAvContext->flags & CODEC_FLAG_TRUNCATED))
    {
	hr = size;
        //m_bUsed = true;
    }

    if (!m_bDirect || m_bUsed)
    {
	unsigned i = m_Order.size();
	int64_t timestamp;
        uint_t position;
	if (pImage)
	{
	    timestamp = pImage->m_lTimestamp;
	    position = pImage->m_uiPosition;
	    while (i > 0)
	    {
		// keep data ordered by timestamps
		if (timestamp > m_Order[i - 1].timestamp)
		    break;
		i--;
	    }
	}
	else
	{
	    timestamp = 0;
            position = 0;
	}
	Debug printf("FF: Insert  %d    %lld   %lld   gotpic:%d\n", i, timestamp, m_Order[i].timestamp, got_picture);
	m_Order.insert(i, fpair(timestamp, position));

	if (m_bDirect)
	    hr |= NEXT_PICTURE;
    }

    Debug printf("FF: r=0x%x  sz=%d  %d  b:%d  img:%p  out:%p\n", hr, size, got_picture, m_bUsed, pImage, pOut);
    Debug printf("FF: frame_size %d  number %d  picnum %d\n", m_pAvContext->frame_size, m_pAvContext->frame_number, m_pAvContext->real_pict_num);
    if (!got_picture)
    {
	Debug printf("FF: NO PICTURE  released=%p\n", m_pReleased);
        if (!m_pReleased)
	    return hr | NO_PICTURE;
        // let's fake got_picture;
	if (!pic.opaque) {
	    pic.type = FF_BUFFER_TYPE_USER;
	    pic.opaque = m_pReleased;
	}
        got_picture = true;
    }

    //printf("CONTEXT %d  %x  %.4s\n", got_picture, m_pAvContext->pix_fmt, (char*)&imfmt);

    if (render && pImage && got_picture && !m_bDirect
	&& !m_pAvContext->draw_horiz_band)
    {
	int imfmt = 0;
	switch (m_pAvContext->pix_fmt)
	{
	case PIX_FMT_BGR24: imfmt = IMG_FMT_BGR24; break;
	case PIX_FMT_RGBA32: imfmt = IMG_FMT_BGR32; break;
	case PIX_FMT_YUV422: imfmt = IMG_FMT_YUY2; break;
	case PIX_FMT_YUV410P: imfmt = IMG_FMT_I410; break;
	case PIX_FMT_YUV411P: imfmt = IMG_FMT_I411; break;
	case PIX_FMT_YUV420P: imfmt = IMG_FMT_I420; break;
	case PIX_FMT_YUV422P: imfmt = IMG_FMT_I422; break;
	case PIX_FMT_YUV444P: imfmt = IMG_FMT_I444; break;
	default: break;
	}
	if (imfmt) {
	    BitmapInfo bi(m_Dest);
	    bi.SetSpace(imfmt);
	    //bi.Print();
	    //printf("FFMPEG SETSPACE\n"); pImage->GetFmt()->Print(); bi.Print();
	    //printf("LINE %d %d %d\n", pic.linesize[0], pic.linesize[1], pic.linesize[2]);
	    //printf("LINE %p %p %p\n", pic.data[0], pic.data[1], pic.data[2]);
	    CImage ci(&bi, (const uint8_t**) pic.data, pic.linesize, false);
	    pImage->Convert(&ci);
	    //pImage->m_iType = pict_type;
	    //printf("xxx %p\n", m_AvPicture.data[0]);
	    //memcpy(pImage->Data(), pic.data[0], 320 * 150);
	} else {
	    AVM_WRITE(m_Info.GetPrivateName(), "Unsupported colorspace: %d, FIXME\n", m_pAvContext->pix_fmt);
            if (pImage)
		pImage->Clear();
	}
    }

    //printf("SWAP  %lld  %lld\n", m_Order.front().timestamp, pImage->m_lTimestamp);
    //printf("SWAP  %d  %d\n", m_Order.front().position, pImage->m_uiPosition);
    //printf("P   %d    %lld\n", p, m_Order[0].timestamp, m_Order.size());

    //printf("PICTYPE %d  %p   %p  %d\n", pic.type, m_pReleased, pOut, FF_BUFFER_TYPE_USER);
#if 1
    if (pOut && pic.opaque &&
	((pic.type == FF_BUFFER_TYPE_USER)
	 || (pic.type == FF_BUFFER_TYPE_COPY)))
    {
	*pOut = (CImage*) pic.opaque;
	(*pOut)->m_lTimestamp = m_Order[0].timestamp;
	(*pOut)->m_uiPosition = m_Order[0].position;
    }
    else if (pImage)
    {
	pImage->m_lTimestamp = m_Order[0].timestamp;
	pImage->m_uiPosition = m_Order[0].position;
    }
#endif
    m_Order.pop();

    //printf("OUT %d %p\n", got_picture, *pOut);

    FFTIMING(ftm += to_float(longcount(), ts));
    Debug printf("FF: pictype %d time:%f  c:%d  d:%d %lld    %p\n", pic.pict_type, (double) pic.pts / 1000000.0,
		 pic.coded_picture_number, pic.display_picture_number, pImage->m_lTimestamp, pic.opaque);
    //pImage->m_lTimestamp = pic.pts / (double) AV_TIME_BASE;
    return hr;
}

int FFVideoDecoder::SetDestFmt(int bits, fourcc_t csp)
{
    if (!CImage::Supported(csp, bits) || csp != fccYV12)
	return -1;

    Restart();
    return 0;
}

int FFVideoDecoder::Stop()
{
    if (m_pAvContext)
    {
	avcodec_close(m_pAvContext);
        free(m_pAvContext);
	m_pAvContext = 0;
    }
    return 0;
}

const avm::vector<AttributeInfo>& FFVideoDecoder::GetAttrs() const
{
    return m_Info.decoder_info;
}

int FFVideoDecoder::GetValue(const char* name, int* value) const
{
    return PluginGetAttrInt(m_Info, name, value);
}
int FFVideoDecoder::SetValue(const char* name, int value)
{
    int r = PluginSetAttrInt(m_Info, name, value);
    m_bRestart = (r == 0);
    return r;
}

AVM_END_NAMESPACE;
