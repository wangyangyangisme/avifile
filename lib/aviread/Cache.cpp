#include "Cache.h"
#include "avm_cpuinfo.h"
#include "avm_output.h"
#include "utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>

// limit the maximum size of each prefetched stream in bytes
#define STREAM_SIZE_LIMIT 3000000

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

// Isn't this bug in NetBSD configuration - it should not have HAVE_LSEEK64
#ifdef __NetBSD__
#define lseek64 lseek
#else
#ifndef HAVE_LSEEK64
#define lseek64 lseek
#endif
#endif


// uncomment if you do not want to use threads for precaching
//#define NOTHREADS               // do not commit with this define

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "StreamCache"
//static float ttt = 0;
//static float ttt1 = 0;

Cache::Cache(uint_t size)
    :m_uiSize(size), thread(0), m_pPacket(0), m_iFd(-1), m_uiId(0),
    cache_access(0), cache_right(0), cache_miss(0), m_bQuit(false),
#ifndef NOTHREADS
    m_bThreaded(true) // standard operation mode
#else
    m_bThreaded(false) // for debugging purpose only - never commit
#endif
{
}

Cache::~Cache()
{
    mutex.Lock();
    m_bQuit = true;
    cond.Broadcast();
    mutex.Unlock();
    delete thread;
    clear();
    //printf("CACHE A %d  MIS %d  RIGH %d\n", cache_access, cache_miss, cache_right);
    if (cache_access != 0)
	AVM_WRITE(__MODULE__, "Destroy... (Total accesses %d, hits %.2f%%, misses %.2f%%, errors %.2f%%)\n",
		  cache_access, 100. * double(cache_right - cache_miss) / cache_access,
		  100. * double(cache_miss) / cache_access,
		  100. * double(cache_access - cache_right) / cache_access);
    //printf("TTTT %f  %f\n", ttt, ttt1);
}

int Cache::addStream(uint_t id, const avm::vector<uint32_t>& table)
{
    AVM_WRITE(__MODULE__, 3, "Adding stream, %d chunks\n", table.size());
    mutex.Lock();
    m_streams.push_back(StreamEntry(&table, 0, m_uiSize));
    cond.Broadcast();
    mutex.Unlock();

    return 0;
}

// starts caching thread once we know file descriptor
int Cache::create(int fd)
{
    m_iFd = fd;
    AVM_WRITE(__MODULE__, 1, "Creating cache for file descriptor: %d\n", m_iFd);
    if (m_streams.size() > 0)
    {
	if (m_bThreaded)
	{
	    mutex.Lock();
	    thread = new PthreadTask(0, &startThreadfunc, this);
	    cond.Wait(mutex);
	    mutex.Unlock();
	}
    }
    else
	AVM_WRITE(__MODULE__, "Warning: No stream for caching!\n");

    return 0;
}

// return true if this stream should be waken and read new data
inline bool Cache::isCachable(StreamEntry& stream, uint_t id)
{
    // trick to allow precaching even very large image sizes
    //printf("ISCHACHE f:%d  s:%d  sum:%d   l: %d ll: %d\n", stream.packets.full(), stream.packets.size(), stream.sum, stream.last, stream.table->size());
    return ((stream.sum < STREAM_SIZE_LIMIT
	     // assuming id=0 is video stream
	     // uncompressed video could be really huge!
	     //|| ((id == 0) && stream.packets.size() < 3)
	    )
	    && stream.last < stream.table->size()
	    && !stream.packets.full()
	    && (stream.filling
                || (stream.sum < STREAM_SIZE_LIMIT/2
		    && stream.packets.size() < m_uiSize/2)));
}

//
// currently preffered picking alghorithm
//
// seems to be having good seek strategy
uint_t Cache::pickChunk()
{
    uint_t id = m_uiId;

    do
    {
	StreamEntry& se = m_streams[id];
	// determine next needed chunk in this stream
	se.last = (se.packets.empty()) ?
	    se.position : se.packets.back()->position + 1;
	//printf("Pick sid:%d  pos:%d  wants:%d  size:%d\n", id, se.last, se.position, se.packets.size());

	if (isCachable(se, id))
	    return id;
	// try next stream
	if (++id >= m_streams.size())
	    id = 0; // wrap around
    }
    while (id != m_uiId);

    return WAIT; // nothing for caching found
}

