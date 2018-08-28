#ifndef ASFNETWORKINPUTSTREAM_H
#define ASFNETWORKINPUTSTREAM_H

#include "AsfInputStream.h"
#include "avm_locker.h"

AVM_BEGIN_NAMESPACE;

class ASX_Reader;
class NetworkIterator;

class URLString : public avm::string
{
public:
    URLString() {}
    URLString(const avm::string& s) : avm::string(s) {}
    void escape();
    //int getPort(int def = 80) { }
};

class AsfNetworkInputStream: public AsfInputStream
{
    friend class NetworkIterator;
    static const uint_t CACHE_PACKETS = 160;

public:
    enum Content
    {
	Unknown,
	Plain,
	Live,
	Prerecorded,
	Redirect
    };

    AsfNetworkInputStream() :m_pReader(0), m_pThread(0), m_pBuffer(0) {}
    virtual ~AsfNetworkInputStream();
    virtual AsfIterator* getIterator(uint_t id);
    virtual void interrupt();
    virtual double cacheSize() const;
    virtual void clear();
    virtual bool isOpened() { return (m_bHeadersValid || m_pReader); }
    virtual bool isValid();// { return ((m_bHeadersValid && !m_bQuit) || m_pReader); }
    virtual bool isRedirector() { return (m_pReader != 0); }
    virtual bool getURLs(avm::vector<avm::string>& urls);

    int init(const char* pszFile);

protected:
    static void* threadStarter(void*);
    void* threadFunc();
    int createSocket();
    Content checkContent(const char* request);
    int readContent();
    int readRedirect();
    void readHeader(uint_t size, uint_t skip = 0);
    int read(void* buffer, uint_t size);
    int write(const void* buffer, uint_t size);
    int dwrite(const void* buffer, uint_t size);
    int seekInternal(uint_t time, NetworkIterator* requester);
    void unregister(NetworkIterator* requester);
    void flushPipe();
    void escapeUrl();

    int64_t m_lReadBytes;
    ASX_Reader* m_pReader;
    PthreadTask* m_pThread;
    avm::vector<NetworkIterator*> m_Iterators;
    avm::string m_File, m_Server, m_Proxyhost;
    URLString m_Filename;
    URLString m_ServerFilename;
    int m_iSocket;
    int m_iPipeFd[2];//message on this pipe interrupts main reading thread
    int m_lfd; // localy stored asf stream
    int m_iProxyport;
    int m_iRandcntx;    // random number to identify connection
    int m_iSeekId;      // first stream which calls seek will be the master
    uint_t m_uiTime;
    uint_t m_uiDataOffset;
    uint_t m_uiTimeshift; // network files should be moved to zero time
    char* m_pBuffer;
    int m_iRedirectSize;
    int m_iReadSize;
    mutable PthreadMutex m_Mutex;
    mutable PthreadCond m_Cond;
    Content m_Ctype;
    struct __attribute__((__packed__))
    {
	uint16_t kind;
	uint16_t size;
	uint32_t seq;
	uint16_t partflag;
	uint16_t size_confirm;

	void le16()
	{
	    kind = avm_get_le16(&kind);
	    size = avm_get_le16(&size);
	    seq = avm_get_le32(&seq);
	    partflag = avm_get_le16(&partflag);
	    size_confirm = avm_get_le16(&size_confirm);
	}
    } chhdr;

    bool m_bQuit;
    bool m_bHeadersValid;
    bool m_bFinished;
    bool m_bWaiting;
    bool m_bAcceptRanges;
};

AVM_END_NAMESPACE;

#endif // ASFNETWORKINPUTSTREAM_H
