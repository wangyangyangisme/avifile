#include "AsfReadHandler.h"
#include "AsfReadStream.h"
#include "asf_guids.h"

#include "AsfFileInputStream.h"
#include "AsfRedirectInputStream.h"
#include "AsfNetworkInputStream.h"
#include "avm_output.h"

#include <time.h>
#include <string.h>
#include <stdio.h>

#define Debug if (0)

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "AsfReaderHandler"

AsfReadHandler::AsfReadHandler()
    : m_pInput(0)
{
}

int AsfReadHandler::init(const char* pszFile)
{
    bool isurl = strstr(pszFile, "://");
    if (!isurl)
    {
	AsfFileInputStream* af = new AsfFileInputStream();

	if (af->init(pszFile) == 0)
	    m_pInput = af;
	else
	    delete af;
    }
    if (!isurl && !m_pInput)
    {
	AsfRedirectInputStream* ar = new AsfRedirectInputStream();
	if (ar->init(pszFile) == 0)
            m_pInput = ar;
        else
	    delete ar;
    }
    if (!m_pInput)
    {
	AsfNetworkInputStream* ai = new AsfNetworkInputStream();
        if (ai->init(pszFile) == 0)
            m_pInput = ai;
	else
	    delete ai;
    }

    if (!m_pInput)
	return -1;

    IsValid();
    return 0;
}

AsfReadHandler::~AsfReadHandler()
{
    for (unsigned i = 0; i < m_SeekInfo.size(); i++)
	delete m_SeekInfo[i];

    for (unsigned i = 0; i < m_Streams.size(); i++)
	delete m_Streams[i];

    delete m_pInput;
}

uint_t AsfReadHandler::GetHeader(void* pheader, uint_t size)
{
    if (pheader && (size >= sizeof(AVIStreamHeader)))
    {
	memset(pheader, 0, size);

	AVIStreamHeader* p = (AVIStreamHeader*) pheader;

	if (!m_Streams.size())
	{
	    p->dwRate=15;
	    p->dwScale=1;
	    p->dwLength=0x7FFFFFFF;
	}
	else
	{
	    double stream_length = m_Header.play_time/10000000.;
	    p->dwRate=1000000;
	    if (!m_SeekInfo.size())
		p->dwScale=1000000/15;
	    else
		p->dwScale=uint_t(1000000*stream_length/m_SeekInfo.size());
	    p->dwLength=m_SeekInfo.size();
	}
        /*
	if (m_bIsAudio)
	{
	    p->fccType=mmioFOURCC('a', 'u', 'd', 's');
	    p->fccHandler=m_Header.hdr.aud.wfex.wFormatTag;
	    p->dwSampleSize=m_Header.hdr.aud.wfex.nBlockAlign;
	}
	else if (m_Header.hdr.uid_type == guid_video_stream)
	{
	    p->fccType=mmioFOURCC('v', 'i', 'd', 's');
	    p->fccHandler=m_Header.hdr.vid.biCompression;
	    p->rcFrame.right=m_Header.hdr.vid.biWidth;
	    p->rcFrame.top=m_Header.hdr.vid.biHeight;
	}
        */
    }
    return sizeof(AVIStreamHeader);
}

IMediaReadStream* AsfReadHandler::GetStream(uint_t stream_id,
					    IStream::StreamType type)
{
    guidid_t guidid;
    switch (type)
    {
    case IStream::Audio: guidid = GUID_ASF_AUDIO_MEDIA; break;
    case IStream::Video: guidid = GUID_ASF_VIDEO_MEDIA; break;
    default: return 0;
    }

    //
    // Selecting from the last available stream - as usualy this
    // stream contains the best quality video.
    // yet it's a little bit problematic
    // so some more advanced selection mechanism is necessary...
    // FIXME FIXME FIXME
    uint_t match = 0;
    for (
#if 1
	 int i = m_Streams.size() - 1; i >= 0; i--
#else
	 uint_t i = 0; i < m_Streams.size(); i++
#endif
    )
    {
	if (guid_is_guidid(&m_Streams[i]->m_Header.hdr.stream_guid, guidid))
	{
	    if (match == stream_id)
	    {
		if (!m_Streams[i]->m_pIterator)
		{
		    m_Streams[i]->m_pIterator = m_pInput->getIterator(i);
		    if (!m_Streams[i]->m_pIterator)
                        continue;
		    m_Streams[i]->m_pSeekInfo = m_Streams[i]->m_pIterator->getSeekInfo();
		    m_Streams[i]->m_iMaxBitrate = m_pInput->getMaxBitrate(m_Streams[i]->m_pIterator->getId());
		    //printf("STREAM %d  %d   BITRATE %d  %d\n", stream_id, i, m_Streams[i]->m_iMaxBitrate, m_Streams[i]->m_pIterator->getId());
		}
		return m_Streams[i];
	    }
	    match++;
	}
    }

    return 0;
}

