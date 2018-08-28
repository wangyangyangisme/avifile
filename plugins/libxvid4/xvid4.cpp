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
static const char* strDebug = "XviD4 plugin";

PLUGIN_TEMP(xvid4);

static const char* getError(int xviderr)
{
    switch (xviderr)
    {
    case XVID_ERR_FAIL:
	return "general fault";
    case XVID_ERR_MEMORY:
	return "memory allocation error";
    case XVID_ERR_FORMAT:
	return "file format error";
    case XVID_ERR_VERSION:
	return "structure version not supported";
    case XVID_ERR_END:
	return "end of stream reached";
    default:
	return "unknown";
    }
}

class XVID4_VideoDecoder: public IVideoDecoder, public IRtConfig
{
    void* m_pHandle;
    int m_iLastPPMode;
    int m_iMaxAuto;
    int m_iDeblockingY;
    int m_iDeblockingUV;
    int m_iFilmEffect;
    int m_General;
public:
    XVID4_VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
	:IVideoDecoder(info, bh), m_pHandle(0), m_General(0)
    {
	m_Dest.SetSpace(fccYV12);
	if (flip)
	    m_Dest.biHeight *= -1;
    }
    virtual ~XVID4_VideoDecoder()
    {
	Stop();
    }
    virtual CAPS GetCapabilities() const
    {
	//return CAPS(CAP_YV12|CAP_YUY2|CAP_UYVY|CAP_I420);
	return CAPS(CAP_YV12);
	//return CAPS(CAP_NONE);
    }
    virtual IRtConfig* GetRtConfig()
    {
	return (IRtConfig*) this;
    }
    virtual int Start()
    {
	xvid_gbl_init_t xinit;
	xvid_dec_create_t param;
	int cs;

	if (m_pHandle)
            return -1;

	memset(&xinit, 0, sizeof(xinit));
	xinit.version = XVID_VERSION;
	//xinit.cpu_flags = 0;
	//init.cpu_flags = XVID_CPU_FORCE;// | XVID_CPU_MMX;
	xvid_global(NULL, XVID_GBL_INIT, &xinit, NULL);

	memset(&param, 0, sizeof(param));
	param.version = XVID_VERSION;
	param.width = m_Dest.biWidth;
	param.height = labs(m_Dest.biHeight);

	if (xvid_decore(NULL, XVID_DEC_CREATE, &param, NULL) != 0)
	{
	    AVM_WRITE(strDebug, "start failed!\n");
            return -1;
	}
	m_pHandle = param.handle;

        SetValue(0, 0);
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
	xvid_dec_frame_t param;
	if (!size || !m_pHandle)
	    return 0;
	//AVM_WRITE(strDebug, "csp: 0x%x bitcount: %d height: %d\n", csp, m_Dest.biBitCount, m_Dest.biHeight);

	memset(&param, 0, sizeof(param));
        param.version = XVID_VERSION;
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
			csp = XVID_CSP_RGB565;
			break;
		    }
		    // 16 bit with csp == 0  is 15bit
		    /* fall through */
		case 15:
		    csp = XVID_CSP_RGB555;
		    break;
		case 24:
		    csp = XVID_CSP_BGR;
		    break;
		case 32:
		    csp = XVID_CSP_BGRA;
		    break;
		default:
		    return -1;
		}
		if (pImage->GetFmt()->biHeight > 0)
		    csp |= XVID_CSP_VFLIP;
		// RGB also doesn't seems work;
		csp = XVID_CSP_USER;
		break;
	    case fccYV12:
		//param.colorspace = XVID_CSP_YV12;
		csp = XVID_CSP_USER;
		break;
	    case fccI420:
		csp = XVID_CSP_I420; // untested
		break;
	    case fccYUY2:
		csp = XVID_CSP_YUY2;
		break;
	    case fccYVYU:
		csp = XVID_CSP_YVYU;
		break;
	    case fccUYVY:
		csp = XVID_CSP_UYVY;
		break;
	    default:
		return -1;
	    }
	    //printf("XXXXXXXXXX   %d  %d  %d\n", size, m_Dest.biWidth, param.colorspace);
	    param.output.csp = csp;

	    param.output.plane[0] = pImage->Data(0);
	    param.output.plane[1] = pImage->Data(2);
	    param.output.plane[2] = pImage->Data(1);
	    param.output.stride[0] = pImage->Stride(0);
	    param.output.stride[1] = pImage->Stride(2);
	    param.output.stride[2] = pImage->Stride(1);
	    //pImage->SetQuality((float) m_iLastPPMode / MAX_QUALITY);
	}
	else
            param.output.csp = XVID_CSP_NULL;

	param.general = m_General;

        int xerr = xvid_decore(m_pHandle, XVID_DEC_DECODE, &param, NULL);
	if (xerr < 0)
	{
	    xvid4_error_set(getError(xerr));
	    return -1;
	}

	if (param.output.csp == XVID_CSP_INTERNAL)
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
	    const struct dp* d = (const struct dp*) param.output.plane;
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
    virtual const avm::vector<AttributeInfo>& GetAttrs() const
    {
        return m_Info.decoder_info;
    }
    virtual int GetValue(const char* name, int* value) const
    {
	return PluginGetAttrInt(m_Info, name, value);
    }
    virtual int SetValue(const char* name, int value)
    {
        int v;
	if (name && PluginSetAttrInt(m_Info, name, value) != 0)
            return -1;

	m_General = XVID_LOWDELAY;
	PluginGetAttrInt(m_Info, xvid4str_deblocking_y, &v);
        if (v) m_General |= XVID_DEBLOCKY;
	PluginGetAttrInt(m_Info, xvid4str_deblocking_uv, &v);
        if (v) m_General |= XVID_DEBLOCKUV;
	PluginGetAttrInt(m_Info, xvid4str_film_effect, &v);
	if (v) m_General |= XVID_FILMEFFECT;
        return 0;
    }
};

