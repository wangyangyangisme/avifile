
#include "AsfNetworkInputStream.h"
#include "AsxReader.h"
#include "asf_guids.h"
#include "utils.h"
#include "avm_output.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <ctype.h>
#include <stdlib.h> // atoi
#include <string.h> // strerror
#include <stdio.h>

AVM_BEGIN_NAMESPACE;

static int l_iFd = -1;

#define USERAGENT "User-Agent: NSPlayer/7.1.0.3055\r\n"
#define CLIENTGUID "Pragma: xClientGUID={c77e7400-738a-11d2-9add-0020af0a3278}\r\n"

static const char* m_pcFirstRequest =
    "GET %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s\r\n"
    "Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=%u,max-duration=0\r\n"
    CLIENTGUID
    "Connection: Close\r\n\r\n";

static const char* m_pcSeekableRequest =
    "GET %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s\r\n"
    "Pragma: no-cache,rate=1.000000,stream-time=%u,stream-offset=%u:%u,request-context=%u,max-duration=%u\r\n"
    CLIENTGUID
    "Pragma: xPlayStrm=1\r\n"
    "Pragma: stream-switch-count=%d\r\n"
    "Pragma: stream-switch-entry=%s\r\n" // ffff:1:0 ffff:2:0
    "Connection: Close\r\n\r\n";

static const char* m_pcPostRequest =
    "POST %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s\r\n"
    "Pragma: client-id=%u\r\n"
    //"Pragma: log-line=no-cache,rate=1.000000,stream-time=%u,stream-offset=%u:%u,request-context=2,max-duration=%u\r\n"
    "Pragma: Content-Length: 0\r\n"
    CLIENTGUID
    "\r\n";

static const char* m_pcLiveRequest =
    "GET %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s\r\n"
    "Pragma: no-cache,rate=1.000000,request-context=%u\r\n"
    "Pragma: xPlayStrm=1\r\n"
    CLIENTGUID
    "Pragma: stream-switch-count=%d\r\n"
    "Pragma: stream-switch-entry=%s\r\n"
    "Connection: Close\r\n\r\n";

static const char* m_pcRangeRequest =
    "GET %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s\r\n"
    "Range: bytes=%llu-\r\n"
    CLIENTGUID
    "Connection: Close\r\n\r\n";



class AsfNetworkInputStream;
class AsfStreamSeekInfo;

class NetworkIterator: public AsfIterator
{
friend class AsfNetworkInputStream;
public:
    NetworkIterator(AsfNetworkInputStream* parent, int id)
	: AsfIterator(id), m_pParent(parent)
    {
    }
    virtual ~NetworkIterator()
    {
	for (uint_t i = 0; i < m_Packets.size(); i++)
	    m_Packets[i]->release();
    }
    virtual const AsfStreamSeekInfo* getSeekInfo()
    {
	return 0;
    }
    virtual asf_packet* getPacket()
    {
	AVM_WRITE("ASF network reader", 1, "getPacket() (Eof: %d, pkts: %d)\n", m_bEof, m_Packets.size());
	PthreadMutex& mutex = m_pParent->m_Mutex;
	PthreadCond& cond = m_pParent->m_Cond;
        Locker locker(mutex);

	for (int i = 0; !m_Packets.size(); i++)
	{
	    if (m_bEof || i > 20)
		return 0;
            if (!m_pParent->m_bWaiting)
		cond.Broadcast();
	    cond.Wait(mutex, 0.5);
	    //printf("WAIT %d\n", i);
	}

	//for (int i = 0; i < m_Packets.size(); i++)
	//    AVM_WRITE("ASF network reader", "GETPACKETLIST  %d  %p  %d\n", i, m_Packets[i], m_Packets[i]->size());
	asf_packet* p = m_Packets.front();
	m_Packets.pop_front();
	//AVM_WRITE("ASF network reader", "packet %d   p:%d\n", m_Packets.size(), p->send_time);
	//AVM_WRITE("ASF network reader", 1, "Returning: length %d, send time %dms  (%d)\n",
	//          p->packet_length, p->send_time, p->length);
	return p;
    }
    virtual bool isEof() { return m_bEof && !m_Packets.size(); }
    virtual int seek(framepos_t pos, chunk_info* pch)
    {
	AVM_WRITE("ASF network reader", "WARNING: seek(%d) is unsupported\n", pos);
	return -1;
    }
    virtual int seekTime(double timestamp, chunk_info* pch)
    {
	if (m_pParent->seekInternal(uint_t(timestamp*1000.0), this) < 0)
	    return -1;

	PthreadMutex& mutex = m_pParent->m_Mutex;
	PthreadCond& cond = m_pParent->m_Cond;
	Locker locker(mutex);

	int i = 0;
	while (i++ < 20 && !m_pParent->m_bQuit && !m_bEof && !m_Packets.size())
	{
	    AVM_WRITE("ASF network reader", "waiting & sleeping (%d, %d, %d)\n", i, m_bEof, m_iId);
	    cond.Broadcast();
	    cond.Wait(mutex, 0.5);
	}
	if (m_Packets.size())
	{
	    asf_packet* p = m_Packets.front();
	    pch->SetKeyFrame(true);
	    pch->fragment_id = 0;
	    pch->object_start_time = p->send_time;
	    return 0;
	}
            // hmmm
	pch->SetKeyFrame(true);
	pch->fragment_id = 0;
	pch->object_start_time = m_pParent->m_uiTime;
	return 0;
    }
protected:
    void setEof(bool b) { m_bEof = b; }
    avm::vector<asf_packet*> m_Packets;
    AsfNetworkInputStream* m_pParent;
};

