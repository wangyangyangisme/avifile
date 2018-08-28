#ifndef WIN32_FILLPLUGINS_H
#define WIN32_FILLPLUGINS_H

#include "infotypes.h"
#include "avm_fourcc.h"

AVM_BEGIN_NAMESPACE;

static const char* none_about =
    "No help available for this codec.";

static void add_divx(avm::vector<CodecInfo>& ci)
{
    static const char* divx_about =
	"Very popular MPEG-4 video codec, based on "
	"Microsoft MPEG-4 implementation. Advanced "
	"compression technologies allow it to "
	"compress 640x480x25 video with a perfect "
	"quality into 100-150 kbytes/s ( 3-4 times "
	"less than MPEG-2 ), by the cost of increased "
	"requirements for playback.<br>"
	"See <a href=\"http://www.divx.com\">http://www.divx.com</a> and "
	"<a href=\"http://www.mydivx.com\">http://www.mydivx.com</a>";
    static const GUID CLSID_WMV1 =
    {
	0x4facbba1, 0xffd8, 0x4cd7,
	{0x82, 0x28, 0x61, 0xe2, 0xf6, 0x5c, 0xb1, 0xae}
    };
    static const GUID CLSID_WMV2 =
    {
	0x521fb373, 0x7654, 0x49f2,
	{0xbd, 0xb1, 0x0c, 0x6e, 0x66, 0x60, 0x71, 0x4f}
    };
    static const GUID CLSID_WMV2DMO =
    {
	0x12b31b03, 0xaf47, 0x4106,
	{0xbb, 0xe8, 0xea, 0x29, 0x60, 0x28, 0x2c, 0x50}
    };
    static const GUID CLSID_WMV3DMO =
    {
	0x82d353df, 0x90bd, 0x4382,
	{0x8b, 0xc2, 0x3f, 0x61, 0x92, 0xb7, 0x6e, 0x34}
    };
    static const GUID CLSID_WMV39DMO =
    {
	0x724bb6a4, 0xe526, 0x450f,
	{0xaf, 0xfa, 0xab, 0x9b, 0x45, 0x12, 0x91, 0x11}
    };
#ifndef GUIDS_H
    static const GUID CLSID_DivxDecompressorCF =
    {
	0x82ccd3e0, 0xf71a, 0x11d0,
	{ 0x9f, 0xe5, 0x00, 0x60, 0x97, 0x78, 0xaa, 0xaa}
    };
#endif
#define DIVX_FCCS \
    fccDIV3, fccdiv3, fccDIV4, fccdiv4, \
    fccDIV5, fccdiv5, fccDIV6, fccdiv6, \
    fccMP41, fccMP43, fccmp43, \
    fccAP41, fccap41, fccAP42, fccap42

    static const fourcc_t wmv1_codecs[] = { fccWMV1, fccwmv1, 0 };
    /* it may be able to decode wmv1, but it's not */
    static const fourcc_t wmv2_codecs[] = { fccWMV2, fccwmv2, 0 };

    static const fourcc_t wmv3_codecs[] = {
	RIFFINFO_WMV3, RIFFINFO_wmv3, fccWMV2, fccwmv2, fccWMV1, fccwmv1,
        mmioFOURCC('M', 'S', 'S', '2'),
	0
    };
    static const fourcc_t wmv39_codecs[] = { RIFFINFO_WMV3, RIFFINFO_wmv3, 0 };
    /* divx directshow is able to decode large variety of formats */
    static const fourcc_t div3_codecsd[] = { DIVX_FCCS, 0 };
    static const fourcc_t div5_codecsd[] = { fccDIV5, DIVX_FCCS, 0 };
    static const fourcc_t div3_codecs[] = { fccDIV3, fccdiv3, DIVX_FCCS, 0 };
    static const fourcc_t div4_codecs[] = { fccDIV4, fccdiv4, 0 };
    // using different codec - divxcfvk.ax when available
    static const fourcc_t div5_codecs[] = { fccDIV5, fccdiv5, fccDIV3, fccdiv3, 0 };
    static const fourcc_t div6_codecs[] = { fccDIV6, fccdiv6, fccDIV4, fccdiv4, 0 };

// FIFO order is important - if this will ever change - few things
// would have to be repaired

    avm::vector<AttributeInfo> vs;
    avm::vector<AttributeInfo> vs_empty;
    avm::vector<AttributeInfo> ds;
    vs.push_back(AttributeInfo("BitRate", "BitRate (kbps)", AttributeInfo::Integer, 0, 10000));
    vs.push_back(AttributeInfo("Crispness", "Crispness (0-100)", AttributeInfo::Integer, 0, 100, 100));
    vs.push_back(AttributeInfo("KeyFrames", "Keyframes (in sec)", AttributeInfo::Integer, 0, 30, 3));
    //ds.push_back(AttributeInfo("Quality", "Quality/CPU balance ( 0 fastest )", AttributeInfo::Integer, 0, 4));

    avm::vector<AttributeInfo> ds1;
    ds1.push_back(AttributeInfo("Quality", "Quality/CPU balance ( 0 fastest )", AttributeInfo::Integer, 0, 4));
    ds1.push_back(AttributeInfo("maxauto", "Maximum autoquality level",
				AttributeInfo::Integer, 0, 4, 4));
    ds1.push_back(AttributeInfo("Brightness", "Brightness", AttributeInfo::Integer, 0, 100));
    ds1.push_back(AttributeInfo("Contrast", "Contrast", AttributeInfo::Integer, 0, 100));
    ds1.push_back(AttributeInfo("Saturation", "Saturation", AttributeInfo::Integer, 0, 100));
    ds1.push_back(AttributeInfo("Hue", "Hue", AttributeInfo::Integer, 0, 100));

    // DS player
    ci.push_back(CodecInfo(div5_codecsd, "W32 DivX ;-) VKI DirectShow", "divxcvki.ax",
			   divx_about, CodecInfo::DShow_Dec, "divxvkids",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_DivxDecompressorCF, vs_empty, ds1));
    // DS player
    ci.push_back(CodecInfo(div3_codecsd, "W32 DivX ;-) DirectShow", "divx_c32.ax",
			   divx_about, CodecInfo::DShow_Dec, "divxds",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_DivxDecompressorCF, vs_empty, ds1));
    // for encoding
    ci.push_back(CodecInfo(div5_codecs,	"W32 DivX ;-) VKI (Low-Motion)", "divxcvki.dll",
			   divx_about, CodecInfo::Win32, "divxvkil",
			   CodecInfo::Video, CodecInfo::Both, 0, vs, ds));
    // for encoding
    ci.push_back(CodecInfo(div6_codecs, "W32 DivX ;-) VKI (Fast-Motion)", "divxcfvk.dll",
			   divx_about, CodecInfo::Win32, "divxvkif",
			   CodecInfo::Video, CodecInfo::Both, 0, vs, ds));
    // for encoding
    ci.push_back(CodecInfo(div3_codecs,	"W32 DivX ;-) Low-Motion", "divxc32.dll",
			   divx_about, CodecInfo::Win32, "divx",
			   CodecInfo::Video, CodecInfo::Both, 0, vs, ds));
    // for encoding
    ci.push_back(CodecInfo(div4_codecs, "W32 DivX ;-) Fast-Motion", "divxc32f.dll",
			   divx_about, CodecInfo::Win32, "divxf",
			   CodecInfo::Video, CodecInfo::Both, 0, vs, ds));

    ci.push_back(CodecInfo(wmv1_codecs, "W32 Windows Media Video 7 DirectShow",
			   "wmvds32.ax",
			   none_about, CodecInfo::DShow_Dec, "wmv7",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_WMV1, vs_empty, ds1));
    ci.push_back(CodecInfo(wmv2_codecs, "W32 Windows Media Video 8 DirectShow",
			   "wmv8ds32.ax",
			   none_about, CodecInfo::DShow_Dec, "wmv8",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_WMV2, vs_empty, ds1));
    avm::vector<AttributeInfo> wmattr;
    wmattr.push_back(AttributeInfo("Quality", "Quality/CPU balance ( 0 fastest )", AttributeInfo::Integer, 0, 4));
    wmattr.push_back(AttributeInfo("Fake Player Behind", "Fake Player Behind", AttributeInfo::Integer, 0, 1));
    wmattr.push_back(AttributeInfo("Omit BF Mode", "Omit BF Mode", AttributeInfo::Integer, 0, 1));
    wmattr.push_back(AttributeInfo("Adapt Post Process Mode", "Adapt Post Process Mode", AttributeInfo::Integer, 0, 1));
    wmattr.push_back(AttributeInfo("Deinterlace", "Deinterlace", AttributeInfo::Integer, 0, 1));

    ci.push_back(CodecInfo(wmv3_codecs,	"W32 Windows Media Video DMO",
			   "wmvdmod.dll",
			   none_about, CodecInfo::DMO, "wmvdmod",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_WMV3DMO, 0, wmattr));
    ci.push_back(CodecInfo(wmv39_codecs, "W32 Windows Media Video 9 DMO",
			   "wmv9dmod.dll",
			   none_about, CodecInfo::DMO, "wmv9dmod",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_WMV39DMO));
#if 0
    ci.push_back(CodecInfo(wmv1_codecs,	"W32 WMV enc dll",
			   "wmvdmoe.dll",
			   none_about, CodecInfo::Win32, "wmvdmo_enc",
			   CodecInfo::Video, CodecInfo::Encode));
#endif
#if 0
    // doesn't work 0x80004005
    ci.push_back(CodecInfo(wmv2_codecs,	"W32 WMV8 DMO decoder",
			   "wmv8dmod.dll",
			   none_about, CodecInfo::DMO, "wmv8dmod",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_WMV2DMO));
#endif
    static const char* mpg4_about =
	"Old beta version of Microsoft MPEG-4 "
	"video codec, incompatible with DivX ;-).";
    static const fourcc_t mpg4_codecs[]=
    {
	fccMP42, fccmp42, fccmp43,
        fccMPG4, fccmpg4, fccDIV2, fccdiv2, 0
    };
    ci.push_back(CodecInfo(mpg4_codecs, "W32 Microsoft MPEG-4 DirectShow", "mpg4ds32.ax",
			   mpg4_about, CodecInfo::DShow_Dec, "mpeg4ds",
			   CodecInfo::Video, CodecInfo::Decode));
    ci.push_back(CodecInfo(mpg4_codecs, "W32 Microsoft MPEG-4", "mpg4c32.dll",
			   mpg4_about, CodecInfo::Win32, "mpeg4",
			   CodecInfo::Video, CodecInfo::Both, 0, vs, ds));

}

