#include "FFAudioDecoder.h"
#include "avm_output.h"
#include <stdlib.h>
//#include <stdio.h>

AVM_BEGIN_NAMESPACE;

FFAudioDecoder::FFAudioDecoder(AVCodec* av, const CodecInfo& info, const WAVEFORMATEX* wf)
    :IAudioDecoder(info, wf), m_pAvCodec(av), m_pAvContext(0)
{
}

FFAudioDecoder::~FFAudioDecoder()
{
    if (m_pAvContext)
    {
	avcodec_close(m_pAvContext);
	free(m_pAvContext);
    }
}

int FFAudioDecoder::Convert(const void* in_data, uint_t in_size,
			    void* out_data, uint_t out_size,
			    uint_t* size_read, uint_t* size_written)
{
    if (!m_pAvContext)
    {
	m_pAvContext = avcodec_alloc_context();
	m_pAvContext->channels = m_pFormat->nChannels;
	if (m_pAvContext->channels > 2)
	    m_pAvContext->channels = 2;
	m_pAvContext->bit_rate = m_pFormat->nAvgBytesPerSec * 8;
	m_pAvContext->sample_rate = m_pFormat->nSamplesPerSec;
	m_pAvContext->block_align = m_pFormat->nBlockAlign;
	m_pAvContext->codec_tag = m_Info.fourcc;
	m_pAvContext->codec_id = (CodecID) m_pAvCodec->id;

	if (m_pFormat->cbSize > 0)
	{
	    m_pAvContext->extradata = (char*)(m_pFormat + 1);
	    m_pAvContext->extradata_size = m_pFormat->cbSize;
	}

	if (avcodec_open(m_pAvContext, m_pAvCodec) < 0)
	{
	    AVM_WRITE("FFAudioDecoder", "WARNING: can't open avcodec\n");
	    free(m_pAvContext);
	    m_pAvContext = 0;
            return -1;
	}
    }
    int framesz = 0;
    int hr = avcodec_decode_audio(m_pAvContext, (int16_t*)out_data, &framesz,
				  (uint8_t*)in_data, in_size);
    //printf("CONVERT  i:%d  o:%d  f:%d   h:%d\n", in_size, out_size, framesz, hr);
    if (size_read)
	*size_read = (hr < 0) ? in_size : hr;
    if (size_written)
	*size_written = framesz;

    if (hr < 0)
    {
	avcodec_close(m_pAvContext);
	free(m_pAvContext);
        m_pAvContext = 0;
    }
    return (hr < 0 || in_size == 0) ? -1 : 0;
}

uint_t FFAudioDecoder::GetMinSize() const
{
    switch (m_Info.fourcc)
    {
    case 0x2000:
	return MIN_AC3_CHUNK_SIZE;
    case 0x160:
    case 0x161:
	return 16 * m_pFormat->nBlockAlign * m_uiBytesPerSec / m_pFormat->nAvgBytesPerSec;
    case 0x11:
	if (!m_pFormat->nBlockAlign)
            return 1024;
	return m_pFormat->nBlockAlign * m_pFormat->nChannels;
    default:
	return 2;
    }
}

uint_t FFAudioDecoder::GetSrcSize(uint_t dest_size) const
{
    switch (m_Info.fourcc)
    {
    case 0x160:
    case 0x161:
	return m_pFormat->nBlockAlign;
    case 0x11:
	if (!m_pFormat->nBlockAlign)
            return 1024;
    default:
	return IAudioDecoder::GetSrcSize(dest_size);
    }
}

AVM_END_NAMESPACE;
