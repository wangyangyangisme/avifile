#ifndef MP3LAME_FILLPLUGINS_H
#define MP3LAME_FILLPLUGINS_H

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

static void mp3lame_FillPlugins(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t mp3_codecs[] = { 0x55, 0 };
    static const char mp3_about[] =
	"Open-source MPEG layer-3 encoder, based on Lame Encoder 3.70.";
    ci.push_back(CodecInfo(mp3_codecs, "Lame 3.70 MPEG layer-3 encoder", "",
			   mp3_about, CodecInfo::Plugin, "mp3lame", CodecInfo::Audio,
			   CodecInfo::Encode));
}

AVM_END_NAMESPACE;

#endif // MP3LAME_FILLPLUGINS_H
