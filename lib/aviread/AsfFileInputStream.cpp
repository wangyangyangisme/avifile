#include "Cache.h" // first for lseek64 support

#include "AsfFileInputStream.h"
#include "asf_guids.h"
#include "avm_output.h"
#include "utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "AsfFileInputStream"

class FileIterator: public AsfIterator
{
public:
    FileIterator(AsfFileInputStream* parent, int id)
        : AsfIterator(id), m_lPos(0), m_pParent(parent),
	m_SeekInfo(m_pParent->m_SeekInfo[id])
    {
    }
    virtual const AsfStreamSeekInfo* getSeekInfo() { return m_SeekInfo; }
    virtual asf_packet* getPacket()
    {
	const ASFMainHeader& header = m_pParent->m_Header;
	//printf("Id: %d  PID %d\n", m_iId, getpid());
	//printf("Pos packer id: %d  pos : %d   (of %d   sz: %d)\n",
	//       m_iId, m_lPos, header.pkts_count,  m_SeekInfo->size());
	//if (m_lPos >= 0 || m_lPos < (int)header.pkts_count)
	{
	    asf_packet* p = new asf_packet(header.max_pktsize);

	    //AVM_WRITE("ASF file reader", "GET PACKET  %lld   size: %d\n", m_lPos, header.min_pktsize);

	    m_pParent->m_Mutex.Lock();
            int64_t pos = m_pParent->m_lDataOffset + m_lPos * header.max_pktsize;
	    lseek64(m_pParent->m_iFd, pos, SEEK_SET);
	    //printf("SEEK SET %lld %llx\n", pos, pos);
	    int i = read(m_pParent->m_iFd, &(*p)[0], header.max_pktsize);
	    m_pParent->m_Mutex.Unlock();

	    if (i == (int) header.max_pktsize)
	    {
		m_lPos++;
		if (p->init() == 0)
		    return p;
		AVM_WRITE("ASF file reader", "incorrect packet\n", (m_lPos - 1));
	    }
	    else
	    {
		// FIXME - missing check for unix signal and restart - maybe add later
		// usually happens with EOF - handle better...
		//AVM_WRITE("ASF network reader", "asfinput:: read fail %d\n", header.pktsize);
		m_bEof = true;
	    }
	    p->release();
	}
	return 0;
    }
    virtual int seek(framepos_t pos, chunk_info* pch)
    {
	if (pos >= m_SeekInfo->size())
	    return -1;

	const chunk_info& ch = (*m_SeekInfo)[pos];
	m_lPos = ch.packet_id;
	*pch = ch;
	m_bEof = false;
	//AVM_WRITE("ASF file reader", "Seekpos %lld  time %d   %d\n", m_lPos, ch.object_start_time/1000, pos);

	return 0;
    }
    virtual int seekTime(double timestamp, chunk_info* pch)
    {
	return seek(m_SeekInfo->find(uint_t(timestamp*1000.)), pch);
    }

protected:
    int64_t m_lPos;
    AsfFileInputStream* m_pParent;
    AsfStreamSeekInfo* m_SeekInfo;
};

/* first, we make a completely stupid implementation */
AsfFileInputStream::AsfFileInputStream()
    :m_iFd(-1), m_SeekInfo(128)
{
    for (unsigned l = 0; l < m_SeekInfo.size(); l++)
        m_SeekInfo[l] = 0;
}

int AsfFileInputStream::init(const char* pszFile)
{
    m_iFd = open(pszFile, O_RDONLY | O_LARGEFILE);
    if (m_iFd < 0)
    {
        AVM_WRITE("ASF reader", "Could not open the file\n");
	return -1;
    }
    char* header = 0;
    int r;
    m_lDataOffset = 0;

    for (;;)
    {
	GUID guid;
	int64_t size;
	if (read(m_iFd, &guid, 16) <= 0 || read(m_iFd, &size, 8) <= 0)
	    break;
        avm_get_leGUID(&guid);
	size = avm_get_le64(&size) - 24;
	if (size < 0)
	    break;
	char buf[100]; printf("GUID type %d    %s\n", (int) guid_get_guidid(&guid), guid_to_string(buf, &guid));
	AVM_WRITE("ASF reader", "Object: %s - object size: %5Ld\n", guidid_to_text(guid_get_guidid(&guid)), size);

	switch (guid_get_guidid(&guid))
	{
	case GUID_ASF_HEADER:
	    header = new char[size];
	    r = read(m_iFd, header, size);
	    if (!parseHeader(header, r))
	    {
		delete[] header;
                header = 0;
	    }
	    break;
	case GUID_ASF_DATA:
	    if (size<26 || !header)
	    {
		AVM_WRITE("ASF reader", "Wrong data chunk size\n");
                return -1;
	    }
	    m_lDataOffset = lseek64(m_iFd, 0, SEEK_CUR) + 26;
	    lseek64(m_iFd, size, SEEK_CUR);
	    break;
	case GUID_ERROR: //unknown chunk type
	default:
	    if (!header) // header must be the first chunk
	    {
		AVM_WRITE("ASF reader", "Not ASF stream\n");
		return -1;
	    }
	    lseek64(m_iFd, size, SEEK_CUR);
	    break;
	}
    }
    if (!header)
    {
	AVM_WRITE("ASF reader", "Could not find ASF header chunk in file\n");
	return -1;
    }
    if (!m_lDataOffset)
    {
	AVM_WRITE("ASF reader", "Could not find data chunk in file\n");
	return -1;
    }
    delete[] header;
    createSeekData();
    return 0;
}

