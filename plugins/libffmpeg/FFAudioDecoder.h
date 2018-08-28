#ifndef AVIFILE_FFAUDIODECODER_H
#define AVIFILE_FFAUDIODECODER_H

#include "audiodecoder.h"
#include "avcodec.h"

AVM_BEGIN_NAMESPACE;

class FFAudioDecoder: public IAudioDecoder
{
    static const uint_t MIN_AC3_CHUNK_SIZE = 16384;
    AVCodec *m_pAvCodec;
    AVCodecContext* m_pAvContext;
public:
    FFAudioDecoder(AVCodec*, const CodecInfo&, const WAVEFORMATEX*);
    ~FFAudioDecoder();
    virtual int Convert(const void*, uint_t, void*, uint_t, uint_t*, uint_t*);
    virtual uint_t GetMinSize() const;
    virtual uint_t GetSrcSize(uint_t dest_size) const;
};

AVM_END_NAMESPACE;

#endif //AVIFILE_FFAUDIODECODER_H
