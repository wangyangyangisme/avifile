#ifndef AVIFILE_DS_AUDIODECODER_H
#define AVIFILE_DS_AUDIODECODER_H

#ifndef NOAVIFILE_HEADERS
#include "audiodecoder.h"
#else
#include "libwin32.h"
#endif
#include "DS_Filter.h"

AVM_BEGIN_NAMESPACE;

class DS_AudioDecoder : public IAudioDecoder
{
public:
    DS_AudioDecoder(const CodecInfo& info, const WAVEFORMATEX*);
    virtual ~DS_AudioDecoder();
    virtual int Convert(const void*, uint_t, void*, uint_t, uint_t*, uint_t*);
    virtual uint_t GetMinSize() const;
#ifdef NOAVIFILE_HEADERS
    virtual uint_t GetSrcSize(uint_t) const;
#endif
    int init();
    const char* getError() const { return m_Error; }
protected:
    AM_MEDIA_TYPE m_sOurType, m_sDestType;
    DS_Filter* m_pDS_Filter;
    WAVEFORMATEX m_sVhdr2;
    WAVEFORMATEX* m_pVhdr;
    char m_Error[128];
};

AVM_END_NAMESPACE;

#endif // AVIFILE_DS_AUDIODECODER_H