int AsfNetworkInputStream::init(const char* pszFile)
{
    m_iSocket = m_lfd = m_iSeekId = -1;
    m_bHeadersValid = false;
    m_bAcceptRanges = false;
    m_bFinished = false;
    m_bWaiting = false;
    m_bQuit = false;

    m_Ctype = Unknown;
    m_uiTime = 0;
    m_pReader = 0;
    m_Server = pszFile;
    m_uiDataOffset = 0;
    m_uiTimeshift = 0;
    m_iRedirectSize = 16000;
    m_iReadSize = 0;
    m_iProxyport = 0;
    pipe(m_iPipeFd);

    avm::string::size_type start = 0, end;

    AVM_WRITE("ASF network reader", 1, "checking URL: %s\n", m_Server.c_str());
    for (;;)
    {
	start = m_Server.find("://", start);
	if (start == avm::string::npos || start + 3 >= m_Server.size())
	{
	    AVM_WRITE("ASF network reader", "Not an URL\n");
	    return -1;
	}
	// find & use last occurence of ://
	if (m_Server.find("://", start + 1) == avm::string::npos)
	    break;
        start++;
    }
    start += 3;
    end = m_Server.find("/", start);
    if (end == avm::string::npos)
    {
	AVM_WRITE("ASF network reader", "Not an URL\n");
	return -1;
    }

    m_ServerFilename = m_Server.substr(0, end);
    m_Filename = m_Server.substr(end);
    m_Filename.escape();
    m_Server = m_Server.substr(start, end-start);

    end = m_Server.find(":", start);
    if (end == avm::string::npos)
	m_Server += ":80";

    //m_iServerport = atoi(m_Server.substr(end+1).c_str());
    //m_Server = m_Server.substr(0,end);
    AVM_WRITE("ASF network reader", "server:%s filename:%s\n", m_Server.c_str(), m_Filename.c_str());
    // style hostname:port eg 192.168.10.10:8080
    char* proxyenv = getenv("HTTP_PROXY");
    if (!proxyenv)
	proxyenv = getenv("http_proxy");
    if (proxyenv)
    {
	if ((strncasecmp(proxyenv, "http://", 7) == 0))
	    proxyenv += 7;
	m_Proxyhost = proxyenv;
	end = m_Proxyhost.find(":");
	if (end==avm::string::npos)
	    m_iProxyport = 80;
	else
	{
	    avm::string p = m_Proxyhost.substr(end + 1,avm::string::npos);
	    m_iProxyport = atoi((const char*)p);
	}
	m_Proxyhost = m_Proxyhost.substr(0, end);
	m_ServerFilename += m_Filename;
	AVM_WRITE("ASF network reader", "proxy host:%s port:%d\n", m_Proxyhost.c_str(), m_iProxyport);
    }
    else
        m_ServerFilename = m_Filename;

    /*
     * This func tries to open URL m_File. If it succeeds,
     * it loads its headers, puts true to m_bHeadersValid
     * and starts caching packets. If it fails, it puts
     * true to m_bHeadersValid and to m_bQuit, and then exits.
     */
    // use some randomized name
    srand((unsigned int) longcount());
    m_iRandcntx = rand();

    // determine max size for posted requests
    int max_request = strlen(m_pcLiveRequest);
    int sz = strlen(m_pcSeekableRequest);
    if (sz > max_request)
	max_request = sz;
    sz = strlen(m_pcFirstRequest);
    if (sz > max_request)
        max_request = sz;
    max_request += m_Filename.size() + m_Server.size() + 512;
    if (max_request < 65536)
        max_request = 65536;
    m_pBuffer = new char[max_request];

    if (getenv("WRITE_ASFLOG") && l_iFd < 0)
	l_iFd = open("./log", O_WRONLY|O_CREAT|O_TRUNC, 00666);

    m_pThread = new PthreadTask(0, &AsfNetworkInputStream::threadStarter, this);
    return 0;
}

AsfNetworkInputStream::~AsfNetworkInputStream()
{
    m_bQuit = true;
    interrupt();
    delete m_pThread;
    clear();
    avm::vector<NetworkIterator*>::iterator it2;
    for (it2 = m_Iterators.begin(); it2 != m_Iterators.end(); it2++)
	(*it2)->release();
    close(m_iPipeFd[0]);
    close(m_iPipeFd[1]);
    delete[] m_pBuffer;
    delete m_pReader;
}

bool AsfNetworkInputStream::isValid()
{
    if (m_bFinished)
    {
	Locker locker(m_Mutex);
	if (m_Iterators.size())
	{
            int s = 0;
	    for(avm::vector<NetworkIterator*>::iterator it2 = m_Iterators.begin();
		it2 != m_Iterators.end(); it2++)
		s += (*it2)->m_Packets.size();
	    if (!s)
	    {
		//printf("BROADCAST\n");
		m_Cond.Broadcast();
	    }
	}
    }
    return ((m_bHeadersValid && !m_bQuit) || m_pReader);
}

void AsfNetworkInputStream::flushPipe()
{
    int pipe_flags = fcntl(m_iPipeFd[0], F_GETFL);
    char c;
    fcntl(m_iPipeFd[0], F_SETFL, pipe_flags | O_NONBLOCK);
    while (::read(m_iPipeFd[0], &c, 1) > 0)
	;
    fcntl(m_iPipeFd[0], F_SETFL, pipe_flags);
}

