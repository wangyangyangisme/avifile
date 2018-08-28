#ifndef FFMPEG_FILLPLUGINS_H
#define FFMPEG_FILLPLUGINS_H

#include "infotypes.h"
#include "avm_fourcc.h"

//#include "avcodec.h"
#ifndef int64_t_C
#define int64_t_C(c)     (c ## LL)
#define uint64_t_C(c)    (c ## ULL)
#endif
#include "avformat.h"
#include "opt.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h> // remove

AVM_BEGIN_NAMESPACE;

#define FFCSTR(name) \
    static const char* ffstr_ ## name = #name

FFCSTR(dr1);

static void libffmpeg_get_attr(avm::vector<AttributeInfo>& a, AVCodec* codec)
{
    if (!codec)
	return;
#if 0
    AVCodecContext* ctx = avcodec_alloc_context();
    static bool first = true;
    const AVOption* o = ctx->av_class->option;
    while (first) {
	if (o->name)
	    printf("OPT %s\n", o->name);
	else
	    first = false;
        o++;
    }
    free(ctx);
#endif

#if 0
    if (c) {
	const AVOption *stack[FF_OPT_MAX_DEPTH];
	int depth = 0;
	for (;;) {
            //printf("NAME  %s\n", c->name);
	    if (!c->name) {
		if (c->help) {
		    stack[depth++] = c;
		    c = (const AVOption*)c->help;
		} else {
		    if (depth == 0)
			break; // finished
		    c = stack[--depth];
                    c++;
		}
	    } else {
		int t = c->type & FF_OPT_TYPE_MASK;
		char* arr[100];
                char* dealloc;
#if 0
		printf("Config   %s  %s\n",
		       t == FF_OPT_TYPE_BOOL ? "bool   " :
		       t == FF_OPT_TYPE_DOUBLE ? "double  " :
		       t == FF_OPT_TYPE_INT ? "integer" :
		       t == FF_OPT_TYPE_STRING ? "string " :
		       "unknown??", c->name);
#endif
		switch (t) {
		case FF_OPT_TYPE_BOOL:
		    a.push_back(AttributeInfo(c->name, c->help,
					      AttributeInfo::Integer,
					      0, 1, (int)c->defval));
		    break;
		case FF_OPT_TYPE_DOUBLE:
		    a.push_back(AttributeInfo(c->name, c->help,
                                              c->defval, c->min,c->max));
		    break;
		case FF_OPT_TYPE_INT:
		    a.push_back(AttributeInfo(c->name, c->help,
					      AttributeInfo::Integer,
					      (int)c->min, (int)c->max,
					      (int)c->defval));
		    break;
		case FF_OPT_TYPE_STRING:
		    t = 0;
		    dealloc = 0;
		    if (c->defstr)
		    {
			char* tmp = dealloc = strdup(c->defstr);
			if (tmp)
			{
                            char* semi;
			    while (tmp && (semi = strchr(tmp, ';')))
			    {
				*semi++ = 0;
				arr[t++] = tmp;
				if (t > 97)
				    break; // weird - too many string options ???
				tmp = semi;
			    }
			    arr[t++] = tmp;
			}
			arr[t] = 0;
		    }
		    //printf("%d   %s\n", i, cnf->name);
		    a.push_back((t > 1) ? AttributeInfo(c->name, c->help, (const char**)arr, (int)c->defval)
				: AttributeInfo(c->name, c->help, AttributeInfo::String));
		    if (dealloc)
			free(dealloc);
		    break;
/*
		    if (c->defstr) {
			char* d = av_strdup(c->defstr);
			char* f = strchr(d, ',');
			if (f)
                            *f = 0;
			i += sprintf(def + i, "%s%s=%s",
				     col, c->name, d);
                        av_free(d);
		    }
		    break;
                    */
		}
		c++;
	    }
	}
    }
#endif
}

static void libffmpeg_fill_decattr(avm::vector<AttributeInfo>& a, const char* cd)
{
    a.clear();
    a.push_back(AttributeInfo(ffstr_dr1, "Direct Rendering 1", AttributeInfo::Integer, 0, 1, 1));
    libffmpeg_get_attr(a, avcodec_find_decoder_by_name(cd));
}

