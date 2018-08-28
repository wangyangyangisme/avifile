#include "AsfReadStream.h"
#include "AsfReadHandler.h"
#include "asf_guids.h"
#include "avm_output.h"

#include <string.h>
#include <stdlib.h> //  abort
#include <stdio.h>

#include <unistd.h>
#define Debug if (0)

AVM_BEGIN_NAMESPACE;

AsfReadStream::AsfReadStream(AsfReadHandler* parent)
    :m_pSeekInfo(0), m_pScrambleDef(0), m_pParent(parent), m_pIterator(0),
    m_pStrPacket(0), m_pAsfPacket(0), m_uiFragId(0),
    m_uiLastPos(0), m_dLastTime(-1.),
    m_iMaxBitrate(-1), m_bIsScrambled(false)
{
}

AsfReadStream::~AsfReadStream()
{
    if (m_pAsfPacket)
	m_pAsfPacket->release();
    if (m_pIterator)
	m_pIterator->release();

    if (m_pStrPacket)
	m_pStrPacket->Release();
}

double AsfReadStream::CacheSize() const
{
    return m_pParent->m_pInput->cacheSize();
}

void AsfReadStream::ClearCache()
{
    //return m_pParent->m_pInput->clear();
}

uint_t AsfReadStream::GetFormat(void *pFormat, uint_t lSize) const
{
    uint_t size;
    const void* fmt;
    if (m_bIsAudio)
    {
	size = m_Header.hdr.stream_size;
	fmt = &m_Header.hdr.aud.wfex;
    }
    else
    {
	size = m_Header.hdr.stream_size - 11;
	fmt = &m_Header.hdr.vid.bh;
    }

    if (pFormat)
	memcpy(pFormat, fmt, (size > lSize) ? lSize : size);
    return size;
}

IStream::StreamType AsfReadStream::GetType() const
{
    switch (guid_get_guidid(&m_Header.hdr.stream_guid))
    {
    case GUID_ASF_AUDIO_MEDIA:
	return IStream::Audio;
    case GUID_ASF_VIDEO_MEDIA:
	return IStream::Video;
    default:
	return IStream::Other;
    }
}

double AsfReadStream::GetFrameTime() const
{
    if (!m_pSeekInfo || !m_pSeekInfo->size())
	return 1.0/15.0;

    return GetLengthTime()/(double)m_pSeekInfo->size();
}

uint_t AsfReadStream::GetHeader(void* pheader, uint_t size) const
{
    if (pheader && size >= sizeof(AVIStreamHeader))
    {
	memset(pheader, 0, size);

	AVIStreamHeader* p = (AVIStreamHeader*) pheader;

	if (!m_pSeekInfo)
	{
	    p->dwRate = 15;
	    p->dwScale = 1;
	    p->dwLength = 0x7FFFFFFF;
	}
	else
	{
	    p->dwRate = 1000000;
	    if (!m_pSeekInfo->size())
		p->dwScale = 1000000/15;
	    else
	    {
		double stream_length = m_pParent->m_Header.play_time/10000000.;
		p->dwScale = int(1000000*stream_length/m_pSeekInfo->size());
	    }
	    p->dwLength = m_pSeekInfo->size();
	}

	if (m_bIsAudio)
	{
	    p->fccType = streamtypeAUDIO;
	    p->fccHandler = m_Header.hdr.aud.wfex.wFormatTag;
	    p->dwSampleSize = m_Header.hdr.aud.wfex.nBlockAlign;
	}
	else if (guid_is_guidid(&m_Header.hdr.stream_guid, GUID_ASF_VIDEO_MEDIA))
	{
	    p->fccType = streamtypeVIDEO;
	    p->fccHandler = m_Header.hdr.vid.bh.biCompression;
	    p->rcFrame.right = m_Header.hdr.vid.bh.biWidth;
	    p->rcFrame.top = m_Header.hdr.vid.bh.biHeight;
	}
    }
    return sizeof(AVIStreamHeader);
}