static void add_angel(avm::vector<CodecInfo>& ci)
{
    // crashing - using divx instead of this
    static const fourcc_t apxx_codecs[] = { fccAP41, fccap41, fccAP42, fccap42, 0 };

    ci.push_back(CodecInfo(apxx_codecs, "W32 AngelPotion MPEG-4", "apmpg4v1.dll",
			   none_about, CodecInfo::Win32, "angelpotion"));
}

static void add_ati(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t vcr1_codecs[] = { fccVCR1, 0 };
    static const fourcc_t vcr2_codecs[] = { fccVCR2, 0 };

    ci.push_back(CodecInfo(vcr1_codecs, "W32 ATI VCR-1", "ativcr1.dll",
			   none_about, CodecInfo::Win32, "vcr1",
			   CodecInfo::Video, CodecInfo::Decode));

    ci.push_back(CodecInfo(vcr2_codecs, "W32 ATI VCR-2", "ativcr2.dll",
			   none_about, CodecInfo::Win32, "vcr2",
			   CodecInfo::Video, CodecInfo::Decode));
}

static void add_asus(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t asv1_codecs[] = { fccASV1, 0 };
    static const fourcc_t asv2_codecs[] = { fccASV2, 0 };

    ci.push_back(CodecInfo(asv1_codecs, "W32 ASUS V1 - crash", "asusasvd.dll",
			   none_about, CodecInfo::Win32, "asv1",
			   CodecInfo::Video, CodecInfo::Decode));

    ci.push_back(CodecInfo(asv2_codecs, "W32 ASUS V2", "asusasv2.dll",
			   none_about, CodecInfo::Win32, "asv2",
                           CodecInfo::Video, CodecInfo::Decode));
}

static void add_brooktree(avm::vector<CodecInfo>& ci)
{
    static const char* brook_about = "W32 BtV Media Stream Version 1.0\n"
	"Copyright Brooktree Corporation 1994-1997\n";
    static const fourcc_t bt20_codecs[] = {
	mmioFOURCC('B', 'T', '2', '0'), 0 };
    static const fourcc_t yuv411_codecs[] = {
	mmioFOURCC('Y', '4', '1', 'P'), 0 };
    static const fourcc_t yvu9_codecs[] = {
	mmioFOURCC('Y', 'V', 'Y', '9'), 0 };

    // missing: Called unk_GetWindowDC

    ci.push_back(CodecInfo(bt20_codecs, "W32 Brooktree(r) ProSummer Video",
			   "btvvc32.drv",
			   brook_about, CodecInfo::Win32, "btree",
                           CodecInfo::Video, CodecInfo::Both));
    ci.push_back(CodecInfo(yuv411_codecs, "W32 Brooktree(r) YUV411 Raw",
			   "btvvc32.drv",
			   brook_about, CodecInfo::Win32, "btree_yuv411",
                           CodecInfo::Video, CodecInfo::Both));

    ci.push_back(CodecInfo(yvu9_codecs, "W32 Brooktree(r) YVU9 Raw",
			   "btvvc32.drv",
			   brook_about, CodecInfo::Win32, "btree_yvu9",
                           CodecInfo::Video, CodecInfo::Decode));

}

