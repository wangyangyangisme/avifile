#ifndef AVIFILE_FFVIDEODECODER_H
#define AVIFILE_FFVIDEODECODER_H

#include "videodecoder.h"

#ifndef int64_t_C
#define int64_t_C(c)     (c ## LL)
#define uint64_t_C(c)    (c ## ULL)
#endif
#include "avformat.h"

AVM_BEGIN_NAMESPACE;

class FFVideoDecoder: public IVideoDecoder, public IRtConfig
{
public:
    FFVideoDecoder(AVCodec* av,const CodecInfo& info, const BITMAPINFOHEADER& format, int flip);
    virtual ~FFVideoDecoder();
    virtual int DecodeFrame(CImage* pImage, const void* src, uint_t size,
			    int is_keyframe, bool render = true,
			    CImage** pOut = 0);
    virtual CAPS GetCapabilities() const { return m_Caps; }
    virtual void Flush();
    virtual int SetDestFmt(int bits = 24, fourcc_t csp = 0);
    virtual int Stop();

    virtual IRtConfig* GetRtConfig() { return this; }
    virtual const avm::vector<AttributeInfo>& GetAttrs() const;
    virtual int GetValue(const char* name, int* value) const;
    virtual int SetValue(const char* name, int value);

protected:
    AVCodec *m_pAvCodec;
    AVCodecContext* m_pAvContext;
    struct AVStream* m_pAvStream;
    CAPS m_Caps;
    unsigned m_uiBuffers;
    bool m_bFlushed;
    bool m_bRestart;
    struct fpair { int64_t timestamp; uint_t position;
	fpair(int64_t t = 0, uint_t p = 0) : timestamp(t), position(p) {}
    };
    avm::qring<fpair> m_Order;
public:
    CImage* m_pImg;
    bool m_bUsed;
    bool m_bDirect;
    int m_iAgeIP[2];
    int m_iAgeB;
    CImage* m_pReleased;
};

AVM_END_NAMESPACE;

#endif //AVIFILE_FFVIDEODECODER_H
