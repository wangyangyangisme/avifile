#include "fillplugins.h"

#include "xvid.h"

#include "videodecoder.h"
#include "videoencoder.h"
#include "configfile.h"
#include "plugin.h"
#include "avm_output.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

#define MAX_QUALITY 6
static const char* strDebug = "XviD plugin";

PLUGIN_TEMP(xvid)

class XVID_VideoDecoder: public IVideoDecoder //, public IRtConfig
{
    void* m_pHandle;
    int m_iLastPPMode;
    int m_iMaxAuto;
    int m_iLastBrightness;
    int m_iLastContrast;
    int m_iLastSaturation;
public:
    XVID_VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
	:IVideoDecoder(info, bh), m_pHandle(0), m_iLastPPMode(0),
	m_iLastBrightness(0), m_iLastContrast(0), m_iLastSaturation(0)
    {
	m_Dest.SetSpace(fccYV12);
	if (flip)
	    m_Dest.biHeight *= -1;
        AVM_WRITE(strDebug, "XviD linux decoder\n");
    }
    virtual ~XVID_VideoDecoder()
    {
	Stop();
    }
    virtual CAPS GetCapabilities() const
    {
	//return CAPS(CAP_YV12|CAP_YUY2|CAP_UYVY|CAP_I420);
	return CAPS(CAP_YV12);
	//return CAPS(CAP_NONE);
    }
    //virtual IRtConfig* GetRtConfig() { return (IRtConfig*) this; }
    virtual int Start()
    {
	int xerr;

	if (m_pHandle)
            return -1;

	XVID_INIT_PARAM init;
	XVID_DEC_PARAM param;

	init.cpu_flags = 0;
	//init.cpu_flags = XVID_CPU_FORCE;// | XVID_CPU_MMX;
	xvid_init(NULL, 0, &init, NULL);

	param.width = m_Dest.biWidth;
	param.height = labs(m_Dest.biHeight);

	xerr = xvid_decore(NULL, XVID_DEC_CREATE, &param, NULL);

	if (xerr != 0)
	{
	    AVM_WRITE(strDebug, "XviD start failed!\n");
            return -1;
	}
	m_pHandle = param.handle;
	return 0;
    }
    virtual int Stop()
    {
	if (!m_pHandle)
	    return -1;

	xvid_decore(m_pHandle, XVID_DEC_DESTROY, 0, 0);
        m_pHandle = 0;
	return 0;
    }
    virtual int DecodeFrame(CImage* pImage, const void* src, uint_t size,
			    int is_keyframe, bool render = true,
			    CImage** pOut = 0)
    {
	XVID_DEC_FRAME param;
	if (!size || !m_pHandle)
	    return 0;
	//AVM_WRITE(strDebug, "csp: 0x%x bitcount: %d height: %d\n", csp, m_Dest.biBitCount, m_Dest.biHeight);
	param.bitstream = (void*) src;
	param.length = size;

	if (pImage)
	{
	    int csp = pImage->GetFmt()->biCompression;
	    switch (csp)
	    {
	    case 0:
	    case 3:
		switch (m_Dest.biBitCount)
		{
		case 16:
		    if (csp == 3)
		    {
			param.colorspace = XVID_CSP_RGB565;
			break;
		    }
		    // 16 bit with csp == 0  is 15bit
		    /* fall through */
		case 15:
		    param.colorspace = XVID_CSP_RGB555;
		    break;
		case 24:
		    param.colorspace = XVID_CSP_RGB24;
		    break;
		case 32:
		    param.colorspace = XVID_CSP_RGB32;
		    break;
		default:
		    return -1;
		}
		if (pImage->GetFmt()->biHeight > 0)
		    param.colorspace |= XVID_CSP_VFLIP;
		// RGB also doesn't seems work;
		param.colorspace = XVID_CSP_USER;
		break;
	    case fccYV12:
		//param.colorspace = XVID_CSP_YV12;
		param.colorspace = XVID_CSP_USER;
		break;
	    case fccI420:
		param.colorspace = XVID_CSP_I420; // untested
		break;
	    case fccYUY2:
		param.colorspace = XVID_CSP_YUY2;
		break;
	    case fccYVYU:
		param.colorspace = XVID_CSP_YVYU;
		break;
	    case fccUYVY:
		param.colorspace = XVID_CSP_UYVY;
		break;
	    default:
		return -1;
	    }
	    //printf("XXXXXXXXXX   %d  %d  %d\n", size, m_Dest.biWidth, param.colorspace);

	    param.stride = pImage->Width(); //+6;
	    param.image = pImage->Data();
	    pImage->SetQuality((float) m_iLastPPMode / MAX_QUALITY);
	}
	else
            param.colorspace = XVID_CSP_NULL;

	if (xvid_decore(m_pHandle, XVID_DEC_DECODE, &param, NULL) != 0)
	    return -1;

	if (param.colorspace == XVID_CSP_USER)
	{
	    // internal format of XVID data - using for YV12
            // for some reason I couldn't read YV12 with CSP setting
	    struct dp
	    {
		const uint8_t *y;
		const uint8_t *u;
		const uint8_t *v;
		int stride_y;
		int stride_uv;
	    };
	    const struct dp* d = (const struct dp*) param.image;
	    const uint8_t* p[3] = { d->y, d->v, d->u };
	    int s[3] = { d->stride_y, d->stride_uv, d->stride_uv };
	    BitmapInfo bi(m_Dest);
	    bi.SetSpace(fccYV12);
	    CImage ci(&bi, (const uint8_t**) p, s, false);
	    pImage->Convert(&ci);
	}

	return size;
    }
    virtual int SetDestFmt(int bits = 24, fourcc_t csp = 0)
    {
	if (csp)
	    bits = csp;

	switch (bits)
	{
	case 15:
	case 16:
	case 24:
	case 32:
	    m_Dest.SetBits(bits);
	    break;
	case fccYV12:
	case fccYUY2:
	    m_Dest.SetSpace(bits);
            break;
	default:
	    return -1;
	}

	if (m_pHandle)
	    Restart();

	return 0;
    }
    virtual int GetValue(const char* name, int& value) const
    {
/*
	if (strcmp(name, strPostProcessing) == 0)
	    value = m_iLastPPMode;
	else if (strcmp(name, strMaxAuto) == 0)
	    value = m_iMaxAuto;
	else if (strcmp(name, strBrightness) == 0)
	    value = m_iLastBrightness;
	else if (strcmp(name, strContrast) == 0)
	    value = m_iLastContrast;
	else if (strcmp(name, strSaturation) == 0)
	    value = m_iLastSaturation;
	    else
*/
	    return -1;

	return 0;
    }
    virtual int SetValue(const char* name, int value)
    {
	return -1;
    }
};