uint_t AsfReadStream::GetLength() const
{
    // FIXME - show better value here
    return (m_pSeekInfo) ? m_pSeekInfo->size() : 0x7fffffff;
}

double AsfReadStream::GetLengthTime() const
{
    if (m_pSeekInfo && m_pSeekInfo->size() > 0)
    {
	return ((*m_pSeekInfo)[m_pSeekInfo->size() - 1].object_start_time) / 1000.0;
    }

    double t = (m_pParent->m_Header.play_time - m_pParent->m_Header.preroll) / 10000000.0;
    if (t == 0)
	t = 0x7fffffff; // default large value
    return t;
}

framepos_t AsfReadStream::GetNearestKeyFrame(framepos_t lFrame) const
{
    if (!m_pSeekInfo)
	return ERR;

    framepos_t info_pos;

    if (lFrame != ERR)
    {
	if (lFrame >= m_pSeekInfo->size())
	    return 0;
	info_pos = lFrame;
    }
    else
	info_pos = m_uiLastPos;

    return m_pSeekInfo->nearestKeyFrame(info_pos);
}

framepos_t AsfReadStream::GetNextKeyFrame(framepos_t lFrame) const
{
    if (!m_pSeekInfo)
	return ERR;// GetPos();

    framepos_t info_pos;

    if (lFrame != ERR)
    {
	if (lFrame >= m_pSeekInfo->size())
	    return 0;
	info_pos = lFrame;
    }
    else
	info_pos = m_uiLastPos;

    return m_pSeekInfo->nextKeyFrame(info_pos + 1);
}

framepos_t AsfReadStream::GetPrevKeyFrame(framepos_t lFrame) const
{
    if (!m_pSeekInfo)
	return 0;

    framepos_t info_pos;
    if (lFrame != ERR)
    {
	if (lFrame >= m_pSeekInfo->size() || lFrame == 0)
	    return 0;
	info_pos = lFrame - 1;
    }
    else
	info_pos = m_uiLastPos;
    return m_pSeekInfo->prevKeyFrame(info_pos);
}

double AsfReadStream::GetTime(framepos_t pos) const
{
    //printf("GET TIME %d  %f   %d\n", pos, m_dLastTime,  m_iId);
    if (pos == ERR)
    {
	if (!m_pStrPacket)
	    ReadPacketInternal();

        return m_dLastTime;
    }

    if (m_pSeekInfo && pos < m_pSeekInfo->size())
	return (*m_pSeekInfo)[pos].object_start_time / 1000.0;

    //AVM_WRITE("ASF reader", "lSample  %d  %f  (%f)\n", pos, t, m_uiLastTimestamp / 1000.0);
    return -1.;
}

uint_t AsfReadStream::GetSampleSize() const
{
    return (m_bIsAudio) ? m_Header.hdr.aud.wfex.nBlockAlign : 0;
}

