#ifndef ASFREADHANDLER_H
#define ASFREADHANDLER_H

#include "ReadHandlers.h"
#include "asffmt.h"

AVM_BEGIN_NAMESPACE;

class AsfInputStream;
class AsfStreamSeekInfo;

class AsfReadHandler : public IMediaReadHandler
{
friend class AsfReadStream;
public:
    AsfReadHandler();
    virtual ~AsfReadHandler();
    virtual uint_t GetHeader(void* pheader, uint_t n);
    virtual IMediaReadStream* GetStream(uint_t stream_id,
					IStream::StreamType type);
    virtual uint_t GetStreamCount(IStream::StreamType type);
    virtual bool GetURLs(avm::vector<avm::string>& urls);
    virtual void Interrupt();

    virtual bool IsOpened();
    virtual bool IsValid();
    virtual bool IsRedirector();

    int init(const char* pszFile);

    static void PrintASFMainHeader(const ASFMainHeader*);
    static void PrintASFStreamHeader(const ASFStreamHeader*);

protected:
    avm::vector<AsfReadStream*> m_Streams;
    avm::vector<const AsfStreamSeekInfo*> m_SeekInfo;
    ASFMainHeader m_Header;
    AsfInputStream* m_pInput;
};

AVM_END_NAMESPACE;

#endif // ASFREADHANDLER_H