uint_t AsfReadHandler::GetStreamCount(IStream::StreamType type)
{
    guidid_t guidid;

    switch (type)
    {
    case IStream::Audio: guidid = GUID_ASF_AUDIO_MEDIA;	break;
    case IStream::Video: guidid = GUID_ASF_VIDEO_MEDIA; break;
    default: guidid = GUID_ERROR;
    }

    uint_t cnt = 0;
    for (uint_t i = 0; i < m_Streams.size(); i++)
	if (guid_is_guidid(&m_Streams[i]->m_Header.hdr.stream_guid, guidid))
	    cnt++;

    return cnt;
}

bool AsfReadHandler::GetURLs(avm::vector<avm::string>& urls)
{
    return m_pInput->getURLs(urls);
}

void AsfReadHandler::Interrupt()
{
    return m_pInput->interrupt();
}

bool AsfReadHandler::IsOpened()
{
    return m_pInput->isOpened();
}

bool AsfReadHandler::IsRedirector()
{
    return m_pInput->isRedirector();
}

bool AsfReadHandler::IsValid()
{
    //printf("ISVALIDE  %d   %d  %d\n", IsOpened(), m_pInput->isValid(), IsRedirector());
    if (!IsOpened() || !m_pInput->isValid())
	return false;
    if (IsRedirector())
        return true;

    if (!m_Streams.size())
    {
	m_Header = m_pInput->getHeader();
	const avm::vector<ASFStreamHeader>& str = m_pInput->getStreams();
	for (unsigned i = 0; i < str.size(); i++)
	{
	    AsfReadStream* stream = new AsfReadStream(this);
	    m_Streams.push_back(stream);
	    stream->m_Header = str[i];
	    // stream id could be <1, 127>
	    stream->m_iId = stream->m_Header.hdr.stream & 0x7f;
	    stream->m_bIsAudio = guid_is_guidid(&stream->m_Header.hdr.stream_guid, GUID_ASF_AUDIO_MEDIA);
	    if (stream->m_bIsAudio
		&& guid_is_guidid(&stream->m_Header.hdr.error_guid, GUID_ASF_AUDIO_SPREAD)
		&& stream->m_Header.hdr.aud.wfex.cbSize < 200 //FIXME
	       )
	    {
                // read/calculate descrambling parameters
		char* ctmp = (char*) &stream->m_Header.hdr.aud.wfex
		    + sizeof(WAVEFORMATEX) + stream->m_Header.hdr.aud.wfex.cbSize;
		stream->m_pScrambleDef = (const AsfSpreadAudio*) ctmp;

		//printf("Scrambling check %d\n", stream.m_Header.hdr.aud.wfex.cbSize);
		AVM_WRITE("ASF reader", "Interleave info: blocksize=%d  packetlen=%d  chunklen=%d\n",
			  (int)stream->m_pScrambleDef->span,
			  stream->m_pScrambleDef->packet_length,
			  stream->m_pScrambleDef->chunk_length);
		if ((stream->m_pScrambleDef->span != 1)
		    && (stream->m_pScrambleDef->chunk_length > 0)
		    && (stream->m_pScrambleDef->packet_length/stream->m_pScrambleDef->chunk_length!=1))
		{
		    stream->m_bIsScrambled = true;
		    int msize = stream->m_pScrambleDef->packet_length * stream->m_pScrambleDef->span;
		    AVM_WRITE("ASF reader", "Scrambling scrsize: %d\n", msize);
		}
	    }
	    if (stream->m_Header.hdr.stream & 0x8000)
	    {
		AVM_WRITE("ASF reader", "The content of the stream: %d is ENCRYPTED (and for now unplayable!)\n", stream->m_iId);
	    }
	}
    }

    return true;
}


