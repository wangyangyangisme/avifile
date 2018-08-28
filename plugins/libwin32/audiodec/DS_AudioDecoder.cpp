/**
 * DirectShow audio decoder
 * Copyright 2001 Eugene Kuznetsov  (divx@euro.ru)
 */

#include "DS_AudioDecoder.h"
#include "ldt_keeper.h"

#include "utils.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


AVM_BEGIN_NAMESPACE;

DS_AudioDecoder::DS_AudioDecoder(const CodecInfo& info, const WAVEFORMATEX* wf)
    :IAudioDecoder(info, wf), m_pDS_Filter(0), m_pVhdr(0)
{
    const unsigned int WFSIZE = sizeof(WAVEFORMATEX);

    int sz = WFSIZE + wf->cbSize;
    m_pVhdr = (WAVEFORMATEX*) malloc(sz);
    memcpy(m_pVhdr, wf, sz);

    //m_Format.nBlockAlign *= 2;
    //for (int i = 0; i < WFSIZE + wf->cbSize; i++)
    //    printf("%d  %x\n", i, ((unsigned char*)m_pVhdr)[i]);

    m_sVhdr2 = *((WAVEFORMATEX*) m_pVhdr);
    m_sVhdr2.wFormatTag = 1;
    m_sVhdr2.wBitsPerSample = 16;
    m_sVhdr2.nBlockAlign = m_sVhdr2.nChannels * (m_sVhdr2.wBitsPerSample + 7) / 8;
    m_sVhdr2.cbSize = 0;
    m_sVhdr2.nAvgBytesPerSec = m_sVhdr2.nBlockAlign * m_sVhdr2.nSamplesPerSec;

    //char bb[200]; avm_wave_format(bb, sizeof(bb), &m_sVhdr2);
    //printf("DSFORMAT %d  %s\n", sizeof(m_sVhdr2), bb);
    // in IAudioDecoder: m_Format = *wf;
    memset(&m_sOurType, 0, sizeof(m_sOurType));
    m_sOurType.majortype = MEDIATYPE_Audio;
    m_sOurType.subtype = MEDIASUBTYPE_PCM;
    m_sOurType.subtype.f1 = wf->wFormatTag;
    m_sOurType.formattype = FORMAT_WaveFormatEx;
    m_sOurType.lSampleSize = wf->nBlockAlign;
    m_sOurType.bFixedSizeSamples = true;
    m_sOurType.bTemporalCompression = false;
    m_sOurType.pUnk = 0;
    m_sOurType.cbFormat = sz;
    m_sOurType.pbFormat = (char*) m_pVhdr;

    memset(&m_sDestType, 0, sizeof(m_sDestType));
    m_sDestType.majortype = MEDIATYPE_Audio;
    m_sDestType.subtype = MEDIASUBTYPE_PCM;
    m_sDestType.formattype = FORMAT_WaveFormatEx;
    m_sDestType.bFixedSizeSamples = true;
    m_sDestType.bTemporalCompression = false;
    m_sDestType.lSampleSize = m_sVhdr2.nBlockAlign;
    if (wf->wFormatTag == 0x130)
	// ACEL hack to prevent memory corruption
        // obviosly we are missing something here
	m_sDestType.lSampleSize *= 288;
    m_sDestType.pUnk = 0;
    m_sDestType.cbFormat = sizeof(m_sVhdr2);
    m_sDestType.pbFormat = (char*) &m_sVhdr2;
}

int DS_AudioDecoder::init()
{
    Setup_FS_Segment();
    m_pDS_Filter = DS_FilterCreate((const char*)m_Info.dll, &m_Info.guid, &m_sOurType, &m_sDestType);
    if (!m_pDS_Filter)
    {
	sprintf(m_Error, "can't open DS_Filter");
        return -1;
    }

    m_pDS_Filter->Start(m_pDS_Filter);

    ALLOCATOR_PROPERTIES props, props1;
    props.cBuffers = 1;
    props.cbBuffer = m_sOurType.lSampleSize;
    props.cbAlign = props.cbPrefix = 0;

    //printf("CBBUFFER %d  %d\n", props.cbBuffer, m_sDestType.lSampleSize);
    if (!m_pDS_Filter->m_pAll)
    {
	sprintf(m_Error, "can't open DS_Filter");
        return -1;
    }
    m_pDS_Filter->m_pAll->vt->SetProperties(m_pDS_Filter->m_pAll, &props, &props1);
    m_pDS_Filter->m_pAll->vt->Commit(m_pDS_Filter->m_pAll);

    return 0;
}

