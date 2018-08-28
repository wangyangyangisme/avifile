#include "FFReadStream.h"
#include "FFReadHandler.h"
#include "avm_fourcc.h"
#include "avm_output.h"

#ifndef int64_t_C
#define int64_t_C(c)     (c ## LL)
#define uint64_t_C(c)    (c ## ULL)
#endif
#include "avformat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

static const struct id2fcc {
    enum CodecID id;
    uint32_t fcc;
} id2fcct[] = {
    { CODEC_ID_MPEG1VIDEO, RIFFINFO_MPG1 },
    { CODEC_ID_H263, mmioFOURCC('H', '2', '6', '3') },
    { CODEC_ID_H263P, mmioFOURCC('H', '2', '6', '3') },
    { CODEC_ID_MP2, 0x50 },
    { CODEC_ID_MP3, 0x55 },
    { CODEC_ID_AC3, 0x2000 },
    { CODEC_ID_DVVIDEO, mmioFOURCC('D', 'V', 'S', 'D') },
    { CODEC_ID_DVAUDIO, ('D' << 8) | 'A' },
    { CODEC_ID_NONE }
};

static int get_fcc(enum CodecID id)
{
    for (const struct id2fcc* p = id2fcct; p->id; p++)
	if (p->id == id)
	    return p->fcc;
    return 0;
}


FFReadStream::FFReadStream(FFReadHandler* handler, uint_t sid, AVStream* avs)
    : m_pHandler(handler), m_pAvStream(avs), m_pAvContext(0), m_uiSId(sid),
    m_Packets(1000), m_uiPosition(0), m_dTimestamp(0.),
    m_uiKeyChunks(0), m_uiKeySize(0), m_uiKeyMaxSize(0), m_uiKeyMinSize(0),
    m_uiDeltaSize(0), m_uiDeltaMaxSize(0), m_uiDeltaMinSize(0)
{
    AVM_WRITE("FF stream", "Starttime: %lld  Duration: %lld\n",
	      m_pHandler->m_pContext->start_time, m_pHandler->m_pContext->duration);
    m_dLength = m_pHandler->m_pContext->duration / (double) AV_TIME_BASE;
    //printf("CODECRA %d  %d   %d\n", avs->codec.frame_rate, avs->codec->frame_rate_base, m_pAvStream->r_frame_rate_base);
    if (0 && avs->codec->codec_id == CODEC_ID_MPEG1VIDEO)
    {
	m_pAvContext = avcodec_alloc_context();
	//AVCodec* codec = avcodec_find_encoder(avs->codec->codec_id);
	if (m_pAvContext)
	{
	    AVCodec* codec = avcodec_find_decoder(avs->codec->codec_id);
	    if (codec && avcodec_open(m_pAvContext, codec) == 0)
	    {
		m_pAvContext->flags |= CODEC_FLAG_TRUNCATED;
		m_pAvContext->hurry_up = 5;
		//printf("Opened hurryup decoder %p  %p\n", codec, m_pAvContext->codec->decode);
	    }
	    else
	    {
		avcodec_close(m_pAvContext);
		m_pAvContext = 0;
	    }
	}
    }
}

FFReadStream::~FFReadStream()
{
    if (m_pAvContext)
    {
	avcodec_close(m_pAvContext);
	free(m_pAvContext);
    }
}

double FFReadStream::CacheSize() const
{
    return 1.0;
}

void FFReadStream::ClearCache()
{
}

uint_t FFReadStream::GetHeader(void* pheader, uint_t size) const
{
    return 0;
}

framepos_t FFReadStream::GetPrevKeyFrame(framepos_t lFrame) const
{
    return 0;
}

framepos_t FFReadStream::GetNextKeyFrame(framepos_t lFrame) const
{
    return ERR;
}

framepos_t FFReadStream::GetNearestKeyFrame(framepos_t lFrame) const
{
    return ERR;
}

framepos_t FFReadStream::GetLength() const
{
    return 0;
}

double FFReadStream::GetFrameTime() const
{
    return 1/15.;
}

double FFReadStream::GetLengthTime() const
{
    return m_dLength;
}

