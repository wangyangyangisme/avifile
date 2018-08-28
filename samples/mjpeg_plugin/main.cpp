#include "fillplugins.h"
#include "mjpeg.h"
#include <videodecoder.h>
#include <videoencoder.h>
#include <plugin.h>
#include <image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

// FIXME  mjpeg -> ijpg - with 0.6
PLUGIN_TEMP(mjpeg)

class MJPEG_VideoDecoder: public IVideoDecoder
{
    struct jpeg_decompress_struct* mjpg_dinfo;
    CImage* tci;
public:
    MJPEG_VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& format, int flip)
	:IVideoDecoder(info, format), mjpg_dinfo(0), tci(0)
    {
	m_Dest = format;
	m_Dest.SetBits(24);
	if (flip)
	    m_Dest.biHeight = labs(m_Dest.biHeight);
    }
    ~MJPEG_VideoDecoder()
    {
	Stop();
	if (tci)
            tci->Release();
    }
    virtual int SetDestFmt(int bits = 24, fourcc_t csp = 0)
    {
	if (!CImage::Supported(csp, bits))
	    return -1;

	if (!csp)
	    switch (bits)
	    {
	    case 15:
	    case 16:
	    case 24:
	    case 32:
		m_Dest.SetBits(bits);
		Restart();
		return 0;
	    }
	else
	    m_Dest.SetSpace(csp);

	if (tci)
	    tci->Release();
        tci = 0;
	return 0;
    }
    virtual int DecodeFrame(CImage* dest, const void* src, uint_t size,
			    int is_keyframe, bool render = true,
			    CImage** pOut = 0)
    {
	if (mjpg_dinfo)
	{
	    BitmapInfo bi(dest->GetFmt());
	    bi.SetBits(24);
	    if (tci && !tci->IsFmt(&bi))
	    {
		tci->Release();
		tci = 0;
	    }
	    if (!tci)
		tci = new CImage(&bi);
	    mjpg_bgr_decompress(mjpg_dinfo, tci->Data(), (unsigned char*)src, size);
	    dest->Convert(tci);

	    return 0;
	}
	return -1;
    }
    virtual int Start()
    {
	if (!mjpg_dinfo)
	{
	    mjpg_dinfo = mjpg_dec_bgr_init(m_Dest.biWidth, m_Dest.biHeight);
	}
	return 0;
    }
    virtual int Stop()
    {
	if (mjpg_dinfo)
	{
	    mjpg_dec_cleanup(mjpg_dinfo);
	    mjpg_dinfo = 0;
	}
	return 0;
    }
};


class MJPEG_VideoEncoder: public IVideoEncoder
{
    BitmapInfo header;
    BitmapInfo of;
    struct jpeg_compress_struct* mjpg_cinfo;
    int quality;
public:
    MJPEG_VideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
	:IVideoEncoder(info), mjpg_cinfo(0)
    {
	//if (!CImage::Supported(bh))
	//    throw FATAL("Unsupported video format");
	of = bh;
	header = bh;
	of.biCompression = compressor;
	//of.biHeight = labs(of.biHeight);
    }
    ~MJPEG_VideoEncoder()
    {
	Stop();
    }
    virtual int GetOutputSize() const
    {
	return header.biWidth * labs(header.biHeight + 1) * 4;
    }
    const BITMAPINFOHEADER& GetOutputFormat() const { return of; }
    virtual int EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid=0)
    {
	if ((dest == 0) || (src == 0))
	{
	    if (size)
		*size = 0;
	    return 0;
	}
	const CImage* tmp;
	if (header.biBitCount != 24 || header.biCompression)
	{
	    BitmapInfo supp(header);
	    supp.SetBits(24);
	    supp.biBitCount=24;
	    supp.biCompression=0;
	    tmp = new CImage(src, &supp);
	}
	else
	{
	    tmp = src;//new CImage(src);
	}

	int sz = mjpg_bgr_compress(mjpg_cinfo, (unsigned char*)dest,
				   tmp->Data(), tmp->Pixels());
	if (size)
	    *size = sz;
	if (is_keyframe)
	    *is_keyframe = 16;//AVIIF_KEYFRAME
	if (src != tmp)
	    delete tmp;
	return 0;
    }
    virtual int Start()
    {
	mjpg_cinfo = mjpg_bgr_init(header.biWidth, header.biHeight, quality);
	printf("Create %p\n", mjpg_cinfo);
	return 0;
    }
    virtual int Stop()
    {
	if (mjpg_cinfo)
	{
	    mjpg_cleanup(mjpg_cinfo);
	    mjpg_cinfo = 0;
	}
	return 0;
    }
    virtual int SetQuality(int q)
    {
	quality = q / 100;
	return 0;
    }
    virtual int GetQuality() const
    {
	return quality * 100;;
    }
};


IVideoEncoder* ijpg_CreateVideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
{
    return new MJPEG_VideoEncoder(info, compressor, bh);
}

IVideoDecoder* ijpg_CreateVideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
{
    return new MJPEG_VideoDecoder(info, bh, flip);
}

AVM_END_NAMESPACE;

avm::codec_plugin_t avm_codec_plugin_mjpeg =
{
    PLUGIN_API_VERSION,

    0,
    0, 0,
    0, //avm::mjpeg_GetAttrInt,
    0, //avm::mjpeg_SetAttrInt,
    0, 0, // attrs
    avm::ijpg_FillPlugins,
    0, 0, // audio
    avm::ijpg_CreateVideoDecoder,
    avm::ijpg_CreateVideoEncoder,
};
