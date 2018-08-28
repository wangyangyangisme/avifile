#include "FFReadHandler.h"
#include "FFReadStream.h"
#include "avm_output.h"

#ifndef int64_t_C
#define int64_t_C(c)     (c ## LL)
#define uint64_t_C(c)    (c ## ULL)
#endif
#include "avformat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

AVM_BEGIN_NAMESPACE;

static int g_iInitilized = 0;

FFReadHandler::FFReadHandler()
    : m_pContext(0)
{
    if (!g_iInitilized)
    {
	av_register_all();
        g_iInitilized++;
    }
}

FFReadHandler::~FFReadHandler()
{
    if (m_pContext)
    {
	flush();

	while (m_Streams.size() > 0)
	{
	    delete m_Streams.back();
            m_Streams.pop_back();
	}
        av_close_input_file(m_pContext);
    }
}

int FFReadHandler::Init(const char* url)
{
    AVFormatParameters avfp;
    AVInputFormat* fmt = 0;
    // av_find_input_format(url);
    // printf("find input format  %p   %s\n", fmt, url);
    memset(&avfp, 0, sizeof(avfp));
    //if (!fmt)  return -1;
    int r = av_open_input_file(&m_pContext, url,
			       fmt, 64000, &avfp);
    if (r < 0)
    {
	AVM_WRITE("FF reader", "OPEN INPUT failed\n");
	return -1;
    }

    if (av_find_stream_info(m_pContext) < 0)
	return -1;

    AVM_WRITE("FF reader", "Format  %s   streams:%d\n", m_pContext->iformat->long_name, m_pContext->nb_streams);

    m_Streams.resize(m_pContext->nb_streams);
    for (int i = 0; i < m_pContext->nb_streams; i++)
    {
	AVCodecContext* avc = m_pContext->streams[i]->codec;
	AVM_WRITE("FF reader", "S: %d id:%x  bitrate:%d (%d) samprate:%d  chn:%d  framerate:%d/%d  wxh %dx%d  %d/%d\n",
		  i, avc->codec_id, avc->bit_rate, avc->bit_rate_tolerance,
		  avc->sample_rate, avc->channels, avc->time_base.num, avc->time_base.den,
		  avc->width, avc->height, avc->sample_aspect_ratio.num, avc->sample_aspect_ratio.den);
	m_Streams[i] = new FFReadStream(this, i, m_pContext->streams[i]);
    }

    //m_pContext->iformat->read_header(m_pContext, &avfp);
    //printf("samprate %d\nchn %d\nframerate %d\nwidth %d\nheigth %d\n",
    //       avfp.sample_rate, avfp.channels, avfp.frame_rate, avfp.width, avfp.height);

    return 0;
}

uint_t FFReadHandler::GetHeader(void* pheader, uint_t size)
{
    if (pheader)//&& (size >= sizeof(AVIMainHeader)))
    {
	memset(pheader, 0, size);
	//memcpy(pheader, &m_MainHeader, sizeof(AVIMainHeader));
    }
    return 0;//sizeof(AVIMainHeader);
}

IMediaReadStream* FFReadHandler::GetStream(uint_t stream_id,
					   IStream::StreamType type)
{
    int t;
    uint_t j = 0;
    switch (type)
    {
    case IStream::Audio: t = CODEC_TYPE_AUDIO; break;
    case IStream::Video: t = CODEC_TYPE_VIDEO; break;
    default: return 0;
    }

    for (int i = 0; i < m_pContext->nb_streams; i++)
    {
	if (m_pContext->streams[i]->codec->codec_type == t)
	{
	    if (j == stream_id)
		return m_Streams[i];
            j++;
	}
    }

    return 0;
}

uint_t FFReadHandler::GetStreamCount(IStream::StreamType type)
{
    int t;
    uint_t j = 0;

    switch (type)
    {
    case IStream::Audio: t = CODEC_TYPE_AUDIO; break;
    case IStream::Video: t = CODEC_TYPE_VIDEO; break;
    default: return 0;
    }

    for (int i = 0; i < m_pContext->nb_streams; i++)
	if (m_pContext->streams[i]->codec->codec_type == t)
            j++;
    return j;
}

void FFReadHandler::flush()
{
    for (unsigned i = 0; i < m_Streams.size(); i++)
    {
	while (m_Streams[i]->m_Packets.size())
	{
            m_Streams[i]->m_Packets.front()->Release();
	    m_Streams[i]->m_Packets.pop();
	}
        m_Streams[i]->m_uiPosition = 0;
    }
}

int FFReadHandler::seek(framepos_t pos)
{
    Locker locker(m_Mutex);
    url_fseek(&m_pContext->pb, 0, SEEK_SET);
    flush();
    //av_find_stream_info(m_pContext);
    return 0;
}