/* time in milliseconds */
int AsfNetworkInputStream::seekInternal(uint_t seektime, NetworkIterator* requester)
{
    if (m_Ctype == Live
	|| (m_Ctype == Plain
	    && (!m_bAcceptRanges || !m_uiDataOffset)))
    {
	if (seektime == 0)
	{
	    m_bFinished = true; // stop reading
            clear();
	}
	return -1;
    }

    if (m_iSeekId < 0)
	m_iSeekId = requester->getId();

    if (m_iSeekId != requester->getId())
        return 0;

    int d = (m_uiTime > seektime) ? m_uiTime - seektime : seektime - m_uiTime;
    //printf("DIFFERENCE %d\n", d);
    if (d < 2000)
	return 0; // ignore too close seeks

    //AVM_WRITE("ASF network reader", "***********SEEKINTERNAL %d\n", seektime);
    m_bFinished = true;
    if (!m_bWaiting)
    {
	interrupt();
	while (!m_bQuit && !m_bWaiting)
	{
	    //printf("SLEEP %d  %d\n", m_bQuit, m_bWaiting);
	    avm_usleep(100000);
	}
    }
    m_uiTime = seektime;
    interrupt();
    m_bWaiting = false;
    return 0;
}

void AsfNetworkInputStream::unregister(NetworkIterator* it)
{
    m_Iterators.remove(it);
}

void AsfNetworkInputStream::interrupt()
{
    AVM_WRITE("ASF network reader", 1, "interrupt()\n");
    Locker locker(m_Mutex);
    m_Cond.Broadcast();
    char c = 0;
    ::write(m_iPipeFd[1], &c, 1);
}

double AsfNetworkInputStream::cacheSize() const
{
    Locker locker(m_Mutex);
    AVM_WRITE("ASF network reader", 1, "cacheSize()   finished: %d\n", m_bFinished);
    if (m_bFinished) return 1.;

    int min = m_Iterators.size() ? CACHE_PACKETS : 0;
    for(avm::vector<NetworkIterator*>::const_iterator it2 = m_Iterators.begin();
	it2!=m_Iterators.end(); it2++)
    {
	int s = (*it2)->m_Packets.size();
	//AVM_WRITE("ASF network reader", "cachesz  %d\n", s);
	if (s < min)
            min = s;
    }
    //AVM_WRITE("ASF network reader", "cacheSizeMin %d\n", min);
    return min / (double) CACHE_PACKETS;
}

void AsfNetworkInputStream::clear()
{
    AVM_WRITE("ASF network reader", 1, "clear()\n");
    Locker locker(m_Mutex);
    for(avm::vector<NetworkIterator*>::iterator it2 = m_Iterators.begin();
	it2!=m_Iterators.end(); it2++)
    {
	//printf("SIZE %d\n", (*it2)->m_Packets.size());
	for (uint_t i = 0; i < (*it2)->m_Packets.size(); i++)
	    (*it2)->m_Packets[i]->release();

	(*it2)->m_Packets.clear(); //FIXME - release
    }
}

void* AsfNetworkInputStream::threadFunc()
{
    int result = -1;
    m_lReadBytes = 0;

    if (createSocket() < 0)
	goto terminated;

    m_lfd = -12345;
    sprintf(m_pBuffer, m_pcFirstRequest,
	    m_ServerFilename.c_str(), m_Server.c_str(), m_iRandcntx);
    AVM_WRITE("ASF network reader", 1, "Request1 [ %s ]\n", m_pBuffer);
    m_Ctype = checkContent(m_pBuffer);

    switch (m_Ctype)
    {
    case Unknown:
	AVM_WRITE("ASF network reader", "Unknown Content-Type - closing...\n");
	goto terminated;
    case Redirect:
	AVM_WRITE("ASF network reader", "Redirector\n");
	if (readRedirect() == 0)
	    goto ok;
        goto terminated;
    case Plain:
        break;
    default:
	// reopen for broadcasted  prerecorder & live streams
        // user can select which streams he wants to receive
	close(m_iSocket);
	m_iSocket = -1;
    }

    while(!m_bQuit)
    {
	int scnt;
	avm::string sffff;
	m_bFinished = true;
	m_Mutex.Lock();
	if (!m_Iterators.size())
	    m_Cond.Wait(m_Mutex); // wait for read or IsValid
        avm::vector<NetworkIterator*>::iterator it2;
	for(it2 = m_Iterators.begin(); it2 != m_Iterators.end(); it2++)
	    (*it2)->setEof(false);
	for (avm::vector<ASFStreamHeader>::iterator it = m_Streams.begin();
	    it != m_Streams.end(); it++)
	{
	    int id = (*it).hdr.stream & 0x7f;
            bool in = false;
	    for (it2 = m_Iterators.begin(); it2 != m_Iterators.end(); it2++)
		if ((*it2)->getId() == id)
		{
		    in = true;
                    break;
		}
	    char b[30];
	    sprintf(b, "ffff:%d:%d ", id, (in) ? 0 : 2);
            sffff += b;
	}
	scnt = m_Streams.size();
	m_Mutex.Unlock();
	clear();
	if (m_iSocket < 0)
	{
	    if (createSocket() < 0)
		goto terminated;
	}
	//printf("INIT %d   %s\n", scnt, sffff.c_str());
	switch(m_Ctype)
	{
	case Prerecorded:
            // CHECK which stream are actually requested by user
	    sprintf(m_pBuffer, m_pcSeekableRequest,
		    m_ServerFilename.c_str(),
		    m_Server.c_str(),
		    m_uiTime,
		    ~0, ~0, // offset
		    m_iRandcntx,
		    0x7FFFFFFF,
		    scnt, sffff.c_str() // stream_count
		    //4, "ffff:1:0 ffff:2:0 ffff:3:0 ffff:4:0"
		   );
            AVM_WRITE("ASF network reader", 1, "Requesting prerecorded [ %s ]\n", m_pBuffer);
	    m_Ctype = checkContent(m_pBuffer);
	    break;
	case Live:
	    sprintf(m_pBuffer, m_pcLiveRequest,
		    m_ServerFilename.c_str(),
		    m_Server.c_str(),
		    m_iRandcntx,
		    scnt, sffff.c_str() // stream_count
		   );
	    m_Ctype = checkContent(m_pBuffer);
            AVM_WRITE("ASF network reader", 1, "Requesting live [ %s ]\n", m_pBuffer);
	    break;
	case Plain:
	    if (m_bAcceptRanges && m_uiDataOffset && m_Header.max_bitrate)
	    {
                // estimate packet in the file
		int64_t o = (int64_t) m_Header.max_bitrate * (int64_t) m_uiTime;
		o /= (8 * 1000); // bits -> bytes * milisec -> sec
		o = (o / m_Header.max_pktsize) * m_Header.max_pktsize;
                o += m_uiDataOffset;
		sprintf(m_pBuffer, m_pcRangeRequest,
			m_ServerFilename.c_str(),
			m_Server.c_str(),
			o,
			m_iRandcntx
		       );
		AVM_WRITE("ASF network reader", 1, "Requesting range [ %s ]\n", m_pBuffer);
                m_Ctype = checkContent(m_pBuffer);
	    }
	default:
            break;
	}

	if (m_Ctype == Unknown)
	{
	    AVM_WRITE("ASF network reader", "threadFunc() unknown Content-Type\n");
	}
	else
	{
	    if (readContent() < 0) // main reading loop
		AVM_WRITE("ASF network reader", "read_content() aborted\n");
	    else
		AVM_WRITE("ASF network reader", "read_content() successful\n");
	    m_uiTime = 0;
	}

	if (m_bQuit)
	    goto terminated;

	switch (m_Ctype)
	{
	case Prerecorded:
	    sprintf(m_pBuffer, m_pcPostRequest,
		    m_ServerFilename.c_str(),
		    m_Server.c_str(),
                    m_iRandcntx);
            AVM_WRITE("ASF network reader", 1, "Posting prerecorded [ %s ]\n", m_pBuffer);
	    write(m_pBuffer, strlen(m_pBuffer));
	default:
	    break;
	}
	m_Mutex.Lock();
	m_bWaiting = true;
	close(m_iSocket);
	m_iSocket = -1;
	AVM_WRITE("ASF network reader", "Waiting for wake up\n");
	m_Cond.Broadcast();
	m_Cond.Wait(m_Mutex);
	m_Mutex.Unlock();
	flushPipe();
	AVM_WRITE("ASF network reader", "Continuing...\n");
    }
    result = 0;
    goto ok;

terminated:
    m_bHeadersValid = true;
    m_bQuit = true;
ok:
    m_Mutex.Lock();
    for(avm::vector<NetworkIterator*>::iterator it2 = m_Iterators.begin();
	it2 != m_Iterators.end(); it2++)
	(*it2)->setEof(true);
    m_Cond.Broadcast();
    m_Mutex.Unlock();
    clear();
    delete[] m_pBuffer;
    m_pBuffer = 0;
    if (m_iSocket >= 0)
	close(m_iSocket);
    m_iSocket = -1;
    flushPipe();
    return (void*)result;
}

