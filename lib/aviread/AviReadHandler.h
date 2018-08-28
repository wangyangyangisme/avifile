#ifndef AVIREADHANDLER_H
#define AVIREADHANDLER_H

#include "Cache.h"
#include "ReadHandlers.h"

AVM_BEGIN_NAMESPACE;

class AviReadStream;

class AviStreamPacket : public StreamPacket
{
public:
    int64_t offset;
    uint_t id;

    AviStreamPacket(uint_t off = 0, uint_t i = 0, uint_t bsize = 0)
	: StreamPacket(bsize), offset(off), id(i) {}
};


class AviReadHandler : public IMediaReadHandler
{
    friend class AviReadStream;
    static const uint_t MAX_CHUNK_SIZE = 10000000;
public:
    AviReadHandler(unsigned int flags = 0);
    virtual ~AviReadHandler();
    virtual uint_t GetHeader(void* pheader, uint_t size);
    virtual IMediaReadStream* GetStream(uint_t stream_id,
					IStream::StreamType type);
    virtual uint_t GetStreamCount(IStream::StreamType type);
    virtual bool GetURLs(avm::vector<avm::string>& urls) { return false; }
    virtual void Interrupt() {}
    /* avi files are opened synchronously */
    virtual bool IsOpened() { return true; }
    virtual bool IsValid() { return true; }
    virtual bool IsRedirector() { return false; }

    int init(const char* pszFile);

    static void PrintAVIMainHeader(const AVIMainHeader* hdr);
    static void PrintAVIStreamHeader(const AVIStreamHeader* hdr);
    static char* GetAviFlags(char* buffer, uint_t flags);

protected:
    int readAVIMainHeader(uint_t);
    int readAVIStreamHeader();
    void readStreams(uint_t);
    void readInfoChunk(uint_t);
    int readIndexChunk(uint_t, uint_t movie_chunk_offset);

    //AviStreamPacket* readPacket(bool fill = true);
    int reconstructIndexChunk(uint_t);

    AVIMainHeader m_MainHeader;
    avm::vector<AviReadStream*> m_Streams;
    InputStream m_Input;
    uint_t m_uiFlags;
};

AVM_END_NAMESPACE;

#endif // AVIREADHANDLER_H