// caching thread
void* Cache::threadfunc()
{
    int r = 0;
    mutex.Lock();
    while (!m_bQuit)
    {
	m_uiId = pickChunk();
	cond.Broadcast();
	// allow waiting tasks to use cache during read or Wait()
        if (m_uiId == WAIT)
	{
	    m_uiId = 0;

	    // one could be trying to send signal to this thread
	    //AVM_WRITE(__MODULE__, 4, "full packets - waiting for read: s#1: %d s#2: %d\n",
	    //          m_streams[0].packets.size(), m_streams[1].packets.size());
	    cond.Wait(mutex);
	    //AVM_WRITE(__MODULE__, 4, "full packets - waiting done\n");
            continue;
	}

	StreamEntry& stream = m_streams[m_uiId];
#if 0
	if (stream.packets.size() > 0)
	{
	    if (r > 20000)
	    {
		r = 0;
		// don't stress hardrive too much
		//printf("sleep  %d\n", stream.packets.size());
		//cond.Wait(mutex, 0.05);
		continue; // recheck if the same chunk is still needed
	    }
	}
#endif
	//printf("read %d   l:%d  p:%d  s:%d\n", m_uiId, stream.last, stream.position, stream.packets.size());
	uint_t coffset = (*stream.table)[stream.last];
	char bfr[8];
	//ttt1 += to_float(longcount(), sss);
	//cond.Broadcast();
	mutex.Unlock();
	//int64_t sss = longcount();
	if (lseek64(m_iFd, coffset & ~1, SEEK_SET) == -1
	    || ::read(m_iFd, bfr, 8) != 8)
	{
	    AVM_WRITE(__MODULE__, "Warning: Offset %d unreachable! %s\n", coffset & ~1, strerror(errno));
            mutex.Lock();
	    stream.error = stream.last;
	    cond.Broadcast();
	    cond.Wait(mutex);
	    continue;
	}

	uint_t ckid = avm_get_le32(bfr);
	uint_t clen = avm_get_le32(bfr + 4);

	//printf("READ - pos %d  ckid:0x%x  %d  of:0x%x\n", stream.last, ckid, clen, coffset);
	// get free buffer
	if (clen > StreamPacket::MAX_PACKET_SIZE)
	{
            // this is wrong and should be replaced by a better code
	    AVM_WRITE(__MODULE__, "Warning: Too large chunk %d\n", clen);
	    clen = 10000;
	}
	m_pPacket = new StreamPacket(clen);
	m_pPacket->position = stream.last;
	//AVM_WRITE(__MODULE__, 4, "id: %d   %d   buffered: %d sum: %d - %d\n",
	//	  m_uiId, stream.last, (stream.last - stream.position),
	//	  m_streams[0].sum, m_streams[1].sum);

	// cache might be read while this chunk is being filled
        uint_t rs = 0;
	while (rs < m_pPacket->size)
	{
	    int rd = ::read(m_iFd, m_pPacket->memory + rs, m_pPacket->size - rs);
	    //printf("READ %d  %d  of %d\n", rd, rs, m_pPacket->size);
	    if (rd <= 0)
	    {
		if (stream.error == stream.OK)
		    AVM_WRITE(__MODULE__, "Warning: Offset %d short read (%d < %d)! %s\n", coffset, rs, m_pPacket->size, (rd < 0) ? strerror(errno) : "");
		break;
	    }
	    rs += rd;
	}
        r += rs;
	mutex.Lock();
	//printf("memch: "); for (int i = 8; i < 20; i++)  printf(" 0x%02x", (uint8_t) *(m + i)); printf("\n");
	//printf("readsize  %d  %d\n", rs, m_pPacket->size);

	// check if we still want same buffer
	if (rs != m_pPacket->size)
	{
	    stream.error = stream.last;
            m_pPacket->Release();
	    m_pPacket = 0;
	    cond.Broadcast();
	    cond.Wait(mutex);
            continue;
	}
	//printf("Stream sum  %d  %d  %d\n", stream.sum, stream.last, stream.position);
	if (stream.packets.empty() && stream.position != stream.last) {
	    //printf("CH******************************  %d   %d\n", stream.position, stream.last);
	    m_pPacket->Release();
            m_pPacket = 0;
	    continue;
	}

	stream.error = stream.OK;
	//uint_t sum = 0; for (uint_t i = 0 ; i < m_pPacket->size; i++) sum += ((unsigned char*) m_pPacket->memory)[i]; printf("PACKETSUM %d   pos: %d  size: %d\n", sum, m_pPacket->position, m_pPacket->size);

	stream.sum += rs;
	m_pPacket->size = rs;
	m_pPacket->flags = (coffset & 1) ? AVIIF_KEYFRAME : 0;
	stream.filling = !(stream.sum > STREAM_SIZE_LIMIT);
	stream.packets.push(m_pPacket);
	//AVM_WRITE(__MODULE__, 4,
	//	  "---  id: %d   pos: %d  sum: %d  size: %d filling: %d\n",
	//	  m_uiId, m_pPacket->position, stream.sum, m_pPacket->size, stream.filling);
	m_pPacket = 0;
    }

    mutex.Unlock();
    return 0;
}

