/*
 * ReadStreamV ( video AVI stream ) functions
 */

#include "ReadStreamV.h"
#include "avm_creators.h"
#include "avm_fourcc.h"
#include "avm_cpuinfo.h"
#include "avm_output.h"

#include "utils.h"

#include <string.h>
#include <stdio.h>

AVM_BEGIN_NAMESPACE;
static float ttt = 0;
static float pttt = 0;
static float gttt = 0;
//#define VALGRIND

// it's quite similar to qring - but
// for now there are few differencies which do not
// make possible to use qring instead of this VideoQueue
class VideoQueue : public IImageAllocator
{
    friend class ReadStreamV;
    static const unsigned int MAX_QSIZE = 100;
public:
    VideoQueue(IVideoDecoder* vd, uint_t maxsize, IImageAllocator* ia)
	: m_Free(MAX_QSIZE), m_Ready(MAX_QSIZE), m_pIA(ia), m_uiImages(0)
    {
	const BITMAPINFOHEADER& bh = vd->GetDestFmt();
	IVideoDecoder::CAPS caps = vd->GetCapabilities();
	uint_t align = 0;
	if (caps & IVideoDecoder::CAP_ALIGN16)
	    align = 16;
	if (caps & IVideoDecoder::CAP_ALIGN64)
	    align = 64;
	//printf("CAPS %d  %d  %p\n", align, caps, m_pIA);
	//BitmapInfo(bh).Print();

	if (!m_pIA)
	    m_pIA = this;
	while (m_Buffers.size() < maxsize)
	{
	    CImage* ci = m_pIA->ImageAlloc(bh, m_Buffers.size(), align);
	    if (!ci)
	    {
		if (!m_Buffers.size() && m_pIA)
		{
		    m_pIA = this; // no direct
                    AVM_WRITE("video reader", "NODIRECT\n");
                    continue;
		}
		break;
	    }
	    m_Buffers.push_back(ci);
	    //AVM_WRITE("video reader", "w:%d h:%d   bpp:%d 0x%08x -  %d:%d (%p)\n", bh.biWidth, bh.biHeight, bh.biBitCount, bh.biCompression, m_uiSize, maxsize, m_pFrames[m_uiSize - 1]);
	}
	//assert(m_Buffers.size() < m_Free.capacity());
	//for (unsigned i = 0; i < m_Buffers.size(); i++) m_Free.push(m_Buffers[i]);
	Flush();
	//AVM_WRITE("video reader", "VideoQueue: size %d  (%p)\n", m_Buffers.size(), ia);
    }
    virtual ~VideoQueue()
    {
	if (m_pIA)
	    m_pIA->ReleaseImages();
	for (unsigned i = 0; i < m_Buffers.size(); i++)
	    m_Buffers[i]->Release();
    }
    void Flush()
    {
	m_Free.clear();
        m_Ready.clear();
	for (unsigned i = 0; i < m_Buffers.size(); i++)
	{
	    m_Buffers[i]->m_iAge = -256*256*256*64;
	    m_Buffers[i]->m_lTimestamp = 0;
	    m_Free.push(m_Buffers[i]);
	}
    }
    uint_t GetSize() const { return m_Buffers.size(); }
    uint_t GetFreeSize() const { return m_Free.size(); }
    uint_t GetReadySize() const { return m_Ready.size(); }

    CImage* FrontFree() const { return GetFreeSize() ? m_Free.front() : 0; }
    void PopFree() { if (GetFreeSize()) m_Free.pop(); }
    void PushFree(CImage* ci) { if (GetFreeSize() < GetSize()) m_Free.push(ci); }

	CImage* FrontReady() const { return GetReadySize() ? m_Ready.front() : 0; }
    void PopReady() { if (GetReadySize()) m_Ready.pop(); }
    void PushReady(CImage* ci) { if (GetReadySize() < GetSize()) m_Ready.push(ci); }