StreamInfo* AsfReadStream::GetStreamInfo() const
{
    if (!m_pSeekInfo)
    {
	AVM_WRITE("ASF reader", "GetStreamInfo() no seek info\n");
    }
    if (m_StreamInfo.m_p->m_dLengthTime == 0.0)
    {
	uint_t kfmax = 0;
	uint_t kfmin = ~0U;
	uint_t kfchunks = 0;
	int64_t kfsize = 0;
	uint_t fmax = 0;
	uint_t fmin = ~0U;
	uint_t chunks = 0;
	int64_t size = 0;

	if (m_pSeekInfo)
	    for (unsigned i = 0; i < m_pSeekInfo->size(); i++)
	    {
		uint_t l = (*m_pSeekInfo)[i].GetChunkLength();
		if ((*m_pSeekInfo)[i].IsKeyFrame() || m_bIsAudio)
		{
		    kfmax = (kfmax > l) ? kfmax : l;
		    kfmin = (kfmin < l) ? kfmin : l;
		    kfsize += l;
		    kfchunks++;
		}
		else
		{
		    fmax = (fmax > l) ? fmax : l;
		    fmin = (fmin < l) ? fmin : l;
		    size += l;
		    chunks++;
		}
	    }
	m_StreamInfo.m_p->setKfFrames(kfmax, kfmin, kfchunks, kfsize);
	m_StreamInfo.m_p->setFrames(fmax, fmin, chunks, size);

        double ft = 0;
	if (m_pSeekInfo && m_pSeekInfo->size())
	    ft = (*m_pSeekInfo)[0].object_start_time / 1000.0;
	m_StreamInfo.m_p->m_dLengthTime = GetLengthTime() - ft;
	m_StreamInfo.m_p->m_iQuality = 0;

	if (m_bIsAudio)
	{
	    const WAVEFORMATEX* w = (const WAVEFORMATEX*) &m_Header.hdr.aud.wfex;

	    m_StreamInfo.m_p->setAudio(w->nChannels, w->nSamplesPerSec,
				       w->wBitsPerSample, w->nAvgBytesPerSec);
	    m_StreamInfo.m_p->m_Type = StreamInfo::Audio;
	    m_StreamInfo.m_p->m_uiFormat = w->wFormatTag;
	    m_StreamInfo.m_p->m_iSampleSize = 1;
	}
	else
	{
	    const BITMAPINFOHEADER* h = (const BITMAPINFOHEADER*) &m_Header.hdr.vid.bh;
	    m_StreamInfo.m_p->setVideo(h->biWidth, h->biHeight, m_iMaxBitrate/8);
	    m_StreamInfo.m_p->m_Type = StreamInfo::Video;
	    m_StreamInfo.m_p->m_uiFormat = h->biCompression;
	    m_StreamInfo.m_p->m_iSampleSize = 0;
	}
    }

    return new StreamInfo(m_StreamInfo);
}

bool AsfReadStream::IsKeyFrame(framepos_t lFrame) const
{
    if (!m_pSeekInfo || m_bIsAudio)
	return true;

    framepos_t info_pos;

    if (lFrame != ERR)
    {
	if (lFrame >= m_pSeekInfo->size())
	    return true;
	info_pos = lFrame;
    }
    else
	info_pos = m_uiLastPos;

    //printf("ISKEY %d  %d\n", info_pos, (*m_pSeekInfo)[info_pos].IsKeyFrame());
    return (info_pos != ERR) ? (*m_pSeekInfo)[info_pos].IsKeyFrame() : true;
}

