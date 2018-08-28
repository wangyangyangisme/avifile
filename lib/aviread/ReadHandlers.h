#ifndef READHANDLERS_H
#define READHANDLERS_H

/**
 *
 * Interfaces that provide uniform access to data in Audio Video streams,
 * local and networked files  (Avi, Asf, maybe more later)
 *
 */

#include "avifile.h"
#include "StreamInfoPriv.h"

AVM_BEGIN_NAMESPACE;

// StreamPacket implemented in ReadStream.cpp
struct StreamPacket
{
    char* memory;
    uint_t size;
    uint_t read;
    uint_t flags;
    uint_t position;
    int64_t timestamp;  // timestamp in microseconds
    int refcount;
    static const uint_t MAX_PACKET_SIZE = 3000000;
    static const int64_t NO_TIMESTAMP = -1LL;

    StreamPacket(uint_t bsize = 0, char* mem = 0);
    ~StreamPacket();
    void AddRef() { refcount++; }
    void Release() { refcount--; if (!refcount) delete this; }
};

class IMediaReadStream : public IStream
{
public:

    virtual ~IMediaReadStream() {}

    /* 1 cache full, 0 cache empty */
    virtual double CacheSize() const				= 0;

    /* Clear the internal stream cache */
    virtual void ClearCache()					= 0;

    /**
     * Return structure describing main stream header
     *
     * NOTE: format is in the stream based one!
     * Use GetStreamInfo to obtain standard set of informations
     * about stream
     * this call will be eliminated later!!
     */
    virtual uint_t GetHeader(void *pheader, uint_t size) const = 0;

    /** Retrieval of stream-specific format data */
    // XXXXXXXXXXXXX
    virtual uint_t GetFormat(void *pFormat = 0, uint_t lSize = 0) const	= 0;

    /** Stream frame time - might be just average for whole stream */
    // XXXXXXXXXXXXX
    virtual double GetFrameTime() const				= 0;

    /** Length of the stream in samples - equals with last frame position */
    virtual framepos_t GetLength() const			= 0;

    /** Length of the stream in seconds */
    // XXXXXXXXXXXXX
    virtual double GetLengthTime() const			= 0;

    /**
     * position of previous, next and nearest key frame
     * related to frame pos ( ERR means current frame )
     */
    virtual framepos_t GetNearestKeyFrame(framepos_t pos = ERR) const = 0;
    virtual framepos_t GetNextKeyFrame(framepos_t pos = ERR) const = 0;
    virtual framepos_t GetPrevKeyFrame(framepos_t pos = ERR) const = 0;

    /** Timestamp of sample no. lSample ( ERR = current sample ) */
    virtual double GetTime(framepos_t lSample = ERR) const = 0;

    /** Returns 0 for variable frame size stream (usually video), >0 for audio */
    virtual uint_t GetSampleSize() const			= 0;

    /**
     * Various useful stream information
     *
     * this is preffered way to obtain information about stream!
     */
    virtual StreamInfo* GetStreamInfo() const			= 0;

    /** Type of stream - usually fcc 'vids' or 'auds' */
    virtual StreamType GetType() const				= 0;

    /**
     * Return true if frame no. pos ( ERR means current frame ) is keyframe
     */
    virtual bool IsKeyFrame(framepos_t pos = ERR) const		= 0;

    /** Obtain whole packet - new prefered way for reading streams */
    virtual StreamPacket* ReadPacket()				= 0;

    /** Seek to frame position pos in stream */
    virtual int Seek(framepos_t pos)				= 0;

    /** Seek to time */
    virtual int SeekTime(double time)				= 0;

    virtual int SkipFrame()					= 0;
    virtual int SkipTo(double time)				= 0;

    // this is currently cruel hack
    virtual int FixAvgBytes(uint_t bps) { return -1; }
};

class IMediaReadHandler : public IStream
{
public:
    virtual ~IMediaReadHandler() {}

    /* vids/auds, 0-... */
    virtual IMediaReadStream* GetStream(uint_t stream_id, StreamType type) = 0;
    virtual uint_t GetStreamCount(StreamType type)		= 0;

    virtual uint_t GetHeader(void* pheader, uint_t n) 		= 0;

    /* All calls for network-oriented readers should be interruptible  */
    virtual void Interrupt() 					= 0;

    /* in case of asynchronous opening */
    virtual bool IsOpened() 					= 0;
    virtual bool IsValid()					= 0;
    virtual bool IsRedirector() 				= 0;
    virtual bool GetURLs(avm::vector<avm::string> &urls) 	= 0;
};

/*  Read handler for AVI file  */
IMediaReadHandler* CreateAviReadHandler(const char *pszFile, unsigned int flags);

/*  Read handler for ASF/WMV/ASX file  */
IMediaReadHandler* CreateAsfReadHandler(const char *pszFile);

/*  Read handler for everything :) file  */
IMediaReadHandler* CreateFFReadHandler(const char *pszFile);

AVM_END_NAMESPACE;

#endif // READHANDLERS_H
