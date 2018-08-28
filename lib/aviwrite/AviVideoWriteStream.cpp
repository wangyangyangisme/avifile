#include "AviWrite.h"
#include "avm_creators.h"
#include "videoencoder.h"
#include <string.h>
#include <stdio.h>

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "VideoWrite"

AviVideoWriteStream::AviVideoWriteStream(AviWriteFile* file, int ckid,
					 const CodecInfo& ci,
					 const BITMAPINFOHEADER* srchdr,
					 int frame_rate, int flags)

    :AviWriteStream(file, ckid, Video, ci.fourcc, frame_rate, flags),
    m_vstatus(0), m_pBuffer(0)
{
    m_pVideoEnc = CreateEncoderVideo(ci, *srchdr);
    if (!m_pVideoEnc)
	throw FATAL("Could not create encoder");
    AviWriteStream::m_stop = 1;
}

AviVideoWriteStream::~AviVideoWriteStream()
{
    //AVM_WRITEI("AVI writer", "AviAudioWriteStream::~AviAudioWriteStream()\n"); fflush(stdout);
    Stop();
    FreeEncoderVideo(m_pVideoEnc);
}

int AviVideoWriteStream::AddFrame(CImage* chunk, uint_t* pSize, int* pKeyframe, char** ppData)
{
    if (m_vstatus != 1)
    {
	if (pSize)
	    *pSize = 0;
	return -1;
    }

    uint_t size = 0;
    int is_keyframe = 0;
    int hr;
    if (chunk)
    {
	hr = m_pVideoEnc->EncodeFrame(chunk, m_pBuffer, &is_keyframe, &size);
	if (hr == 0)
	{
	    hr = AviWriteStream::AddChunk(m_pBuffer, size, is_keyframe);
	}
	else
	{
	    size = 0;
            is_keyframe = 0;
	}
    }
    else
	hr = AviWriteStream::AddChunk(NULL, 0);

    if (pSize)
	*pSize = size;
    if (pKeyframe)
	*pKeyframe = (is_keyframe != 0);
    if (ppData)
        *ppData = m_pBuffer;
    return hr;
}

const CodecInfo& AviVideoWriteStream::GetCodecInfo() const
{
    return m_pVideoEnc->GetCodecInfo();
}

int AviVideoWriteStream::Start()
{
    if (m_vstatus)
	return -1;
    const BITMAPINFOHEADER& bh = m_pVideoEnc->GetOutputFormat();
    m_uiFormatSize = bh.biSize;
    delete[] m_pcFormat;
    m_pcFormat = new char[bh.biSize];
    memcpy(m_pcFormat, &bh, bh.biSize);
    m_pVideoEnc->Start();
    m_pBuffer = new char[m_pVideoEnc->GetOutputSize()];
    SetVideoHeader(100,
		   //m_pVideoEnc->GetQuality(),
		   bh.biWidth, bh.biHeight);
    m_vstatus = 1;
    return 0;
}

int AviVideoWriteStream::Stop()
{
    if (!m_vstatus)
	return -1;
    m_pVideoEnc->Stop();
    delete[] m_pBuffer;
    m_pBuffer = 0;
    m_vstatus = 0;
    return 0;
}

uint_t AviVideoWriteStream::GetLength() const
{
    return AviWriteStream::GetLength();
}

#undef __MODULE__

AVM_END_NAMESPACE;
