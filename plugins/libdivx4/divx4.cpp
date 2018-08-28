#include "fillplugins.h"
#include "videodecoder.h"
#include "videoencoder.h"
#include "avm_fourcc.h"
#include "configfile.h"
#include "avm_output.h"
#include "plugin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

#ifdef DIVXBIN
#include <dlfcn.h>
static const char* divxdecore = "libdivxdecore.so.0";
static const char* divxencore = "libdivxencore.so.0";
#endif /* DIVXBIN */

PLUGIN_TEMP(divx4);

static const int DIVX4_MAX_QUALITY = 6;
static const char* strDebugEnc = "DivX4 plugin";

#ifndef DECORE_VERSION
#define DECORE_VERSION 0
#endif

#if DECORE_VERSION >= 20021112
typedef void* divxhandle_t;
typedef int divxopts_t;
#else
extern "C" int quiet_encore;
typedef unsigned long divxhandle_t;
typedef unsigned long divxopts_t;
#endif


#ifdef HAVE_LIBDIVXDECORE
class DIVX_VideoDecoder: public IVideoDecoder, public IRtConfig
{
    void* m_pDll;
    divxhandle_t m_Handle;
    int m_iLastPPMode;
    int m_iMaxAuto;
    int m_iLastBrightness;
    int m_iLastContrast;
    int m_iLastSaturation;
//    bool m_bCompatWMV2;
    bool m_bCompat311;
    bool m_bSetFlg;
    bool m_bFlip;
    char m_cFormatBuf[128];
    avm::vector<AttributeInfo> m_YvAttrs;
    typedef int STDCALL (*pDecoreFun)(divxhandle_t handle,  // handle  - the handle of the calling entity, must be unique
			     divxopts_t dec_opt, // dec_opt - the option for docoding, see below
			     void *param1,	    // param1  - the parameter 1 (it's actually meaning depends on dec_opt
			     void *param2);	    // param2  - the parameter 2 (it's actually meaning depends on dec_opt
    pDecoreFun m_pDecore;
public:
    DIVX_VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh,
		      int flip) :
    IVideoDecoder(info, bh), m_pDll(0), m_Handle(0), m_iLastPPMode(0),
	m_iLastBrightness(0), m_iLastContrast(0), m_iLastSaturation(0),
	m_bSetFlg(true), m_bFlip(flip)
    {
    }
    virtual ~DIVX_VideoDecoder()
    {
	Stop();
#ifdef DIVXBIN
	if (m_pDll)
	    dlclose(m_pDll);
#endif
    }
    int init()
    {
#ifdef DIVXBIN
	if (!m_pDll) {
	    m_pDll = dlopen(divxdecore, RTLD_LAZY);
	    if (!m_pDll)
	    {
		divx4_error_set("can't open %s", divxdecore);
		return -1;
	    }
	}
	m_pDecore = (pDecoreFun) dlsym(m_pDll, "decore");
	if (!m_pDecore)
	{
	    divx4_error_set("can't resolve %s:decore", divxdecore);
            return -1;
	}

	int ver = m_pDecore(0, DEC_OPT_VERSION, 0, 0);
	if ((DECORE_VERSION == 20020322 && ver != 2)) {
	    divx4_error_set("plugin version %d is different from %s library version %d!", DECORE_VERSION, divxdecore, ver);
            return -1;
	}
#else
	m_pDecore = decore;
#endif
	switch(m_Dest.biCompression)
	{
#ifdef USE_311_DECODER
	case fccdiv3:
	case fccDIV3:
	case fccdiv4:
	case fccDIV4:
	case fccdiv5:
	case fccDIV5:
	case fccdiv6:
	case fccDIV6:
	case fccMP41:
	case fccMP43:
	    m_bCompat311=true;
//	    m_bCompatWMV2=false;
	    break;
//	case fccWMV2:
//	    m_bCompat311=true;
//	    m_bCompatWMV2=true;
//	    break;
#endif
	default:
	    m_bCompat311=false;
	    break;
	}
	memset(m_cFormatBuf, 0, sizeof(m_cFormatBuf));
	memcpy(m_cFormatBuf,  ((const char*)&m_Dest) + sizeof(BITMAPINFOHEADER),
	       m_Dest.biSize - sizeof(BITMAPINFOHEADER));
	m_Dest.SetBits(24);
	if (m_bFlip)
	    m_Dest.biHeight *= -1;

	PluginGetAttrInt(m_Info, divx4str_postprocessing, &m_iLastPPMode);
	PluginGetAttrInt(m_Info, divx4str_maxauto, &m_iMaxAuto);
	PluginGetAttrInt(m_Info, divx4str_brightness, &m_iLastBrightness);
	PluginGetAttrInt(m_Info, divx4str_contrast, &m_iLastContrast);
	PluginGetAttrInt(m_Info, divx4str_saturation, &m_iLastSaturation);

	m_YvAttrs.push_back(AttributeInfo(divx4str_postprocessing,
					  "Image postprocessing mode ( 6 slowest )",
					  AttributeInfo::Integer, 0, 6));
	m_YvAttrs.push_back(AttributeInfo(divx4str_maxauto,
					  "Maximum autoquality level",
					  AttributeInfo::Integer, 0, 6, 6));
        return 0;
    }
    void Flush()
    {
#ifdef DEC_OPT_FLUSH
        if (m_Handle)
	    m_pDecore(m_Handle, (divxopts_t)DEC_OPT_FLUSH, 0, 0);
#endif
    }
    virtual CAPS GetCapabilities() const
    {
	return CAPS(CAP_YV12|CAP_YUY2);
    }
    virtual IRtConfig* GetRtConfig()
    {
	return this;
    }
    virtual int Start()
    {
	fprintf(stderr,"Decore  START %d\n", DECORE_VERSION);
#if DECORE_VERSION >= 20021112
	DEC_INIT dinit;
	memset(&dinit, 0, sizeof(dinit));
	switch(m_Info.fourcc)
	{
	case fccDIV3: dinit.codec_version = 311; break;
	case fccDIVX: dinit.codec_version = 412; break;
	default:
	case fccDX50: dinit.codec_version = 500; break;
	}
	fprintf(stderr,"Decore  Version %d\n",dinit.codec_version);
	int result = m_pDecore(&m_Handle, (divxopts_t)DEC_OPT_INIT, &dinit, 0);
	if (result || !m_Handle)
	{
    	    divx4_error_set("Error %d creating decoder instance", result);
	    return -1;
	}
	result = m_pDecore(m_Handle, (divxopts_t)DEC_OPT_SETOUT, &m_Dest, 0);
	if (result) // unacceptable color space
	{
	    divx4_error_set("Error %d setting output", 	result);
	    return -1;
	}
#else /* version < 20021112 */
	DEC_PARAM param;

        memset(&param, 0, sizeof(param));
	int csp = m_Dest.biCompression;
	// AVM_WRITE("DivX4 plugin", "csp: 0x%x  %.4s  bitcount: %d height: %d\n", csp, (char*)&csp, m_obh.biBitCount, m_obh.biHeight);

	if (csp == 0 || csp == 3)
	{
	    bool up = (m_Dest.biHeight > 0);
	    switch (m_Dest.biBitCount)
	    {
	    case 16:
		if (csp == 3)
		{
		    param.output_format = (up) ? DEC_RGB565 : DEC_RGB565_INV;
		    break;
		}
                // 16 bit with csp == 0  is 15bit
                /* fall through */
	    case 15:
		param.output_format = (up) ? DEC_RGB555 : DEC_RGB555_INV;
		break;
	    case 24:
		param.output_format = (up) ? DEC_RGB24 : DEC_RGB24_INV;
		break;
	    case 32:
		param.output_format = (up) ? DEC_RGB32 : DEC_RGB32_INV;
		break;
	    default:
		return -1;
	    }
	}
	else
	{
	    switch(csp)
	    {
	    case IMG_FMT_I420:
		param.output_format = DEC_420;
		break;
	    case IMG_FMT_YV12:
		param.output_format = DEC_YV12;
		break;
	    case IMG_FMT_YUY2:
		param.output_format = DEC_YUV2;
		break;
	    case IMG_FMT_I422:
	    case IMG_FMT_UYVY:
		param.output_format = DEC_UYVY;
		break;
	    default:
                return -1;
	    }
	}
	param.x_dim = m_Dest.biWidth;
	param.y_dim = labs(m_Dest.biHeight);
	param.time_incr = 15;
	// DEC_MEM_REQS dmr;
	// decore((unsigned long)this, DEC_OPT_MEMORY_REQS, &dmr, 0);
	memset(&param.buffers, 0, sizeof(param.buffers));
#if DECORE_VERSION >= 20020303
	switch(m_Info.fourcc)
	{
	case fccDIV3: param.codec_version = 311; break;
	case fccDIVX: param.codec_version = 412; break;
	default:
	case fccDX50: param.codec_version = 500; break;
	}
	param.build_number = 0;
#endif /* 20020303 */
	m_Handle = (divxhandle_t) this;
	m_pDecore(m_Handle, DEC_OPT_INIT, &param, m_cFormatBuf);
#endif /* 20021112 */
	return 0;
    }
    virtual int Stop()
    {
#if DECORE_VERSION >= 20021112
	if (!m_Handle)
	    return -1;
	m_pDecore(m_Handle, (divxopts_t)DEC_OPT_RELEASE, 0, 0);
#else /* 20021112 */
	if (!m_Handle)
	    return -1;
#if DECORE_VERSION > 20011009
	// older DivX4 release has been broken and crashed here -
	// so release memory only with newer versions
        // and leave the memory leak with the older version
	m_pDecore(m_Handle, DEC_OPT_RELEASE, 0, 0);
#else /* 20011009 */
#warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#warning !USING OLD DIVX4 library - memory will be unreleased!
#warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#endif /* 20011009 */
#endif /* 20021112 */
	PluginSetAttrInt(m_Info, divx4str_saturation, m_iLastSaturation);
	PluginSetAttrInt(m_Info, divx4str_contrast, m_iLastContrast);
	PluginSetAttrInt(m_Info, divx4str_brightness, m_iLastBrightness);
	PluginSetAttrInt(m_Info, divx4str_maxauto, m_iMaxAuto);
	PluginSetAttrInt(m_Info, divx4str_postprocessing, m_iLastPPMode);
	m_Handle = 0;
        return 0;
    }
    virtual int DecodeFrame(CImage* pImage, const void* src, uint_t size,
			    int is_keyframe, bool render = true,
			    CImage** pOut = 0)
    {
	DEC_FRAME param;

	if (!size || !m_Handle)
	    return 0;
	if (m_Dest.biHeight != pImage->GetFmt()->biHeight)
	{
            m_Dest = pImage->GetFmt();
	    Restart();
	}
	memset(&param, 0, sizeof(param));

	// leave here for some time - until all users will use new headers
	param.bitstream = (void*) src;
	param.length = size;

	if (pImage)
	{
	    param.bmp = pImage->Data();
	    param.stride = pImage->Width();
	    param.render_flag = 1;
	}
	else
	{
	    param.bmp = 0;
            param.stride = 0;
	    param.render_flag = 0;
	}
	if (m_bSetFlg)
	{
            m_bSetFlg = false;
#if DECORE_VERSION >= 20021112
	    int iSetOperation = DEC_ADJ_SET | DEC_ADJ_POSTPROCESSING;
	    int iPP=m_iLastPPMode * 10;
	    m_pDecore(m_Handle, (divxopts_t)DEC_OPT_ADJUST, &iSetOperation, &iPP);
	    iSetOperation = DEC_ADJ_SET | DEC_ADJ_BRIGHTNESS;
	    m_pDecore(m_Handle, (divxopts_t)DEC_OPT_ADJUST, &iSetOperation, &m_iLastBrightness);
	    iSetOperation = DEC_ADJ_SET | DEC_ADJ_CONTRAST;
	    m_pDecore(m_Handle, (divxopts_t)DEC_OPT_ADJUST, &iSetOperation, &m_iLastContrast);
	    iSetOperation = DEC_ADJ_SET | DEC_ADJ_SATURATION;
	    m_pDecore(m_Handle, (divxopts_t)DEC_OPT_ADJUST, &iSetOperation, &m_iLastSaturation);
#else /* version < 20021112 */
	    DEC_SET set;
	    set.postproc_level = m_iLastPPMode * 10;
	    m_pDecore(m_Handle, DEC_OPT_SETPP, &set, 0);
#ifdef DEC_OPT_GAMMA
	    m_pDecore(m_Handle, DEC_OPT_GAMMA, (void*)DEC_GAMMA_BRIGHTNESS, (void*)m_iLastBrightness);
	    m_pDecore(m_Handle, DEC_OPT_GAMMA, (void*)DEC_GAMMA_CONTRAST, (void*)m_iLastContrast);
	    m_pDecore(m_Handle, DEC_OPT_GAMMA, (void*)DEC_GAMMA_SATURATION, (void*)m_iLastSaturation);
#endif /* DEC_OPT_GAMMA */
#endif /* 20021112 */
	}

#if DECORE_VERSION >= 20021112
	m_pDecore(m_Handle, (divxopts_t)DEC_OPT_FRAME, &param, 0);
#else /* version < 20021112 */
#ifdef USE_311_DECODER
#if DECORE_VERSION < 20020303
	if(m_bCompat311)
	{
	    m_pDecore(m_Handle, DEC_OPT_FRAME_311, &param, 0);
	}
	else
#endif /* 20020303 */
#endif /* USE_311_DECODER */
	{
	    m_pDecore(m_Handle, DEC_OPT_FRAME, &param, 0);
	}
#endif
	pImage->SetQuality((float) m_iLastPPMode / DIVX4_MAX_QUALITY);

	return size;
    }
    virtual int SetDestFmt(int bits = 24, fourcc_t csp = 0)
    {
	if (!bits)
	    bits = csp;

	switch(bits)
	{
	case 15:
	case 16:
	case 24:
	case 32:
	    m_Dest.SetBits(bits);
            break;
	case IMG_FMT_I420:
	case IMG_FMT_YV12:
	case IMG_FMT_YUY2:
	case IMG_FMT_I422:
	case IMG_FMT_UYVY:
	    m_Dest.SetSpace(csp);
	    break;
	default:
            return -1;
	}
#if DECORE_VERSION >= 20021112
	if(m_Handle)
	    m_pDecore(m_Handle, (divxopts_t)DEC_OPT_SETOUT, &m_Dest, 0);
#else
	if (m_Handle)
	    Restart();
#endif
	return 0;
    }
    virtual const avm::vector<AttributeInfo>& GetAttrs() const
    {
	switch (m_Dest.biCompression)
	{
	case IMG_FMT_I420:
	case IMG_FMT_YV12:
            // only limited set of supported attrs
	    return m_YvAttrs;
	}
        return m_Info.decoder_info;
    }
    virtual int GetValue(const char* name, int* value) const
    {
	if (strcmp(name, divx4str_postprocessing) == 0)
	    *value = m_iLastPPMode;
	else if (strcmp(name, divx4str_maxauto) == 0)
	    *value = m_iMaxAuto;
	else if (strcmp(name, divx4str_brightness) == 0)
	    *value = m_iLastBrightness;
	else if (strcmp(name, divx4str_contrast) == 0)
	    *value = m_iLastContrast;
	else if (strcmp(name, divx4str_saturation) == 0)
	    *value = m_iLastSaturation;
	else
	    return -1;

	return 0;
    }
    virtual int SetValue(const char* name, int value)
    {
	if (strcmp(name, divx4str_postprocessing) == 0)
	{
	    m_iLastPPMode = value;
            m_bSetFlg = true;
	    return 0;
	}
	else if (strcmp(name, divx4str_maxauto) == 0 && value >= 0 && value <= 6)
	{
	    m_iMaxAuto = value;
            return 0;
	}
	else if (strcmp(name, divx4str_brightness) == 0 && value >= -128 && value <= 127)
	{
	    m_iLastBrightness = value;
            m_bSetFlg = true;
	    return 0;
	}
	else if (strcmp(name, divx4str_contrast) == 0 && value >= -128 && value <= 127)
	{
	    m_iLastContrast = value;
            m_bSetFlg = true;
	    return 0;
	}
	else if (strcmp(name, divx4str_saturation) == 0 && value >= -128 && value <= 127)
	{
	    m_iLastSaturation = value;
            m_bSetFlg = true;
	    return 0;
	}
	return -1;
    }
};
#endif

