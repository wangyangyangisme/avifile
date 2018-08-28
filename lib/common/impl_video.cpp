#include "frame.h"
#include "videodecoder.h"
#include "videoencoder.h"
#include "avm_fourcc.h"
#include "avm_cpuinfo.h"
#include "renderer.h"
#include "utils.h"

#include <unistd.h>
#include <sys/time.h> //gettimeofday
#include <stdlib.h> // labs
#include <string.h> // memset

AVM_BEGIN_NAMESPACE;

IVideoDecoder::IVideoDecoder(const CodecInfo& info, const BITMAPINFOHEADER& format)
    :m_Info(info), m_pFormat(0),
    m_Dest(format.biWidth, format.biHeight, 24),
    //m_Dest(format),
    m_pImage(0)
{
//    m_Dest.biWidth = format.biWidth;
//    m_Dest.biHeight = format.biHeight;
//    m_Dest.SetBits(24);
    unsigned bihs = (format.biSize < (int) sizeof(BITMAPINFOHEADER)) ?
	sizeof(BITMAPINFOHEADER) : format.biSize;
    m_pFormat = (BITMAPINFOHEADER*) new char[bihs];
    memcpy(m_pFormat, &format, format.biSize);
}

IVideoDecoder::~IVideoDecoder()
{
    delete[] (char*) m_pFormat;
    if (m_pImage)
	m_pImage->Release();
}

const BITMAPINFOHEADER& IVideoDecoder::GetDestFmt() const
{
    return (const BITMAPINFOHEADER&) m_Dest;
}

IVideoDecoder::CAPS IVideoDecoder::GetCapabilities() const
{
    return CAP_NONE;
}

const CodecInfo& IVideoDecoder::GetCodecInfo() const
{
    return m_Info;
}

IRtConfig* IVideoDecoder::GetRtConfig()
{
    return 0;
}

void IVideoDecoder::Flush()
{
}

int IVideoDecoder::Start()
{
    return 0;
}

int IVideoDecoder::Stop()
{
    return 0;
}

int IVideoDecoder::Restart()
{
    Stop();
    return Start();
}

int IVideoDecoder::SetDirection(int d)
{
    if (m_Dest.biHeight < 0)
	m_Dest.biHeight = -m_Dest.biHeight;
    if (!d)
	m_Dest.biHeight = -m_Dest.biHeight;

    return 0;
}

#ifdef AVM_COMPATIBLE
int IVideoDecoder::DecodeFrame(const void* src, uint_t size, framepos_t p, double t,
			       int is_keyframe, bool render)
{
    if (m_pImage && !m_pImage->IsFmt(&m_Dest))
    {
	m_pImage->Release();
        m_pImage = 0;
    }
    if (!m_pImage)
	m_pImage = new CImage(&m_Dest);

    return DecodeFrame(m_pImage, src, size, is_keyframe, render);
}
#endif

/**
 * Implementation for Encoder
 */

IVideoEncoder::IVideoEncoder(const CodecInfo& info)
    :m_Info(info)
{
}
IVideoEncoder::~IVideoEncoder()
{
}
const CodecInfo& IVideoEncoder::GetCodecInfo() const
{
    return m_Info;
}
float IVideoEncoder::GetFps() const
{
    return -1.;
}
int IVideoEncoder::SetFps(float fps)
{
    return -1;
}

AVM_END_NAMESPACE;