int FFReadHandler::readPacket()
{
    Locker locker(m_Mutex);
    AVPacket pkt;
    AVM_WRITE("FF reader", "readPacket()\n");
    if (av_read_packet(m_pContext, &pkt) < 0)
    {
        if (!url_feof(&m_pContext->pb))
	    AVM_WRITE("FF reader", "ffmpeg packet error and not eof??\n");
        return -1;
    }

    FFReadStream* s = m_Streams[pkt.stream_index];

    if (s->m_pAvContext)
    {
	AVFrame pic;
	int got_pic = 0;
	memset(&pic, 0, sizeof(pic));
	int r = avcodec_decode_video(s->m_pAvContext,
				     &pic, &got_pic, pkt.data, pkt.size);
	AVM_WRITE("FF reader", "____  %d   %d\n", r, got_pic);
    }
    //printf("FFMPEG pktsize: %u %llu   %d\n", pkt.size, pkt.pts, pkt.stream_index);fflush(stdout);
    StreamPacket* p = new StreamPacket(pkt.size, (char*) pkt.data);
    pkt.data = 0;

    AVStream *st = m_pContext->streams[pkt.stream_index];
    //int sum = 0; for (int i = 0; i < pkt.size; i++) sum += p->memory[i]; printf("PK %d  %d\n", pkt.size, sum);
    //printf("RATE  %f\n", m_Streams[pkt.stream_index]->m_dRate);

    p->position = s->m_uiPosition;
    p->timestamp = 0;//pkt.pts / AV_TIME_BASE;
    if (pkt.pts != (int64_t) AV_NOPTS_VALUE)
	p->timestamp = 1000000 * pkt.pts * st->time_base.num / st->time_base.den;
    else if (pkt.dts != (int64_t) AV_NOPTS_VALUE)
	p->timestamp = 1000000 * pkt.dts * st->time_base.num / st->time_base.den;

    AVM_WRITE("FF reader", "stream:%d  n:%d d:%d p:%llx d:%llx  dur:%d  p:%lld\n", pkt.stream_index, st->time_base.num, st->time_base.den, p->timestamp, pkt.dts, pkt.duration, pkt.pts);
#if 0
    else if (st->codec.frame_rate) {
	p->timestamp = p->position * AV_TIME_BASE * st->codec.frame_rate_base
	    / st->codec.frame_rate;
    }
#endif
    //if (st->codec.codec_type == CODEC_TYPE_VIDEO) printf("FRATE %d pts:%lld %d  %d  t:%lld\n", p->position, pkt.pts,st->codec.frame_rate_base, st->codec.frame_rate, p->timestamp);
    //else printf("Bitrate  %d\n", st->codec.bit_rate);
    //printf("TIMESTAMP %lld    %d %d   bitrate:%d\n", p->timestamp, s->m_pAvStream->r_frame_rate_base, s->m_pAvStream->r_frame_rate, st->codec.bit_rate);

    switch (st->codec->codec_type)
    {
    case CODEC_TYPE_AUDIO:
	if (!pkt.pts && st->codec->bit_rate)
	    p->timestamp = (int64_t)p->position * 8 * 1000000 /
		st->codec->bit_rate;
	s->m_uiPosition += pkt.size;
	break;
    case CODEC_TYPE_VIDEO:
    default:
	s->m_uiPosition++;
	break;
    }

    //printf("Rate %f  %f    %lld\n", m_Streams[pkt.stream_index]->m_dRate, p->timestamp, pkt.pts);
#if 0
    printf("---ReadPacket i:%d   sz:%d   pts:%lld  size:%d   (%lld,  %d)\n",
	   pkt.stream_index, m_Streams[pkt.stream_index]->m_Packets.size(),
	   pkt.pts, pkt.size, p->timestamp, pkt.flags);
#endif
    if (pkt.flags & PKT_FLAG_KEY)
	p->flags |= KEYFRAME;
    av_free_packet(&pkt);

    if (s->m_Packets.size() >= s->m_Packets.capacity() - 1)
    {
	if (0)
	    AVM_WRITE("FF reader", "got too many packets in the queue??? (%d, %d)\n",
		   s->m_Packets.size(), s->m_Packets.capacity());
	s->m_Packets.front()->Release();
        s->m_Packets.pop();
    }

    s->m_Packets.push(p);
    return 0;
}

IMediaReadHandler* CreateFFReadHandler(const char *pszFile)
{
    FFReadHandler* h = new FFReadHandler();
    if (h->Init(pszFile) == 0)
	return h;
    delete h;

    return 0;
}

AVM_END_NAMESPACE;