class XVID_VideoEncoder: public IVideoEncoder
{
    BITMAPINFOHEADER m_bh;
    BITMAPINFOHEADER m_obh;
    unsigned int m_uiFrames;
    void* m_pHandle;
    int m_iQuant;
    bool m_bRtMode;
    int general;
    int motion;
public:
    XVID_VideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
	:IVideoEncoder(info), m_bh(bh), m_obh(bh), m_pHandle(0), m_bRtMode(false)
    {
	m_obh.biCompression = fccDIVX;
	m_obh.biHeight = labs(m_obh.biHeight);
	AVM_WRITE(strDebug, "XviD linux encoder\n");
    }
    virtual ~XVID_VideoEncoder()
    {
	Stop();
    }
    virtual int EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid=0)
    {
	int xerr;
	XVID_ENC_FRAME param;
	XVID_ENC_STATS result;
	param.bitstream = dest;

	switch (m_bh.biCompression)
	{
	case 0:
	    param.colorspace = XVID_CSP_RGB24;
	    break;
	case fccYUY2:
	    param.colorspace = XVID_CSP_YUY2;
	    break;
	case fccYV12:
	    param.colorspace = XVID_CSP_YV12;
	    break;
	case fccI420:
	    param.colorspace = XVID_CSP_I420;
	    break;
	}

	param.image = (void*) src->Data();
	param.length = -1; 	// this is written by the routine
	param.quant = 0;
	param.intra = (!m_uiFrames++) ? 1 : -1;

#if API_VERSION >= (2 << 16)
	param.general = general;
        param.motion = motion;
#endif
	if (m_bRtMode)
	{
	    param.quant=m_iQuant;
            // let encoder decide if frame is INTER/INTRA
	    param.intra=-1;
	    xerr = xvid_encore(m_pHandle, XVID_ENC_ENCODE, &param, &result);
	}
	else
	{
	    xerr = xvid_encore(m_pHandle, XVID_ENC_ENCODE, &param, &result);
	}

	//printf("%d   %d  %d\n", param.intra, param.colorspace, param.length);
	// xframe.quant = QUANTI;	// is quant != 0, use a fixed quant (and ignore bitrate)

	if (is_keyframe)
	    *is_keyframe = param.intra ? 16 : 0;
	if (size)
	    *size = param.length;
	return 0;
    }
    virtual int GetOutputSize() const
    {
	return m_bh.biWidth * labs(m_bh.biHeight) * 4;
    }
    virtual const BITMAPINFOHEADER& GetOutputFormat() const
    {
	return m_obh;
    }
    virtual int GetQuality() const
    {
	return 0;//RegReadInt(strXviDplugin, "quality2", 8500);
    }
    virtual int SetQuality(int quality)
    {
	AVM_WRITE(strDebug, "quality: %d\n", quality);
	//RegWriteInt(strXviDplugin, "quality2", quality);
	return 0;
    }
    virtual int Start()
    {
	int xerr;

	XVID_INIT_PARAM xinit;
	XVID_ENC_PARAM param;

	m_uiFrames = 0;
	xinit.cpu_flags = 0;
	xvid_init(NULL, 0, &xinit, NULL);

	memset(&param, 0, sizeof(param));
	param.width = m_bh.biWidth;
	param.height = labs(m_bh.biHeight);

	param.fincr = 100000;
	param.fbase = 2500000;
        int t;

	PluginGetAttrInt(m_Info, xvidstr_rc_bitrate, &t);

#if API_VERSION >= ((2 << 16) | (1))
	param.rc_bitrate = t; // such API change are not funny at all
#else
	param.bitrate = t;
#endif

#if API_VERSION >= (2 << 16)
	PluginGetAttrInt(m_Info, xvidstr_rc_buffer, &t);

#if API_VERSION >= ((2 << 16) | (1))
	param.rc_buffer = t;
#else
	param.rc_buffersize = t;
#endif
	PluginGetAttrInt(m_Info, xvidstr_quant_type, &t);
	switch (t)
	{
	default:
	case 0: general = XVID_H263QUANT; break;
	case 1: general = XVID_MPEGQUANT; break;
	}

#if API_VERSION >= ((2 << 16) | (1))
	PluginGetAttrInt(m_Info, xvidstr_inter4v, &t);
	if (t) general |= XVID_INTER4V;

	PluginGetAttrInt(m_Info, xvidstr_diamond_search, &t);
	if (t) general |= PMV_HALFPELDIAMOND16 | PMV_HALFPELDIAMOND8;

	PluginGetAttrInt(m_Info, xvidstr_adaptive_quant, &t);
	if (t) general |= XVID_ADAPTIVEQUANT;
#endif
	PluginGetAttrInt(m_Info, xvidstr_halfpel, &t);
        if (t) general |= XVID_HALFPEL;

#ifdef XVID_INTERLACING
	PluginGetAttrInt(m_Info, xvidstr_interlacing, &t);
        if (t) general |= XVID_INTERLACING;
#endif
	PluginGetAttrInt(m_Info, xvidstr_lum_masking, &t);
        if (t) general |= XVID_LUMIMASKING;

        // table from the xvid sample source code
	static const int motion_presets[7] =
	{
	    0,
	    PMV_EARLYSTOP16,
	    PMV_EARLYSTOP16,
	    PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,
	    PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,
	    PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8
	    | PMV_HALFPELREFINE8,
	    PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EXTSEARCH16
	    | PMV_EARLYSTOP8 | PMV_HALFPELREFINE8 | PMV_EXTSEARCH8
#ifdef XVID_INTERLACING
	    | PMV_USESQUARES16 | PMV_USESQUARES8
#endif
	};
	PluginGetAttrInt(m_Info, xvidstr_motion_search, &t);
	motion = (t >= 0 && t < 7) ? motion_presets[t] : 0;

#if 0
	// Motion estimation options
	PluginGetAttrInt(m_Info, xvidstr_me_zero, &t);
        if (t) motion |= XVID_ME_ZERO;

	PluginGetAttrInt(m_Info, xvidstr_me_logarithmic, &t);
	if (t) motion |= XVID_ME_LOGARITHMIC;

	PluginGetAttrInt(m_Info, xvidstr_me_fullsearch, &t);
        if (t) motion |= XVID_ME_FULLSEARCH;
#endif // undef

#ifdef XVID_ME_EPZS
	PluginGetAttrInt(m_Info, xvidstr_me_pmvfast, &t);
        if (t) motion |= XVID_ME_PMVFAST;

	PluginGetAttrInt(m_Info, xvidstr_me_epzs, &t);
	if (t) motion |= XVID_ME_EPZS;
#endif
#else
	PluginGetAttrInt(m_Info, xvidstr_rc_period, &param.rc_period);
	PluginGetAttrInt(m_Info, xvidstr_rc_reaction_period, &param.rc_reaction_period);
	PluginGetAttrInt(m_Info, xvidstr_rc_reaction_ratio, &param.rc_reaction_ratio);
	PluginGetAttrInt(m_Info, xvidstr_motion_search, &param.motion_search);
#endif
	PluginGetAttrInt(m_Info, xvidstr_max_quantizer, &param.max_quantizer);
	PluginGetAttrInt(m_Info, xvidstr_min_quantizer, &param.min_quantizer);
	PluginGetAttrInt(m_Info, xvidstr_max_key_interval, &param.max_key_interval);
        // fast deinterlace
        // use bidirectional coding
        // flag to enable overlapped block motion compensation mode
#if 0
	if (param.quality == 1)
	{
	    m_bRtMode=true;
	    int quality=GetQuality();
	    m_iQuant=int(1+.003*(10000-quality));
	    AVM_WRITE(strDebug, "New quant: %d\n", m_iQuant);
	    if(m_iQuant>31)
		m_iQuant=31;
	    if(m_iQuant<1)
		m_iQuant=1;
	}
	else
	    m_bRtMode=false;
#endif

	xerr = xvid_encore(NULL, XVID_ENC_CREATE, &param, NULL);
	m_pHandle = param.handle;

	return xerr;
    }
    virtual int Stop()
    {
	if (!m_pHandle)
	    return -1;
	int err = xvid_encore(m_pHandle, XVID_ENC_DESTROY, NULL, NULL);
	m_pHandle = 0;
	return err;
    }
};

static IVideoEncoder* xvid_CreateVideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
{
    if (bh.biCompression == 0 && bh.biBitCount != 24)
    {
	xvid_error_set("unsupported input bit depth");
        return 0;
    }

    switch (bh.biCompression)
    {
    case 0:
    case fccYUY2:
    case fccYV12:
    case fccI420:
    case fccDIVX:
    case fccXVID:
	break;
    default:
	xvid_error_set("unsupported input format");
        return 0;
    }

    return new XVID_VideoEncoder(info, compressor, bh);
}

static IVideoDecoder* xvid_CreateVideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
{
    if (bh.biSize < 40)
    {
	xvid_error_set("unsupported biSize");
	return 0;
    }

    return new XVID_VideoDecoder(info, bh, flip);
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_xvid;

avm::codec_plugin_t avm_codec_plugin_xvid =
{
    PLUGIN_API_VERSION,

    0,
    0, 0,
    avm::PluginGetAttrInt,
    avm::PluginSetAttrInt,
    0, 0, // attrs
    avm::xvid_FillPlugins,
    0, 0, // audio
    avm::xvid_CreateVideoDecoder,
    avm::xvid_CreateVideoEncoder,
};
