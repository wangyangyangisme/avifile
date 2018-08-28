#ifndef DIVX4_FILLPLUGINS_H
#define DIVX4_FILLPLUGINS_H

#include "infotypes.h"
#include "avm_fourcc.h"

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif

#ifdef HAVE_LIBDIVXENCORE
#include "encore2.h"
#endif

#ifdef HAVE_LIBDIVXDECORE
#include "decore.h"
#endif

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif


AVM_BEGIN_NAMESPACE;

#define DIVX4CSTR(name) \
    static const char* divx4str_ ## name = #name

DIVX4CSTR(bitrate);
DIVX4CSTR(rc_period);
DIVX4CSTR(rc_reaction_period);
DIVX4CSTR(rc_reaction_ratio);
DIVX4CSTR(min_quantizer);
DIVX4CSTR(max_quantizer);
DIVX4CSTR(max_key_interval);
DIVX4CSTR(quality);
DIVX4CSTR(deinterlace);
DIVX4CSTR(bidirect);
DIVX4CSTR(obmc);
DIVX4CSTR(enable_crop);
DIVX4CSTR(crop_left);
DIVX4CSTR(crop_right);
DIVX4CSTR(crop_top);
DIVX4CSTR(crop_bottom);
DIVX4CSTR(enable_resize);
DIVX4CSTR(resize_width);
DIVX4CSTR(resize_height);
DIVX4CSTR(resize_mode);
DIVX4CSTR(resize_bicubic_B);
DIVX4CSTR(resize_bicubic_C);
DIVX4CSTR(interlace_mode);
DIVX4CSTR(temporal_enable);
DIVX4CSTR(temporal_level);
DIVX4CSTR(spatial_passes);
DIVX4CSTR(spatial_level);

DIVX4CSTR(postprocessing);
DIVX4CSTR(maxauto);
DIVX4CSTR(saturation);
DIVX4CSTR(brightness);
DIVX4CSTR(contrast);

