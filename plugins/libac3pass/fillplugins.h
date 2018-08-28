#ifndef AC3PASS_FILLPLUGINS_H
#define AC3PASS_FILLPLUGINS_H

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

static void ac3pass_FillPlugins(avm::vector<CodecInfo>& ac)
{
    static const fourcc_t ac3pass_codecs[] = { 0x2000, 0 };
    ac.push_back(CodecInfo(ac3pass_codecs, "AC3 pass-through", "",
			   "AC3 hardware pass through SPDIF on SBLive card",
			   CodecInfo::Plugin, "hwac3",
			   CodecInfo::Audio, CodecInfo::Decode));
}

AVM_END_NAMESPACE;

#endif // AC3PASS_FILLPLUGINS_H