void* AsfNetworkInputStream::threadStarter(void* arg)
{
    return ((AsfNetworkInputStream*)arg)->threadFunc();
}

int AsfNetworkInputStream::createSocket()
{
    avm::string sn;
    int proxyport;
    int sp;

    h_errno = 0;
    if (m_iProxyport != 0)
    {
	sp = m_iProxyport;
	sn = m_Proxyhost.c_str();
    }
    else
    {
	uint_t end = m_Server.find(":");
	assert(end != avm::string::npos);
	sp = atoi(m_Server.substr(end+1).c_str());
	sn = m_Server.substr(0, end);
    }

    // if(!inet_aton(m_Server.c_str(), &sa.sin_addr))
    hostent* he = gethostbyname(sn.c_str());
    if (!he || !he->h_addr_list || !he->h_addr_list[0])
    {
	AVM_WRITE("ASF network reader", "Warning: could not resolve server name %s:%d ( %s )\n",
		  sn.c_str(), sp, strerror(h_errno));
        return -1;
    }

    sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = he->h_addrtype;
    sa.sin_port = htons(sp);
    sa.sin_addr = *(in_addr*)(he->h_addr_list[0]);

    m_iSocket = socket(he->h_addrtype, SOCK_STREAM, 0);
    if (m_iSocket < 0)
    {
	AVM_WRITE("ASF network reader", "Socket() failed ( %s )\n", strerror(errno));
	return -1;
    }

    int oflg = fcntl(m_iSocket, F_GETFL);
    fcntl(m_iSocket, F_SETFL, oflg | O_NONBLOCK);
    int r = connect(m_iSocket, (sockaddr*)&sa, sizeof(sa));
    if (r == -1	&& errno == EINPROGRESS)
    {
	int maxtry = 50; // hmm should be settable by user ???
	while (!m_bQuit && --maxtry >= 0)
	{
	    fd_set wset;
	    fd_set rset;
	    int mx = (m_iSocket > m_iPipeFd[0]) ? m_iSocket : m_iPipeFd[0];
	    struct timeval tout = { 1, 0 }; // 1s timeout
	    FD_ZERO(&wset);
	    FD_ZERO(&rset);
	    FD_SET(m_iSocket, &wset);
	    FD_SET(m_iPipeFd[0], &rset);

	    r = select(mx + 1, &rset, &wset, 0, &tout);
	    if (r > 0)
	    {
		if (FD_ISSET(m_iPipeFd[0], &rset))
		{
		    flushPipe();
		    AVM_WRITE("ASF network reader", 1, "connect: interrupted\n");
		    r = -1;
		}
		break;
	    }
	    else if (r < 0)
	    {
		AVM_WRITE("ASF network reader", "connect: select failed\n");
                break;
	    }
	    else // r == 0
	    {
		if (maxtry == 0)
		{
		    AVM_WRITE("ASF network reader", "connect: timeout\n");
		    Locker locker(m_Mutex);
		    m_Cond.Broadcast();
		}
		continue;
	    }
	}
	if (r > 1)
	{
	    socklen_t errlen = sizeof(int);
	    int err;
	    r = getsockopt(m_iSocket, SOL_SOCKET, SO_ERROR, &err, &errlen);
	    if (r < 0)
	    {
		AVM_WRITE("ASF network reader", "connect: getsockopt failed %s\n", strerror(errno));
	    }
	    else if (err > 0)
	    {
		AVM_WRITE("ASF network reader", "connect: error %s\n", strerror(err));
		r = -1;
	    }
	    else
                r = 0;
	}
    }
    fcntl(m_iSocket, F_SETFL, oflg);
    //printf("ENDLOOP %d  %d\n", r, m_iSocket);
    if (r < 0)
    {
	AVM_WRITE("ASF network reader", "Warning: connection failed ( %s )\n", strerror(errno));
	close(m_iSocket);
        m_iSocket = -1;
    }
    else
    {
	AVM_WRITE("ASF network reader", 1, "Socket created\n");
	/*
	 char hostname[256];
	 if (gethostname(hostname, sizeof(hostname) - 1))
	 {
	 AVM_WRITE("ASF network reader", "WARNING: gethostname() failed ( %s )\n", strerror(errno));
	 hostname[sizeof(hostname) - 1]=0;
	 }
	 */
    }
    return m_iSocket;
}