    // implement ImageAllocator interface
    virtual uint_t GetImages() const { return m_uiImages; }
    virtual CImage* ImageAlloc(const BITMAPINFOHEADER& bh, uint_t idx, uint_t align)
    {
	CImage* ci = new CImage((const BitmapInfo*)&bh);
	if (ci)
	{
	    m_uiImages++;
	    ci->SetAllocator(this);
	}
        return ci;
    }
    virtual void ReleaseImages() { m_uiImages = 0; }

protected:
    avm::qring<CImage*> m_Free;
    avm::qring<CImage*> m_Ready;
    IImageAllocator* m_pIA;
    avm::vector<CImage*> m_Buffers;
    CImage** m_pFrames;  // buffer for frames (currently with fixed size)
    uint_t m_uiImages;
};


ReadStreamV::ReadStreamV(IMediaReadStream* stream)
    :ReadStream(stream), m_pVideodecoder(0), m_pQueue(0), m_pFrame(0),
    m_pQueueAllocator(0), m_uiQueueSize(1), flip(0)
{
    Flush();
}

ReadStreamV::~ReadStreamV()
{
    StopStreaming();
    //printf("XXTTT %f   %f  %f\n", ttt, pttt, gttt);
}

bool ReadStreamV::Eof() const
{
    if (GetBuffering() > 0)
	return false;
    //AVM_WRITE("video reader", "-- %d   p:%d l:%d  %d\n", ReadStream::Eof(), GetPos(), GetLength(), GetBuffering());
    return ReadStream::Eof();
}

void ReadStreamV::Flush()
{
    ReadStream::Flush();
    if (m_pVideodecoder)
    {
	m_pVideodecoder->Flush();
	//m_pVideodecoder->Stop();
	//m_pVideodecoder->Start();
    }

    m_uiSubBPos = 0;
    m_dSubBTime = 0.;
    if (m_pQueue)
	m_pQueue->Flush();
    m_pFrame = 0;
    Update();

    //m_bHadKeyFrame = false;
    m_bHadKeyFrame = true;
}

uint_t ReadStreamV::GetBuffering(uint_t* bufsz) const
{
    uint_t bs = m_uiQueueSize;
    uint_t s;

    if (m_pQueue)
    {
	s = m_pQueue->GetReadySize();
	bs = m_pQueue->GetFreeSize() + s;
	//printf("free:%d  sz:%d  fil:%d\n", m_pQueue->GetSize(), bs, s);
    }
    else
        s = 0;
    if (bufsz)
	*bufsz = bs;

    return s;
}

framepos_t ReadStreamV::GetPos() const
{
    return m_uiVPos;
}

double ReadStreamV::GetTime(framepos_t framep) const
{
    if (framep == ERR)
        return m_dVTime;

    return m_pStream->GetTime(framep);
}

/******************************************************************

    These functions are meaningless for 'unknown type' stream

******************************************************************/

uint_t ReadStreamV::GetVideoFormat(void* format, uint_t size) const
{
    if (format)
	memcpy(format, m_pFormat, (size < m_uiFormatSize) ? size : m_uiFormatSize);

    return m_uiFormatSize;
}

CImage* ReadStreamV::GetFrame(bool readframe)
{
    if (m_pVideodecoder)
    {
        //printf("GETGRAME  %p  %d\n", m_pFrame, readframe);
	// we decode frame here in direct unbuffered mode
	// it's not being decoded in separate thread
	if (!m_pFrame && readframe)
	    ReadFrame();
	if (m_pFrame)
	{
	    CImage* ci = m_pFrame;
	    ci->AddRef();
	    m_pQueue->PopReady();
	    //printf("GETFRAME:READY  %d   f:%d\n", m_pQueue->GetReadySize(),m_pQueue->GetFreeSize());
	    m_pQueue->PushFree(m_pFrame);
	    m_pFrame = m_pQueue->GetReadySize() ? m_pQueue->FrontReady() : 0;
	    //printf("GetFrame new: %p  (ready: %d)\n", m_pFrame, m_pQueue->GetReadySize());
            Update();
	    return ci;
	}
	//else { printf("Avifile ERROR\n"); abort(); }
    }
    return 0;
}