static void add_dvsd(avm::vector<CodecInfo>& ci)
{
    static const char* dvsd_about = "W32 Sony Digital Video (DV)";
    static const fourcc_t dvsd_codecs[] = {
	fccdvsd, fccDVSD, fccdvhd, fccdvsl, 0 };
    static const GUID CLSID_DVSD =
    {
	0xB1B77C00, 0xC3E4, 0x11CF,
	{0xAF, 0x79, 0x00, 0xAA, 0x00, 0xB6, 0x7A, 0x42}
    };
    ci.push_back(CodecInfo(dvsd_codecs,	"W32 DVSD (MainConcept)", "qdv.dll",
			   dvsd_about, CodecInfo::DShow_Dec, "qdv",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_DVSD));
    //  not a true Win32 library
    //ci.push_back(CodecInfo(dvsd_codecs,
    //    				"DVSD",	"qdv.dll",
    //    				dvsd_about, CodecInfo::Win32,
    //    				CodecInfo::Video, CodecInfo::Both));
}

static void add_divx4(avm::vector<CodecInfo>& ci)
{
    // External func COMCTL32.dll:17
#ifndef GUIDS_H
    static const GUID IID_IDivxFilterInterface =
    {
	0x78766964, 0x0000, 0x0010,
	{0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
    };
#endif
    avm::vector<AttributeInfo> vs_empty;
    avm::vector<AttributeInfo> ds;
    ds.push_back(AttributeInfo("postprocessing",
			       "Image postprocessing mode ( 6 slowest )",
			       AttributeInfo::Integer, 0, 6));
    ds.push_back(AttributeInfo("maxauto", "Maximum autoquality level",
			       AttributeInfo::Integer, 0, 6));
    ds.push_back(AttributeInfo("Brightness", "Brightness", AttributeInfo::Integer, -128, 127));
    ds.push_back(AttributeInfo("Contrast", "Contrast", AttributeInfo::Integer, -128, 127));
    ds.push_back(AttributeInfo("Saturation", "Saturation", AttributeInfo::Integer, -128, 127));
    avm::vector<AttributeInfo> ds1;
    ds1.push_back(AttributeInfo("postprocessing",
				"Image postprocessing mode ( 6 slowest )",
				AttributeInfo::Integer, 0, 6));
    static const char* opendivxw_about = "W32 DivX 4.0 Beta Codec MPEG-4";
    static const fourcc_t opendivxw_codecs[] =
    {
	fccDIVX, fccdivx, fccDIV1, fccdiv1, fccMP4S, fccmp4s, 0x4,
        RIFFINFO_XVID, RIFFINFO_XviD, RIFFINFO_xvid,
	0
    };
    static const fourcc_t div3_codecs[] =
    {
	// DIV3 - works but only for some movies
	fccDIVX, fccDIV3, fccdiv3, fccDIV4, fccdiv4,
	fccDIV5, fccdiv5, fccDIV6, fccdiv6,
	fccMP41, fccMP43, fccmp43,
	// unsupported fccAP41, fccap41, fccAP42, fccap42,
	0
    };
    static const fourcc_t divxall_codecs[] =
    {
	fccDIVX, fccdivx, fccDIV1, fccdiv1, fccMP4S, fccmp4s, 0x4,
	fccDIVX, fccDIV3, fccdiv3, fccDIV4, fccdiv4,
	fccDIV5, fccdiv5, fccDIV6, fccdiv6,
	fccMP41, fccMP43, fccmp43, 0
    };

    // implementation of some Direct Draw interface is not necessary
    // check fails but codec works
    ci.push_back(CodecInfo(opendivxw_codecs, "W32 DivX4 OpenDivX DirectShow",
			   "divxdec.ax",
			   opendivxw_about, CodecInfo::DShow_Dec, "divx4ds",
			   CodecInfo::Video, CodecInfo::Decode,
			   &IID_IDivxFilterInterface, vs_empty, ds));
    ci.push_back(CodecInfo(div3_codecs, "W32 DivX4 DivX ;-) DirectShow",
			   "divxdec.ax",
			   opendivxw_about, CodecInfo::DShow_Dec, "divx4ds311",
			   CodecInfo::Video, CodecInfo::Decode,
			   &IID_IDivxFilterInterface, vs_empty, ds));

    ci.push_back(CodecInfo(divxall_codecs, "W32 DivX4 4.0 Beta Codec",
			   "divx.dll",
			   opendivxw_about, CodecInfo::Win32, "divx4vfw",
			   CodecInfo::Video,
			   // there is some bug while checking for support
                           // for YUY2 & YV12 support for encoder  FIXME
			   CodecInfo::Decode,
			   //CodecInfo::Both,
			   &IID_IDivxFilterInterface, vs_empty, ds1));
}

static void add_divx5(avm::vector<CodecInfo>& ci)
{
    // maybe we could add support for DirectShow Win32 filter later
#ifndef GUIDS_H
    static const GUID IID_IDivxFilterInterface =
    {
	0x78766964, 0x0000, 0x0010,
	{0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
    };
#endif
    avm::vector<AttributeInfo> vs_empty;
    avm::vector<AttributeInfo> ds;
    ds.push_back(AttributeInfo("postprocessing",
			       "Image postprocessing mode ( 6 slowest )",
			       AttributeInfo::Integer, 0, 6));
    ds.push_back(AttributeInfo("maxauto", "Maximum autoquality level",
			       AttributeInfo::Integer, 0, 6));
    ds.push_back(AttributeInfo("Brightness", "Brightness", AttributeInfo::Integer, -128, 127));
    ds.push_back(AttributeInfo("Contrast", "Contrast", AttributeInfo::Integer, -128, 127));
    ds.push_back(AttributeInfo("Saturation", "Saturation", AttributeInfo::Integer, -128, 127));
    avm::vector<AttributeInfo> ds1;
    ds1.push_back(AttributeInfo("postprocessing",
				"Image postprocessing mode ( 6 slowest )",
				AttributeInfo::Integer, 0, 6));
    static const char* opendivxw_about = "W32 DivX 5.0 MPEG-4";
    static const fourcc_t opendivxw_codecs[] =
    {
	fccDIVX, fccdivx, fccDIV1, fccdiv1, fccMP4S, fccmp4s, 0x4, 0
    };
    static const fourcc_t div3_codecs[] =
    {
	// DIV3 - works but only for some movies

	fccDIVX, fccDIV3, fccdiv3, fccDIV4, fccdiv4,
	fccDIV5, fccdiv5, fccDIV6, fccdiv6,
	fccMP41, fccMP43, fccmp43,
	fccAP41, fccap41, fccAP42, fccap42, 0
    };
    static const fourcc_t divxall_codecs[] =
    {
	fccDX50, fccdx50,
	fccDIVX, fccdivx, fccDIV1, fccdiv1, fccMP4S, fccmp4s, 0x4,
	fccDIVX, fccDIV3, fccdiv3, fccDIV4, fccdiv4,
	fccDIV5, fccdiv5, fccDIV6, fccdiv6,
	fccMP41, fccMP43, fccmp43,
	fccAP41, fccap41, fccAP42, fccap42, 0
    };

    // implementation of some Direct Draw interface is not necessary
    // check fails but codec works
    ci.push_back(CodecInfo(divxall_codecs, "W32 DivX5 5.0 DirectShow",
			   "divxdec.ax",
			   opendivxw_about, CodecInfo::DShow_Dec, "divx4ds",
			   CodecInfo::Video, CodecInfo::Decode,
			   &IID_IDivxFilterInterface, vs_empty, ds));
    ci.push_back(CodecInfo(div3_codecs, "W32 DivX5 DirectShow 3.11 compatible decoder",
			   "divxdec.ax",
			   opendivxw_about, CodecInfo::DShow_Dec, "divx4ds311",
			   CodecInfo::Video, CodecInfo::Decode,
			   &IID_IDivxFilterInterface, vs_empty, ds));
#if 0
    // is there someone who could 'make this' usable outside of Win32 :)
    ci.push_back(CodecInfo(divxall_codecs, "W32 DivX5 5.0 Codec MPEG-4",
			   "divx5.dll",
			   opendivxw_about, CodecInfo::Win32, "divx5vfw",
			   CodecInfo::Video,
			   CodecInfo::Decode,
			   &IID_IDivxFilterInterface, vs_empty, ds1));
#endif
}

static void add_xvid(avm::vector<CodecInfo>& ci)
{
    // maybe we could add support for DirectShow Win32 filter later
    avm::vector<AttributeInfo> vs_empty;
    avm::vector<AttributeInfo> ds;
    ds.push_back(AttributeInfo("post_enabled", "Enable postprocessing",
			       AttributeInfo::Integer, 0, 1));
    ds.push_back(AttributeInfo("post_mv_visible", "Show motion vectors",
			       AttributeInfo::Integer, 0, 1));
    ds.push_back(AttributeInfo("post_histogram", "Show histogram",
			       AttributeInfo::Integer, 0, 1));
    ds.push_back(AttributeInfo("post_comparision", "Show comparision",
			       AttributeInfo::Integer, 0, 1));
    ds.push_back(AttributeInfo("post_brightness", "Brightness", AttributeInfo::Integer, 0, 100));
    ds.push_back(AttributeInfo("post_contrast", "Contrast", AttributeInfo::Integer, 0, 100));
    ds.push_back(AttributeInfo("post_saturation", "Saturation", AttributeInfo::Integer, 0, 100));

    avm::vector<AttributeInfo> ds1;
    static const char* xvid_about =
	"W32 XviD MPEG-4 (extra)\n"
	"this code is very cool if you will download\n"
	"Win32 xvid.dll from:\n"
	"<a href=\"http://www.geocities.com/avihpit/xvid/post.html\">"
	"http://www.geocities.com/avihpit/xvid/post.html</a>";
    static const fourcc_t xvid_codecs[] =
    {
	fccXVID,
	fccDIVX, fccdivx, 0
    };

    static const GUID IID_IXvidDecoder =
    {
	0x00000000, 0x4fef, 0x40d3,
	{0xb3, 0xfa, 0xe0, 0x53, 0x1b, 0x89, 0x7f, 0x98}
    };

    ci.push_back(CodecInfo(xvid_codecs, "W32 XviD MPEG-4 Video Decoder",
			   "xvid.ax",
			   xvid_about, CodecInfo::DShow_Dec, "xvidds",
			   CodecInfo::Video, CodecInfo::Decode,
			   &IID_IXvidDecoder, vs_empty, ds));

    ci.push_back(CodecInfo(xvid_codecs, "W32 XviD MPEG-4 Codec",
			   "xvid.dll",
			   xvid_about, CodecInfo::Win32, "xvidvfw",
			   CodecInfo::Video, CodecInfo::Decode,
			   0, vs_empty, ds));
}

static void add_indeo(avm::vector<CodecInfo>& ci)
{
    static const char* ivxx_about =
	"A set of wavelet video codecs, developed "
	"by Intel and currently owned by Ligos corp. "
	"Indeo Video 5.0 with turned on \"QuickCompress\" "
	"option is a good choice for real-time video "
	"capture - it is faster than DivX ;-) and compresses "
	"data better than Motion JPEG.<br>"
	"Home page: <a href=\"http://www.ligos.com/indeo\">"
	"http://www.ligos.com/indeo</a>";
#ifndef GUIDS_H
    static const GUID CLSID_IV50_Decoder =
    {
	0x30355649, 0x0000, 0x0010,
	{0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
    };
#endif
    static const fourcc_t iv50_codecs[] = { fccIV50, fcciv50, 0 };
    static const fourcc_t iv41_codecs[] = { fccIV41, fcciv41, 0 };
    static const fourcc_t iv32_codecs[] = { fccIV32, fcciv32, 0 };
    static const fourcc_t iv31_codecs[] = { fccIV31, fcciv31, 0 };
    static const fourcc_t iv3132_codecs[] = { fccIV31, fcciv31, fccIV32, fcciv32, 0 };
    avm::vector<AttributeInfo> vs;
    avm::vector<AttributeInfo> ds;

    ds.push_back(AttributeInfo("Brightness", "Brightness", AttributeInfo::Integer, -100, 100));
    ds.push_back(AttributeInfo("Contrast", "Contrast", AttributeInfo::Integer, -100, 100));
    ds.push_back(AttributeInfo("Saturation", "Saturation", AttributeInfo::Integer, -100, 100));
    ci.push_back(CodecInfo(iv50_codecs, "W32 Indeo(r) Video 5.0 DirectShow", "ir50_32.dll",
			   ivxx_about, CodecInfo::DShow_Dec, "indeo5ds",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_IV50_Decoder, vs, ds));
    vs.push_back(AttributeInfo("QuickCompress", "Quick Compress", AttributeInfo::Integer, 0, 1));
    //vs.push_back(AttributeInfo("Transparency", "Transparency", AttributeInfo::Integer, 0, 1));
    //vs.push_back(AttributeInfo("Scalability", "Scalability", AttributeInfo::Integer, 0, 1));
    ci.push_back(CodecInfo(iv50_codecs, "W32 Indeo(r) Video 5.04", "ir50_32.dll",
			   ivxx_about, CodecInfo::Win32, "indeo5",
			   CodecInfo::Video, CodecInfo::Both, 0, vs));
    ci.push_back(CodecInfo(iv41_codecs,	"W32 Indeo(r) Video 4.1", "ir41_32.dll",
			   ivxx_about, CodecInfo::Win32, "indeo4",
			   CodecInfo::Video, CodecInfo::Both, 0, vs));
    ci.push_back(CodecInfo(iv3132_codecs, "W32 Indeo(r) Video 3.1/3.2 decoder", "ir32_32.dll",
			   ivxx_about, CodecInfo::Win32, "indeo3",
			   CodecInfo::Video, CodecInfo::Decode));
#if 1
    /* these two doesn't work and causes crashes */
    ci.push_back(CodecInfo(iv32_codecs, "W32 Indeo(r) Video 3.2", "ir32_32.dll",
			   ivxx_about, CodecInfo::Win32, "indeo32_enc",
			   CodecInfo::Video, CodecInfo::Encode));
    //ci.push_back(CodecInfo(iv31_codecs, "Indeo Video 3.1", "ir32_32.dll",
    //    		   ivxx_about, CodecInfo::Win32, "indeo31_enc",
    //    		   CodecInfo::Video, CodecInfo::Encode));
#endif
}

static void add_morgan(avm::vector<CodecInfo>& ci)
{
    static const char* mjpg_about =
	"Very fast Motion JPEG video codec, by Morgan "
	"Multimedia company. Recommended for "
	"video capture on slow machines or in high "
	"resolutions. Shareware. Current version "
	"is time-limited and will stop working after "
	"Mar. 1, 2001. Registration costs $25. Visit "
	"their web site for details.<br>"
	"Web site: <a href=\"http://www.morgan-multimedia.com\">"
	"http://www.morgan-multimedia.com</a>";
    static const GUID CLSID_MorganMjpeg =
    {
	0x6988b440, 0x8352, 0x11d3,
	{0x9b, 0xda, 0xca, 0x86, 0x73, 0x7c, 0x71, 0x68}
    };
    static const fourcc_t mjpg_codecs[] = { fccMJPG, fccmjpg, 0 };

    static const char* mjpeg_modes[] =
    {
	"Fast integer",	"Integer", "Float", 0
    };

    avm::vector<AttributeInfo> vs;

    vs.push_back(AttributeInfo("Mode", "Calculation precision mode", (const char**)mjpeg_modes));
    vs.push_back(AttributeInfo("LicenseKey", "License key", AttributeInfo::String));
    vs.push_back(AttributeInfo("UserName", "User name", AttributeInfo::String));
//    ci.push_back(CodecInfo(mjpg_codecs, "Motion JPEG DirectShow Decoder", 	"m3jpegdec.ax",mjpg_about, CodecInfo::DShow_Dec, &CLSID_MorganMjpeg, vs));

    /*
     missing  ICOpen
    ci.push_back(CodecInfo(mjpg_codecs, "Morgan Motion JPEG Direct Show",
			   "m3jpegdec.ax",
			   mjpg_about, CodecInfo::DShow_Dec,
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_MorganMjpeg, 0, vs));
    */

    ci.push_back(CodecInfo(mjpg_codecs, "W32 Morgan Motion JPEG", "m3jpeg32.dll",
			   mjpg_about, CodecInfo::Win32, "morgands",
			   // this is actually not a morgands, but we don't care (?)
			   CodecInfo::Video,
			   // buggy - CompressBegin reference 0x0  FIXME
			   // CodecInfo::Both,
			   CodecInfo::Decode,
			   0, vs));
}

static void add_mcmjpeg(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t msjpeg_codecs[] = {
	fccMJPG, fccmjpg,
	mmioFOURCC('A', 'V', 'R', 'n'),
	mmioFOURCC('A', 'V', 'D', 'J'),
	0
    };

    ci.push_back(CodecInfo(msjpeg_codecs, "W32 Microsoft Motion JPEG", "mcmjpg32.dll",
			   "", CodecInfo::Win32, "mjpeg",
			   CodecInfo::Video, CodecInfo::Both
			   //CodecInfo::Video, CodecInfo::Decode
			  ));
}

static void add_pegas(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t mjpg_codecs[] = {
	fccMJPG, fccmjpg,
	mmioFOURCC('J', 'P', 'E', 'G'),
	0 };
    static const fourcc_t pvw2_codecs[] = {
	mmioFOURCC('P', 'V', 'W', '2'),
	0 };
    static const fourcc_t pimj_codecs[] = {
	fccPIM1,
	mmioFOURCC('P', 'I', 'M', 'J'),
	mmioFOURCC('J', 'P', 'G', 'L'),
	mmioFOURCC('J', 'P', 'E', 'G'),
	0 };
    static const fourcc_t pim1_codecs[] = {
	fccPIM1,
	0 };
    ci.push_back(CodecInfo(pvw2_codecs, "W32 PicVideo [PVW2]", "pvwv220.dll",
			   none_about, CodecInfo::Win32, "picvideo",
			   //CodecInfo::Video, CodecInfo::Both
			   CodecInfo::Video, CodecInfo::Decode
			  ));
    ci.push_back(CodecInfo(mjpg_codecs, "W32 PicVideo [MJPG]", "pvmjpg21.dll",
			   none_about, CodecInfo::Win32, "pv_mjpg",
			   //CodecInfo::Video, CodecInfo::Both
			   CodecInfo::Video, CodecInfo::Decode
			  ));
    ci.push_back(CodecInfo(pimj_codecs, "W32 PicVideo [PIMJ]", "pvljpg20.dll",
			   none_about, CodecInfo::Win32, "pv_pimj",
			   //CodecInfo::Video, CodecInfo::Both
			   CodecInfo::Video, CodecInfo::Decode
			  ));
    ci.push_back(CodecInfo(pim1_codecs, "W32 PinnacleS [PIM1]", "avi_pass.ax",
			   none_about, CodecInfo::DShow_Dec, "pv_pim1",
			   //CodecInfo::Video, CodecInfo::Both
			   CodecInfo::Video, CodecInfo::Decode
			  ));

}

static void add_avid(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t avrn_codecs[] = {
	mmioFOURCC('A', 'V', 'R', 'n'),
	mmioFOURCC('A', 'V', 'D', 'J'),
	0 };
    ci.push_back(CodecInfo(avrn_codecs, "W32 AVID Codec [AVRn]", "avidavicodec.dll",
			   none_about, CodecInfo::Win32, "avid",
			   CodecInfo::Video, CodecInfo::Both
			  ));
}

static void add_techsmith(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t tscc_codecs[] = { fccTSCC, fcctscc, 0 };
    ci.push_back(CodecInfo(tscc_codecs, "W32 TechSmith Screen Capture [TSCC]", "tsccvid.dll",
                           "TechSmith's Screen Capture Codec [TSCC],"
			   "provides lossless image quality coupled with "
			   "excellent compression ratios. Since the TSCC is "
			   "lossless, it preserves 100% of the image quality, "
			   "even through multiple decompression/recompression "
			   "cycles that are typical during the production process."
			   "The TSCC is optimized for screen capture so that "
			   "the resulting files are small and highly compressed."
			   "Visit <a href=\"http://www.techsmith.com\">"
                           "http://www.techsmith.com</a> "
			   "for the latest product information.",
			   CodecInfo::Win32, "tscc",
			   CodecInfo::Video, CodecInfo::Both
			  ));
}

static void add_huffyuv(avm::vector<CodecInfo>& ci)
{
    static const char* huffyuv_about =
	"Huffyuv is a very fast, lossless Win32 video "
	"codec. \"Lossless\" means that the output "
	"from the decompressor is bit-for-bit "
	"identical with the original input to the "
	"compressor. \"Fast\" means a compression "
	"throughput of up to 38 megabytes per second "
	"on author\'s 416 MHz Celeron."
	"Web site: <a href=\"http://http://www.math.berkeley.edu/~benrg/huffyuv.html\">"
	"http://http://www.math.berkeley.edu/~benrg/huffyuv.html</a>";

    static const fourcc_t huffyuv_codecs[] = { fccHFYU, 0 };
    static const char* huffyuv_modes[] =
    {
	"Predict left (fastest)",
	"Predict gradient",
	"Predict median (best)",
	0
    };

    avm::vector<AttributeInfo> vs;

    //vs.push_back(AttributeInfo("Mode", "Calculation precision mode", (const char**)mjpeg_modes));
    //ci.push_back(CodecInfo(mjpg_codecs, "Motion JPEG DirectShow Decoder", 	"m3jpegdec.ax",mjpg_about, CodecInfo::DShow_Dec, &CLSID_MorganMjpeg, vs));

    ci.push_back(CodecInfo(huffyuv_codecs, "W32 Huffyuv lossless codec [HFYU]",
			   "huffyuv.dll",
			   huffyuv_about, CodecInfo::Win32, "huffyuv",
			   //CodecInfo::Video, CodecInfo::Decode));
			   CodecInfo::Video, CodecInfo::Both));
}

/* On2 Truemotion VP3.x support */
static void add_vp3(avm::vector<CodecInfo>& ci)
{
    /* The On2 copyright message taken from the Windows project
       resource file */
    static const char* vp3_about =
	"<a href=\"http://www.vp3.com/\">VP3 Codec</a> "
	"- Version 3.2.1.0<br>\nCopyright (c) 2001 On2 Technologies. "
	"All Rights Reserved. "
	"<a href=\"http://www.on2.com/\">"
	"http://www.on2.com</a>";
    static const GUID CLSID_on2 =
    {
	0x4cb63e61, 0xc611, 0x11d0,
	{ 0x83, 0xaa, 0x00, 0x00, 0x92, 0x90, 0x01, 0x84 }
    };
    static const fourcc_t apxx_codecs[] = {
	fccVP31, fccvp31, fccVP30, fccVP30,
	mmioFOURCC('V', 'P', '4', '0'), mmioFOURCC('T', 'M', '2', 'X'), 0
    };

    avm::vector<AttributeInfo> vs_empty;
    avm::vector<AttributeInfo> ds;
    ds.push_back(AttributeInfo("strPostProcessingLevel", "Postprocessing",
			       AttributeInfo::Integer, 0, 8));
    /* vp3 codec contains a bug in ICDECOMPRESSEX initializer
    ( Codecs/vp31vfw/Win32/vp31vfw.h ) that breaks decoding upside-down
    images in 'normal' mode ( ICDECOMPRESS ) */
#if 1
    /* crashing somewhere after IsRect == TRUE */
    ci.push_back(CodecInfo(apxx_codecs, "W32 VP31(r) DirectShow", "on2.ax",
			   vp3_about, CodecInfo::DShow_Dec, "vp3ds",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_on2));
#endif
    ci.push_back(CodecInfo(apxx_codecs, "W32 VP31(r) Codec", "vp31vfw.dll",
			   vp3_about, CodecInfo::Win32Ex, "vp3",
			   CodecInfo::Video, CodecInfo::Both, 0, vs_empty, ds));
}

static void add_3ivx(avm::vector<CodecInfo>& ci)
{
    static const GUID CLSID_3ivxO =
    {
	0x73f7a062, 0x8829, 0x11d1,
	{ 0xb5, 0x50, 0x00, 0x60, 0x97, 0x24, 0x2d, 0x8d }
    };
    static const GUID CLSID_3ivx =
    {
	0x0e6772c0, 0xdd80, 0x11d4,
	{ 0xb5, 0x8f, 0xa8, 0x6b, 0x66, 0xd0, 0x61, 0x1c }
    };
    static const fourcc_t ivx_codecs[] = {
	mmioFOURCC('3', 'I', 'V', '1'), mmioFOURCC('3', 'i', 'v', 'X'),	0
    };
    ci.push_back(CodecInfo(ivx_codecs, "W32 3ivX", "3ivxdmo.dll",
			   none_about, CodecInfo::DMO, "3ivx",
			   CodecInfo::Video, CodecInfo::Decode,
                           &CLSID_3ivx));
    static const fourcc_t ucod_codecs[] = { mmioFOURCC('U', 'C', 'O', 'D'), 0 };
    ci.push_back(CodecInfo(ucod_codecs,	"W32 UCOD-ClearVideo", "clrviddd.dll",
			   none_about, CodecInfo::Win32, "ucod",
			   CodecInfo::Video, CodecInfo::Decode));

    static const fourcc_t ubmp4_codecs[] = { mmioFOURCC('U', 'M', 'P', '4'), mmioFOURCC('m', 'p', '4', 'v'), 0 };
    ci.push_back(CodecInfo(ucod_codecs,	"W32 UB Video MPEG 4", "ubvmp4d.dll",
			   none_about, CodecInfo::Win32, "ubmp4",
			   CodecInfo::Video, CodecInfo::Decode));

    static const fourcc_t qpeg_codecs[] = {
	mmioFOURCC('Q', '1', '.', '0'),
	mmioFOURCC('Q', 'P', 'E', 'G'),
	mmioFOURCC('Q', '1', '.', '1'),
	mmioFOURCC('q', 'p', 'e', 'q'), 0
    };
    ci.push_back(CodecInfo(qpeg_codecs,	"W32 Q-Team's QPEG (www.q-team.de)",
			   "qpeg32.dll",
			   none_about, CodecInfo::Win32, "qpeg",
			   CodecInfo::Video, CodecInfo::Decode));

    static const fourcc_t sp5x_codecs[] = {
	mmioFOURCC('S', 'P', '5', '3'),
	mmioFOURCC('S', 'P', '5', '4'),
	mmioFOURCC('S', 'P', '5', '5'),
	mmioFOURCC('S', 'P', '5', '6'),
	mmioFOURCC('S', 'P', '5', '7'),
	mmioFOURCC('S', 'P', '5', '8'), 0
    };
    ci.push_back(CodecInfo(sp5x_codecs,
			   "W32 SP5x codec - used by Aiptek MegaCam", "sp5x_32.dll",
			   none_about, CodecInfo::Win32, "sp5x",
			   CodecInfo::Video, CodecInfo::Decode));


    /* to be supported */
    static const fourcc_t qtsvq3_codecs[] = { mmioFOURCC('S', 'V', 'Q', '3'), 0 };
    ci.push_back(CodecInfo(sp5x_codecs,
			   "W32 Qt SVQ3 decoder", "QuickTime.qts",
			   none_about, CodecInfo::Win32, "qtvideo",
			   CodecInfo::Video, CodecInfo::Decode));
}

static void add_audio(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t msadpcm_codecs[] = { 0x02, 0 };
    ci.push_back(CodecInfo(msadpcm_codecs, "W32 MS ADPCM", "msadp32.acm",
			   "A Microsoft implementation of Adaptive "
			   "Differential Pulse Code Modulation (ADPCM), "
			   "a common digital audio format capable of "
			   "storing CD-quality audio.",
			   CodecInfo::Win32, "msadpcmacm",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const GUID CLSID_Voxware =
    {
	0x73f7a062, 0x8829, 0x11d1,
	{ 0xb5, 0x50, 0x00, 0x60, 0x97, 0x24, 0x2d, 0x8d }
    };
    static const fourcc_t voxware_codecs[] = { 0x75, 0};
    ci.push_back(CodecInfo(voxware_codecs, "W32 Voxware Metasound", "voxmsdec.ax",
			   none_about, CodecInfo::DShow_Dec, "dsvoxware",
			   CodecInfo::Audio, CodecInfo::Decode,
			   &CLSID_Voxware));

    static const GUID CLSID_Acelp =
    {
	0x4009f700, 0xaeba, 0x11d1,
	{ 0x83, 0x44, 0x00, 0xc0, 0x4f, 0xb9, 0x2e, 0xb7 }
    };
    static const fourcc_t acelp_codecs[] = { 0x130, 0 };
    ci.push_back(CodecInfo(acelp_codecs, "W32 ACELP(r).net DirectShow", "acelpdec.ax",
			   "A net-based codec using frame-concatenation and "
			   "interlacing for improved music quality. "
			   "ACELP(r).net allows a dual-rate bit-rate of 8.5/6.5 "
			   "kbps or a fixed-rate bit-rate of 5.0 kbps.",
			   CodecInfo::DShow_Dec, "acelp", CodecInfo::Audio,
			   CodecInfo::Decode, &CLSID_Acelp));

    // codec ID ?
    static const fourcc_t anet_codecs[] = { 0x130, 0 };
    ci.push_back(CodecInfo(anet_codecs, "W32 ACELP(r).net acm", "sl_anet.acm",
			   "Sipro Lab Telcom Audio Codec ACELP(r).net codec",
			   CodecInfo::Win32, "anetacm",
			   CodecInfo::Audio, CodecInfo::Both));

    static const fourcc_t wma_codecs[] = { 0x160, 0x161, 0 };
    ci.push_back(CodecInfo(wma_codecs, "W32 Windows Media Audio", "divxa32.acm",
			   "More fully known as Microsoft(c) Windows Media(tm) "
			   "audio compression. This is the standard codec for "
			   "Microsoft Active Streaming Format which combines "
			   "fast encoding with high music quality and is "
			   "optimized for Pentium II (MMX) and Pentium III "
			   "(SSE/SIMD) processors. WM-AUDIO has a wide bit-rate "
			   "range from 5 kbps to 128 kbps and offers high quality "
			   "sound over the Internet even over 28.8 modems. "
                           "WM-AUDIO is considered a future replacement for MP3.",
			   CodecInfo::Win32, "divxacm", CodecInfo::Audio,
			   CodecInfo::Decode));
    /* WMA3 */
    static const GUID CLSID_WMA3DMO =
    {
	0x2eeb4adf, 0x4578, 0x4d10,
	{ 0xbc, 0xa7, 0xbb, 0x95, 0x5f, 0x56, 0x32, 0x0a }
    };
    static const fourcc_t wma3_codecs[]={ 0x162, 0x160, 0x161, 0};
    ci.push_back(CodecInfo(wma3_codecs, "W32 Windows Media Audio DMO", "wmadmod.dll",
			   none_about, CodecInfo::DMO, "wmadmod",
			   CodecInfo::Audio, CodecInfo::Decode,
			   &CLSID_WMA3DMO));
    static const GUID CLSID_WMA39DMO =
    {
	0x27ca0808, 0x01f5, 0x4e7a,
	{ 0x8b, 0x05, 0x87, 0xf8, 0x07, 0xa2, 0x33, 0xd1 }
    };
    ci.push_back(CodecInfo(wma3_codecs, "W32 Windows Media Audio 9 DMO", "wma9dmod.dll",
			   none_about, CodecInfo::DMO, "wma9dmod",
			   CodecInfo::Audio, CodecInfo::Decode,
			   &CLSID_WMA39DMO));
    static const GUID CLSID_WMSP =
    {
	0x874131cb, 0x4ecc, 0x443b,
	{ 0x89, 0x48, 0x74, 0x6b, 0x89, 0x59, 0x5d, 0x20 }
    };
    static const fourcc_t wmsp_codecs[] = { 0xa, 0 };
    ci.push_back(CodecInfo(wmsp_codecs, "W32 Windows Media Audio Speach DMO", "wmspdmod.dll",
			   none_about, CodecInfo::DMO, "wmspdmod",
			   CodecInfo::Audio, CodecInfo::Decode,
			   &CLSID_WMSP));
#if 0
    ci.push_back(CodecInfo(wma_codecs, "W32 WMA enc", "wmadmoe.dll",
			   "Winmedadio", CodecInfo::Win32, "wmadmo_enc",
			   CodecInfo::Audio, CodecInfo::Encode));
#endif
    static const fourcc_t imc_codecs[] = { 0x401, 0 };
    ci.push_back(CodecInfo(imc_codecs, "W32 Intel Music Coder", "imc32.acm",
			   none_about, CodecInfo::Win32, "imcacm",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t ima_codecs[] = { 0x11, 0 };
    ci.push_back(CodecInfo(ima_codecs, "W32 IMA ADPCM", "imaadp32.acm",
			   "An implementation of Adaptive Differential Pulse"
			   "Code Modulationn (ADPCM), useful for "
			   "cross-platform audio for multimedia, "
			   "developed by the Interactive Multimedia "
                           "Association (IMA).", CodecInfo::Win32, "imaacm",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t msn_codecs[] = { 0x32, 0 };
    ci.push_back(CodecInfo(msn_codecs, "W32 MSN", "msnaudio.acm",
			   "MSN Audio codec", CodecInfo::Win32, "msnaudioacm",
			   CodecInfo::Audio, CodecInfo::Decode));
    static const fourcc_t gsm_codecs[] = { 0x31, 0x32, 0 };
    ci.push_back(CodecInfo(gsm_codecs, "W32 GSM", "msgsm32.acm",
			   "MSN GSM Audio codec", CodecInfo::Win32, "msgsmacm",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t dsp_codecs[] = { 0x22, 0 };
    ci.push_back(CodecInfo(dsp_codecs, "W32 DSP Group TrueSpeech(TM)", "tssoft32.acm",
			   none_about, CodecInfo::Win32, "truespeechacm",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t lh_codecs[] = { 0x1101, 0x1102, 0x1103, 0x1104, 0 };
    ci.push_back(CodecInfo(lh_codecs, "W32 Lernout & Hauspie", "lhacm.acm",
			   "Lernout & Hauspie  CELP, SCB", CodecInfo::Win32, "lhacm",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t vox_codecs[] = { 0x181c, 0 };
    ci.push_back(CodecInfo(vox_codecs, "W32 VoxWare RT24 speech codec", "nsrt2432.acm",
			   none_about, CodecInfo::Win32, "voxwarert24acm",
			   CodecInfo::Audio, CodecInfo::Decode));

    static const fourcc_t alf2_codecs[] = { 0x1fc4, 0 };
    ci.push_back(CodecInfo(alf2_codecs, "W32 ALF2 codec", "alf2cd.acm",
			   "http://www.nctsoft.com/products/NCTALFCD/"
			   "jdp@mail.sonofon.dk",
			   CodecInfo::Win32, "alf2acm",
			   CodecInfo::Audio, CodecInfo::Decode));
#if 0
    static const GUID CLSID_Vorbis =
    {
        0x6bA47966, 0x3F83, 0x4178,
	{0x96, 0x65, 0x00, 0xF0, 0xBF, 0x62, 0x92, 0xE5}
    };
    static const fourcc_t vorbis_codecs[] = { 0xFFFE, 0 };
    ci.push_back(CodecInfo(vorbis_codecs, "W32 Vorbis decoder", "vorbis.acm",
			   "Vorbis Audio codec", CodecInfo::Win32, "vorbisacm",
			   CodecInfo::Audio, CodecInfo::Decode,
			   &CLSID_Vorbis));
#endif
#if 0
    // works now - but has some problems with buffering - somehow
    // we need to flash buffers after seek
    static const fourcc_t mpeg_codecs[] = { 0x55, 0 };
#if 0

    // DirectShow version causing segfault in Convert
    static const GUID CLSID_Mpeg =
    {
	0x38BE3000, 0xDBF4, 0x11D0,
	{ 0x86, 0x0e, 0x00, 0xA0, 0x24, 0xCF, 0xEF, 0x6D }
    };
    // DS doesn't work
    ci.push_back(CodecInfo(mpeg_codecs, "W32 MPEG Layer-3 DS", "l3codecx.ax",
			   none_about, CodecInfo::DShow_Dec, "mp3ds",
			   CodecInfo::Audio, CodecInfo::Decode,
			   &CLSID_Mpeg));
#endif
    // acm version is somewhat problematic - disable
    ci.push_back(CodecInfo(mpeg_codecs, "W32 MPEG Layer-3", "l3codeca.acm",
			   none_about, CodecInfo::Win32, "mp3acm",
			   CodecInfo::Audio, CodecInfo::Decode));
#endif
#if 0
    // this works but only as a last resource - it has quite poor quality
    static const fourcc_t mpeg_codecs[] = { 0x55, 0 };
    ci.push_back(CodecInfo(mpeg_codecs, "W32 MPEG Layer-3", "divxa32.acm",
			   none_about, CodecInfo::Win32, "divxmp3",
			   CodecInfo::Audio, CodecInfo::Decode));
#endif

}


static void win32_FillPlugins(avm::vector<CodecInfo>& ci)
{
    // disabled when crashing
    // add_angel(ci); - using divx instead
    add_divx(ci);
    //add_divx5(ci);
    add_divx4(ci);
    add_xvid(ci);
    add_ati(ci);
    add_asus(ci);
    add_brooktree(ci);
    add_dvsd(ci);
    add_indeo(ci);
    add_morgan(ci);
    add_mcmjpeg(ci);
    add_pegas(ci);
    add_avid(ci);
    add_techsmith(ci);
    add_huffyuv(ci);
    /* On2 Truemotion VP3.x support */
    add_vp3(ci);
    add_3ivx(ci);

    add_audio(ci);

    //const fourcc_t zero[]={ 0 };
    static const char* cvid_about =
	"Very old video codec, usually "
	"available as a part of Windows.";
    static const fourcc_t cvid_codecs[] = { fccCVID, fcccvid, 0 };
    ci.push_back(CodecInfo(cvid_codecs, "W32 Cinepak Video", "iccvid.dll",
			   cvid_about, CodecInfo::Win32, "cvid"));

    static const fourcc_t i263_codecs[] = { fccI263, fcci263, 0 };
    ci.push_back(CodecInfo(i263_codecs,	"W32 I263", "i263_32.drv",
			   none_about, CodecInfo::Win32, "i263"));

    static const GUID CLSID_U263DecompressorCF = {
	0x00af1181, 0x6ebb, 0x11D4,
	{ 0x9d, 0x5a, 0x00, 0x50, 0x04, 0x79, 0x6c, 0xc0}
    };
    static const fourcc_t u263_codecs[] = { fccU263, 0 };
    ci.push_back(CodecInfo(i263_codecs,	"W32 U263", "ubv263d+.ax",
			   none_about, CodecInfo::DShow_Dec, "u263",
                           CodecInfo::Video, CodecInfo::Both,
			   &CLSID_U263DecompressorCF));

    static const fourcc_t mwv1_codecs[] = { fccMWV1, 0 };
    ci.push_back(CodecInfo(mwv1_codecs,	"W32 Motion Wavelets",
			   "icmw_32.dll",
			   "Aware Motion Wavelets Video Codec [MWV1]",
			   CodecInfo::Win32, "mwv1",
			   CodecInfo::Video, CodecInfo::Both));

    static const fourcc_t mszh_codecs[] = { fccMSZH, 0 };
    ci.push_back(CodecInfo(mszh_codecs,	"W32 AVI Mszh", "avimszh.dll",
			   none_about, CodecInfo::Win32, "mszh",
			   CodecInfo::Video, CodecInfo::Both));

    static const fourcc_t zlib_codecs[] = { fccZLIB, 0 };
    ci.push_back(CodecInfo(zlib_codecs,	"W32 AVI Zlib", "avizlib.dll",
			   none_about, CodecInfo::Win32, "zlib",
			   CodecInfo::Video, CodecInfo::Both));

    static const fourcc_t m261_codecs[] = { fccM261, fccm261, 0 };
    ci.push_back(CodecInfo(m261_codecs,	"W32 M261", "msh261.drv",
			   none_about, CodecInfo::Win32, "m261",
                           // crash with encode FIXME
			   CodecInfo::Video, CodecInfo::Decode));

    static const fourcc_t msrle_codecs[] = { 1, 0 };
    ci.push_back(CodecInfo(msrle_codecs, "W32 MS RLE", "msrle32.dll",
			   none_about, CodecInfo::Win32, "msrlevfw",
			   CodecInfo::Video, CodecInfo::Both));

#if 1
    /*
     * crash after IsRectEmpty and some memory allocation - mystery....
     */
    static const GUID CLSID_Tm20 =
    {
        0x4cb63e61, 0xc611, 0x11D0,
	{ 0x83, 0xaa, 0x00, 0x00, 0x92, 0x90, 0x01, 0x84 }
    };
    static const fourcc_t tm20_codecs[] = { fccTM20, 0 };
    ci.push_back(CodecInfo(tm20_codecs,	"W32 TrueMotion 2.0 DS Decompressor",
			   "tm20dec.ax",
			   none_about, CodecInfo::DShow_Dec, "tm20ds",
			   CodecInfo::Video, CodecInfo::Decode,
			   &CLSID_Tm20));
#endif
#if 0
    ci.push_back(CodecInfo(tm20_codecs,	"W32 TrueMotion 2.0 Decompressor",
			   "tm2x.dll",
			   none_about, CodecInfo::Win32, "tm20",
			   CodecInfo::Video, CodecInfo::Decode));
#endif
    static const fourcc_t cram_codecs[] = { fccCRAM, fcccram, fccMSVC, 0 };
    ci.push_back(CodecInfo(cram_codecs,	"W32 Microsoft Video 1", "msvidc32.dll",
			   none_about, CodecInfo::Win32, "cram",
                           CodecInfo::Video, CodecInfo::Decode));
}

AVM_END_NAMESPACE;

#endif // WIN32_FILLPLUGINS_H
