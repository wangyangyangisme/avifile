#ifndef AVIFILE_UNCOMPRESSED_H
#define AVIFILE_UNCOMPRESSED_H

#include "videodecoder.h"
#include "videoencoder.h"
#include "avm_fourcc.h"

AVM_BEGIN_NAMESPACE;

class Unc_Decoder: public IVideoDecoder
{
    CAPS m_Cap;
    BitmapInfo dest;
public:
    Unc_Decoder(const CodecInfo&, const BITMAPINFOHEADER&, int);
    virtual ~Unc_Decoder();
    virtual CAPS GetCapabilities() const { return m_Cap; }
    virtual int DecodeFrame(CImage* dest, const void* src, uint_t size,
			    int is_keyframe, bool render = true,
			    CImage** out = 0);
    virtual int SetDestFmt(int bits, fourcc_t csp);
};

class Unc_Encoder: public IVideoEncoder
{
    BitmapInfo head;
    BitmapInfo chead;
public:
    Unc_Encoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& header);
    virtual ~Unc_Encoder();
    virtual int Start() { return 0; }
    virtual int Stop() { return 0; }
    virtual int GetOutputSize() const;
    virtual const BITMAPINFOHEADER& GetOutputFormat() const;
    virtual int EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid=0);
};

static void uncompressed_FillPlugins(avm::vector<CodecInfo>& ci)
{
    static const fourcc_t unc_rgb24[] = {
	0, 3,
	mmioFOURCC('D', 'I', 'B', ' '),
	mmioFOURCC('R', 'G', 'B', ' '),
        mmioFOURCC('r', 'a', 'w', ' '),
        0
    };
    static const fourcc_t unc_yuy2[] = {fccYUY2, 0};
    static const fourcc_t unc_yv12[] = {fccYV12, 0};
    static const fourcc_t unc_i420[] = {fccI420, 0};
    static const fourcc_t unc_uyvy[] = {fccUYVY, 0};
    static const fourcc_t unc_y800[] = {
	mmioFOURCC('Y', '8', '0', '0'),
	mmioFOURCC('Y', '8', ' ', ' '),
	0
    };

    ci.push_back(CodecInfo(unc_rgb24, "Uncompressed RGB", "",
			   "", CodecInfo::Source, "rgb"));
    ci.push_back(CodecInfo(unc_yuy2, "Uncompressed YUY2", "",
			   "", CodecInfo::Source, "yuy2"));
    ci.push_back(CodecInfo(unc_yv12, "Uncompressed YV12", "",
			   "", CodecInfo::Source, "yv12"));
    ci.push_back(CodecInfo(unc_i420, "Uncompressed I420", "",
			   "", CodecInfo::Source, "i420"));
    ci.push_back(CodecInfo(unc_uyvy, "Uncompressed UYVY", "",
			   "", CodecInfo::Source, "uyvy"));
    ci.push_back(CodecInfo(unc_y800, "Uncompressed Y800", "",
			   "", CodecInfo::Source, "y800"));
}

AVM_END_NAMESPACE;

#endif /* AVIFILE_UNCOMPRESSED_H */
