#ifndef FFREADSTREAM_H
#define FFREADSTREAM_H

#include "ReadHandlers.h"

struct AVStream;
struct AVCodecContext;
AVM_BEGIN_NAMESPACE;

class FFReadHandler;

class FFReadStream : public IMediaReadStream
{
friend class FFReadHandler;
public:
    FFReadStream(FFReadHandler* handler, uint_t sid, AVStream*);
    virtual ~FFReadStream();

    virtual double CacheSize() const;
    virtual void ClearCache();
    virtual uint_t GetHeader(void* pheader, uint_t size) const;
    virtual framepos_t GetPrevKeyFrame(framepos_t lFrame = ERR) const;
    virtual framepos_t GetNextKeyFrame(framepos_t lFrame = ERR) const;
    virtual framepos_t GetNearestKeyFrame(framepos_t lFrame = ERR) const;
    virtual framepos_t GetLength() const;
    virtual double GetFrameTime() const;
    virtual double GetLengthTime() const;
    virtual StreamInfo* GetStreamInfo() const;
    virtual double GetTime(framepos_t lSample = ERR) const;
    virtual uint_t GetSampleSize() const;
    virtual IStream::StreamType GetType() const;
    virtual uint_t GetFormat(void *pFormat = 0, uint_t plSize = 0) const;
    virtual bool IsKeyFrame(framepos_t lFrame = ERR) const;
    virtual StreamPacket* ReadPacket();
    virtual int Seek(framepos_t pos);
    virtual int SeekTime(double time);
    virtual int SkipFrame();
    virtual int SkipTo(double pos);

protected:
    FFReadHandler* m_pHandler;
    AVStream* m_pAvStream;
    AVCodecContext* m_pAvContext;
    uint_t m_uiSId;
    avm::qring<StreamPacket*> m_Packets;
    framepos_t m_uiPosition;
    double m_dTimestamp;
    double m_dLength;
    mutable StreamInfo m_StreamInfo;
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

#endif // FFREADSTREAM_H
