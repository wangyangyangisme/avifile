#include "fillplugins.h"

#include "FFAudioDecoder.h"
#include "FFVideoDecoder.h"
#include "FFVideoEncoder.h"

#include "videoencoder.h"
#include "avm_output.h"

#include "plugin.h"

#include <ctype.h>
#include <stdio.h>

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(ffmpeg);

#if 0
class FFVideoDec: public IVideoDecoder
{
public:
    FFVideoDec(AVCodec* av,const CodecInfo& info, const BITMAPINFOHEADER& format, int flip);
        :IVideoDecoder(info, bh), m_pAvCodec(av), m_Caps(CAP_YV12), m_bOpened(false)
    {
	m_Dest.SetSpace(fccYV12);
	if (flip)
	    m_Dest.biHeight *= -1;

	if (avcodec(0, AVCODEC_OPEN_BY_FCC, (void*)info.fourcc, &m_pHandle) < 0)
	    throw;
	AVM_WRITE("FFMPEG video decoder", "opened\n");
    }
    virtual ~FFVideoDec();
    {
	Stop();
    }
    virtual int DecodeFrame(CImage* pImage, const void* src, uint_t size, int is_keyframe, bool render = true);
    {
	if (!m_bOpened)
	{
	    memset(&m_AvContext, 0, sizeof(AVCodecContext));
	    m_AvContext.width = m_Dest.biWidth;
	    m_AvContext.height = labs(m_Dest.biHeight);
	    m_AvContext.pix_fmt = PIX_FMT_YUV422;
	    if (avcodec_open(&m_AvContext, m_pAvCodec) < 0)
	    {
		AVM_WRITE("FFMPEG video decoder", "WARNING: FFVideoDecoder::DecodeFrame() can't open avcodec\n");
		return -1;
	    }
	    else
		m_bOpened = true;
	}
	while (size > 0)
	{
	    int got_picture = 0;
	    int hr = avcodec_decode_video(&m_AvContext, &m_AvPicture,
					  &got_picture, (unsigned char*) src, size);
	    if (hr < 0)
	    {
		AVM_WRITE("FFMPEG video decoder", "WARNING: FFVideoDecoder::DecodeFrame() hr=%d\n", hr);
		return hr;
	    }

	    //printf("DECFF got_picture  %d  %p  %d   del:%d  hr:%d\n"  , got_picture, src, size, m_AvContext.delay, hr);
	    if (!got_picture)
		return hr + 1;

	    int imfmt;
	    switch (m_AvContext.pix_fmt)
	    {
	    case PIX_FMT_YUV420P: imfmt = IMG_FMT_I420; break;
	    case PIX_FMT_YUV422: imfmt = IMG_FMT_I420; break;
	    case PIX_FMT_RGB24: imfmt = IMG_FMT_RGB24; break;
	    case PIX_FMT_BGR24: imfmt = IMG_FMT_BGR24; break;
	    case PIX_FMT_YUV422P: imfmt = IMG_FMT_Y422; break;
	    case PIX_FMT_YUV444P: imfmt = IMG_FMT_IYU2; break;
	    default:
		AVM_WRITE("FFMPEG video decoder", "Unknown colorspace\n");
		return -1;
	    }
	    //printf("CONTEXT %d  %x  %.4s\n", got_picture, m_AvContext.pix_fmt, (char*)&imfmt);

	    if (got_picture && pImage && render)
	    {
		BitmapInfo bi(m_Dest);
		bi.SetSpace(imfmt);
		//printf("LINE %d %d %d\n", m_AvPicture.linesize[0], m_AvPicture.linesize[1], m_AvPicture.linesize[2]);
		//printf("LINE %p %p %p\n", m_AvPicture.data[0], m_AvPicture.data[1], m_AvPicture.data[2]);
		CImage ci(&bi, (const uint8_t**) m_AvPicture.data, m_AvPicture.linesize, false);
		pImage->Convert(&ci);
		//printf("xxx %p\n", m_AvPicture.data[0]);
		//memcpy(pImage->Data(), m_AvPicture.data[0], 320 * 50);
	    }

	    size -= hr;
	    src = (const char*) src + hr;
	}

	return 0;
    }
    virtual CAPS GetCapabilities() const { return m_Caps; }
    virtual void Flush();
    {
	if (m_bOpened)
	    avcodec_flush_buffers(&m_AvContext);
    }
    virtual int SetDestFmt(int bits = 24, fourcc_t csp = 0);
    {
	if (!CImage::Supported(csp, bits) || csp != fccYV12)
	    return -1;

	Restart();
	return 0;
    }
    virtual int Stop();
    {
	if (m_bOpened)
	    avcodec_close(&m_AvContext);
	m_bOpened = false;
	return 0;
    }

protected:
    void* m_pHandle;
    AVPicture m_AvPicture;
    CAPS m_Caps;
    bool m_bOpened;
    bool m_bFlushed;
};
#endif


// figure out which codec we need to use
static void av_init()
{
    static int is_init = 0;
    if (!is_init)
    {
	avcodec_init();
	avcodec_register_all();
	is_init++;
    }
}

static IVideoEncoder* ffmpeg_CreateVideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
{
    av_init();
    AVCodec* av = avcodec_find_encoder_by_name(info.dll.c_str());
    if (av)
    {
	switch (bh.biCompression)
	{
	case 0:
	case fccYUY2:
	case fccYV12:
	case fccI420:
	case fccDIVX:
	case fccDX50:
	    return new FFVideoEncoder(av, info, compressor, bh);
	default:
	    ffmpeg_error_set("unsupported input format");
	}
    }
    else
	ffmpeg_error_set("video codec not found");
    return 0;
}

static IVideoDecoder* ffmpeg_CreateVideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
{
    av_init();
    AVM_WRITE("FFMPEG video decoder", "looking %s\n", info.dll.c_str());
    AVCodec* av = avcodec_find_decoder_by_name(info.dll.c_str());
    if (av)
	return new FFVideoDecoder(av, info, bh, flip);
    ffmpeg_error_set("video codec not found");
    return 0;
}

static IAudioDecoder* ffmpeg_CreateAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* fmt)
{
    av_init();
    AVCodec* av = avcodec_find_decoder_by_name(info.dll.c_str());
    if (av)
	return new FFAudioDecoder(av, info, fmt);
    ffmpeg_error_set("audio codec not found");
    return 0;
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_ffmpeg;

avm::codec_plugin_t avm_codec_plugin_ffmpeg =
{
    PLUGIN_API_VERSION,

    0,
    avm::PluginGetAttrFloat,
    avm::PluginSetAttrFloat,
    avm::PluginGetAttrInt,
    avm::PluginSetAttrInt,
    avm::PluginGetAttrString,
    avm::PluginSetAttrString,
    avm::ffmpeg_FillPlugins,
    avm::ffmpeg_CreateAudioDecoder,
    0,
    avm::ffmpeg_CreateVideoDecoder,
    avm::ffmpeg_CreateVideoEncoder,
};