#ifdef HAVE_LIBDIVXENCORE
class DIVX_VideoEncoder: public IVideoEncoder
{
    BitmapInfo m_bh;
    BITMAPINFOHEADER m_obh;
    void* m_pDll;
    void* m_pHandle;
    int m_iState;
    int m_iQuant;
    int m_iCSP;
    bool m_bRtMode;
    int (*m_pEncore)(void *handle,   // handle   - the handle of the call
		     int enc_opt,    // enc_opt  - the option for encodin
		     void *param1,   // param1   - the parameter 1 (its a
		     void *param2);  // param2   - the parameter 2 (its a
public:
    DIVX_VideoEncoder(const CodecInfo& info, int compressor, const BITMAPINFOHEADER& bh)
	:IVideoEncoder(info), m_bh(bh), m_obh(bh),
	m_pDll(0), m_iState(0), m_bRtMode(false) {}
    virtual ~DIVX_VideoEncoder()
    {
        Stop();
#ifdef DIVXBIN
	if (m_pDll)
            dlclose(m_pDll);
#endif
    }
    int init()
    {
#ifdef DIVXBIN
	m_pDll = dlopen(divxencore, RTLD_LAZY);
	if (!m_pDll)
	{
	    divx4_error_set("can't open %s", divxencore);
            return -1;
	}
	m_pEncore = (int (*)(void *, int, void *, void *))
	    dlsym(m_pDll, "encore");
	if (!m_pEncore)
	{
	    divx4_error_set("can't resolve %s:encore", divxencore);
            dlclose(m_pDll);
            return -1;
	}
#else
        m_pEncore = encore;
#endif
	AVM_WRITE(strDebugEnc, "%s linux encoder\n",
		  (m_Info.fourcc == fccDIVX) ? "DivX4" : "DivX5" );
	m_obh.biCompression = m_Info.fourcc;
	m_obh.biHeight = labs(m_obh.biHeight);
#if ENCORE_VERSION < 5200
	switch (m_bh.biCompression)
	{
        // supports only bottom-up RGB for encoding
	case 0:
	    if (m_bh.biBitCount != 24) {
		divx4_error_set("Unsupported input bit depth");
		return -1;
	    }
	    m_iCSP = ENC_CSP_RGB24; m_bh.SetDirection(true); break;
	case fccYUY2: m_iCSP = ENC_CSP_YUY2; break;
	case fccYV12: m_iCSP = ENC_CSP_YV12; break;
	case fccI420: m_iCSP = ENC_CSP_I420; break;
	default:
	    divx4_error_set("Unsupported input format");
            return -1;
	}
#else
	switch (m_bh.biCompression)
	{
        // supports only bottom-up RGB for encoding
	case 0:
	    if (m_bh.biBitCount != 24) {
		divx4_error_set("Unsupported input bit depth");
		return -1;
	    }
	    break;
	case fccYUY2:
	case fccYV12:
	case fccI420:
	    break;
	default:
	    divx4_error_set("Unsupported input format");
            return -1;
	}
#endif
        return 0;
    }
    virtual int Start()
    {
#if ENCORE_MAJOR_VERSION < 5200
	ENC_PARAM param;
#else
	SETTINGS param;
#endif
	memset(&param, 0, sizeof(param));
#if ENCORE_MAJOR_VERSION < 5200
	param.x_dim = m_bh.biWidth;
	//param.y_dim=m_bh.biHeight;// labs(m_bh.biHeight);
	param.y_dim = labs(m_bh.biHeight);
	param.framerate = 30;//frames/sec
#else
	param.input_clock = 1000000;
	param.input_frame_period = 33333;
#endif
	PluginGetAttrInt(m_Info, divx4str_bitrate, &param.bitrate);//bits/sec
#if ENCORE_MAJOR_VERSION < 5200
	PluginGetAttrInt(m_Info, divx4str_rc_period, &param.rc_period);
	PluginGetAttrInt(m_Info, divx4str_rc_reaction_period, &param.rc_reaction_period);
	PluginGetAttrInt(m_Info, divx4str_rc_reaction_ratio, &param.rc_reaction_ratio);
	PluginGetAttrInt(m_Info, divx4str_max_quantizer, &param.max_quantizer);//just guess
	PluginGetAttrInt(m_Info, divx4str_min_quantizer, &param.min_quantizer);
#endif
	PluginGetAttrInt(m_Info, divx4str_max_key_interval, &param.max_key_interval);
	PluginGetAttrInt(m_Info, divx4str_quality, &param.quality);

        int deinterlace = 0;
	int bidirect = 0;
	int obmc = 0;
#if ENCORE_MAJOR_VERSION < 5200
// to be done: all extended parameters of new encore
#ifdef IF_SUPPORT_PRO
	// fast deinterlace
	PluginGetAttrInt(m_Info, divx4str_deinterlace, &deinterlace);
        // use bidirectional coding
	PluginGetAttrInt(m_Info, divx4str_bidirect, &bidirect);
        // flag to enable overlapped block motion compensation mode
	PluginGetAttrInt(m_Info, divx4str_obmc, &obmc);
#endif /* IF_SUPPORT_PRO */
        param.deinterlace = deinterlace;
#ifndef ENCORE_MAJOR_VERSION
	param.use_bidirect = bidirect;
	param.obmc = obmc;
#else /* ENCORE_MAJOR_VERSION */
	memset(&param.extensions, 0, sizeof(param.extensions));
#ifdef IF_SUPPORT_PRO
	param.extensions.use_bidirect = bidirect;
	param.extensions.obmc = obmc;

#if ENCORE_MAJOR_VERSION >= 5010

	PluginGetAttrInt(m_Info, divx4str_enable_crop, &param.extensions.enable_crop);
	PluginGetAttrInt(m_Info, divx4str_crop_left, &param.extensions.crop_left);
	PluginGetAttrInt(m_Info, divx4str_crop_right, &param.extensions.crop_right);
	PluginGetAttrInt(m_Info, divx4str_crop_top, &param.extensions.crop_top);
	PluginGetAttrInt(m_Info, divx4str_crop_bottom, &param.extensions.crop_bottom);

	PluginGetAttrInt(m_Info, divx4str_enable_resize, &param.extensions.enable_resize);
	PluginGetAttrInt(m_Info, divx4str_resize_width, &param.extensions.resize_width);
	PluginGetAttrInt(m_Info, divx4str_resize_height, &param.extensions.resize_height);
	PluginGetAttrInt(m_Info, divx4str_resize_mode, &param.extensions.resize_mode);
        int tmp;
	PluginGetAttrInt(m_Info, divx4str_resize_bicubic_B, &tmp);
        param.extensions.bicubic_B = tmp / (double) 1000.0;
	PluginGetAttrInt(m_Info, divx4str_resize_bicubic_C, &tmp);
        param.extensions.bicubic_C = tmp / (double) 1000.0;

	PluginGetAttrInt(m_Info, divx4str_interlace_mode, &param.extensions.interlace_mode);

	PluginGetAttrInt(m_Info, divx4str_temporal_enable, &param.extensions.temporal_enable);
	PluginGetAttrInt(m_Info, divx4str_temporal_level, &tmp);
        param.extensions.temporal_level = tmp / (double) 1000.0;

	PluginGetAttrInt(m_Info, divx4str_spatial_passes, &param.extensions.spatial_passes);
	PluginGetAttrInt(m_Info, divx4str_spatial_level, &tmp);
        param.extensions.spatial_level = tmp / (double) 1000.0;

#endif /* ENCORE_MAJOR_VERSION >= 5010 */

#endif /* IF_SUPPORT_PRO */

#endif /* ENCORE_MAJOR_VERSION */

#endif /* 5200 */
	if (param.quality == 1)
	{
	    m_bRtMode=true;
	    int quality=GetQuality();
	    m_iQuant=int(1+.003*(10000-quality));
	    AVM_WRITE(strDebugEnc, "New quant: %d\n", m_iQuant);
	    if(m_iQuant>31)
		m_iQuant=31;
	    if(m_iQuant<1)
		m_iQuant=1;
	}
	else
	    m_bRtMode=false;
	//	param.flip=1;
#if ENCORE_MAJOR_VERSION < 5200
	m_pEncore(0, ENC_OPT_INIT, &param, 0);
	m_pHandle=param.handle;
#else
	int result=m_pEncore((void*)&m_pHandle, ENC_OPT_INIT, &param, &m_bh);
	if(result!=ENC_OK)
	{
	    divx4_error_set("Error %d creating encoder", result);
	    return -1;
	}
#endif
        return 0;
    }
    virtual int Stop()
    {
	if (!m_iState)
	    return -1;
	m_pEncore(m_pHandle, ENC_OPT_RELEASE, 0, 0);
	m_pHandle = 0;
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
    virtual int EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid=0)
    {
        CImage* ci = 0;
	//src->GetFmt()->Print();
	if (!src->IsFmt(&m_bh))
	{
            // slow conversion
            ci = new CImage(src, &m_bh);
	    //src->GetFmt()->Print();
	    //AVM_WRITE(strDebugEnc, "enconding format doesn't match!\n");
	    //return -1;
	}
	ENC_FRAME param;
	memset(&param, 0, sizeof(param));
	ENC_RESULT result;
	param.bitstream = dest;
#if ENCORE_MAJOR_VERSION < 5200
	param.colorspace = m_iCSP;
	param.mvs = 0;
#endif
	param.image = (ci) ? ci->Data() : (void*) src->Data();
	param.length = GetOutputSize();
#if ENCORE_MAJOR_VERSION < 5200
	if (m_bRtMode)
	{
	    param.quant=m_iQuant;
            // let encoder decide if frame is INTER/INTRA
	    param.intra=-1;
	    m_pEncore(m_pHandle, ENC_OPT_ENCODE_VBR, &param, &result);
	}
	else
#endif
	    m_pEncore(m_pHandle, ENC_OPT_ENCODE, &param, &result);
#if ENCORE_MAJOR_VERSION < 5200
	if (is_keyframe)
	    *is_keyframe = result.is_key_frame ? 16 : 0;
#else
	if (is_keyframe)
	    *is_keyframe = (result.cType == 'I') ? 16 : 0;
#endif
	if (size)
	    *size = param.length;
	if (ci)
            ci->Release();
	return 0;
    }
};
#endif

