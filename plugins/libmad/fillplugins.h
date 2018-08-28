#ifndef MAD_FILLPLUGINS_H
#define MAD_FILLPLUGINS_H

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

#define MADCSTR(name) \
    static const char* madstr_ ## name = #name

MADCSTR(gain);

static void mad_FillPlugins(avm::vector<CodecInfo>& ac)
{
    static const fourcc_t mad_codecs[]={ 0x55, 0x50, 0 };
    avm::vector<AttributeInfo> d;
    d.push_back(AttributeInfo(madstr_gain, "Gain",
			      AttributeInfo::Integer, 1, 32, 8));
    ac.push_back(CodecInfo(mad_codecs, "MAD MPEG Layer-2/3", "",
			   "High quality MAD MPEG Layer-2/3 audio decoder "
                           "made by Robert Leslie <rob@mars.org> (c) 2000-2001",
			   CodecInfo::Plugin, "mad",
			   CodecInfo::Audio, CodecInfo::Decode, 0, 0, d));
}

AVM_END_NAMESPACE;

#endif // MAD_FILLPLUGINS_H