DS_AudioDecoder::~DS_AudioDecoder()
{
    Setup_FS_Segment();
    if (m_pVhdr)
	free(m_pVhdr);
    if (m_pDS_Filter)
	DS_Filter_Destroy(m_pDS_Filter);
}

int DS_AudioDecoder::Convert(const void* in_data, uint_t in_size,
			     void* out_data, uint_t out_size,
			     uint_t* size_read, uint_t* size_written)
{
    char* ptr;
    char* frame_pointer;
    uint_t frame_size = 0;
    uint_t written = 0;
    uint_t read = 0;
    IMediaSample* sample = 0;

    Setup_FS_Segment();
    //printf("INSIZE %d  %d  %d\n", in_size,m_pFormat->nBlockAlign, in_size % m_pFormat->nBlockAlign);
    m_pDS_Filter->m_pOurOutput->SetFramePointer(m_pDS_Filter->m_pOurOutput,
						&frame_pointer);
    m_pDS_Filter->m_pOurOutput->SetFrameSizePointer(m_pDS_Filter->m_pOurOutput,
						    (long*)&frame_size);
    m_pDS_Filter->m_pAll->vt->GetBuffer(m_pDS_Filter->m_pAll, &sample, 0, 0, 0);
    if (sample)
    {
	while (in_size >= m_pFormat->nBlockAlign)
	{
            int r;
	    sample->vt->SetActualDataLength(sample, m_pFormat->nBlockAlign);
	    sample->vt->GetPointer(sample, (BYTE **)&ptr);
	    if (!ptr)
		break;
	    memcpy(ptr, in_data, m_pFormat->nBlockAlign);
	    sample->vt->SetSyncPoint(sample, true);
	    sample->vt->SetPreroll(sample, 0);

	    r = m_pDS_Filter->m_pImp->vt->Receive(m_pDS_Filter->m_pImp, sample);
	    if (r)
		Debug printf("DS_AudioDecoder::Convert() Error: putting data into input pin %x\n", r);

	    if (frame_size > out_size)
		frame_size = out_size;
	    memcpy(out_data, frame_pointer, frame_size);
	    //printf("Frame %p  size: %d    %d\n", frame_pointer, frame_size, m_pFormat->nBlockAlign);
	    read = m_pFormat->nBlockAlign;
	    written = frame_size;
	    break;
	}
	sample->vt->Release((IUnknown*)sample);
    }
    else
    {
	Debug printf("DS_AudioDecoder::Convert() Error: null sample\n");
    }

    //printf("READ %d  %d   %d  %d\n", read, written, in_size, out_size);
    if (size_read)
	*size_read = read;
    if (size_written)
	*size_written = written;
    return (read > 0 || written > 0) ? 0 : -1;
}

uint_t DS_AudioDecoder::GetMinSize() const
{
    // For Voxware - use 80KB large buffers
    // - minimizing ugly clics & pops in the sound
    //if (m_Info.fourcc == 0x75)
    //    return 80000;
    return IAudioDecoder::GetMinSize();
}

#ifdef NOAVIFILE_HEADERS
uint_t DS_AudioDecoder::GetSrcSize(uint_t dest_size) const
{
    double efficiency = (double) m_pFormat.nAvgBytesPerSec
	/ (m_pFormat.nSamplesPerSec*m_pFormat.nBlockAlign);
    int frames = int(dest_size*efficiency);
    if (frames < 1)
	frames = 1;
    return frames * m_pFormat.nBlockAlign;
}
#endif

AVM_END_NAMESPACE;