static IVideoDecoder* divx4_CreateVideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
{
#if DECORE_VERSION < 20020303
    quiet_encore = 1;
#endif
    if (bh.biSize < 40)
    {
	divx4_error_set("to short biSize");
        return 0;
    }

#ifdef HAVE_LIBDIVXDECORE
    DIVX_VideoDecoder* d = new DIVX_VideoDecoder(info, bh, flip);
    if (d->init() == 0)
	return d;
    delete d;
#endif
    return 0;
}

static IVideoEncoder* divx4_CreateVideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
{
#if DECORE_VERSION < 20020303
    quiet_encore = 1;
#endif
#ifdef HAVE_LIBDIVXENCORE
    DIVX_VideoEncoder* e = new DIVX_VideoEncoder(info, compressor, bh);
    if (e->init() == 0)
	return e;
    delete e;
#endif
    return 0;
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_divx4;

avm::codec_plugin_t avm_codec_plugin_divx4 =
{
    PLUGIN_API_VERSION,

    0,
    0, 0,
    avm::PluginGetAttrInt,
    avm::PluginSetAttrInt,
    0, 0,
    avm::divx4_FillPlugins,
    0, 0,
    avm::divx4_CreateVideoDecoder,
    avm::divx4_CreateVideoEncoder,
};
