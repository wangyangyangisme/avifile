#ifndef AVIFILE_DS_VIDEODECODER_H
#define AVIFILE_DS_VIDEODECODER_H

#include "dshow/DS_Filter.h"
#include "videodecoder.h"

AVM_BEGIN_NAMESPACE;

class DS_VideoDecoder: public IVideoDecoder, public IRtConfig
{
public:
    DS_VideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& format, int flip);
    virtual ~DS_VideoDecoder();
    virtual int DecodeFrame(CImage* dest, const void* src, uint_t size,
			    int is_keyframe, bool render = true,
			    CImage** pOut = 0);
    virtual CAPS GetCapabilities() const { return m_Caps; }
    virtual IRtConfig* GetRtConfig() { return (IRtConfig*) this; }
    virtual int SetDestFmt(int bits = 24, fourcc_t csp = 0);
    virtual int SetDirection(int d);
    virtual int Start();
    virtual int Stop();

    // IRtConfig interface
    virtual const avm::vector<AttributeInfo>& GetAttrs() const
    {
        return m_Info.decoder_info;
    }
    virtual int GetValue(const char*, int*) const;
    virtual int SetValue(const char*, int);

    int init(); // avoid throws
protected:
    int setCodecValues();
    int getCodecValues();
    DS_Filter* m_pDS_Filter;
    AM_MEDIA_TYPE m_sOurType, m_sDestType;
    VIDEOINFOHEADER* m_sVhdr;
    VIDEOINFOHEADER* m_sVhdr2;
    IDivxFilterInterface* m_pIDivx4;
    CAPS m_Caps;                // capabilities of DirectShow decoder
    int m_iStatus;
    int m_iMaxAuto;
    int m_iLastPPMode;
    int m_iLastBrightness;
    int m_iLastContrast;
    int m_iLastSaturation;
    int m_iLastHue;
    enum { CT_NONE, CT_DIVX3, CT_DIVX4, CT_IV50 } m_CType;
    bool m_bHaveUpsideDownRGB;
    bool m_bSetFlg;
    bool m_bFlip;
};

AVM_END_NAMESPACE;

#endif /* AVIFILE_DS_VIDEODECODER_H */