StreamInfo* FFReadStream::GetStreamInfo() const
{
    AVStream* avs = m_pHandler->m_pContext->streams[m_uiSId];
    uint_t s;
    if (m_StreamInfo.m_p->m_dLengthTime == 0.0)
    {
	m_StreamInfo.m_p->setKfFrames(m_uiKeyMaxSize, m_uiKeyMinSize,
				      m_uiKeyChunks, m_uiKeySize);

	m_StreamInfo.m_p->setFrames(m_uiDeltaMaxSize,
				    (m_uiDeltaMinSize > m_uiDeltaMaxSize)
                                    // usually no delta frames
				    ? m_uiDeltaMaxSize : m_uiDeltaMinSize,
				    m_Offsets.size() - m_uiKeyChunks,
				    m_uiStreamSize - m_uiKeySize);

	m_StreamInfo.m_p->m_dLengthTime = GetLengthTime();
	m_StreamInfo.m_p->m_iQuality = 0;
	m_StreamInfo.m_p->m_iSampleSize = 1;//m_Header.dwSampleSize;

	switch (avs->codec->codec_type)
	{
	case CODEC_TYPE_AUDIO:
	    m_StreamInfo.m_p->setAudio(avs->codec->channels,
				       avs->codec->sample_rate,
				       avs->codec->frame_bits);
	    m_StreamInfo.m_p->m_Type = StreamInfo::Audio;
	    m_StreamInfo.m_p->m_uiFormat = avs->codec->codec_tag;
	    AVM_WRITE("FF stream", "Audio Format:  %.4s (0x%x)\n",
		      (const char*)&avs->codec->codec_tag, avs->codec->codec_tag);
	    break;
	case CODEC_TYPE_VIDEO:
	    m_StreamInfo.m_p->setVideo(avs->codec->width, avs->codec->height,
				       0, avs->codec->sample_aspect_ratio.num /
				       (float) avs->codec->sample_aspect_ratio.den);
	    m_StreamInfo.m_p->m_Type = StreamInfo::Video;
	    m_StreamInfo.m_p->m_uiFormat = avs->codec->codec_tag;
	    break;
	default:
            return 0;
	}
	if (m_StreamInfo.m_p->m_uiFormat == 0)
            m_StreamInfo.m_p->m_uiFormat = get_fcc(avs->codec->codec_id);
    }

    return new StreamInfo(m_StreamInfo);
}

double FFReadStream::GetTime(framepos_t lSample) const
{
    if (lSample == ERR)
	return m_dTimestamp;

    return 0.;
}

uint_t FFReadStream::GetSampleSize() const
{
    return 0;
}

IStream::StreamType FFReadStream::GetType() const
{
    switch (m_pHandler->m_pContext->streams[m_uiSId]->codec->codec_type)
    {
    case CODEC_TYPE_AUDIO: return IStream::Audio;
    case CODEC_TYPE_VIDEO: return IStream::Video;
    default: return IStream::Other;
    }
}

