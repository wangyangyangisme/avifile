#ifndef AUDIODEC_FILLPLUGINS_H
#define AUDIODEC_FILLPLUGINS_H

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

static void audiodec_FillPlugins(avm::vector<CodecInfo>& audiocodecs)
{
    static const fourcc_t pcm_codecs[] = { 0x01, 0x6172 /* raw */, 0 };
    static const fourcc_t alaw_codecs[] = { 0x06, 0 };
    static const fourcc_t ulaw_codecs[] = { 0x07, 0 };
    static const fourcc_t ima_adpcm_codecs[] = { 0x11, 0x200, 0 };
    static const fourcc_t gsm_codecs[] = { 0x31, 0x32, 0 };
    static const fourcc_t mpeg_codecs[] = { 0x50, 0x55, 0 };
    static const char* none_about_ad = "";
    audiocodecs.push_back(CodecInfo(pcm_codecs, "PCM", "", none_about_ad,
				    CodecInfo::Plugin, "pcm",
				    CodecInfo::Audio, CodecInfo::Decode));
    audiocodecs.push_back(CodecInfo(alaw_codecs, "ALaw", "", none_about_ad,
				    CodecInfo::Plugin, "alaw",
				    CodecInfo::Audio, CodecInfo::Decode));
    audiocodecs.push_back(CodecInfo(ulaw_codecs, "uLaw", "", none_about_ad,
				    CodecInfo::Plugin, "ulaw",
				    CodecInfo::Audio, CodecInfo::Decode));
    audiocodecs.push_back(CodecInfo(ima_adpcm_codecs, "IMA ADPCM", "", none_about_ad,
				    CodecInfo::Plugin, "imaadpcm",
				    CodecInfo::Audio, CodecInfo::Decode));
    audiocodecs.push_back(CodecInfo(gsm_codecs, "GSM", "", none_about_ad,
				    CodecInfo::Plugin, "msgsm",
				    CodecInfo::Audio, CodecInfo::Decode));
#ifdef HAVE_LIBA52
    static const fourcc_t ac3_codecs[] = { 0x2000, 0 };
    audiocodecs.push_back(CodecInfo(ac3_codecs, "A52", "",
				    none_about_ad,
				    CodecInfo::Plugin, "a52",
				    CodecInfo::Audio, CodecInfo::Decode));
#endif
}

AVM_END_NAMESPACE;

#endif // AUDIODEC_FILLPLUGINS_H