uint_t ReadStreamV::GetFrameSize() const
{
    return (m_pVideodecoder) ? m_pVideodecoder->GetDestFmt().biSizeImage : 0;
}

uint_t ReadStreamV::GetOutputFormat(void* format, uint_t size) const
{
    if (!m_pVideodecoder)
	return 0;

    if (format)
	memcpy(format, &m_pVideodecoder->GetDestFmt(),
	       (size < sizeof(BITMAPINFOHEADER))
	       ? size : sizeof(BITMAPINFOHEADER));

    return sizeof(BITMAPINFOHEADER);
}

int ReadStreamV::ReadDirect(void* buffer, uint_t bufsize, uint_t samples,
			    uint_t& samples_read, uint_t& bytes_read,
			    int* flags)
{
    int r = ReadStream::ReadDirect(buffer, bufsize, samples,
				   samples_read, bytes_read, flags);
    m_pFrame = 0;
    Update();
    return r;
}

int ReadStreamV::ReadFrame(bool render)
{
    if (!m_pVideodecoder)
	return -1;

    CImage* pFrame = 0;
    int hr = -1;
    for (;;)
    {
	if (!ReadPacket())
	{
	    // regular Eof usually - we should check if we have reached last pos
	    m_iEof++;
	    return -1;
	}
	m_iEof = 0;
	int size = m_pPacket->size - m_pPacket->read;
	//printf("pkttm %lld  %d\n", m_pPacket->timestamp, size);
	if (size > 0)
	{
	    if (!pFrame)
	    {
		if (m_pQueue)
		{
		    //printf("READFRAME  F:%d  R:%d   %p\n", m_pQueue->GetFreeSize(), m_pQueue->GetReadySize(), pFrame);
		    if (!m_pQueue->GetFreeSize())
		    {
			pFrame = m_pQueue->FrontReady();
			m_pQueue->PopReady();
			m_pQueue->PushFree(pFrame);
			printf("???NO FREE FRAME???\n");
			//abort();
		    }
		    const BITMAPINFOHEADER& bh = m_pVideodecoder->GetDestFmt();
		    pFrame = m_pQueue->FrontFree();
		    if (!pFrame)
                        return -1;
		    if (pFrame->GetFmt()->biCompression != bh.biCompression)
		    {
			int x = pFrame->Format();
			//printf("Frame format: 0x%x %.4s  0x%x %.4s\n",
			//x, (char*)&x, bh.biCompression, (char*)&bh.biCompression);
			//frame->GetImage()->GetFmt()->Print();
			//AVM_WRITE("video reader", "FORMAT doesn't match!!!\n");
			// reinitialize queue
			delete m_pQueue;
			m_pQueue = 0;
		    }
		}
		if (!m_pQueue)
		{
		    AVM_WRITE("video reader", 1, "queue create:  %d  %p\n", m_uiQueueSize, m_pQueueAllocator);
		    m_pFrame = 0;
		    m_pQueue = new VideoQueue(m_pVideodecoder, m_uiQueueSize, m_pQueueAllocator);
		    pFrame = m_pQueue->FrontFree();
		}
		assert(pFrame);
	    }
	    //printf("READFRAME  F:%d  R:%d   %p\n", m_pQueue->GetFreeSize(), m_pQueue->GetReadySize(), pFrame);
	    if (m_pPacket->timestamp != StreamPacket::NO_TIMESTAMP)
	    {
		pFrame->m_uiPosition = m_pPacket->position;
		pFrame->m_lTimestamp = m_pPacket->timestamp;
	    }

#ifndef VALGRIND
	    if (avm_is_mmx_state())
	    {
                m_pPacket->read = m_pPacket->size;
                AVM_WRITE("AVI Read", "Internal ERROR - avifile left MMX STATE!\n");
		break;
	    }
#endif
	    //uint_t sum = 0; for (uint_t i = 0 ; i < m_pPacket->size; i++) sum += ((unsigned char*) m_pPacket->memory)[i];
	    //printf("Mem %p   Size %d   Flg %d  PACKETSUM %d   pos: %d  size: %d\n", m_pPacket->memory, m_pPacket->size, m_pPacket->flags, sum, m_pPacket->position, m_pPacket->size);
	    //int s = 0; for (unsigned i = 0; i < lBytes; i++) s += ((unsigned char*)temp_buffer)[i]; printf("SUM %d\n", s);
            hr = 0;
	    if (!m_bHadKeyFrame && m_pPacket->flags)
		m_bHadKeyFrame = true;

	    CImage* pOut = 0;
#ifndef VALGRIND  // check without codec decompresion VALGRIND
	    //int64_t sss = longcount();
	    //printf("FRAMEin  %p %lld\n", pFrame, m_pPacket->timestamp);
	    if (m_bHadKeyFrame)
	    {
		memset(m_pPacket->memory + m_pPacket->size, 0, 8);
		hr = m_pVideodecoder->DecodeFrame(pFrame, m_pPacket->memory + m_pPacket->read,
						  size, m_pPacket->flags, render, &pOut);
	    }
//ttt += to_float(longcount(), sss);
#endif
	    //printf("RENDER %d  decomp res:%d  %f\n", render, hr, m_pStream->GetTime());
#ifndef VALGRIND
	    if (avm_is_mmx_state())
	    {
		AVM_WRITE("AVI Read", "Warning - codec left MMX STATE!\n");
#if defined(ARCH_X86) && !defined(VALGRIND)
		__asm__ __volatile__ ("emms");// ::: "memory" );
#endif
	    }
#endif
	    m_pPacket->read += (hr <= 0) ? size : hr
		& ~(IVideoDecoder::NEXT_PICTURE | IVideoDecoder::NO_PICTURE);

	    if (hr >= 0)
	    {
		//printf("PACKET %p %f  %f\n", frame,  m_pPacket->timestamp, m_dSubBTime);
		//printf("SETTIME  %d   %f  %f\n", m_uiLastPos, m_dLastTime, frame->GetTime());
		// even when there is no new picture always
		// rotate buffers - as it could be used to store
		// temporal image (i.e. by ffmpeg)
		if (hr & IVideoDecoder::NEXT_PICTURE)
		{
		    //printf("GET NEW  %f\n", m_pPacket->timestamp);
		    assert(m_pQueue->GetFreeSize());
		    m_pQueue->PopFree();
		    pFrame = 0;
		    //printf("NEXTPIC  %d\n", m_pQueue->GetFreeSize());
                    hr &= ~IVideoDecoder::NEXT_PICTURE;
		}
		if (hr & IVideoDecoder::NO_PICTURE)
		{
		    if (pOut)
		    {
			m_pQueue->PushFree(pOut);
			printf("???OUTPUSH-NOPICTURE  %lld FREE: %d\n", pOut->m_lTimestamp, m_pQueue->GetFreeSize());
		    }
		    //printf("NOPICTURE  %x\n", hr);
		    continue;
		}
		//printf("HAVE PICTURE  %p  %p\n", pOut, pFrame);
		if (!pOut)
		{
		    if (pFrame)
		    {
			m_pQueue->PopFree();
			//printf("NOOUTPOP_FRAME  %d\n", m_pQueue->GetFreeSize());
		    }
		}
		else // pOut
		    pFrame = pOut;

		if (render)
		{
                    if (!m_pQueue->m_Ready.size() || m_pQueue->m_Ready.back() != pFrame)
			m_pQueue->PushReady(pFrame);
		    else
                        printf("FFMPEG BUG - repeated insert\n");
		    m_pFrame = m_pQueue->FrontReady();
		}
		else
		    m_pQueue->PushFree(pFrame);

#if 0
		printf("PACKET %p   Frame %p   Queue %p\n", m_pPacket, pFrame, m_pQueue);
		if (pFrame)
		    printf("Packet  %d  %d  0x%x  t:%f  p:%d  ft:%f  f:%d tp:%d  p:%p\n",
			   m_pPacket->size, m_pPacket->read, hr,
			   m_pPacket->timestamp / 1000000.0,
			   m_pPacket->position,
			   pFrame->m_lTimestamp / 1000000.0,
			   pFrame->m_iType,
			   m_pQueue->GetFreeSize(), pOut);
#endif
		//printf("FRAMEout  %p  %f\n", pFrame, pFrame->m_dTimestamp);
		pFrame = 0;
		///printf("SETPF %d  %d\n", m_pQueue->GetReadySize(), m_pQueue->GetFreeSize());
		//printf("SetFrame %p  %d  %f\n", m_pFrame, m_pFrame->GetPos(), m_pFrame->GetTime());
		break;
	    }
	}
    }

    ReadPacket(); // just to be sure we always try next packet
    Update();

    return hr;
}