uint_t FFReadStream::GetFormat(void *pFormat, uint_t lSize) const
{
    AVStream* avs = m_pHandler->m_pContext->streams[m_uiSId];
    switch (avs->codec->codec_type)
    {
    case CODEC_TYPE_AUDIO:
	if (pFormat && lSize >= sizeof(WAVEFORMATEX))
	{
	    WAVEFORMATEX* wf = (WAVEFORMATEX*) pFormat;
	    wf->wFormatTag = avs->codec->codec_tag;
	    if (wf->wFormatTag == 0)
                wf->wFormatTag = get_fcc(avs->codec->codec_id);
	    //if (avs->codec->codec_tag == 0) wf->wFormatTag = av_codec_get_fourcc(avs->codec->codec_id);
	    //printf("CODEC  %x   %x   %x\n", wf->wFormatTag, avs->codec->codec_id, avs->codec->codec_tag);
	    wf->nChannels = avs->codec->channels;
	    wf->nSamplesPerSec = avs->codec->sample_rate;
	    wf->nAvgBytesPerSec = avs->codec->bit_rate / 8;
            wf->nBlockAlign = avs->codec->block_align;
	    wf->wBitsPerSample = avs->codec->bits_per_sample;
	    if (lSize >= (sizeof(WAVEFORMATEX) + avs->codec->extradata_size)
		&& avs->codec->extradata)
	    {
		wf->cbSize = avs->codec->extradata_size;
		memcpy(wf + 1, avs->codec->extradata, avs->codec->extradata_size);
	    }
	    else
                wf->cbSize = 0;
	}
	//printf("EEEEEEEEEEE %d\n", avs->codec->extradata_size);
	return sizeof(WAVEFORMATEX)
	    + ((avs->codec->extradata) ? avs->codec->extradata_size : 0);
    case CODEC_TYPE_VIDEO:
	if (pFormat && lSize >= sizeof(BITMAPINFOHEADER))
	{
	    BITMAPINFOHEADER* bh = (BITMAPINFOHEADER*) pFormat;
	    //printf("FORMAT %p  %d   %d\n", pFormat, m_uiSId, avc->width);
	    memset(bh, 0, sizeof(BITMAPINFOHEADER));
	    bh->biSize = sizeof(BITMAPINFOHEADER);
	    bh->biWidth = avs->codec->width;
	    bh->biHeight = avs->codec->height;
	    bh->biPlanes = 1;
	    bh->biCompression = avs->codec->codec_tag;
            bh->biBitCount = avs->codec->bits_per_sample;
            // hack which might be eventually usefull
	    memcpy(&bh->biXPelsPerMeter, &m_pHandler->m_pContext, sizeof(void*));
	    if (bh->biCompression == 0)
		bh->biCompression = get_fcc(avs->codec->codec_id);
	    if (lSize >= (sizeof(BITMAPINFOHEADER) + avs->codec->extradata_size)
		&& avs->codec->extradata)
	    {
		bh->biSize += avs->codec->extradata_size;
		memcpy(bh + 1, avs->codec->extradata, avs->codec->extradata_size);
		//printf("COPY EXTRA %d\n", avs->extradata_size);
		//for (unsigned i = 0; i < lSize; i++) printf("%d  0x%x\n", i, ((uint8_t*)pFormat)[i]);
	    }
	    //BitmapInfo(*bh).Print();
	}
	return sizeof(BITMAPINFOHEADER)
	    + ((avs->codec->extradata) ? avs->codec->extradata_size : 0);
    default:
	return 0;
    }
}

bool FFReadStream::IsKeyFrame(framepos_t lFrame) const
{
    AVM_WRITE("FF stream", "IsKeyFrame unimplemented!\n");
    return true;
}

StreamPacket* FFReadStream::ReadPacket()
{
    for (unsigned i = 0; i < m_Packets.capacity() - 2; i++)
    {
	if (m_Packets.size() > 0)
	    break;

	if (m_pHandler->readPacket() < 0)
	{
	    if (m_dLength < m_dTimestamp)
		m_dLength = m_dTimestamp;
	    return 0;
	}
    }
    //printf("PACKET SIZE   %d   %d\n", m_Packets.size(), m_Packets.capacity());
    if (!m_Packets.size())
	return 0;

    Locker locker(m_pHandler->m_Mutex);
    StreamPacket* p = m_Packets.front();
    m_Packets.pop();
    m_dTimestamp = p->timestamp / 1000000.0;

    if (m_dLength < m_dTimestamp)
	m_dLength = m_dTimestamp;

    //printf("READSTREAM  %f   sid:%d  qsz:%d  s:%d   frm: %d\n", m_dTimestamp, m_uiSId, m_Packets.size(), p->size, m_pAvStream->codec->frame_number);
    return p;
}

int FFReadStream::Seek(framepos_t pos)
{
    if (pos < 1)
    {
        printf("SEEKPOS %d\n", pos);
	m_pHandler->seek(0);
        return 0;
    }
    return -1;
}

int FFReadStream::SeekTime(double time)
{
    if (time < 1.)
    {
	if (m_pAvStream->codec->codec_type == CODEC_TYPE_AUDIO)
            // check if more streams are available
            // and seek only with the video
            return 0;
	printf("FFSEEKTIME %f\n", time);
	m_pHandler->seek(0);
	return 0;
    }
    return -1;
}

int FFReadStream::SkipFrame()
{
    printf("SKIP FRAME\n");
    StreamPacket* p = ReadPacket();
    if (p)
	p->Release();
    return 0;
}

int FFReadStream::SkipTo(double pos)
{
    printf("SKIP TO: %f\n", pos);
    while (pos < GetTime())
	SkipFrame();
    return 0;
}

AVM_END_NAMESPACE;
