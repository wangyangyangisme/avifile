#ifndef MP3LAMEBIN_FILLPLUGINS_H
#define MP3LAMEBIN_FILLPLUGINS_H

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

static void mp3lamebin_FillPlugins(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t mp3_codecs[] = { 0x55, 0 };
    static const char mp3_about[] =
	"Open-source MPEG layer-3 encoder, "
	"based on you currently installed "
	"libmp3lame library";

    static const char* stereo_opt[] =
    {
	"stereo",
	"joint",
	"dual",
        0
    };
    avm::vector<AttributeInfo> ea;
    ea.push_back(AttributeInfo("VBR", "VBR audio", AttributeInfo::Integer, 0, 1));
    ea.push_back(AttributeInfo("stereo_mode", "Stereo mode", stereo_opt));

    ci.push_back(CodecInfo(mp3_codecs, "Lame MPEG layer-3 encoder (runtime)", "",
			   mp3_about, CodecInfo::Plugin, "mp3lamebin", CodecInfo::Audio,
			   CodecInfo::Encode, 0, ea));
}

AVM_END_NAMESPACE;

#endif // MP3LAMEBIN_FILLPLUGINS_H