bool AsfInputStream::parseHeader(char* ptr, uint_t size, int level)
{
    ASFStreamHeader strhdr;
    bool bResult = false;
    uint32_t a = avm_get_le32(ptr);
    ptr += 4 + 2 /* reserved */;
    AVM_WRITE("ASF reader", 1, "header objects: %d  (%d)\n", a, size);
    for (unsigned i = 0; i <= ASF_STREAMS; i++)
	m_MaxBitrates[i] = ~0U;

    while (size >= 24)
    {
	int64_t ch_size;
	GUID* guid = (GUID*) ptr;
	avm_get_leGUID(guid);
	guidid_t gid = guid_get_guidid(guid);
	ptr += 16;
	ch_size = avm_get_le64(ptr);
	ptr += 8;

	if (ch_size > size) ch_size = size; // some error
	if (ch_size < 24) break;
	ch_size -= 24;
	size -= 24;

	AVM_WRITE("ASF reader", 1, "chunk_size %d, size: %d GUID: %s\n", (uint_t)ch_size, size, guidid_to_text(gid));
	switch (gid)
	{
	case GUID_ASF_FILE_PROPERTIES:
	    if (ch_size < sizeof(ASFMainHeader))
	    {
		AVM_WRITE("ASF reader", "Wrong ASF header size");
		return false;
	    }
	    if (ch_size > sizeof(ASFMainHeader))
		AVM_WRITE("ASF reader", "Warning: unexpected size of ASF header %d!\n", size);
	    m_Header = *(ASFMainHeader*)ptr;
            m_Streams.clear();
	    avm_get_leAsfMainHeader(&m_Header);
	    if (level == 0)
		AsfReadHandler::PrintASFMainHeader(&m_Header);
            bResult = true;
	    break;
	case GUID_ASF_STREAM_PROPERTIES:
	    memset(&strhdr, 0, sizeof(strhdr));
	    // FIXME
	    if (ch_size > sizeof(strhdr))
		AVM_WRITE("ASF reader", "FIXME: unexpected size of ASF stream header %d\n", size);
	    memcpy(&strhdr, ptr, (ch_size < sizeof(strhdr)) ? ch_size : sizeof(strhdr));
	    avm_get_leAsfStreamHeader(&strhdr);
	    if (level == 0)
                AsfReadHandler::PrintASFStreamHeader(&strhdr);
	    m_Streams.push_back(strhdr);
	    break;
	case GUID_ASF_CONTENT_DESCRIPTION:
	    {
		static const char* comment_txt[] =
		{
		    "Title       ", "Author      ", "Copyright   ",
		    "Description ", "Rating      "
		};
		uint_t ps = 5 * 2;
		for (uint_t i = 0; i < 5; i++)
		{
		    uint_t s = avm_get_le16(ptr + i * 2);
		    if (s > 0)
		    {
			char* cs = avm_convert_asfstring(ptr + ps, s);
                        m_Description.push_back(cs);
			AVM_WRITE("ASF reader", level, " %s: %s\n",
				  comment_txt[i], cs);
			ps += s;
		    }
		}
	    }
	    break;
	case GUID_ASF_EXTENDED_CONTENT_DESCRIPTION:
	    {
		uint_t off = 0;
		while (off < ch_size)
		{
		    uint_t sz = avm_get_le16(ptr + 2 + off);
		    off += 4;
		    if (off + sz < ch_size)
		    {
			AVM_WRITE("ASF reader", level, "VersionInfo: %s\n",
				  avm_convert_asfstring(ptr + off, sz));
		    }
		    off += sz;
		}
	    }
	    break;
	case GUID_ASF_CODEC_LIST:
	    {
		char* b = (char*) ptr + 16; // skip reserved GUID
		uint32_t ccount;
		ccount = avm_get_le32(b); b += 4;
		for (uint_t i = 0; i < ccount; i++)
		{
		    static const char* cstr[] = {
			"Codec Name", "Codec Description", "Information"
			// wide, wide, byte -
		    };
		    uint16_t ctype;
		    ctype = avm_get_le16(b); b += 2;
		    AVM_WRITE("ASF reader", level, "Codec Type: %s\n", (ctype == 1) ? "Video" : ((ctype == 2) ? "Audio" : "Unknown"));

		    for (uint_t j = 0; j < 3; j++)
		    {
			uint16_t csize;
			csize = avm_get_le16(b); b += 2;
			if (j < 2)
			{
			    csize *= 2; // WCHAR
			    avm_convert_asfstring(b, csize);
			    AVM_WRITE("ASF reader",  level, "%s: %s\n", cstr[j], b);
			}
			b += csize;
		    }
		}
	    }
	    break;
	case GUID_ERROR:
	    {
		char guidstr[64];
		AVM_WRITE("ASF reader", "Unknown guid \"%s\"!\n",
			  guid_to_string(guidstr, guid));
	    }
            break;
	case GUID_ASF_STREAM_BITRATE_PROPERTIES:
	    {
		const char* b = ptr;
		avm::string t;
		unsigned sc = avm_get_le16(b); b += 2;
		for (unsigned i = 0; i < sc; i++)
		{
                    char tl[50];
		    unsigned sid = avm_get_le16(b); b += 2;
		    unsigned mbit = avm_get_le32(b); b += 4;
		    m_MaxBitrates[sid & ASF_STREAMS] = mbit;
		    sprintf(tl, " %d-%d", sid, mbit);
                    t += tl;
		}
		AVM_WRITE("ASF reader", level, "Stream-MaxBitrate:%s\n", t.c_str());
	    }
            break;
	default:
            // some known but not parsed GUIDs at this moment
	    AVM_WRITE("ASF reader", level, "header contains \"%s\" (%db)\n",
		      guidid_to_text(gid), ch_size);
	    break;
	}
	size -= ch_size;
	ptr += ch_size;
    }

    return bResult;
}



