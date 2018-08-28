#ifndef FFREADHANDLER_H
#define FFREADHANDLER_H

#include "ReadHandlers.h"
#include "avm_locker.h"

struct AVInputStream;
struct AVFormatContext;

AVM_BEGIN_NAMESPACE;

class FFReadStream;

class FFReadHandler : public IMediaReadHandler
{
friend class FFReadStream;
public:
    FFReadHandler();
    virtual ~FFReadHandler();
    int Init(const char* url);
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

protected:
    void flush();
    int seek(framepos_t pos);
    int readPacket();

    AVFormatContext* m_pContext;
    avm::vector<FFReadStream*> m_Streams;
    AVInputStream* m_pInput;
    PthreadMutex m_Mutex;
};

AVM_END_NAMESPACE;

#endif // FFREADHANDLER_H
