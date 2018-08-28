#include "AviWrite.h"
#include "audioencoder.h"
#include "avm_output.h"
#include "avm_creators.h"
#include <stdio.h>

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "AudioWrite"

AviAudioWriteStream::AviAudioWriteStream(AviWriteFile* file, int ckid, const CodecInfo& ci,
					 const WAVEFORMATEX* fmt,
					 int bitrate, int flags)
    :AviWriteStream(file, ckid, IStream::Audio, ci.fourcc, bitrate, flags),
    m_pAudioEnc(0), m_astatus(0), m_bitrate(bitrate)
{
    if (fmt)
	srcfmt = *fmt;

    m_pAudioEnc = CreateEncoderAudio(ci, &srcfmt);
    if (!m_pAudioEnc)
	throw FATAL("Could not create audio encoder");

    AviWriteStream::m_stop = 1;
}

AviAudioWriteStream::~AviAudioWriteStream()
{
    //AVM_WRITE("AVI writer", "AviAudioWriteStream::~AviAudioWriteStream()\n"); fflush(stdout);
    Stop();
    FreeEncoderAudio(m_pAudioEnc);
}

int AviAudioWriteStream::AddData(void* data, uint_t size)
{
    if (!m_astatus)
	return -1;

    uint_t outsize = 2*size/srcfmt.nBlockAlign+7200;
    char* buf = new char[outsize];
    uint_t written = 0;
    int hr;
    if (data)
    {
	hr = m_pAudioEnc->Convert((uint8_t*)data, size/srcfmt.nBlockAlign, buf,
			      outsize, (uint_t*)0, &written);
	//AVM_WRITE("AVI writer", "Converted %d  bytes to  %d bytes\n", size, written);
	if (hr == 0)
	    hr = AviWriteStream::AddChunk(buf, written, AVIIF_KEYFRAME);
    }
    else
	hr = AviWriteStream::AddChunk(NULL, 0);

    delete[] buf;
    return hr;
}

const CodecInfo& AviAudioWriteStream::GetCodecInfo() const
{
    return m_pAudioEnc->GetCodecInfo();
}

int AviAudioWriteStream::AviAudioWriteStream::Start()
{
    if (m_astatus)
	return -1;

    m_pAudioEnc->SetBitrate(m_bitrate);
    m_uiFormatSize = m_pAudioEnc->GetFormatSize();
    delete[] m_pcFormat;
    m_pcFormat = new char[m_uiFormatSize];
    m_pAudioEnc->GetFormat(m_pcFormat, m_uiFormatSize);
    WAVEFORMATEX* fmt = (WAVEFORMATEX*)m_pcFormat;
#if 1
    SetAudioHeader(fmt->nBlockAlign, m_bitrate, fmt->nBlockAlign);
#else
    printf("BITRATE  %d  %d\n", m_bitrate, fmt->nBlockAlign);
    //SetAudioHeader(0, 44100, 23040);
    SetAudioHeader(0, 44100, 41472);
#endif
    m_pAudioEnc->Start();
    m_astatus = 1;
    return 0;
}

int AviAudioWriteStream::Stop()
{
    if (!m_astatus)
	return -1;

    uint_t written = 0;
    char* buf = new char[7200];// given by mp3 spec
    m_pAudioEnc->Close(buf, 7200, &written);
    if (written)
	AviWriteStream::AddChunk(buf, written);
    m_astatus = 0;
    delete[] buf;
    return 0;
}

uint_t AviAudioWriteStream::GetLength() const
{
    return AviWriteStream::GetLength();
}

#undef __MODULE__

AVM_END_NAMESPACE;