/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * 86400ULL)

void AsfReadHandler::PrintASFMainHeader(const ASFMainHeader* h)
{
    char b[64];
    char sec1970str[128];
    struct tm result;

    time_t sec1970 = (int64_t) (h->create_time / 10000000) - SECS_1601_TO_1970;
#ifdef HAVE_LOCALTIME_R
    localtime_r(&sec1970, &result);
#else
    struct tm* r = localtime(&sec1970);
    memcpy(&result, r, sizeof(result));
#endif

#ifdef HAVE_ASCTIME_R
#ifdef HAVE_ASCTIME_R_LONG
    asctime_r(&result, sec1970str, sizeof(sec1970str) - 1);
#else
    asctime_r(&result, sec1970str);
#endif // HAVE_ASCTIME_R_LONG
#else  // HAVE_ASCTIME_R
    char* bb = asctime(&result);
    strcpy(sec1970str, bb);
#endif // HAVE_ASCTIME_R
    char* d = strchr(sec1970str, '\n'); if (d) *d = 0;
    AVM_WRITE("ASF reader", "MainHeader: %s\n"
	      " Created: %s   File size=%.0f   Packets=%.0f\n"
	      " Total time=%.1f sec   Play time=%.1f sec   Preroll=%.1f sec\n"
	      " Flags=0x%x  Packet size=%d  (=%d)  MaxBandwidth=%d bps\n",
	      guid_to_string(b, &h->guid),
	      sec1970str,
	      (double)h->file_size,
	      (double)h->pkts_count,
	      h->play_time / 10000000.0,
	      h->send_time / 10000000.0,
	      h->preroll / 1000.0, h->flags,
	      h->min_pktsize, h->max_pktsize,
	      h->max_bitrate);
}

void AsfReadHandler::PrintASFStreamHeader(const ASFStreamHeader* h)
{
    char b[64], c[64];

    AVM_WRITE("ASF reader", "StreamHeader: %s   Error correction: %s\n"
	      " Time offset=%.0f  Stream size=%d  Error size=%d  Stream=%d  Reserved=0x%x\n",
	      guidid_to_text(guid_get_guidid(&h->hdr.stream_guid)),
	      guidid_to_text(guid_get_guidid(&h->hdr.error_guid)),
              (double)h->hdr.time_offset,
	      h->hdr.stream_size,
	      h->hdr.error_size,
	      h->hdr.stream,
	      h->hdr.reserved);
}


IMediaReadHandler *CreateAsfReadHandler(const char *pszFile)
{
    AsfReadHandler* r = new AsfReadHandler();
    if (r->init(pszFile) == 0)
	return r;

    delete r;
    return 0;
}

#undef __MODULE__

AVM_END_NAMESPACE;