// called by stream reader - most of the time this read should
// be satisfied from already precached chunks
StreamPacket* Cache::readPacket(uint_t id, framepos_t position)
{
    //AVM_WRITE(__MODULE__, 4, "Cache: read(id %d, pos %d)\n", id, position);
    int rsize = 1;
    cache_access++;
    if (id >= m_streams.size()) {
	AVM_WRITE(__MODULE__, 1, "stream:%d  out ouf bounds (%d)\n",
		  id, m_streams.size());
	return 0;
    }

    StreamEntry& stream = m_streams[id];
    if (position >= stream.table->size()) {
	AVM_WRITE(__MODULE__, 1, "to large stream:%d pos: %d (of %d)\n",
		  id, position, stream.table->size());
	return 0;
    }

    if (!m_bThreaded)
    {
        // code path for single threaded reading
	//int64_t sss = longcount();
	Locker locker(mutex);
	char bfr[8];
	if (lseek64(m_iFd, (*stream.table)[position] & ~1, SEEK_SET) == -1
	    || ::read(m_iFd, bfr, 8) != 8) {
	    AVM_WRITE(__MODULE__, "Warning: Read error\n");
	    return 0;
	}

	uint_t ckid = avm_get_le32(bfr);
	uint_t clen = avm_get_le32(bfr + 4);
	if (clen > StreamPacket::MAX_PACKET_SIZE)
	{
	    AVM_WRITE(__MODULE__, "Warning: Too large chunk %d\n", clen);
	    clen = 100000;
	}
	StreamPacket* p = new StreamPacket(clen);
	if (p->size > 0)
	{
	    rsize = ::read(m_iFd, p->memory, p->size);
	    //printf("read_packet: id:%x   len:%d   rsize:%d  %d  m:%x\n", ckid, clen, rsize, p->size, *(int*)(p->memory));
	    if (rsize <= 0)
	    {
		p->Release();
		return 0;
	    }
	}
	p->flags = (*stream.table)[position] & 1 ? AVIIF_KEYFRAME : 0;
	p->position = position;
	//ttt1 += to_float(longcount(), sss);
	return p;
    }

    mutex.Lock();
    //while (stream.actual != position || stream.packets.size() == 0)
    StreamPacket* p = 0;
    while (!m_bQuit)
    {
	//printf("STREAMPOS:%d  sp:%d  id:%d  ss:%d\n", position, stream.position, id, stream.packets.size());
	if (!stream.packets.empty())
	{
	    p = stream.packets.front();
	    stream.packets.pop();
	    stream.sum -= p->size;
	    if (p->position == position)
	    {
		//AVM_WRITE(__MODULE__, 4, "id: %d bsize: %d memory: %p pp: %d\n",
		//	  id, stream.packets.size(), p->memory, p->position);
		cache_right++;
                break;
	    }
	    //AVM_WRITE(__MODULE__, 4, "position: 0x%x want: 0x%x\n", p->position, position);
	    // remove this chunk
	    //printf("delete chunkd %d   (wants %d)\n", p->position, position);
	    p->Release();
	    p = 0;
            continue;
	}
	if (stream.error == position) {
	    //printf("READERROR  e:%d   p:%d   pl:%d\n", stream.error, position, stream.last);
	    break;
	}
	cache_miss++;
        rsize = 0;
	m_uiId = id;
	stream.position = position;

	//AVM_WRITE(__MODULE__, 4, "--- actual: %d size: %d\n", id, stream.packets.size());
	//int64_t w = longcount();
	//printf("ToWait read  sid:%d  pos:%d  size:%d\n", id, position, stream.packets.size());
	cond.Broadcast();
	cond.Wait(mutex);
	//printf("----------- DoneWait read  size:%d\n", stream.packets.size());
	//ttt += to_float(longcount(), w);
	//AVM_WRITE(__MODULE__, 4, "--- actual: %d done - size: %d\n", id, stream.packets.size());
    }

    if (stream.packets.size() < (CACHE_SIZE / 2))
	cond.Broadcast(); // wakeup only when buffers are getting low...
    mutex.Unlock();
    //printf("RETURN packet %p\n", p);
    return p;
}

