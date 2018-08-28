#ifndef VORBIS_FILLPLUGINS_H
#define VORBIS_FILLPLUGINS_H

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

static void vorbis_FillPlugins(avm::vector<CodecInfo>& ci)
{
    // just WAVE_FORMAT_EXTENSIBLE
    static const fourcc_t vorbis_codecs[] = { 0xFFFE, 0 };
    static const GUID vorbis_clsid =
    {
        0x6bA47966, 0x3F83, 0x4178,
	{0x96, 0x65, 0x00, 0xF0, 0xBF, 0x62, 0x92, 0xE5}
    };

    ci.push_back(CodecInfo(vorbis_codecs, "Vorbis decoder", "",
			   "",
			   CodecInfo::Plugin, "vorbis",
			   CodecInfo::Audio, CodecInfo::Decode, &vorbis_clsid));
}

AVM_END_NAMESPACE;

#endif // VORBIS_FILLPLUGINS_H
