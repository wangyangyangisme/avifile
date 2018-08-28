#include "audiodecoder.h"
#include "audioencoder.h"
#include "utils.h"
#include "avm_output.h"
#include <stdio.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

IAudioDecoder::IAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* inw)
    :m_Info(info), m_pFormat(0)
{
    uint_t sz = sizeof(WAVEFORMATEX) + inw->cbSize;
    m_pFormat = (WAVEFORMATEX*) new char[sz];
    memcpy(m_pFormat, inw, sz);

    m_uiBytesPerSec = m_pFormat->wBitsPerSample;
    switch (m_uiBytesPerSec)
    {
    case 0:
    case 2: // IMC
    case 4: // IMA_ADPCM
	m_uiBytesPerSec = 16;
	break;
    }

    switch (m_pFormat->wFormatTag)
    {
    case 6: // A-Law  8 -> 16b
    case 7: // u-Law  8 -> 16b
	m_uiBytesPerSec = 16;
	break;
    }

    m_uiBytesPerSec = ((m_uiBytesPerSec + 7) / 8)
	* m_pFormat->nSamplesPerSec * m_pFormat->nChannels;
}

IAudioDecoder::~IAudioDecoder()
{
    delete[] m_pFormat;
}

void IAudioDecoder::Flush()
{
}

const CodecInfo& IAudioDecoder::GetCodecInfo() const
{
    return m_Info;
}

uint_t IAudioDecoder::GetMinSize() const
{
    if (m_pFormat->nBlockAlign > 2)
	return m_pFormat->nBlockAlign;
    return 2;
}

int IAudioDecoder::GetOutputFormat(WAVEFORMATEX* destfmt) const
{
    if (!destfmt)
	return -1;

    // couple tricks here
    switch (m_pFormat->wBitsPerSample)
    {
    case 0:
    case 2: // IMC
    case 4: // IMA_ADPCM
	destfmt->wBitsPerSample = 16;
	break;
    default:
	destfmt->wBitsPerSample = m_pFormat->wBitsPerSample;
    }

    destfmt->nSamplesPerSec = m_pFormat->nSamplesPerSec;
    destfmt->nChannels = m_pFormat->nChannels;
    if (destfmt->nChannels > 2)
	destfmt->nChannels = 2; // AC3 5 channel

    switch (m_pFormat->wFormatTag)
    {
    case 6: // A-Law  8 -> 16b
    case 7: // u-Law  8 -> 16b
        destfmt->wBitsPerSample = 16;
	break;
    case 0x2000:
	if (destfmt->nSamplesPerSec > 48000)
	    destfmt->nSamplesPerSec = 48000;
	if (m_pFormat->nAvgBytesPerSec < m_pFormat->nSamplesPerSec)
            m_pFormat->nAvgBytesPerSec = m_pFormat->nSamplesPerSec;
    }

    destfmt->wFormatTag = WAVE_FORMAT_PCM;
    destfmt->nBlockAlign = destfmt->nChannels * ((destfmt->wBitsPerSample + 7) / 8);
    destfmt->nAvgBytesPerSec = destfmt->nSamplesPerSec * destfmt->nBlockAlign;
    destfmt->cbSize = 0;

    return 0;
}

IRtConfig* IAudioDecoder::GetRtConfig()
{
    return 0;
}

uint_t IAudioDecoder::GetSrcSize(uint_t dest_size) const
{
    //printf("GET SRC  %d  %d   %d %d\n", m_uiBytesPerSec, m_pFormat->nChannels, m_pFormat->wBitsPerSample, m_pFormat->nSamplesPerSec);
    if (!m_uiBytesPerSec || !m_pFormat->nBlockAlign)
    {
	if (!m_pFormat->nSamplesPerSec)
	    return 1152;

	return dest_size;
    }

    int frames = dest_size * m_pFormat->nAvgBytesPerSec / m_uiBytesPerSec;
    if (frames < m_pFormat->nBlockAlign)
	frames = m_pFormat->nBlockAlign;
    else if (m_pFormat->nBlockAlign > 1)
	frames -= frames % m_pFormat->nBlockAlign;

    return frames;
}

int IAudioDecoder::SetOutputFormat(const WAVEFORMATEX* destfmt)
{
    return -1;
}

IAudioEncoder::IAudioEncoder(const CodecInfo& info)
    :m_Info(info)
{
}

IAudioEncoder::~IAudioEncoder()
{
}

const CodecInfo& IAudioEncoder::GetCodecInfo() const
{
    return m_Info;
}

int IAudioEncoder::SetBitrate(int bitrate)
{
    return -1;
}

int IAudioEncoder::SetQuality(int quality)
{
    return -1;
}

AVM_END_NAMESPACE;
