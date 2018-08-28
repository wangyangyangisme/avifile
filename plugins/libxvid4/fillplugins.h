#ifndef XVID4_FILLPLUGINS_H
#define XVID4_FILLPLUGINS_H

#include "infotypes.h"
#include "avm_fourcc.h"

#ifdef HAVE_LIBXVIDCORE4
#include "xvid.h"
#endif

AVM_BEGIN_NAMESPACE;

#define XVID4CSTR(name) \
    static const char* xvid4str_ ## name = #name

XVID4CSTR(debug);

// decoding options
XVID4CSTR(deblocking_y);
XVID4CSTR(deblocking_uv);
//XVID4CSTR(deringing);
XVID4CSTR(film_effect);
//XVID4CSTR(postprocessing);
//XVID4CSTR(maxauto);

// encoding options
XVID4CSTR(mode);
XVID4CSTR(bitrate);

XVID4CSTR(motion_search);
XVID4CSTR(chroma_motion);
XVID4CSTR(inter4v);
XVID4CSTR(trellisquant);
XVID4CSTR(diamond_search);
XVID4CSTR(adaptive_quant);

#if 0
XVID4CSTR(me_zero);
XVID4CSTR(me_logarithmic);
XVID4CSTR(me_fullsearch);
#endif

XVID4CSTR(me_pmvfast);
XVID4CSTR(me_epzs);


static void xvid4_FillPlugins(avm::vector<CodecInfo>& ci)
{
    static char xvid_about[] = "XviD4 MPEG-4 video codec";
    static const fourcc_t xvid_codecs[] =
    {
	fccDIVX, fccdivx,
	RIFFINFO_XVID, RIFFINFO_XviD, RIFFINFO_xvid, 4,
        fccDX50, fccdx50,
	0
    };

    static const char* xvid_passopt[] =
    {
	"1Pass-CBR",
	"1Pass-quality",
	"1Pass-quantizer",
	"2Pass-1stPass",
	"2Pass-2ndPassExt",
	"2Pass-2ndPassInt",
        0
    };
    static const char* xvid_quantopt[] =
    {
	"H.263",
	"MPEG-4",
	// not used in source "Adaptive",
        0
    };
    avm::vector<AttributeInfo> ds;
    ds.push_back(AttributeInfo(xvid4str_deblocking_y, "Deblocking Y",
			       AttributeInfo::Integer, 0, 1));
    ds.push_back(AttributeInfo(xvid4str_deblocking_uv, "Deblocking UV",
			       AttributeInfo::Integer, 0, 1));
    //ds.push_back(AttributeInfo(xvid4str_deringing, "Deringing",
    //    		       AttributeInfo::Integer, 0, 1));
    ds.push_back(AttributeInfo(xvid4str_film_effect, "Film Effect",
			       AttributeInfo::Integer, 0, 1));
    //ds.push_back(AttributeInfo(xvid4str_maxauto, "Maxauto - unsupported",
    //    		       AttributeInfo::Integer, -1, -1));

// FIFO order is important - if this will ever change - few things
// would have to be repaired

    avm::vector<AttributeInfo> vs;
    vs.push_back(AttributeInfo(xvid4str_debug, "Debug",
			       AttributeInfo::Integer, 0, 1));
    vs.push_back(AttributeInfo(xvid4str_mode, "Mode", xvid_passopt));
    vs.push_back(AttributeInfo(xvid4str_bitrate,
			       "Desired stream bitrate in bits/second",
			       AttributeInfo::Integer, 0, 10000000, 800000));
    vs.push_back(AttributeInfo(xvid4str_motion_search,
			       "Motion search ( 6 - Ultra high )",
			       AttributeInfo::Integer, 0, 6, 6));
#if 0
    vs.push_back(AttributeInfo(xvid4str_quant_type,
			       "Quantization type", xvid_quantopt));

    vs.push_back(AttributeInfo(xvid4str_chroma_motion,
			       "Use Chroma Motion",
			       AttributeInfo::Integer, 0, 1));

    vs.push_back(AttributeInfo(xvid4str_diamond_search,
			       "Diamond search",
			       AttributeInfo::Integer, 0, 1));

    vs.push_back(AttributeInfo(xvid4str_adaptive_quant,
			       "Adaptive quantisation",
			       AttributeInfo::Integer, 0, 1));
    vs.push_back(AttributeInfo(xvid4str_halfpel,
			       "Halfpel interpolation",
			       AttributeInfo::Integer, 0, 1));
    vs.push_back(AttributeInfo(xvid4str_rc_buffer,
			       "Rate control buffer size",
			       AttributeInfo::Integer, 0, 10000, 20));
    vs.push_back(AttributeInfo(xvid4str_interlacing,
			       "Interlacing",
			       AttributeInfo::Integer, 0, 1));

    vs.push_back(AttributeInfo(xvid4str_min_quantizer,
			       "Minimum quantizer",
			       AttributeInfo::Integer, 1, 31, 1));
    vs.push_back(AttributeInfo(xvid4str_max_quantizer,
			       "Maximum quantizer",
			       AttributeInfo::Integer, 1, 31, 16));

    vs.push_back(AttributeInfo(xvid4str_max_key_interval,
			       "Maximum keyframe interval",
			       AttributeInfo::Integer, 1, 500, 100));
    vs.push_back(AttributeInfo(xvid4str_lum_masking,
			       "Luminance masking",
			       AttributeInfo::Integer, 0, 1));
#endif
    ci.push_back(CodecInfo(xvid_codecs, "XviD4", "", xvid_about,
			   CodecInfo::Plugin, "xvid4", CodecInfo::Video,
			   CodecInfo::Both, 0, vs, ds));
}

AVM_END_NAMESPACE;

#endif // XVID4_FILLPLUGINS_H
