#ifndef XVID_FILLPLUGINS_H
#define XVID_FILLPLUGINS_H

#include "infotypes.h"
#include "avm_fourcc.h"

#ifdef HAVE_LIBXVIDCORE
#include "xvid.h"
#endif

AVM_BEGIN_NAMESPACE;

#define XVIDCSTR(name) \
    static const char* xvidstr_ ## name = #name

XVIDCSTR(mode);
XVIDCSTR(rc_bitrate);
XVIDCSTR(rc_period);
XVIDCSTR(rc_reaction_period);
XVIDCSTR(rc_reaction_ratio);
XVIDCSTR(rc_buffer);
XVIDCSTR(min_quantizer);
XVIDCSTR(max_quantizer);
XVIDCSTR(max_key_interval);
XVIDCSTR(motion_search);
XVIDCSTR(halfpel);
XVIDCSTR(lum_masking);
XVIDCSTR(interlacing);
XVIDCSTR(quant_type);

#if API_VERSION >= ((2 << 16) | (1))
XVIDCSTR(inter4v);
XVIDCSTR(diamond_search);
XVIDCSTR(adaptive_quant);
#endif

#if 0
XVIDCSTR(me_zero);
XVIDCSTR(me_logarithmic);
XVIDCSTR(me_fullsearch);
#endif

XVIDCSTR(me_pmvfast);
XVIDCSTR(me_epzs);

XVIDCSTR(postprocessing);
XVIDCSTR(maxauto);

static void xvid_FillPlugins(avm::vector<CodecInfo>& ci)
{
    static char xvid_about[] = "XviD MPEG-4 video codec";
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
    // it looks like XviD doesn't support postprocessing for now
    avm::vector<AttributeInfo> ds;
    //ds.push_back(AttributeInfo(xvidstr_maxauto, "Maxauto - unsupported",
    //    		       AttributeInfo::Integer, -1, -1));

// FIFO order is important - if this will ever change - few things
// would have to be repaired

    avm::vector<AttributeInfo> vs;
    vs.push_back(AttributeInfo(xvidstr_mode, "Mode", xvid_passopt));
    vs.push_back(AttributeInfo(xvidstr_rc_bitrate,
			       "Desired stream bitrate in bits/second",
			       AttributeInfo::Integer, 0, 10000000, 800000));
    vs.push_back(AttributeInfo(xvidstr_motion_search,
			       "Motion search ( 6 - Ultra high )",
			       AttributeInfo::Integer, 0, 6, 6));
    vs.push_back(AttributeInfo(xvidstr_quant_type,
			       "Quantization type", xvid_quantopt));
#if API_VERSION >= ((2 << 16) | (1))
    vs.push_back(AttributeInfo(xvidstr_inter4v,
			       "4 vectors per 16x16 block",
			       AttributeInfo::Integer, 0, 1));

    vs.push_back(AttributeInfo(xvidstr_diamond_search,
			       "Diamond search",
			       AttributeInfo::Integer, 0, 1));

    vs.push_back(AttributeInfo(xvidstr_adaptive_quant,
			       "Adaptive quantisation",
			       AttributeInfo::Integer, 0, 1));
#endif
#if API_VERSION >= (2 << 16)
    vs.push_back(AttributeInfo(xvidstr_halfpel,
			       "Halfpel interpolation",
			       AttributeInfo::Integer, 0, 1));
    vs.push_back(AttributeInfo(xvidstr_rc_buffer,
			       "Rate control buffer size",
			       AttributeInfo::Integer, 0, 10000, 20));
    vs.push_back(AttributeInfo(xvidstr_interlacing,
			       "Interlacing",
			       AttributeInfo::Integer, 0, 1));
#if 0
// one day they might be enabled
    vs.push_back(AttributeInfo(xvidstr_me_zero,
			       "Motion estimation ZERO",
			       AttributeInfo::Integer, 0, 1));
    vs.push_back(AttributeInfo(xvidstr_me_logarithmic,
			       "Motion estimation LOGARITHMIC",
			       AttributeInfo::Integer, 0, 1));
    vs.push_back(AttributeInfo(xvidstr_me_fullsearch,
			       "Motion estimation FULLSEARCH",
			       AttributeInfo::Integer, 0, 1));
#endif // undefined
    vs.push_back(AttributeInfo(xvidstr_me_pmvfast,
			       "Motion estimation PMVFAST",
			       AttributeInfo::Integer, 0, 1));
#ifdef XVID_ME_EPZS
    vs.push_back(AttributeInfo(xvidstr_me_epzs,
			       "Motion estimation EPZS",
			       AttributeInfo::Integer, 0, 1));
#endif
#else
    vs.push_back(AttributeInfo(xvidstr_rc_period,
			       "Rate control averaging period",
			       AttributeInfo::Integer, 0, 10000, 2000));
    vs.push_back(AttributeInfo(xvidstr_rc_reaction_period,
			       "Rate control reaction period",
			       AttributeInfo::Integer, 0, 100, 10));
    vs.push_back(AttributeInfo(xvidstr_rc_reaction_ratio,
			       "Rate control motion sensitivity",
			       AttributeInfo::Integer, 0, 100, 20));
#endif
    vs.push_back(AttributeInfo(xvidstr_min_quantizer,
			       "Minimum quantizer",
			       AttributeInfo::Integer, 1, 31, 1));
    vs.push_back(AttributeInfo(xvidstr_max_quantizer,
			       "Maximum quantizer",
			       AttributeInfo::Integer, 1, 31, 16));
    vs.push_back(AttributeInfo(xvidstr_max_key_interval,
			       "Maximum keyframe interval",
			       AttributeInfo::Integer, 1, 500, 100));
    vs.push_back(AttributeInfo(xvidstr_lum_masking,
			       "Luminance masking",
			       AttributeInfo::Integer, 0, 1));

    ci.push_back(CodecInfo(xvid_codecs, "XviD", "", xvid_about,
			   CodecInfo::Plugin, "xvid", CodecInfo::Video,
			   CodecInfo::Both, 0, vs, ds));
}

AVM_END_NAMESPACE;

#endif // XVID_FILLPLUGINS_H