static void libffmpeg_fill_encattr(avm::vector<AttributeInfo>& a, const char* cd)
{
    a.clear();
    libffmpeg_get_attr(a, avcodec_find_encoder_by_name(cd));
}

static void libffmpeg_add_divx(avm::vector<CodecInfo>& ci)
{
    static const char divx_about[] =
	"FFMPEG LGPL version of popular M$ MPEG-4 video codec v3. "
	"Advanced compression technologies allow it to "
	"compress 640x480x25 video with a perfect "
	"quality into 100-150 kbytes/s ( 3-4 times "
	"less than MPEG-2 ).";

    static const fourcc_t div3_codecs[] = {
	fccDIV3, fccdiv3, fccDIV4, fccdiv4,
	fccDIV5, fccdiv5, fccDIV6, fccdiv6,
	fccMP41, fccMP43, RIFFINFO_MPG3, RIFFINFO_mpg3,
	fccAP41, fccap41, fccAP42, fccap42,
	mmioFOURCC('C', 'O', 'L', '1'), // Cool codec
	mmioFOURCC('c', 'o', 'l', '1'),
	mmioFOURCC('C', 'O', 'L', '0'),
	mmioFOURCC('c', 'o', 'l', '0'),
	mmioFOURCC('3', 'I', 'V', 'D'), // 3ivx.com
	mmioFOURCC('3', 'i', 'v', 'd'),
	0 };

    avm::vector<AttributeInfo> da;

    static const char* msmpeg4_str = "msmpeg4";
    libffmpeg_fill_decattr(da, msmpeg4_str);
    //da.push_back(AttributeInfo("Hue", "Hue", AttributeInfo::Integer, 0, 100));
    //da.push_back(AttributeInfo("Brightness", "Brightness", AttributeInfo::Integer, 0, 100));
    //da.push_back(AttributeInfo("Contrast", "Contrast", AttributeInfo::Integer, 0, 100));

    // for encoding
    ci.push_back(CodecInfo(div3_codecs, "FF DivX ;-)", msmpeg4_str,
			   divx_about, CodecInfo::Plugin, "ffdivx",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t opendix_codecs[] = {
	fccDIVX, fccdivx, RIFFINFO_XVID, RIFFINFO_XviD, RIFFINFO_xvid,
	fccMP4S, fccmp4s, mmioFOURCC('m', 'p', '4', 'v'),
	mmioFOURCC('U', 'M', 'P', '4'),
        mmioFOURCC('3', 'I', 'V', '1'),
	mmioFOURCC('3', 'I', 'V', '2'),
	mmioFOURCC('3', 'i', 'v', '2'),	0x4, 0 };
    static const char* mpeg4_str = "mpeg4";
    libffmpeg_fill_decattr(da, mpeg4_str);
    ci.push_back(CodecInfo(opendix_codecs, "FF OpenDivX", mpeg4_str,
			   "FF OpenDivX MPEG-4 codec",
			   CodecInfo::Plugin, "ffodivx",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t codecs[] = { fccDX50, fccdx50, 0 };
    //libffmpeg_fill_decattr(da, mpeg4_str);
    avm::vector<AttributeInfo> va;
    libffmpeg_fill_encattr(va, mpeg4_str);

    ci.push_back(CodecInfo(codecs, "FF DivX5", mpeg4_str,
			   "FF DivX 5.0 codec",
			   CodecInfo::Plugin, "ffdx50",
			   CodecInfo::Video,
			   //CodecInfo::Decode,
			   CodecInfo::Both, // working on
			   0, va, da));

    static const fourcc_t wmv_codecs[] = { fccWMV1, fccwmv1, 0 };
    static const char* wmv1_str = "wmv1";
    libffmpeg_fill_decattr(da, wmv1_str);
    ci.push_back(CodecInfo(wmv_codecs, "FF Windows Media Video 7", wmv1_str,
			   "FF Windows Media Video 7 codec",
			   CodecInfo::Plugin, "ffwmv1",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t wmv2_codecs[] = { fccWMV2, fccwmv2, 0 };
    static const char* wmv2_str = "wmv2";
    libffmpeg_fill_decattr(da, wmv2_str);
    ci.push_back(CodecInfo(wmv2_codecs, "FF Windows Media Video 8", wmv2_str,
			   "FF Windows Media Video 7 codec",
			   CodecInfo::Plugin, "ffwmv2",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t mp41_codecs[] = { fccMPG4, fccmpg4, fccDIV1, fccdiv1, 0 };
    static const char* msmpeg4v1 = "msmpeg4v1";
    libffmpeg_fill_decattr(da, msmpeg4v1);
    ci.push_back(CodecInfo(mp41_codecs, "FF M$ MPEG-4 v1", msmpeg4v1,
			   "FF M$ MPEG-4 v1 codec",
			   CodecInfo::Plugin, "ffmp41",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t mp42_codecs[] = { fccMP42, fccmp42, fccDIV2, fccdiv2, 0 };
    static const char* msmpeg4v2 = "msmpeg4v2";
    libffmpeg_fill_decattr(da, msmpeg4v2);
    ci.push_back(CodecInfo(mp42_codecs, "FF M$ MPEG-4 v2", msmpeg4v2,
			   "FF M$ MPEG-4 v2 codec",
			   CodecInfo::Plugin, "ffmp42",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t mpeg12_codecs[] = {
	RIFFINFO_MPG1, RIFFINFO_MPG2,
	0x10000001, 0x10000002, // mplayer's MPEG-PES 1/2
	0 };
    static const char* mpegvideo_str = "mpegvideo";
    libffmpeg_fill_decattr(da, mpegvideo_str);
    ci.push_back(CodecInfo(mpeg12_codecs, "FF MPEG 1/2", mpegvideo_str,
			   "FF MPEG1/2 decoder",
			   CodecInfo::Plugin, "ffmpeg",
			   CodecInfo::Video, CodecInfo::Decode,  0, 0, da));

    static const fourcc_t pim_codecs[] = { fccPIM1, 0 };
    ci.push_back(CodecInfo(pim_codecs, "FF PinnacleS PIM1", mpegvideo_str,
			   "FF PinnacleS PIM1",
			   CodecInfo::Plugin, "ffpim1",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

}

static void libffmpeg_add_vdec(avm::vector<CodecInfo>& ci)
{
    avm::vector<AttributeInfo> da;
    //libffmpeg_fill_attr(da, false);

    static const fourcc_t mjpg_codecs[] = {
	fccMJPG, fccmjpg,
	mmioFOURCC('A', 'V', 'R', 'n'),
	mmioFOURCC('A', 'V', 'D', 'J'),
	mmioFOURCC('J', 'P', 'E', 'G'),
	mmioFOURCC('j', 'p', 'e', 'g'),
	mmioFOURCC('m', 'j', 'p', 'b'),
	0 };
    static const char* mjpeg_str = "mjpeg";
    ci.push_back(CodecInfo(mjpg_codecs, "FF Motion JPEG", mjpeg_str,
			   "FF Motion JPEG",
			   CodecInfo::Plugin, "ffmjpeg",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t h264_codecs[] = {
	mmioFOURCC('H', '2', '6', '4'),
	mmioFOURCC('h', '2', '6', '4'),
	0 };
    static const char* h264_str = "h264";
    ci.push_back(CodecInfo(h264_codecs, "FF H264", h264_str,
			   "FF H263+ codec",
			   CodecInfo::Plugin, "ffh263",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t h263_codecs[] = {
	fccH263, fcch263, fccU263, fccu263,
	mmioFOURCC('s', '2', '6', '3'),
	0 };
    static const char* h263_str = "h263";
    ci.push_back(CodecInfo(h263_codecs, "FF H263+", h263_str,
			   "FF H263+ codec",
			   CodecInfo::Plugin, "ffh263",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t i263_codecs[] = { fccI263, fcci263, 0 };
    static const char* h263i_str = "h263i";
    ci.push_back(CodecInfo(i263_codecs, "FF I263", h263i_str,
			   "FF I263 codec",
			   CodecInfo::Plugin, "ffi263",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));

    static const fourcc_t dvsd_codecs[] = {
	fccDVSD, fccdvsd, fccdvhd, fccdvsl,
	mmioFOURCC('D', 'V', 'C', 'S'),
	mmioFOURCC('d', 'v', 'c', 's'),
	mmioFOURCC('d', 'v', 'c', 'p'),
	mmioFOURCC('d', 'v', 'c', ' '),
	0 };
    ci.push_back(CodecInfo(dvsd_codecs, "FF DV Video", "dvvideo",
			   "FF DV Video decoder", CodecInfo::Plugin, "ffdv",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));
    static const fourcc_t huf_codecs[] = { fccHFYU, 0 };
    ci.push_back(CodecInfo(huf_codecs, "FF Huffyuv", "huffyuv",
			   "FF Huffyuv codec",
			   CodecInfo::Plugin, "ffhuffyuv",
			   CodecInfo::Video, CodecInfo::Both, 0, 0, da));
    static const fourcc_t vp3_codecs[] = { fccVP31, fccvp31, fccVP30, fccVP30, 0 };
    ci.push_back(CodecInfo(vp3_codecs, "FF VP3", "vp3",
			   "FF VP3 codec",
			   CodecInfo::Plugin, "ffvp3",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));
    static const fourcc_t svq1_codecs[] = { mmioFOURCC('S', 'V', 'Q', '1'), 0 };
    ci.push_back(CodecInfo(svq1_codecs, "FF SVQ1", "svq1",
			   "FF Sorenson1 decoder", CodecInfo::Plugin, "ffsvq1",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));
    static const fourcc_t svq3_codecs[] = { mmioFOURCC('S', 'V', 'Q', '3'), 0 };
    ci.push_back(CodecInfo(svq3_codecs, "FF SVQ3", "svq3",
			   "FF Sorenson3 decoder", CodecInfo::Plugin, "ffsvq3",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));
    static const fourcc_t indeo3_codecs[] = {
	RIFFINFO_IV31, RIFFINFO_iv31, RIFFINFO_IV32, RIFFINFO_iv31, 0
    };
    ci.push_back(CodecInfo(indeo3_codecs, "FF Indeo 3", "indeo3",
			   "FF Indeo 3 decoder",
			   CodecInfo::Plugin, "ffindeo3",
			   CodecInfo::Video, CodecInfo::Decode));
    static const fourcc_t asv1_codecs[] = { fccASV1, 0 };
    ci.push_back(CodecInfo(asv1_codecs, "FF ASUSV1", "asv1",
			   "FF ASUS V1 codec", CodecInfo::Plugin, "ffasv1",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));
    static const fourcc_t ffv1_codecs[] = { mmioFOURCC('F', 'F', 'V', '1'), 0 };
    ci.push_back(CodecInfo(asv1_codecs, "FF FFV1", "ffv1",
			   "FF FFV1 looseless codec", CodecInfo::Plugin, "ffv1",
			   CodecInfo::Video, CodecInfo::Decode, 0, 0, da));
}

static void libffmpeg_add_ac3dec(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t ac3_codecs[] = { 0x2000, 0 };
    ci.push_back(CodecInfo(ac3_codecs, "FF AC3", "ac3",
			   "AC3 audio codec", CodecInfo::Plugin, "ffac3",
			   CodecInfo::Audio, CodecInfo::Decode));
}

static void libffmpeg_add_adec(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t mpg_codecs[] = { 0x55, 0x50,
    mmioFOURCC('.', 'm', 'p', '3'),
    0 };
    ci.push_back(CodecInfo(mpg_codecs, "FF MPEG Layer-3", "mp2",
			   "FF MPEG Layer-III audio decoder",
			   CodecInfo::Plugin, "ffmp3",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t alaw_codecs[] = { 0x06, 0 };
    ci.push_back(CodecInfo(alaw_codecs, "FF ALaw", "pcm_alaw",
			   "FF ALaw audio decoder",
			   CodecInfo::Plugin, "ffalaw",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t ulaw_codecs[] = { 0x07, 0 };
    ci.push_back(CodecInfo(ulaw_codecs, "FF uLaw", "pcm_mulaw",
			   "FF uLaw audio decoder",
			   CodecInfo::Plugin, "ffmulaw",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t wmav1_codecs[] = { 0x160, 0 };
    ci.push_back(CodecInfo(wmav1_codecs, "FF WMA v1", "wmav1",
			   "FF Window Media Audio v1 decoder",
			   CodecInfo::Plugin, "ffwmav1",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t wmav2_codecs[] = { 0x161, 0 };
    ci.push_back(CodecInfo(wmav2_codecs, "FF WMA v2", "wmav2",
			   "FF Window Media Audio v2 decoder",
			   CodecInfo::Plugin, "ffwmav2",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t ms_codecs[] = { 0x02, 0 };
    ci.push_back(CodecInfo(ms_codecs, "FF MS ADPCM", "adpcm_ms",
			   "FF MS ADPCM audio decoder",
			   CodecInfo::Plugin, "ffadpcmms",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t imaqt_codecs[] = { 0x11,
    0x6d69, // ima4 0x34616d69
    0 }; // unsure
    ci.push_back(CodecInfo(imaqt_codecs, "FF IMA Qt", "adpcm_ima_qt",
			   "FF IMA Qt audio decoder",
			   CodecInfo::Plugin, "ffimaqt",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t imawav_codecs[] = { 0x11, 0 }; // unsure
    ci.push_back(CodecInfo(imawav_codecs, "FF IMA WAV", "adpcm_ima_wav",
			   "FF IMA WAV audio decoder",
			   CodecInfo::Plugin, "ffimawav",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t dvaudio_codecs[] = { ('D' << 8) | 'A', 0 };
    ci.push_back(CodecInfo(dvaudio_codecs, "FF DV Audio", "dvaudio",
			   "FF DV Audio decoder",
			   CodecInfo::Plugin, "ffdva",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t mace6_codecs[] = { ('M' << 8) | 'A', 0x414d, 0 }; // unsure
    ci.push_back(CodecInfo(mace6_codecs, "FF MACE6 Qt", "mace6",
			   "FF Macintosh Audio Compression and Expansion 6:1",
			   CodecInfo::Plugin, "ffmac6",
			   CodecInfo::Audio, CodecInfo::Decode));


    static const fourcc_t ra144_codecs[] = { mmioFOURCC('1', '4', '_', '4'), 0x3431, 0 };
    ci.push_back(CodecInfo(ra144_codecs, "FF Real 144", "real_144",
			   "FF Real Audio 14.4kbps",
			   CodecInfo::Plugin, "ffra144",
			   CodecInfo::Audio, CodecInfo::Decode));
    static const fourcc_t ra288_codecs[] = { mmioFOURCC('2', '8', '_', '8'), 0x3431, 0 };
    ci.push_back(CodecInfo(ra144_codecs, "FF Real 288", "real_288",
			   "FF Real Audio 28.8kbps",
			   CodecInfo::Plugin, "ffra288",
			   CodecInfo::Audio, CodecInfo::Decode));
    static const fourcc_t mpeg4aac_codecs[] = {
	(('p' << 8) | 'm'), (('P' << 8) | 'M'),
	mmioFOURCC('m', 'p', '4', 'a'), mmioFOURCC('M', 'P', '4', 'A'),
	0
    };
    ci.push_back(CodecInfo(mpeg4aac_codecs, "FAAD (runtime)", "mpeg4aac",
			   "FAAD - MPEG2/MPEG4 AAC"
			   "Freeware Advanced Audio Decoder, "
			   "http://www.audiocoding.com/"
			   "Copyright (C) 2002 M. Bakker",
			   CodecInfo::Plugin, "ffmpeg4aac",
			   CodecInfo::Audio, CodecInfo::Decode));
}

static void ffmpeg_FillPlugins(avm::vector<CodecInfo>& ci)
{
    // video
    libffmpeg_add_divx(ci);
    libffmpeg_add_vdec(ci);

    // audio
#ifdef HAVE_FFMPEG_A52
    libffmpeg_add_ac3dec(ci);
#endif
    libffmpeg_add_adec(ci);
}

AVM_END_NAMESPACE;

#endif // FFMPEG_FILLPLUGINS_H