void AsfReadStream::ReadPacketInternal() const
{
    int m_iRealId = m_iId;
    StreamPacket* spkt = 0;
    uint_t seq_num = ~0U;
    int newpos = -1;
    // loop until packet is completed
    for (;;)
    {
	//printf("READPACKET %p %d  id:%d\n", m_pAsfPacket, m_uiFragId, m_iId);
	if (!m_pAsfPacket || m_uiFragId >= m_pAsfPacket->fragments.size())
	{
            newpos = -1;
	    if (GetNextAsfPacket() == 0)
	    {
                if (spkt)
		    spkt->Release();
                spkt = 0;
		break;
	    }
	}

        // go through all fragments in this packet
	const asf_packet_fragment& f = m_pAsfPacket->fragments[m_uiFragId++];

	Debug AVM_WRITE("ASF reader", 4, "Pos id:%d == %d  (rid: %d)  spos: %d off: %d osize: %d  dsize: %d seq: %d  fsize: %d\n",
			m_iId, f.stream_id, m_iRealId, m_uiFragId, f.fragment_offset, f.object_length, f.data_length, f.seq_num, m_pAsfPacket->fragments.size());

	if (f.stream_id != m_iRealId)
	{
	    if (!m_iRealId && !f.fragment_offset && f.stream_id > 1)
	    {
		m_iRealId = f.stream_id;
	    }
	    else
		continue;
	}
	//printf("S %d\n", f.stream_id);
	if (spkt && seq_num != f.seq_num)
	{
	    AVM_WRITE("ASF reader", "WARNING: fragment with different"
		      " sequence number ( expected %d, found %d ), "
		      "packet timestamp %f  ignoring ???\n", seq_num,
		      f.seq_num, m_pAsfPacket->send_time / 1000.0);
	    // we could check if the current packet doesn't have any
            // other fragments which would fit our needs
	    unsigned np = 0;
	    for (unsigned i = m_uiFragId + 1; i < m_pAsfPacket->fragments.size(); i++)
	    {
		const asf_packet_fragment& fn = m_pAsfPacket->fragments[i];
		// ok skip and check if the next fragment would fit
		//printf("FRAGS  %d: %d\n", i, fn.seq_num);
		if (fn.seq_num == seq_num && f.stream_id == m_iRealId)
		{
		    np = i;
		    break;
		}
	    }
	    if (np)
	    {
                newpos = m_uiFragId - 1;
		m_uiFragId = np;
	    }
	    else
	    {
                if (f.fragment_offset == 0)
		    m_uiFragId--;
                if (spkt)
		    spkt->Release();
		spkt = 0;
	    }
	}

	if (!spkt)
	{
	    //printf("OBJLEN %d  t:%d  o:%d\n", f.object_length, f.object_start_time, f.fragment_offset);
	    seq_num = f.seq_num;
	    if (f.object_length > 2000000) // too large - some bug
		continue;
	    spkt = new StreamPacket(f.object_length);
	    spkt->flags = f.keyframe ? 0x10 : 0;
	    //printf("Duration %d\n", m_pAsfPacket->duration);
	    uint_t tpos = f.object_start_time - m_pParent->m_Header.preroll;
	    spkt->position = m_uiLastPos = (m_pSeekInfo) ? m_pSeekInfo->find(tpos) : 0;
	    spkt->timestamp = (int64_t) tpos * 1000LL;
            m_dLastTime = spkt->timestamp / 1000000.0;
	    //printf("CREATED pkt: %d   0x%x   tm:%lld  %d  id:%d\n", spkt->size, spkt->flags, spkt->timestamp, spkt->position, m_iId);
	}

	if ((spkt->read + f.data_length) > spkt->size)
	{
	    // this fragment won't fit into the buffer
	    AVM_WRITE("ASF reader", "WARNING: fragment too big "
		      "( read bytes %d, fragment data length %d ), "
		      "position %d, offset: %d, packet timestamp %f skipping",
		      spkt->read, f.data_length, m_uiFragId,
		      f.fragment_offset, m_pAsfPacket->send_time / 1000.0);
            if (spkt)
		spkt->Release();
	    spkt = 0;
	    continue;
	}

	if (spkt->read)
	{
	    if (f.fragment_offset != spkt->read)
	    {
		AVM_WRITE("ASF reader", "WARNING: fragment with unexpected offset while"
			  " reassembling ( expected %d, found %d ), "
			  "packet timestamp %f skipping\n", spkt->read,
			  f.fragment_offset, m_pAsfPacket->send_time / 1000.0);
		if (spkt)
		    spkt->Release();
		spkt = 0;
		continue;
	    }
	}
        else if (f.fragment_offset) // !spkt->read
	{
	    AVM_WRITE("ASF reader", "WARNING: incomplete fragment found ( offset %d, length %d )"
		      ", packet timestamp %f skipping\n", f.fragment_offset,
		      f.object_length, m_pAsfPacket->send_time / 1000.0);
	    if (spkt)
		spkt->Release();
	    spkt = 0;
	    continue;
	}

	memcpy(spkt->memory + spkt->read, f.pointer, f.data_length);
	spkt->read += f.data_length;

	if (spkt->read == f.object_length)
	{
	    spkt->read = 0; // OK
	    break;
	}
    }
    if (newpos >= 0)
	m_uiFragId = newpos;

    //if (m_iId>0) {int sum = 0; for (int i = 0; i < spkt->size; i++) sum += spkt->memory[i]; printf("PK %d  %d\n", spkt->size, sum);}
    m_pStrPacket = spkt;
}