int AsfNetworkInputStream::read(void* buffer, uint_t size)
{
    uint_t result = 0;

    while (!m_bQuit)
    {
	fd_set fset;
	FD_ZERO(&fset);
	FD_SET(m_iSocket, &fset);
	FD_SET(m_iPipeFd[0], &fset);

	struct timeval tout = { 20, 0}; // 20s timeout
	int mx = (m_iSocket > m_iPipeFd[0]) ? m_iSocket : m_iPipeFd[0];

	if (select(mx + 1, &fset, 0, 0, &tout) == 0)
	{
	    AVM_WRITE("ASF network reader", "NetRead: TIMEOUTED\n");
	    Locker locker(m_Mutex);
	    m_Cond.Broadcast();
	    return -1;//continue;
	}
	//printf("socket %d %d\n", FD_ISSET(m_iPipeFd[0], &fset), FD_ISSET(m_iSocket, &fset));
	if (FD_ISSET(m_iPipeFd[0], &fset))
	{
	    flushPipe();
	    AVM_WRITE("ASF network reader", 1, "Interrupted\n");
            return -1;
	}
	if (FD_ISSET(m_iSocket, &fset))
	{
	    int tmp = ::read(m_iSocket, (char*)buffer + result, size - result);
	    //AVM_WRITE("ASF network reader", "NetRead: %d bytes    (want: %d)\n", tmp, size-result);

	    if (tmp <= 0)
	    {
		if (errno != EAGAIN && errno != EINTR)
		{
                    if (errno)
			AVM_WRITE("ASF network reader", "Aborting: read() returned %d  ( %s )\n", errno, strerror(errno));
		    if (result>0)
			return result;
                    if (tmp < 0)
			AVM_WRITE("ASF network reader", "read() failed\n");
                    return tmp;
		}
		if (tmp == 0)
                    return 0;
                continue;
	    }
	    result += tmp;
	    //printf("result  %d:%d\n", result, size);
	    if (result >= size)
	    {
		if (l_iFd >= 0)
		{
		    if (result > 32)
			AVM_WRITE("ASF network reader", 1, "read(): returned %d of %d bytes\n", result, size);
		    ::write(l_iFd, buffer, result);
		    fsync(l_iFd);
		}
                m_lReadBytes += result;
		return result;
	    }
	}
    }
    return -1;
}

int AsfNetworkInputStream::write(const void* b, uint_t size)
{
    const char* buffer = (const char*) b;
    int wsize = size;
    while (wsize > 0)
    {
	int i = ::write(m_iSocket, buffer, wsize);
	if (i <= 0)
            return i;
	buffer += i;
        wsize -= i;
    }
    return size;
}

int AsfNetworkInputStream::dwrite(const void* b, uint_t size)
{
    const char* buffer = (const char*) b;
    if (m_lfd < 0)
    {
	if (m_lfd == -12345)
	{
	    if (getenv("WRITE_ASF"))
	    {
		// check if recording is requested
		char tmpfilename[256];
		char* tmpdir = getenv("TMPDIR");
		strncpy(tmpfilename, (tmpdir) ? tmpdir : "/tmp", 240);
		tmpfilename[240] = 0;
		strcat(tmpfilename, "/asfXXXXXX");
		m_lfd = mkstemp(tmpfilename);
		AVM_WRITE("ASF network reader", "Writing ASF file: %s  (%d)\n", tmpfilename, m_lfd);
	    }
	    else
	    {
		AVM_WRITE("ASF network reader", "\n\n"
			  "\tIf you wish to store the stream into a local file\n"
			  "\tyou may try to use this before starting aviplay:\n"
			  "\t\texport WRITE_ASF=1\n"
			  "\tthis will create /tmp/asfXXXXX with received data\n\n");

		m_lfd = -1;
	    }
	}
        if (m_lfd < 0)
	    return -1;
    }
    while (size > 0)
    {
	int i = ::write(m_lfd, buffer, size);
	if (i < 0)
            return i;
	buffer += i;
	size -= i;
    }
    fsync(m_lfd);
    return 0;
}