#define USE_311_DECODER
static void divx4_FillPlugins(avm::vector<CodecInfo>& ci)
{
    static char divx_about[] = "DivX4"
#if DECORE_VERSION >= 20020303
	"/DivX5\n"
#endif
	" MPEG-4 Codec is produced by DivXNetworks, Inc. "
	"Visit: <a href=\"http://www.divx.com\">"
	"http://www.divx.com</a>";
    static const char* resize_opt[] =
    {
	"bilinear",
	"bicubic",
        0
    };
    static const char* ivtc_opt[] =
    {
	"progressive (off)",
	"adaptive",
        "intelligent IVTC",
        0
    };
    static const fourcc_t divx4_codecs[] =
    {
	fccDIVX, fccdivx, fccDIV1, fccdiv1, fccMP4S, fccmp4s,
        RIFFINFO_XVID, RIFFINFO_XviD, RIFFINFO_xvid,
	0x4, 0
    };
    static const fourcc_t divx3_codecs[] =
    {
	fccDIV3, fccdiv3, fccDIV4, fccdiv4,
	fccDIV5, fccdiv5, fccDIV6, fccdiv6,
	/* untested, may work */
	fccMP43, fccmp43,
	0
    };

// FIFO order is important - if this will ever change - few things
// would have to be repaired

    avm::vector<AttributeInfo> vs;
    vs.push_back(AttributeInfo(divx4str_bitrate,
			       "Desired stream bitrate in bits/second",
			       AttributeInfo::Integer, 0, 10000000, 800000));
    vs.push_back(AttributeInfo(divx4str_quality,
			       "Performance/quality balance ( 5 slowest )",
			       AttributeInfo::Integer, 0, 5, 3));
    vs.push_back(AttributeInfo(divx4str_rc_period,
			       "Rate control averaging period",
			       AttributeInfo::Integer, 0, 10000, 2000));
    vs.push_back(AttributeInfo(divx4str_rc_reaction_period,
			       "Rate control reaction period",
			       AttributeInfo::Integer, 0, 100, 10));
    vs.push_back(AttributeInfo(divx4str_rc_reaction_ratio,
			       "Rate control motion sensitivity",
			       AttributeInfo::Integer, 0, 100, 20));

    vs.push_back(AttributeInfo(divx4str_max_key_interval,
			       "Maximum key frame interval",
			       AttributeInfo::Integer, 0, 500, 100));
    vs.push_back(AttributeInfo(divx4str_min_quantizer,
			       "Minimum quantizer",
			       AttributeInfo::Integer, 1, 31, 1));
    vs.push_back(AttributeInfo(divx4str_max_quantizer,
			       "Maximum quantizer",
			       AttributeInfo::Integer, 1, 31, 16));
#ifdef IF_SUPPORT_PRO
    vs.push_back(AttributeInfo(divx4str_deinterlace,
			       "Deinterlace",
			       AttributeInfo::Integer, 0, 1));
#if defined(ENC_MAJOR_VERSION) || defined(ENCORE_MAJOR_VERSION)
    vs.push_back(AttributeInfo(divx4str_bidirect,
			       "Bidirectional encoding",
			       AttributeInfo::Integer, 0, 1));
    vs.push_back(AttributeInfo(divx4str_obmc,
			       "Overlapped block motion compensation",
			       AttributeInfo::Integer, 0, 1));
#endif

#if ENCORE_MAJOR_VERSION >= 5010
    vs.push_back(AttributeInfo(divx4str_interlace_mode,
			       "IVTC/Deinterlace", ivtc_opt));

    vs.push_back(AttributeInfo(divx4str_temporal_enable,
			       "Temporal filter",
			       AttributeInfo::Integer, 0, 1));

    vs.push_back(AttributeInfo(divx4str_temporal_level,
			       "  Temporal filter level (0..1000)",
			       AttributeInfo::Integer, 0, 1000, 0));

    vs.push_back(AttributeInfo(divx4str_spatial_passes,
			       "Spatial filter passes (0=off,..3)",
			       AttributeInfo::Integer, 0, 3, 0));

    vs.push_back(AttributeInfo(divx4str_spatial_level,
			       "  Spatial filter level (0..1000)",
			       AttributeInfo::Integer, 0, 1000, 0));

    vs.push_back(AttributeInfo(divx4str_enable_crop,
			       "Cropping",
			       AttributeInfo::Integer, 0, 1));

    vs.push_back(AttributeInfo(divx4str_crop_left,
			       "  Cropping Left",
			       AttributeInfo::Integer, 0, 3000, 0));
    vs.push_back(AttributeInfo(divx4str_crop_right,
			       "  Cropping Right",
			       AttributeInfo::Integer, 0, 3000, 0));
    vs.push_back(AttributeInfo(divx4str_crop_top,
			       "  Cropping Top",
			       AttributeInfo::Integer, 0, 3000, 0));
    vs.push_back(AttributeInfo(divx4str_crop_bottom,
			       "  Cropping Bottom",
			       AttributeInfo::Integer, 0, 3000, 0));

    vs.push_back(AttributeInfo(divx4str_enable_resize,
			       "Resize",
			       AttributeInfo::Integer, 0, 1));

    vs.push_back(AttributeInfo(divx4str_resize_width,
			       "  Resize Width",
			       AttributeInfo::Integer, 0, 3000, 0));
    vs.push_back(AttributeInfo(divx4str_resize_height,
			       "  Resize Height",
			       AttributeInfo::Integer, 0, 3000, 0));

    vs.push_back(AttributeInfo(divx4str_resize_mode,
			       "  Resize", resize_opt));
    vs.push_back(AttributeInfo(divx4str_resize_bicubic_B,
			       "    Resize Bicubic B (0..1000)",
			       AttributeInfo::Integer, 0, 1000, 500));
    vs.push_back(AttributeInfo(divx4str_resize_bicubic_C,
			       "    Resize Bicubic C (0..1000)",
			       AttributeInfo::Integer, 0, 1000, 0));

#endif // ENCORE_MAJOR_VERSION >= 5010
#endif // IF_SUPPORT_PRO


    avm::vector<AttributeInfo> ds;
    ds.push_back(AttributeInfo(divx4str_postprocessing,
			       "Image postprocessing mode ( 6 slowest )",
			       AttributeInfo::Integer, 0, 6));
    ds.push_back(AttributeInfo(divx4str_maxauto,
			       "Maximum autoquality level",
			       AttributeInfo::Integer, 0, 6, 6));
#ifdef DEC_OPT_GAMMA			       
    ds.push_back(AttributeInfo(divx4str_brightness, "Brightness",
			       AttributeInfo::Integer, -128, 127));
    ds.push_back(AttributeInfo(divx4str_contrast, "Contrast",
			       AttributeInfo::Integer, -128, 127));
    ds.push_back(AttributeInfo(divx4str_saturation, "Saturation",
			       AttributeInfo::Integer, -128, 127));
#endif

    ci.push_back(CodecInfo(divx4_codecs, "DivX4.0", "", divx_about,
			   CodecInfo::Plugin, "odivx4", CodecInfo::Video,
#if ENCORE_MAJOR_VERSION >= 5010
			   CodecInfo::Decode, 0, 0, ds
#else
			   CodecInfo::Both, 0, vs, ds
#endif
			  ));

#if DECORE_VERSION >= 20020303
    static const fourcc_t divx5_codecs[] =
    {
	fccDX50, fccdx50, 0
    };
    ci.push_back(CodecInfo(divx5_codecs, "DivX5.0", "", divx_about,
			   CodecInfo::Plugin, "odivx5", CodecInfo::Video,
			   CodecInfo::Both, 0, vs, ds));
#endif

#ifdef USE_311_DECODER
    ci.push_back(CodecInfo(divx3_codecs, "OpenDivX 3.11 compatible decoder",
			   "", divx_about, CodecInfo::Plugin, "odivx",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, ds));
#endif
}

AVM_END_NAMESPACE;

#endif // DIVX4_FILLPLUGINS_H