framepos_t ReadStreamV::SeekToKeyFrame(framepos_t pos)
{
    AVM_WRITE("video reader", 1, "ReadStreamV::SeekToKeyFrame() %d\n", pos);

    framepos_t n = ReadStream::SeekToKeyFrame(pos);
    if (!(GetFrameFlags(0) & KEYFRAME))
	SeekToPrevKeyFrame();

    return n;
}

double ReadStreamV::SeekTimeToKeyFrame(double timepos)
{
    AVM_WRITE("video reader", 1, "ReadStreamV::SeekTimeToKeyFrame() %f\n", timepos);
    double t = ReadStream::SeekTimeToKeyFrame(timepos);
    if (t >= 0 && !(GetFrameFlags(0) & KEYFRAME))
	SeekToPrevKeyFrame();

    return GetTime();
}

int ReadStreamV::SetDirection(bool d)
{
    flip = d;
    return 0;
}

int ReadStreamV::SetBuffering(uint_t maxsz, IImageAllocator* ia)
{
    m_pFrame = 0;
    delete m_pQueue;
    m_pQueue = 0;
    m_pQueueAllocator = ia;
    m_uiQueueSize = maxsz;
    return 0;
}

int ReadStreamV::SetOutputFormat(void* bi, uint_t size)
{
    if (!m_pVideodecoder)
	return -1;
    // FIXME
    AVM_WRITE("video reader", "ReadStreamV::SetOutputFormat: not implemented!\n");
    return -1; //not implemented
}

int ReadStreamV::StartStreaming(const char* privname=0)
{
    if (m_pVideodecoder)
    {
	AVM_WRITE("video reader", "ReadStreamV already streaming!\n");
	return 0;
    }

    if (GetType() != Video)
	return -1;

    BITMAPINFOHEADER* bh = (BITMAPINFOHEADER*) m_pFormat;
    m_pVideodecoder = CreateDecoderVideo(*bh, 24, flip, privname);

    if (!m_pVideodecoder)
	return -1;

    m_pVideodecoder->Start();
    Flush();

    AVM_WRITE("video reader", 2, "ReadStreamV::StartStreaming()  %f\n", m_dLastTime);
    return 0;
}

int ReadStreamV::StopStreaming()
{
    if (m_pVideodecoder)
    {
	//AVM_WRITE("video reader", "ReadStreamV::StopStreaming()\n");
	FreeDecoderVideo(m_pVideodecoder);
	m_pVideodecoder = 0;
        m_pFrame = 0;
	delete m_pQueue;
	m_pQueue = 0;
    }
    return 0;
}

AVM_END_NAMESPACE;
