#ifndef ASFREADSTREAM_H
#define ASFREADSTREAM_H

#include "ReadHandlers.h"
#include "AsfInputStream.h"
#include "infotypes.h"

AVM_BEGIN_NAMESPACE;

class AsfReadHandler;

class AsfReadStream : public IMediaReadStream
{
friend class AsfReadHandler;
public:
    AsfReadStream(AsfReadHandler* parent);
    virtual ~AsfReadStream();
    virtual void ClearCache();
    virtual double CacheSize() const;
    virtual bool IsKeyFrame(framepos_t lFrame = ERR) const;
    virtual uint_t GetHeader(void* pheader = 0, uint_t size = 0) const;
    virtual uint_t GetFormat(void *format = 0, uint_t size = 0) const;
    virtual double GetFrameTime() const;
    virtual framepos_t GetLength() const;
    virtual double GetLengthTime() const;
    virtual framepos_t GetPrevKeyFrame(framepos_t lFrame = ERR) const;
    virtual framepos_t GetNextKeyFrame(framepos_t lFrame = ERR) const;
    virtual framepos_t GetNearestKeyFrame(framepos_t lFrame = ERR) const;
    virtual uint_t GetSampleSize() const;
    virtual StreamInfo* GetStreamInfo() const;
    virtual double GetTime(framepos_t frame = ERR) const;
    virtual IStream::StreamType GetType() const;
    virtual StreamPacket* ReadPacket();
    virtual int Seek(framepos_t lPos);
    virtual int SeekTime(double time);
    virtual int SkipFrame();
    virtual int SkipTo(double pos);

protected:
    void ReadPacketInternal() const;
    asf_packet* GetNextAsfPacket() const
    {
	for (;;)
	{
	    if (m_pAsfPacket)
		m_pAsfPacket->release();
	    m_pAsfPacket = m_pIterator->getPacket();
	    m_uiFragId = 0;
	    if (!m_pIterator->isEof()
		&& m_pAsfPacket
                && m_pAsfPacket->fragments.size() > 0)
		return m_pAsfPacket;
	    if (m_pIterator->isEof())
                return 0;
	    //printf("Packet %p  %d   for %p\n", m_pAsfPacket, m_pAsfPacket->refcount, this);
	}
    }

    ASFStreamHeader m_Header;
    const AsfStreamSeekInfo* m_pSeekInfo;
    const AsfSpreadAudio* m_pScrambleDef;
    mutable AsfReadHandler* m_pParent;
    mutable StreamInfo m_StreamInfo;
    mutable AsfIterator* m_pIterator;
    mutable StreamPacket* m_pStrPacket;
    mutable asf_packet* m_pAsfPacket;
    mutable unsigned m_uiFragId;
    mutable unsigned m_uiLastPos;
    mutable double m_dLastTime;
    int m_iId;
    int m_iMaxBitrate;
    bool m_bIsAudio;
    bool m_bIsScrambled;
};

AVM_END_NAMESPACE;

#endif // ASFREADSTREAM_H