class XVID4_VideoEncoder: public IVideoEncoder
{
    static const uint_t MAX_ZONES = 64;
    BITMAPINFOHEADER m_bh;
    BITMAPINFOHEADER m_obh;
    xvid_enc_frame_t m_Frame;
    xvid_enc_zone_t m_Zones[MAX_ZONES];
    xvid_enc_plugin_t m_Plugins[7];
    unsigned int m_uiFrames;
    void* m_pHandle;
    int m_iQuant;
    bool m_bRtMode;
    int general;
    int motion;
    uint_t m_uiZones;
    uint_t m_uiPlugins;
public:
    XVID4_VideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
	:IVideoEncoder(info), m_bh(bh), m_obh(bh), m_pHandle(0), m_bRtMode(false),
        m_uiZones(0), m_uiPlugins(0)
    {
	m_obh.biCompression = fccDIVX;
	m_obh.biHeight = labs(m_obh.biHeight);

	xvid_gbl_info_t xinfo;
	memset(&xinfo, 0, sizeof(xinfo));
	xinfo.version = XVID_VERSION;
	if (xvid_global(NULL, XVID_GBL_INFO, &xinfo, NULL) < 0)
	{
		AVM_WRITE(strDebug, "Information about the library unavailable\n");
	}
	else
	{
		AVM_WRITE(strDebug, "Using library version %d.%d.%d (build %s)\n",
			  XVID_VERSION_MAJOR(xinfo.actual_version),
			  XVID_VERSION_MINOR(xinfo.actual_version),
			  XVID_VERSION_PATCH(xinfo.actual_version),
			  xinfo.build);
	}
    }
    virtual ~XVID4_VideoEncoder()
    {
	Stop();
    }
    virtual int EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid=0)
    {
	int length;
	int csp;
	xvid_enc_stats_t stats;

	memset(&stats, 0, sizeof(stats));
        stats.version = XVID_VERSION;

	switch (m_bh.biCompression)
	{
	case fccYUY2:
	    csp = XVID_CSP_YUY2;
	    break;
	case fccYV12:
	    csp = XVID_CSP_YV12;
	    break;
	case fccI420:
	    csp = XVID_CSP_I420;
	    break;
	case 0:
	default:
	    csp = XVID_CSP_BGR;
	    break;
	}

        m_Frame.version = XVID_VERSION;
	m_Frame.type = XVID_TYPE_AUTO;
        m_Frame.input.csp = csp;
	m_Frame.input.plane[0] = (void*) src->Data(0);
	m_Frame.input.plane[2] = (void*) src->Data(2);
	m_Frame.input.plane[1] = (void*) src->Data(1);
	m_Frame.input.stride[0] = src->Stride(0);
	m_Frame.input.stride[2] = src->Stride(2);
	m_Frame.input.stride[1] = src->Stride(1);
	m_Frame.bitstream = dest;
	m_Frame.length = -1; 	// this is written by the routine
	m_Frame.quant = 0;
	//m_Frame.intra = (!m_uiFrames++) ? 1 : -1;

        m_Frame.motion = motion;
	//if (m_bRtMode)
	length = xvid_encore(m_pHandle, XVID_ENC_ENCODE, &m_Frame, &stats);

	//printf("%d   %d  %d\n", m_Frame.intra, m_Frame.colorspace, m_Frame.length);
	// xframe.quant = QUANTI;	// is quant != 0, use a fixed quant (and ignore bitrate)

	if (is_keyframe)
	    *is_keyframe = (m_Frame.out_flags & XVID_KEYFRAME) ? 16 : 0;
	if (size)
	    *size = length;
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
        // table from the xvid sample source code
	static const int motion_presets[7] =
	{
	    /* quality 0 */
	    0,

	    /* quality 1 */
	    XVID_ME_ADVANCEDDIAMOND16,

	    /* quality 2 */
	    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16,

	    /* quality 3 */
	    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
	    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8,

	    /* quality 4 */
	    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
	    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
	    XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,

	    /* quality 5 */
	    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
	    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
	    XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,

	    /* quality 6 */
	    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 |
	    XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 |
	    XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,
	};

	static const int vop_presets[] = {
	    /* quality 0 */
	    0,

	    /* quality 1 */
	    0,

	    /* quality 2 */
	    XVID_VOP_HALFPEL,

	    /* quality 3 */
	    XVID_VOP_HALFPEL | XVID_VOP_INTER4V,

	    /* quality 4 */
	    XVID_VOP_HALFPEL | XVID_VOP_INTER4V,

	    /* quality 5 */
	    XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
	    XVID_VOP_TRELLISQUANT,

	    /* quality 6 */
	    XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
	    XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED,
	};


	int xerr;
	int t;

	xvid_gbl_init_t init;
	xvid_enc_create_t create;

	xvid_plugin_single_t single;
	xvid_plugin_2pass1_t rc2pass1;
	xvid_plugin_2pass2_t rc2pass2;

	m_uiFrames = 0;

	memset(&init, 0, sizeof(init));
	init.version = XVID_VERSION;
	init.cpu_flags = 0;
	PluginGetAttrInt(m_Info, xvid4str_debug, &init.debug);

	xvid_global(NULL, XVID_GBL_INIT, &init, NULL);

	memset(&create, 0, sizeof(create));
	create.width = m_bh.biWidth;
	create.height = labs(m_bh.biHeight);
	create.fincr = 100000;
	create.fbase = 2500000;
	create.zones = m_Zones;
        create.num_zones = m_uiZones;
        create.plugins = m_Plugins;
        create.num_plugins = 0;

	memset(&single, 0, sizeof(single));
	single.version = XVID_VERSION;

// single pass
	m_Plugins[create.num_plugins].func = xvid_plugin_single;
	m_Plugins[create.num_plugins].param = &single;
	create.num_plugins++;

	memset(&m_Frame, 0, sizeof(m_Frame));
	PluginGetAttrInt(m_Info, xvid4str_bitrate, &single.bitrate);

	PluginGetAttrInt(m_Info, xvid4str_motion_search, &t);
	motion = (t >= 0 && t < 7) ? motion_presets[t] : 0;

#if 0
	// Motion estimation options
	PluginGetAttrInt(m_Info, xvid4str_me_zero, &t);
        if (t) motion |= XVID_ME_ZERO;

	PluginGetAttrInt(m_Info, xvid4str_me_logarithmic, &t);
	if (t) motion |= XVID_ME_LOGARITHMIC;

	PluginGetAttrInt(m_Info, xvid4str_me_fullsearch, &t);
        if (t) motion |= XVID_ME_FULLSEARCH;

	PluginGetAttrInt(m_Info, xvid4str_max_quantizer, &param.max_quantizer);
	PluginGetAttrInt(m_Info, xvid4str_min_quantizer, &param.min_quantizer);
	PluginGetAttrInt(m_Info, xvid4str_max_key_interval, &param.max_key_interval);
        // fast deinterlace
        // use bidirectional coding
        // flag to enable overlapped block motion compensation mode

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
	xerr = xvid_encore(NULL, XVID_ENC_CREATE, &create, NULL);
	if (xerr < 0)
	{
	    xvid4_error_set(getError(xerr));
            return -1;
	}
	m_pHandle = create.handle;
	return 0;
    }
    virtual int Stop()
    {
	if (!m_pHandle)
	    return -1;
	xvid_encore(m_pHandle, XVID_ENC_DESTROY, NULL, NULL);
	m_pHandle = 0;
	return 0;
    }
