#ifndef AVIFILE_VIDEODECODER_H
#define AVIFILE_VIDEODECODER_H

/********************************************************

	Video decoder interface
	Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)

*********************************************************/
#include "videodecoder.h"
#include "Module.h"

AVM_BEGIN_NAMESPACE;

class VideoDecoder: public IVideoDecoder
{
public:
    VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& format, int flip);
    virtual ~VideoDecoder();
    virtual int DecodeFrame(CImage* dest, const void* src, uint_t size,
			    int is_keyframe, bool render = true,
			    CImage** pOut = 0);
    virtual CAPS GetCapabilities() const { return m_Caps; }
    virtual int SetDestFmt(int bits = 24, fourcc_t csp = 0);
    virtual int Start();
    virtual int Stop();
    int init();
protected:
    Module* m_pModule;
    HIC m_HIC;
    CAPS m_Caps;
    int m_iStatus;
    CImage* m_pLastImage;
    BITMAPINFOHEADER* m_bitrick;
    bool m_bLastNeeded;
    bool m_divx_trick;
    bool m_bUseEx;
    bool m_bFlip;
    void setDecoder(BitmapInfo& bi);
};

AVM_END_NAMESPACE;

#endif //AVIFILE_VIDEODECODER_H
