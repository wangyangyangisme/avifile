#ifndef AVIREADSTREAM_H
#define AVIREADSTREAM_H

#include "Cache.h"
#include "ReadHandlers.h"

AVM_BEGIN_NAMESPACE;

class AviReadHandler;

class AviReadStream : public IMediaReadStream
{
friend class AviReadHandler;
public:
    AviReadStream(AviReadHandler* handler, const AVIStreamHeader& hdr,
		  uint_t id, const void* format, uint_t fsize);
    virtual ~AviReadStream();
    virtual double CacheSize() const;
    virtual void ClearCache();
    virtual uint_t GetFormat(void *pFormat = 0, uint_t lSize = 0) const;
    virtual double GetFrameTime() const { return 1.0 / m_dAvgBytesPerSec; }
    virtual uint_t GetHeader(void* pheader, uint_t size) const;
    virtual framepos_t GetLength() const;
    virtual double GetLengthTime() const;
    virtual bool IsKeyFrame(framepos_t lFrame = ERR) const;
    virtual framepos_t GetNextKeyFrame(framepos_t lFrame = ERR) const;
    virtual framepos_t GetNearestKeyFrame(framepos_t lFrame = ERR) const;
    virtual framepos_t GetPrevKeyFrame(framepos_t lFrame = ERR) const;
    virtual uint_t GetSampleSize() const { return m_Header.dwSampleSize; }
    virtual StreamInfo* GetStreamInfo() const;
    virtual double GetTime(framepos_t lSample = ERR) const;
    virtual IStream::StreamType GetType() const;
    virtual StreamPacket* ReadPacket();
    virtual int Seek(framepos_t pos);
    virtual int SeekTime(double time);
    virtual int SkipFrame() { return Seek(m_uiPosition + 1); }
    virtual int SkipTo(double pos) { return SeekTime(pos); }

    virtual int FixAvgBytes(uint_t bps);

    void fixHeader();
protected:
    framepos_t find(framepos_t iSample) const;

    void addChunk(uint_t off, uint_t len, bool iskf);
    AviReadHandler* m_pHandler;
    mutable StreamInfo m_StreamInfo;
    uint_t m_iId;
    framepos_t m_uiChunk;
    framepos_t m_uiPosition;
    AVIStreamHeader m_Header;
    char* m_pcFormat;
    uint_t m_uiFormatSize;
    double m_dAvgBytesPerSec;
    uint_t m_uiStart;

    // chunk data for Avi Index
    avm::vector<uint32_t> m_Offsets;
    avm::vector<uint32_t> m_Positions; // needed for audio streams
    uint_t m_uiStreamSize;
    uint_t m_uiKeyChunks;
    uint_t m_uiKeySize;
    uint_t m_uiKeyMaxSize;
    uint_t m_uiKeyMinSize;
    // delta chunks m_Offsets - m_uiKeyChunks
    uint_t m_uiDeltaSize;
    uint_t m_uiDeltaMaxSize;
    uint_t m_uiDeltaMinSize;
};

AVM_END_NAMESPACE;

#endif // AVIREADSTREAM_H
