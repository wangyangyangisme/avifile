#ifndef AVIFILE_AUDIOENCODER_H
#define AVIFILE_AUDIOENCODER_H

/********************************************************
 *
 * Audio encoder interface.
 *
 ********************************************************/

#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

/**
 *
 * \warning
 * As of 1/15/2001, the only supported format is MPEG layer-3
 * ( using Lame encoder v. 3.70 ), format id 0x55. Arguments passed
 * to Create() are not checked for correctness. Only 16-bit input
 * is supported.
 *
 */
class IAudioEncoder
{
public:
    IAudioEncoder(const CodecInfo& info);
    virtual ~IAudioEncoder();
    /**
     * Finishes conversion. May write some more data
     * ( pass buffer of >10kb size here ).
     */
    virtual int Close(void* out_data, uint_t out_size, uint_t* size_written) = 0;
    /**
     * Encodes data. Parameters in_size and size_read are measured
     * in samples ( not in bytes ).
     */
    virtual int Convert(const void* in_data, uint_t in_size,
			void* out_data, uint_t out_size,
			uint_t* size_read, uint_t* size_written)	= 0;
    virtual const CodecInfo& GetCodecInfo() const;
    /// Reads the format structure. (format == 0 -> query for size)
    virtual uint_t GetFormat(void* format = 0, uint_t size = 0) const	= 0;
    /// Starts conversion.
    virtual int Start()							= 0;

    /// USE ATTRIBUTES as for video - we want to support a lot of various parameters
    /// Sets bitrate in bytes/second.
    virtual int SetBitrate(int bitrate);
    /// Sets encoding quality ( 1..10 ).
    virtual int SetQuality(int quality);
    /// Sets more extra options
    //virtual int Set(int option, int param) { return -1; }

    /// Queries for the size of structure describing destination format.
    /// \obsolete - just source backward compatible
    uint_t GetFormatSize() const { return GetFormat(0, 0); }
protected:
    const CodecInfo& m_Info;
};

AVM_END_NAMESPACE;

#ifdef AVM_COMPATIBLE
typedef avm::IAudioEncoder IAudioEncoder;
#endif

#endif // AVIFILE_AUDIOENCODER_H