int AsfNetworkInputStream::readContent()
{
    m_bFinished = false;
    int64_t rsize = 100000000;
    int skip = 0;

    while (!m_bQuit && !m_bFinished)
    {
	if (cacheSize() >= 1.)
	{
	    AVM_WRITE("ASF network reader", 1, "FULLCACHE  - wait for empty place\n");
	    char c;
	    fd_set fset;
	    FD_ZERO(&fset);
	    FD_SET(m_iPipeFd[0], &fset);
	    struct timeval tout = { 0, 1000000 };
	    if (select(m_iPipeFd[0] + 1, &fset, 0, 0, &tout) != 0)
	    {
		if (FD_ISSET(m_iPipeFd[0], &fset))
		{
		    ::read(m_iPipeFd[0], &c, 1);
		    AVM_WRITE("ASF network reader", 1, "read_content(): interrupted\n");
                    break;
        	}
	    }
	    continue;
	}

	if (m_Ctype == Plain)
	{
	    // ugly trick for now - emulating streamed data
	    if (m_uiDataOffset > 0)
	    {
		chhdr.size = m_Header.max_pktsize;
                chhdr.partflag = 0;
                rsize -= chhdr.size;
		//chhdr.kind = (rsize > 0) ? 0x4424 : 0x4524;
		chhdr.kind = 0x4424;
	    }
	    else
	    {
		// after the header the data header
                // should follow
		if (read(m_pBuffer, 24 + 26) < 0)
		{
		    m_bFinished = true;
                    continue;
		}
		dwrite(m_pBuffer, 24 + 26);
                GUID* guid = (GUID*) m_pBuffer;
		avm_get_leGUID(guid);
		if (guid_get_guidid(guid) == GUID_ASF_DATA)
		{
		    rsize = avm_get_le64(m_pBuffer + 16) - 24;
		    // remember offset - so we are able to
                    // seek in Plain streams
		    m_uiDataOffset = m_lReadBytes;
		}
	    }
	}
	else
	{
	    if (read(&chhdr, sizeof(chhdr)) <= 0)
	    {
		AVM_WRITE("ASF network reader", 1, "read() no more data\n");
		break;
	    }
	    chhdr.le16();
	    //AVM_WRITE("ASF network reader", 1, "PACKET:  0x%x  0x%x  0x%x   0x%x  0x%x\n", chhdr.kind, chhdr.size, chhdr.seq, chhdr.tmp, chhdr.size_confirm);
	    if (chhdr.size < 8)
	    {
		AVM_WRITE("ASF network reader", "I don't like chunk size: %d\n", chhdr.size);
		break;
	    }
	    if (chhdr.size != chhdr.size_confirm)
	    {
		AVM_WRITE("ASF network reader", "size != size_confirm (%d != %d)\n", chhdr.size, chhdr.size_confirm);
                break;
	    }
            chhdr.size -= 8;
	}

	asf_packet* p;
	unsigned short& size = chhdr.size;
	//printf("STREAMPACKET 0x%x  sz:%d  %lld   seq:%d part:%d  sc:%d\n", chhdr.kind, size, rsize, chhdr.seq, chhdr.partflag, chhdr.size_confirm);
	switch (chhdr.kind)
	{
	case 0x4824: // $H header
	    if (chhdr.partflag & 0x0400)
	    {
		skip = read(m_pBuffer, chhdr.size);
		if (skip < chhdr.size)
		{
		    AVM_WRITE("ASF network reader", "wrong size (%d != %d)\n", skip, chhdr.size);
		    m_bFinished = true;
		    continue;
		}
	    }
	    else if (skip > 0 && !(chhdr.partflag & 0x0800))
	    {
		int r = read(m_pBuffer + skip, chhdr.size);
		if (r < chhdr.size)
		{
		    AVM_WRITE("ASF network reader", "wrong size (%d != %d)\n", r, chhdr.size);
		    m_bFinished = true;
		    continue;
		}
                skip += chhdr.size;
	    }
	    else
		readHeader(skip + chhdr.size, skip);
	    break;
	case 0x4424: // $D data
	    if (!m_bHeadersValid)
	    {
		AVM_WRITE("ASF network reader", "unexpected data chunk (no headers yet)\n");
		m_bFinished = true;
		continue;
	    }
	    if (size > m_Header.max_pktsize)
	    {
		AVM_WRITE("ASF network reader", "size>m_Header.max_pktsize (%d > %d) ignoring...\n", size, m_Header.max_pktsize);
		continue;
	    }

	    //AVM_WRITE("ASF network reader", "Packet size %d\n", size);
	    p = new asf_packet(m_Header.max_pktsize);
	    if (read(&(*p)[0], size) <= 0)
	    {
		p->release();
		AVM_WRITE("ASF network reader", "read() no more data\n");
		m_bFinished = true;
		continue;
	    }

	    //for (int i = 0; i < 20; i++)
	    //    AVM_WRITE("ASF network reader", "%02x ",(uint_t) (*p)[inloaded + i]);
	    //AVM_WRITE("ASF network reader", "\n");
	    if (m_lfd >= 0 && size < m_Header.max_pktsize)
                memset(&(*p)[size], 0, m_Header.max_pktsize - size); // zero padding
	    //AVM_WRITE("ASF network reader", 0, "Created packet %p  %d - 0x%x (free %d : %.2f)\n", p, p->size(), (*p)[0], m_Header.max_pktsize - size, size/(float)m_Header.max_pktsize);
	    if (p->init(m_uiTimeshift) == 0)
	    {
		if (!m_uiTimeshift && m_Ctype == Live)
		{
		    m_uiTimeshift = p->send_time - m_Header.preroll;
		    p->fragments.clear();
                    // reinit packet with new timeshift
		    p->init(m_uiTimeshift);
		    AVM_WRITE("ASF network reader", "setting timeshift %.3fs\n", m_uiTimeshift / 1000.);
		}
		dwrite(&(*p)[0], m_Header.max_pktsize);
		m_Mutex.Lock();
		//printf("Iterators: %d\n", m_Iterators.size());
                avm::vector<NetworkIterator*>::iterator it;
		for(it = m_Iterators.begin(); it != m_Iterators.end(); it++)
		{
		    p->addRef();
		    (*it)->m_Packets.push_back(p);

		    uint_t psz = (*it)->m_Packets.size();
		    //printf("PACKET SIZE %d  time:%u\n", psz, p->send_time);
		    if (psz >= 2 * CACHE_PACKETS && !(psz & 1))
		    {
			uint_t i = 0;
			while (i + psz/2 < psz)
			{
			    (*it)->m_Packets[i]->release();
			    (*it)->m_Packets[i] = (*it)->m_Packets[i + psz/2];
			    i++;
			}
			(*it)->m_Packets.resize(i);
			//AVM_WRITE("ASF network reader", "Removing unreaded packets %d\n", m_Iterators.size());
		    }
		}
		m_Cond.Broadcast();
		m_Mutex.Unlock();
	    }

	    p->release();
	    break;
	case 0x4524: // $E
	    m_bFinished = true;
	    AVM_WRITE("ASF network reader", "read_content(): finished transmission\n");
	    break;
	} //switch
    }

    if (m_lfd>=0) close(m_lfd);
    m_lfd = -1;

    m_Mutex.Lock();
    for(avm::vector<NetworkIterator*>::iterator it2 = m_Iterators.begin();
	it2 != m_Iterators.end(); it2++)
	(*it2)->setEof(true);
    //not here m_Cond.Broadcast();
    m_Mutex.Unlock();

    if (m_bFinished)
        return 0;

    m_bFinished = true;
    return -1;
}