private:
    char type2Char(int type) const
    {
	switch (type)
	{
	case XVID_TYPE_IVOP:
	    return 'I';
	case XVID_TYPE_PVOP:
	    return 'P';
	case XVID_TYPE_BVOP:
	    return 'B';
	default:
	    return 'S';
	}
    }
};

static IVideoEncoder* xvid4_CreateVideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& bh)
{
    switch (bh.biCompression)
    {
    case 0:
        if (bh.biBitCount != 24)
	{
	    xvid4_error_set("unsupported input bit depth");
	    return 0;
	}
        /* fall through */
    case fccYUY2:
    case fccYV12:
    case fccI420:
    case fccDIVX:
    case fccXVID:
	break;
    default:
	xvid4_error_set("unsupported input format");
        return 0;
    }

    return new XVID4_VideoEncoder(info, compressor, bh);
}

static IVideoDecoder* xvid4_CreateVideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& bh, int flip)
{
    if (bh.biSize < 40)
    {
	xvid4_error_set("unsupported biSize");
	return 0;
    }

    return new XVID4_VideoDecoder(info, bh, flip);
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_xvid4;

avm::codec_plugin_t avm_codec_plugin_xvid4 =
{
    PLUGIN_API_VERSION,

    0,
    0, 0,
    avm::PluginGetAttrInt,
    avm::PluginSetAttrInt,
    0, 0, // attrs
    avm::xvid4_FillPlugins,
    0, 0, // audio
    avm::xvid4_CreateVideoDecoder,
    avm::xvid4_CreateVideoEncoder,
};