// buffer one frame ahead
// after seek we have to throw out the old 'cached' packet
// and prepare new one - this is made via SkipFrame call
StreamPacket* AsfReadStream::ReadPacket()
{
    if (!m_pStrPacket)
	ReadPacketInternal();
    //printf("_____READPAKET %p  id:%d\n", m_pStrPacket, m_iId);
    StreamPacket* tmp = m_pStrPacket;
    if (tmp)
    {
	if (m_bIsScrambled)
	{
	    // descramble packet
	    uint_t offset = 0;
	    char* oldmem = m_pStrPacket->memory;
	    StreamPacket* npkt = new StreamPacket(m_pStrPacket->size);
	    m_pStrPacket->memory = npkt->memory;
	    npkt->memory = oldmem;
	    while (offset < m_pStrPacket->size)
	    {
		int off = offset / m_pScrambleDef->chunk_length;
		int row = off / m_pScrambleDef->span;
		int col = off % m_pScrambleDef->span;
		int idx = row + col * m_pScrambleDef->packet_length / m_pScrambleDef->chunk_length;
		//printf("off:%d  samp_count: %d  samples: %d\n", m_iScrOffset, samp_count, lSamples);
		//printf("%d bytes in buffer,  block align: %d,   lasttime: %d\n", m_pcScrambleBuf.size()-m_iScrOffset, m_pScrambleDef->block_align_1, m_uiLastTimestamp);
		//printf("%d bytes in buffer,  block_size:  %d, offset: %d\n", m_pcScrambleBuf.size()-m_iScrOffset, m_pScrambleDef->chunk_size, m_iScrOffset);
		//printf("%d: writing block %dx%d=%d as %d\n", i, row, col, idx, index);
		memcpy(m_pStrPacket->memory + offset,
		       oldmem + idx * m_pScrambleDef->chunk_length,
		       m_pScrambleDef->chunk_length);
		offset += m_pScrambleDef->chunk_length;
	    }
            if (npkt)
		npkt->Release();
	}
	ReadPacketInternal();
    }

    return tmp;
}

int AsfReadStream::Seek(framepos_t pos)
{
    AVM_WRITE("ASF reader", 1, "AsfReadStream::Seek(%d)\n", pos);
    chunk_info ch;
    if (m_pIterator->seek(pos, &ch) != 0)
	return -1;

    GetNextAsfPacket();
    m_uiFragId = ch.fragment_id;
    if (m_pStrPacket)
	m_pStrPacket->Release();
    m_pStrPacket = 0;
    ReadPacketInternal();

    //printf("SEEK NEW POS  %d  %d   (%d f:%d)\n", pos, GetPos(), ch.packet_id, ch.fragment_id);
    return 0;
}

int AsfReadStream::SeekTime(double timepos)
{
    AVM_WRITE("ASF reader", 1, "AsfReadStream::SeekTime(%f)\n", timepos);
    chunk_info ch;
    if (timepos < 0. || m_pIterator->seekTime(timepos, &ch) != 0)
	return -1;

    GetNextAsfPacket();
    m_uiFragId = ch.fragment_id;
    if (m_pStrPacket)
	m_pStrPacket->Release();
    m_pStrPacket = 0;
    ReadPacketInternal();
    //printf("seektimepos %d   (tpos: %f)\n", m_uiFragId, timepos);
    //AVM_WRITE("ASF reader", "SEEKTIMEB %f   %f\n", m_uiLastTimestamp / 1000.0, ch.object_start_time / 1000.0);
    return 0;
}

int AsfReadStream::SkipFrame()
{
    AVM_WRITE("ASF reader", 1, "Skip frame\n");
    StreamPacket* p = ReadPacket();
    if (p)
        p->Release();
    //printf("SKIP POS  %p  %u\n", m_pAsfPacket, m_uiLastTimestamp / 1000);
    return 0;
}

int AsfReadStream::SkipTo(double pos)
{
    while (m_pStrPacket && m_pStrPacket->timestamp < pos)
	SkipFrame();
    return 0;
}

AVM_END_NAMESPACE;
