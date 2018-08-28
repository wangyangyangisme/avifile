#ifndef AVIFILE_AUDIODECODER_H
#define AVIFILE_AUDIODECODER_H

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

/**
 *
 * Audio decoder abstract class.
 *
 * Performs decompression of data from
 * various audio formats into raw 2 channels PCM
 *
 * Some common methods are already implemented
 * to avoid useless source copying
 *
 * Different output format might be requested by
 * SetOutputFormat method
 */
class IAudioDecoder
{
public:
    IAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* in);
    virtual ~IAudioDecoder();
    /**
     * Decodes data. It is guaranteed that either size_read or size_written
     * will receive nonzero value if out_size>=GetMinSize().
     */
    virtual int Convert(const void* in_data, uint_t in_size,
			void* out_data, uint_t out_size,
			uint_t* size_read, uint_t* size_written)	= 0;

    /**
     * Flush call after seek
     */
    virtual void Flush(); 
    virtual const CodecInfo& GetCodecInfo() const;
    /**
     * Minimal required output buffer size. Calls to Convert() will
     * fail if you pass smaller output buffer to it.
     */
    virtual uint_t GetMinSize() const;
    /**
     * Returns output format for this audio decoder.
     */
    virtual int GetOutputFormat(WAVEFORMATEX* destfmt) const;
    virtual IRtConfig* GetRtConfig(); // can't be const
    /**
     * Estimates the amount ( in bytes ) of input data that's
     * required to produce specified amount of decompressed PCM data.
     */
    virtual uint_t GetSrcSize(uint_t dest_size) const;
    /**
     * Set output format - returns -1 if unsupported
     */
    virtual int SetOutputFormat(const WAVEFORMATEX* destfmt);
protected:
    const CodecInfo& m_Info;
    WAVEFORMATEX* m_pFormat;
    int m_uiBytesPerSec;
};

AVM_END_NAMESPACE;

#ifdef AVM_COMPATIBLE
typedef avm::IAudioDecoder IAudioDecoder;
#endif

#endif // AVIFILE_AUDIODECODER_H
