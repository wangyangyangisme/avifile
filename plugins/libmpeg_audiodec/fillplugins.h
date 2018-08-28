#ifndef MPEG_AUDIODEC_FILLPLUGINS_H
#define MPEG_AUDIODEC_FILLPLUGINS_H

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

#define MPEGADCSTR(name) \
    static const char* mpegadstr_ ## name = #name

MPEGADCSTR(gain);

static void mpeg_audiodec_FillPlugins(avm::vector<CodecInfo>& audiocodecs)
{
    static const fourcc_t mpeg_codecs[] = { 0x55, 0x50, 0 };
    avm::vector<AttributeInfo> d;
    d.push_back(AttributeInfo(mpegadstr_gain,
			      "Gain",
			      AttributeInfo::Integer, 1, 32, 8));
    audiocodecs.push_back(CodecInfo(mpeg_codecs, "MPEG Layer-1,2,3", "",
				    "Also known as MP3. This is the third coding "
				    "scheme for MPEG audio compression. MPEG Layer-3 "
				    "uses perceptual audio coding and psychoacoustic "
                                    "compression to remove parts of the audio "
				    "signal that are imperceptible to the human ear. "
				    "The result is a compression ratio up to 12:1 "
				    "without loss of audio quality. MP3 is a common "
				    "format for distributing music files over the Internet.",
				    CodecInfo::Plugin, "mp3splay", CodecInfo::Audio,
				    CodecInfo::Decode, 0, 0, d));
}

AVM_END_NAMESPACE;

#endif // MPEG_AUDIODEC_FILLPLUGINS_H
