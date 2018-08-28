#ifndef AVIPLAY_AVIREADSTREAM_H
#define AVIPLAY_AVIREADSTREAM_H

#include "ReadStream.h"

AVM_BEGIN_NAMESPACE;

class IVideoDecoder;
class VideoQueue;

class ReadStreamV : public ReadStream
{
public:
    ReadStreamV(IMediaReadStream* stream);
    virtual ~ReadStreamV();
    virtual bool Eof() const;
    virtual uint_t GetBuffering(uint_t* b = 0) const;
    virtual IVideoDecoder* GetVideoDecoder() const { return m_pVideodecoder; }
    virtual CImage* GetFrame(bool readframe = false);
    virtual uint_t GetFrameSize() const;
    virtual framepos_t GetPos() const;
    virtual uint_t GetOutputFormat(void* format = 0, uint_t size = 0) const;
    virtual double GetTime(framepos_t frame = ERR) const;
    virtual uint_t GetVideoFormat(void* format, uint_t size) const;
    virtual bool IsStreaming() const { return (m_pVideodecoder != 0); }
    virtual int ReadDirect(void* buffer, uint_t bufsize, uint_t samples,
			   uint_t& samples_read, uint_t& bytes_read,
			   int* flags = 0);
    virtual int ReadFrame(bool render = true);
    virtual framepos_t SeekToKeyFrame(framepos_t pos);
    virtual double SeekTimeToKeyFrame(double pos);
    virtual int SetDirection(bool d);
    virtual int SetBuffering(uint_t maxsz = 1, IImageAllocator* ia = 0);
    virtual int SetOutputFormat(void* bi, uint_t size);
    virtual int StartStreaming(const char*);
    virtual int StopStreaming();

protected:
    /* flush all precached frames */
    virtual void Flush();
    void ResetBuffers();
    void Update()
    {
	if (m_pFrame)
	{
	    m_dVTime = m_pFrame->m_lTimestamp / 1000000.0;
	    m_uiVPos = m_pFrame->m_uiPosition;
	}
	else
	{
	    m_dVTime = m_dLastTime - m_dSubBTime;
	    m_uiVPos = m_uiLastPos - m_uiSubBPos;
	}
    }
    IVideoDecoder* m_pVideodecoder;
    VideoQueue* m_pQueue;
    CImage* m_pFrame;
    IImageAllocator* m_pQueueAllocator;
    double m_dVTime;
    uint_t m_uiVPos;
    double m_dSubBTime;
    uint_t m_uiSubBPos;
    uint_t m_uiQueueSize;
    int flip;
    bool m_bCapable16b;
    bool m_bHadKeyFrame;
};

AVM_END_NAMESPACE;

#endif // AVIPLAY_READSTREAM_H
