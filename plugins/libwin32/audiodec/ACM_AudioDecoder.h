#ifndef AVIFILE_ACM_AUDIODECODER_H
#define AVIFILE_ACM_AUDIODECODER_H

#ifndef NOAVIFILE_HEADERS
#include "audiodecoder.h"
#else
#include "libwin32.h"
#endif

#ifndef __WINE_MSACM_H
#include "wine/winerror.h"
#include "wine/msacm.h"
#endif

AVM_BEGIN_NAMESPACE;

class ACM_AudioDecoder : public IAudioDecoder
{
public:
    ACM_AudioDecoder(const CodecInfo&, const WAVEFORMATEX*);
    ~ACM_AudioDecoder();
    virtual uint_t GetMinSize() const;
    virtual uint_t GetSrcSize(uint_t dest_size) const;
    virtual int Convert(const void*, uint_t, void*, uint_t, uint_t*, uint_t*);
    int init();
    const char* getError() const { return m_Error; }
protected:
    WAVEFORMATEX wf;
    HACMSTREAM srcstream;
    uint_t m_uiMinSize;
    int m_iOpened;
    bool first;
    char m_Error[128];
};

AVM_END_NAMESPACE;

#endif //AVIFILE_ACM_AUDIODECODER_H
