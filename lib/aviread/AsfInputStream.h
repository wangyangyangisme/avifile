#ifndef AVIFILE_ASFITERATOR_H
#define AVIFILE_ASFITERATOR_H

#include "asffmt.h"

AVM_BEGIN_NAMESPACE;

struct chunk_info
{
    static const uint_t KEYFRAME = 1 << 31;

    uint_t object_start_time;
    uint_t object_length; // 31bit marks keyframe - anyone has frames >2^31 bytes?
    uint_t packet_id; 
    uint16_t fragment_id; // let's cheat here

    uint_t GetChunkLength() const { return object_length & ~KEYFRAME; }
    bool IsKeyFrame() const { return object_length & KEYFRAME; }
    void SetKeyFrame(bool s) { if (s) object_length |= KEYFRAME; }

};

class AsfStreamSeekInfo: public avm::vector<chunk_info>
{
public:
    static const framepos_t ERR = ~0U;

    framepos_t find(uint_t time) const;
    framepos_t nearestKeyFrame(framepos_t kf = ERR) const;
    framepos_t nextKeyFrame(framepos_t kf = ERR) const;
    framepos_t prevKeyFrame(framepos_t kf = ERR) const;
};

/*
 * This logic is needed to properly handle reading from networked streams.
 * Each stream owns an 'iterator' that can be used to access packets.
 * If the stream is local, it allows iterators for different streams
 * to point at completely independent packets ( and seek independently ).
 * If it isn't, seek() on one iterator invalidates the other, and when
 * seek() is called on invalidated iterator, it's automatically
 * reposition at the place of last seek.
 */

class AsfIterator
{
public:
    AsfIterator(int id = 0) : m_iRefcount(1), m_iId(id), m_bEof(false) {}
    virtual ~AsfIterator() {}
    virtual void addRef() { m_iRefcount++; }
    virtual void release() { m_iRefcount--; if (!m_iRefcount) delete this; }
    virtual bool isEof() { return m_bEof; }

    virtual asf_packet* getPacket()				=0;
    virtual const AsfStreamSeekInfo* getSeekInfo()		=0;
    virtual int seek(framepos_t pos, chunk_info* pch)		=0;
    virtual int seekTime(double timestamp, chunk_info* pch)	=0;
    int getId()	{ return m_iId; }
protected:
    int m_iRefcount;
    int m_iId;
    bool m_bEof;
};

class AsfInputStream
{
public:
    static const uint_t ASF_STREAMS = 0x7f;

    virtual ~AsfInputStream() {}
    virtual const ASFMainHeader& getHeader() const { return m_Header; }
    virtual const avm::vector<ASFStreamHeader>& getStreams() const { return m_Streams; }
    virtual AsfIterator* getIterator(uint_t id) { return 0; }
    virtual double cacheSize() const { return 1.; }
    virtual void clear() {}
    virtual void interrupt() {}
    virtual bool isOpened() { return true; }
    virtual bool isValid() { return true; }
    virtual bool isRedirector() { return false; }
    virtual bool getURLs(avm::vector<avm::string>& urls) { return false; }
    int getMaxBitrate(uint_t i)
    {
	return (i <= ASF_STREAMS) ?  m_MaxBitrates[i] : ~0U;
    }
protected:
    bool parseHeader(char* b, uint_t size, int level = 0);
    ASFMainHeader m_Header;
    avm::vector<ASFStreamHeader> m_Streams;
    avm::vector<avm::string> m_Description;
    uint_t m_MaxBitrates[ASF_STREAMS + 1];
};

AVM_END_NAMESPACE;

#endif // AVIFILE_ITERATOR_H
