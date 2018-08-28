#ifndef OSMJPEG_FILLPLUGINS_H
#define OSMJPEG_FILLPLUGINS_H

#include "infotypes.h"
#include "avm_fourcc.h"

AVM_BEGIN_NAMESPACE;

static void ijpg_FillPlugins(avm::vector<CodecInfo>& ci)
{
    static char ijpg_about[] = "OpenSource Motion JPEG codec, based on libjpeg.";
    static const char* dctm[] = { "IntSlow", "IntFast", "Float", 0 };
    static fourcc_t ijpg_codecs[] = { fccmjpg, fccMJPG,
    	mmioFOURCC('A', 'V', 'R', 'n'), mmioFOURCC('A', 'V', 'D', 'J'),
        0
    };

    avm::vector<AttributeInfo> vs;
    avm::vector<AttributeInfo> ds;
    vs.push_back(AttributeInfo("h_samp", "Horizontal sampling (kbps)", AttributeInfo::Integer, 1, 4));
    vs.push_back(AttributeInfo("v_samp", "Vertical sampling (kbps)", AttributeInfo::Integer, 1, 4));
    vs.push_back(AttributeInfo("quant_tbl", "Quantization table", AttributeInfo::Integer, 0, 1));
    vs.push_back(AttributeInfo("smoothing", "Smoothing (0..100)", AttributeInfo::Integer, 0, 100));
    vs.push_back(AttributeInfo("dct", "DCT Method", dctm ));

    static const char* dm[] = { "None", "Ordered", "Floyd-Steinberg", 0 };
    ds.push_back(AttributeInfo("dither", "Dither mode", dm));
    ds.push_back(AttributeInfo("dct", "DCT Method", dctm ));
    ds.push_back(AttributeInfo("upsampling", "Fancy upsampling", AttributeInfo::Integer, 0, 1));
    ds.push_back(AttributeInfo("smoothing", "Block smoothing", AttributeInfo::Integer, 0, 1));

    ci.push_back(CodecInfo(ijpg_codecs, "OS Motion JPEG", "",
			   ijpg_about, CodecInfo::Plugin, "ijpg",
                           CodecInfo::Video, CodecInfo::Both, 0, vs, ds));
}

AVM_END_NAMESPACE;

#endif // OSMJPEG_FILLPLUGINS_H