#undef __MODULE__

AsfFileInputStream::~AsfFileInputStream()
{
    if (m_iFd >= 0)
	close(m_iFd);
    for (unsigned i = 0; i < m_SeekInfo.size(); i++)
	delete m_SeekInfo[i];
}

void AsfFileInputStream::createSeekData()
{
    uint_t last_start[m_SeekInfo.size()];
    for (unsigned l = 0; l < m_SeekInfo.size(); l++)
	last_start[l] = 0;
    for (unsigned l = 0; l < m_Streams.size(); l++)
	m_SeekInfo[m_Streams[l].hdr.stream & 0x7f]
	    = new AsfStreamSeekInfo();

    AVM_WRITE("ASF reader", "Creating seek data, please wait...\n");
    AsfIterator* it = getIterator(~0U);//argument only needed in seek operations
    asf_packet* p = 0;
    unsigned i;

    for (i = 0; /*i < m_Header.pkts_count*/; i++)
    {
	if (p)
            p->release();
	p = it->getPacket();
	if (!p)
	    break;
	for (unsigned j = 0; j < p->fragments.size(); j++)
	{
	    const asf_packet_fragment& f = p->fragments[j];
	    chunk_info ch;
	    if (f.object_length != f.data_length && f.fragment_offset)
		continue;

	    if (!m_SeekInfo[f.stream_id])
	    {
		AVM_WRITE("ASF reader", "WARNING: Unexpected stream_id ( packet %d, send time %f"
			  ", fragment: %d, stream_id: %d)\n",
			  i, p->send_time/1000., j, f.stream_id);
		continue;
	    }

	    ch.object_start_time = f.object_start_time - m_Header.preroll;
	    ch.object_length = f.object_length;
	    //AVM_WRITE("ASF reader", "id:%d  %d starttm: %f   len: %d  idx: %d  kf:%d\n", f.stream_id, m_SeekInfo[f.stream_id]->size(), ch.object_start_time/1000.0, ch.object_length, j, f.keyframe);
	    if (last_start[f.stream_id] > 0)
	    {
		if (ch.object_start_time < last_start[f.stream_id])
		{
		    AVM_WRITE("ASF reader", "WARNING: Negative send time difference ( packet %d"
			      ", packet send time: %f, fragment: %d, stream_id: %d "
			      ", fragment send time: %f, last fragment send time: %f)\n",
			      i, p->send_time/1000., j, (int)f.stream_id,
			      ch.object_start_time/1000.,
			      m_SeekInfo[f.stream_id]->back().object_start_time/1000.);
		    //continue;
                    ch.object_start_time = last_start[f.stream_id];
		}
	    }
            // note - keyframe flag must be set after object_length!
	    ch.SetKeyFrame(f.keyframe);
	    ch.packet_id = i;
	    ch.fragment_id = j;
	    //printf("packet  %d  %d  %d\n", i, j, f.fragment_offset);
	    last_start[f.stream_id] = ch.object_start_time;
	    m_SeekInfo[f.stream_id]->push_back(ch);
	}
    }
    if (p)
        p->release();
#if 0
    AsfStreamSeekInfo* si = m_SeekInfo[1];
    for (unsigned i = 0; i < si->size() && i < 200; i++)
    {
	chunk_info& c = si->operator[](i);

	cout << i << "   " << c.packet_id << "   " << c.object_start_time << "  "
	    << (int)c.keyframe << endl;
    }
#endif

    m_Header.pkts_count = i;
    AVM_WRITE("ASF reader", "Seek data created ( processed %d packets )\n", i);
    it->release();
}

AsfIterator* AsfFileInputStream::getIterator(uint_t id)
{
    if (id >= m_Streams.size())
	return (id == ~0U) ? new FileIterator(this, 0) : 0;

    int sid = m_Streams[id].hdr.stream & 0x7f;
    return (m_SeekInfo[sid] && m_SeekInfo[sid]->size()) ?
	new FileIterator(this, sid) : 0;
}

AVM_END_NAMESPACE;
