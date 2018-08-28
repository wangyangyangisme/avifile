#ifndef AVIFILE_DMO_AUDIODECODER_H
#define AVIFILE_DMO_AUDIODECODER_H

#ifndef NOAVIFILE_HEADERS
#include "audiodecoder.h"
#else
#include "libwin32.h"
#endif
#include "DMO_Filter.h"

AVM_BEGIN_NAMESPACE;

class DMO_AudioDecoder : public IAudioDecoder
{
public:
    DMO_AudioDecoder(const CodecInfo& info, const WAVEFORMATEX*);
    virtual ~DMO_AudioDecoder();
    virtual int Convert(const void*, uint_t, void*, uint_t, uint_t*, uint_t*);
    virtual void Flush();
    virtual uint_t GetMinSize() const;
#ifdef NOAVIFILE_HEADERS
    virtual uint_t GetSrcSize(uint_t) const;
#endif
    virtual int SetOutputFormat(const WAVEFORMATEX* destfmt);
    int init();
    const char* getError() const { return m_Error; }
protected:
    DMO_MEDIA_TYPE m_sOurType, m_sDestType;
    DMO_Filter* m_pDMO_Filter;
    WAVEFORMATEX m_sVhdr2;
    WAVEFORMATEX* m_pVhdr;
    int m_iFlushed;
    char m_Error[128];
};

AVM_END_NAMESPACE;

#endif // AVIFILE_DMO_AUDIODECODER_H