int Cache::clear()
{
    AVM_WRITE(__MODULE__, 4, "*** CLEAR ***\n");

    mutex.Lock();
    for (unsigned i = 0; i < m_streams.size(); i++)
    {
	StreamEntry& stream = m_streams[i];
	while (stream.packets.size())
	{
	    StreamPacket* r = stream.packets.front();
	    stream.packets.pop();
	    r->Release();
	}
	stream.sum = 0;
	stream.position = 0;
    }
    m_uiId = 0;
    cond.Broadcast();
    mutex.Unlock();

    return 0;
}

double Cache::getSize()
{
    /*
       int status=0;
       for(int i=0; i<m_uiSize; i++)
       if(req_buf[i].st==req::BUFFER_READY)status++;
       return (double)status/m_uiSize;
     */
    return 1.;
}

void* Cache::startThreadfunc(void* arg)
{
    Cache* c = (Cache*)arg;
    c->mutex.Lock();
    c->cond.Broadcast();
    c->mutex.Unlock();
    return c->threadfunc();
}


/*************************************************************/

InputStream::~InputStream()
{
    close();
}

int InputStream::open(const char *pszFile)
{
    m_iFd = ::open(pszFile, O_RDONLY | O_LARGEFILE);
    if (m_iFd < 0)
    {
	AVM_WRITE("InputStream", "Could not open file %s: %s\n", pszFile, strerror(errno));
        return -1;
    }

    m_iPos = ~0U;
    m_iBuffered = 0;
    return m_iFd;
}

void InputStream::close()
{
    delete cache;
    cache = 0;
    if (m_iFd >= 0)
	::close(m_iFd);
    m_iFd = -1;
}

int InputStream::async()
{
    if (!cache)
	cache = new Cache();
    return (cache) ? cache->create(m_iFd) : -1;
}

int InputStream::addStream(uint_t id, const avm::vector<uint32_t>& table)
{
    if (!cache)
	cache = new Cache();
    return (cache) ? cache->addStream(id, table) : -1;
}

int64_t InputStream::len() const
{
    struct stat st;
    fstat(m_iFd, &st);
    return st.st_size;
}

int64_t InputStream::seek(int64_t offset)
{
    m_iBuffered = 0;
    m_bEof = false;
    m_iPos = 0;
    return lseek64(m_iFd, offset, SEEK_SET);
}

int64_t InputStream::seekCur(int64_t offset)
{
    //cout << "seekcur " << offset << "   " << m_iPos << endl;
    m_bEof = false;
    if (m_iPos >= m_iBuffered)
	return lseek64(m_iFd, offset, SEEK_CUR);

    if (offset >= 0)
    {
	m_iPos += offset;
	if (m_iPos >= m_iBuffered)
	    return lseek64(m_iFd, m_iPos - m_iBuffered, SEEK_CUR);
    }
    else
    {
	if (m_iPos < -offset)
	{
	    offset += m_iBuffered - m_iPos;
	    m_iBuffered = 0;
	    return lseek64(m_iFd, offset, SEEK_CUR);
	}
	m_iPos += offset;
    }
    return pos();
}

int64_t InputStream::pos() const
{
    int64_t o = lseek64(m_iFd, 0, SEEK_CUR);
    //printf("POS: %lld   %d  %d   %lld\n", o, m_iPos, m_iBuffered, len());
    if (m_iPos < m_iBuffered)
	o -= (m_iBuffered - m_iPos);
    if (o > len())
        o = len();
    return o;
}

int InputStream::read(void* buffer, uint_t size)
{
    int r = 0;
    if (m_iBuffered > 0)
    {
	uint_t copy = m_iBuffered - m_iPos;
	if (size < copy)
	    copy = size;
	memcpy(buffer, bfr + m_iPos, copy);
	m_iPos += copy;
	r = copy;
	size -= copy;
        buffer = (char*) buffer + copy;
    }
    if (size > 0)
    {
	int s = ::read(m_iFd, buffer, size);
	if (s <= 0)
	{
	    m_bEof = true;
	    return -1;
	}
	r += s;
    }

    return r;
}

uint8_t InputStream::readByte()
{
    if (m_iPos >= m_iBuffered)
    {
	int r = ::read(m_iFd, bfr, sizeof(bfr));
	if (r <= 0)
	{
	    m_bEof = true;
	    return 255;
	}
	m_iBuffered = r;
	m_iPos = 0;
    }
    return bfr[m_iPos++];
#if 0
    uint8_t c;
    ::read(m_iFd, &c, 1);

    return c;
#endif
}

uint32_t InputStream::readDword()
{
    return readByte() | (readByte() << 8)
	| (readByte() << 16) | (readByte() << 24);
}

uint16_t InputStream::readWord()
{
    return readByte() | (readByte() << 8);
}


#undef __MODULE__

AVM_END_NAMESPACE;