AsfNetworkInputStream::Content AsfNetworkInputStream::checkContent(const char* request)
{
    int hdrpos = 0;
    uint_t linepos = 0;
    uint_t linenum = 0;
    char *hdrptr;
    // Platform for Privacy Preferences (p3p)
    // http://www.w3.org/P3P/
    bool eol = false;
    char HTTPLine[1024];
    char Features[256];
    char ContentType[256];

    ContentType[0] = 0;
    Features[0] = 0;

    // send http request
    write(request, strlen(request));
    //AVM_WRITE("ASF network reader", "WRITE %s\n", request);

    for (;;)
    {
	char c;
	if (read(&c, 1) <= 0)
	    // header was incorrect
            // this is probably the best we could do
	    return Unknown;

	// \r\n  means http header eof
	if (linepos < (sizeof(HTTPLine) - 1)
	    && (c != '\r') && (c != '\n'))
	{
	    eol = false;
            // all letter in lowercase
	    HTTPLine[linepos++] = tolower(c);
	}
	else
	    HTTPLine[linepos] = 0;

	if (c == '\n')
	{
	    if (eol)
                break; // header eof

	    linepos = 0;
	    eol = true;
	    linenum++;
	    hdrptr = HTTPLine;

	    //printf("PARSEHTTP: %s\n",hdrptr);
	    /* Parse first line of HTTP reply */
	    if (linenum == 1)
	    {
		if (!strncmp(hdrptr, "http/1.0 ", 9)
		    || !strncmp(hdrptr, "http/1.1 ", 9))
		{
		    int errorcode = 0;
		    hdrptr += 9;
		    sscanf(hdrptr, "%d", &errorcode);
		    hdrptr += 4;
		    if (strstr(hdrptr, "redirect")
			|| strstr(hdrptr, "object moved"))
		    {
			m_iRedirectSize = 4096;
			return Redirect;
		    }
		    if (strstr(hdrptr, "bad request"))
			return Unknown;
		}
		else
		{
		    AVM_WRITE("ASF network reader", "Illegal server reply! Expected HTTP/1.0 or HTTP/1.1\n");
		}
	    }
	    else
	    {
		/* parse all other lines of HTTP reply */
		if (!strncmp(hdrptr, "content-type: ", 14))
		{
		    strncpy(ContentType, hdrptr + 14, sizeof(ContentType));
		}
		else if (!strncmp(hdrptr, "content-length: ", 16))
		{
		    m_iRedirectSize = atoi(hdrptr + 16);
		    AVM_WRITE("ASF network reader", 1, "Content-Length: %d\n", m_iRedirectSize);
		}
		else if (!strncmp(hdrptr, "pragma: ", 8))
		{
		    char* f = strstr(hdrptr + 8, "features=");
                    if (f)
			strncpy(Features, f + 9, sizeof(Features));
		}
		else if (!strncmp(hdrptr, "accept-ranges: bytes", 20))
		    m_bAcceptRanges = true;
	    }
	}
    }

    // ok here it is a bit tricky
    // many servers seems to be passing here false headers
    // were they claim they are redirectors while they
    // are already giving us a stream
    // so we try to read some more data from the connection
    // but only in case we have not yet decided type of stream
    // as we could get here for the Plain seek again
    // FIXME
    m_lReadBytes = 0;
    if ((m_Ctype == Unknown) && (m_iReadSize = read(m_pBuffer, sizeof(chhdr))) > 0)
    {
	memcpy(&chhdr, m_pBuffer, m_iReadSize);
	chhdr.le16();
        chhdr.size -= 8;
	//printf("PACKET sz:%d se:%d un:%d  sc:%d\n", chhdr.size, chhdr.seq, chhdr.partflag, chhdr.size_confirm);
	if (chhdr.kind == 0x4824) // $H
	{
	    int hsize = chhdr.size;
	    int skip = 0;

	    if (chhdr.partflag & 0x0400)
		while (chhdr.kind == 0x4824)
		{
		    if (chhdr.partflag & 0x0400)
		    {
                        // reset
			skip = 0;
			hsize = 0;
		    }
		    hsize += chhdr.size;
		    if (chhdr.partflag & 0x0800)
			break; // header packet complete
		    int r = read(m_pBuffer + skip, chhdr.size);
		    if (r < 0)
			break;
		    skip += r;
		    read(&chhdr, sizeof(chhdr));
		    chhdr.le16();
		    chhdr.size -= 8;
		}
	    readHeader(hsize, skip);
	}
	else
	{
	    uint32_t h = avm_get_le32(m_pBuffer);
	    if (h == 0x75b22630) // guid_asf_header
	    {
		m_Ctype = Plain;
		readHeader(16 + 8, m_iReadSize);
		// if this is the ASF header -> no socket reopen
	    }
	}
    }

    /* Determine whether this is live content or not */
    if (!strcmp(ContentType, "application/octet-stream")
	|| !strcasecmp(ContentType, "application/vnd.ms.wms-hdr.asfv1")
	|| !strcasecmp(ContentType, "application/x-mms-framed"))
    {
	if (!m_bHeadersValid) return Redirect;
	return (strstr(Features, "broadcast")) ? Live : Prerecorded;
    }

    if ((m_bHeadersValid && m_Ctype == Plain)
	|| !strcmp(ContentType, "text/plain"))
        // if the redirector's size is small it's not a plain file
	return (m_iRedirectSize < 300) ? Redirect : Plain;

    if ((!strcmp(ContentType, "text/html"))
	|| (!strcmp(ContentType, "audio/x-ms-wax"))
	|| (!strcmp(ContentType, "audio/x-ms-wma"))
	|| (!strcmp(ContentType, "video/s-ms-asf"))
	|| (!strcmp(ContentType, "video/x-ms-afs"))
	|| (!strcmp(ContentType, "video/x-ms-asf"))
	|| (!strcmp(ContentType, "video/x-ms-wma"))
	|| (!strcmp(ContentType, "video/x-ms-wmv"))
	|| (!strcmp(ContentType, "video/x-ms-wmx"))
	|| (!strcmp(ContentType, "video/x-ms-wvx"))
	|| (!strcmp(ContentType, "video/x-msvideo")))
	return Redirect;

    return Unknown;
}

void AsfNetworkInputStream::readHeader(uint_t size, uint_t skip)
{
    AVM_WRITE("ASF network reader", "read hdrbl %d  skip: %d\n", size, skip);
    int r = read(m_pBuffer + skip, size - skip);
    if (r < 0)
	return;

    // check if this is really asf header
    GUID guid = *(GUID*) m_pBuffer;
    avm_get_leGUID(&guid);
    if (guid_get_guidid(&guid) != GUID_ASF_HEADER)
	return; // let's just ignore this junk
    if (size == 24) // read the header (trick used for Plain)
    {
	int64_t hsz = avm_get_le64(m_pBuffer + 16) - 24;
        read(m_pBuffer + 24, hsz);
	size += hsz;
        skip = 0;
    }
    else
        r += skip;

    size = avm_get_le64(m_pBuffer + 16);
    if (!m_bHeadersValid)
	// first time header + data size beging (50 bytes)
        // should come right after header
	dwrite(m_pBuffer, size + 50);

    //printf("SIZE   %d  %d\n", size, r);
    m_Mutex.Lock();
    m_bHeadersValid = parseHeader(m_pBuffer + 24, size - 24,
				  m_bHeadersValid ? 1 : 0);
    if (m_bHeadersValid)
	AVM_WRITE("ASF network reader", "received valid headers\n");
    m_Cond.Broadcast();
    m_Mutex.Unlock();
}

int AsfNetworkInputStream::readRedirect()
{
    if (m_iRedirectSize > 65536)
    {
	AVM_WRITE("ASF network reader", "Redirector size too large! (%d)\n", m_iRedirectSize);
        m_iRedirectSize = 65536;
    }

    while (m_iReadSize < m_iRedirectSize
	   && read(&m_pBuffer[m_iReadSize], 1) > 0)
	m_iReadSize++;

    ASX_Reader* r = new ASX_Reader(m_Server, m_Filename);
    if (!r->create(m_pBuffer, m_iReadSize))
    {
	AVM_WRITE("ASF network reader", "No redirector found\n");
	delete r;
        return -1;
    }
    else
        m_pReader = r;
    return 0;
}

bool AsfNetworkInputStream::getURLs(avm::vector<avm::string>& urls)
{
    return (m_pReader) ? m_pReader->getURLs(urls) : false;
}

AsfIterator* AsfNetworkInputStream::getIterator(uint_t id)
{
    //AVM_WRITE("ASF network reader", "GETITERATOR %d\n", id);
    if (id >= m_Streams.size())
        return 0;
    m_Iterators.push_back(new NetworkIterator(this, m_Streams[id].hdr.stream & 0x7f));
    AsfIterator* it = m_Iterators.back();
    it->addRef();
    return it;
}

/*
 * replace specific characters in the URL string by an escape sequence
 */
void URLString::escape()
{
    uint_t sz = size();
    if (!sz)
	return;

    const char *inbuf = c_str();
    char *nstr = new char[sz * 3];
    char *outbuf = nstr;
    unsigned char c;
    do
    {
	/* mark characters  do not touch escape character */
	/* reserved characters  see RFC 2396 */
	static const char* avoid = "#`'<>[]^|\\\"";
	c = *inbuf++;
	if (c && (c <= ' ' || strchr(avoid, c)))
	{
	    /* these should be escaped */
	    unsigned char c1 = (c >> 4) & 0x0f;
	    unsigned char c2 = c & 0x0f;
	    if (c1 < 10) c1 += '0';
	    else c1 += 'A';
	    if (c2 < 10) c2 += '0';
	    else c2 += 'A';
	    *outbuf++ = '%';
	    *outbuf++ = c1;
	    *outbuf++ = c2;
	}
	else
	    *outbuf++ = c;
    } while (c);

    //printf("Before: %s\nAfter: %s\n", str, n);
    delete[] str;
    str = nstr;
}

AVM_END_NAMESPACE;
