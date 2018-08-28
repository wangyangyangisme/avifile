/**
 * DMO audio decoder
 * Copyright 2002 Zdenek Kabelac (kabi@users.sf.net)
 */

#include "DMO_AudioDecoder.h"
#include "ldt_keeper.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

AVM_BEGIN_NAMESPACE;

DMO_AudioDecoder::DMO_AudioDecoder(const CodecInfo& info, const WAVEFORMATEX* wf)
    :IAudioDecoder(info, wf)
{
    m_pDMO_Filter = 0;
    m_pVhdr = 0;
    m_iFlushed = 1;

    int sz = sizeof(WAVEFORMATEX) + wf->cbSize;
    m_pVhdr = (WAVEFORMATEX*) malloc(sz);
    memcpy(m_pVhdr, wf, sz);

    //m_Format.nBlockAlign *= 2;
    //for (int i = 0; i < WFSIZE + wf->cbSize; i++)
    //    printf("%d  %x\n", i, ((unsigned char*)m_pVhdr)[i]);

    m_sVhdr2 = *((WAVEFORMATEX*) m_pVhdr);
    m_sVhdr2.wFormatTag = 1; // PCM
    m_sVhdr2.wBitsPerSample = 16;
    m_sVhdr2.nChannels = (wf->nChannels > 1) ? 2 : 1;
    m_sVhdr2.nBlockAlign = m_sVhdr2.nChannels * ((m_sVhdr2.wBitsPerSample + 7) / 8);
    m_sVhdr2.nAvgBytesPerSec = m_sVhdr2.nBlockAlign * m_sVhdr2.nSamplesPerSec;
    m_sVhdr2.cbSize = 0;

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
    m_sOurType.cbFormat = sz;
    m_sOurType.pbFormat = (char*) m_pVhdr;

    memset(&m_sDestType, 0, sizeof(m_sDestType));
    m_sDestType.majortype = MEDIATYPE_Audio;
    m_sDestType.subtype = MEDIASUBTYPE_PCM;
    m_sDestType.formattype = FORMAT_WaveFormatEx;
    m_sDestType.bFixedSizeSamples = true;
    m_sDestType.bTemporalCompression = false;
    m_sDestType.lSampleSize = m_sVhdr2.nBlockAlign;
    m_sDestType.cbFormat = sizeof(m_sVhdr2);
    m_sDestType.pbFormat = (char*) &m_sVhdr2;
}

int DMO_AudioDecoder::init()
{
    Setup_FS_Segment();
    m_pDMO_Filter = DMO_FilterCreate((const char*)m_Info.dll, &m_Info.guid, &m_sOurType, &m_sDestType);
    if (!m_pDMO_Filter)
    {
	sprintf(m_Error, "can't open DMO_Filter");
        return -1;
    }

    return 0;
}

DMO_AudioDecoder::~DMO_AudioDecoder()
{
    Setup_FS_Segment();
    if (m_pDMO_Filter)
	DMO_Filter_Destroy(m_pDMO_Filter);
    if (m_pVhdr)
	free(m_pVhdr);
}

void DMO_AudioDecoder::Flush()
{
    Setup_FS_Segment();
    m_pDMO_Filter->m_pMedia->vt->Flush(m_pDMO_Filter->m_pMedia);
    m_iFlushed = 1;
}

int DMO_AudioDecoder::Convert(const void* in_data, uint_t in_size,
			     void* out_data, uint_t out_size,
			     uint_t* size_read, uint_t* size_written)
{
    DMO_OUTPUT_DATA_BUFFER db;
    CMediaBuffer* bufferin;
    unsigned long written = 0;
    unsigned long read = 0;
    int r = 0;

    Setup_FS_Segment();
    //m_pDMO_Filter->m_pMedia->vt->Lock(m_pDMO_Filter->m_pMedia, 1);
    bufferin = CMediaBufferCreate(in_size, (void*)in_data, in_size, 1);
    r = m_pDMO_Filter->m_pMedia->vt->ProcessInput(m_pDMO_Filter->m_pMedia, 0,
						  (IMediaBuffer*)bufferin,
						  (m_iFlushed) ? DMO_INPUT_DATA_BUFFERF_SYNCPOINT : 0,
						  0, 0);
    if (r == 0)
    {
	((IMediaBuffer*)bufferin)->vt->GetBufferAndLength((IMediaBuffer*)bufferin, 0, &read);
	m_iFlushed = 0;
    }
    ((IMediaBuffer*)bufferin)->vt->Release((IUnknown*)bufferin);
    //printf("RESULTA: %d 0x%x %ld    %d   %d\n", r, r, read, m_iFlushed, out_size);
    if (r == 0 || (unsigned)r == DMO_E_NOTACCEPTING)
    {
	unsigned long status = 0;
	/* something for process */
	db.rtTimestamp = 0;
	db.rtTimelength = 0;
	db.dwStatus = 0;
	db.pBuffer = (IMediaBuffer*) CMediaBufferCreate(out_size, out_data, 0, 0);
	//printf("OUTSIZE  %d\n", out_size);
	r = m_pDMO_Filter->m_pMedia->vt->ProcessOutput(m_pDMO_Filter->m_pMedia,
						       0, 1, &db, &status);

	((IMediaBuffer*)db.pBuffer)->vt->GetBufferAndLength((IMediaBuffer*)db.pBuffer, 0, &written);
	((IMediaBuffer*)db.pBuffer)->vt->Release((IUnknown*)db.pBuffer);

	//printf("RESULTB: %d 0x%x %ld\n", r, r, written);
	//printf("Converted  %d  -> %d\n", in_size, out_size);
    }
    else if (in_size > 0)
	printf("ProcessInputError  r:0x%x=%d\n", r, r);
    //m_pDMO_Filter->m_pMedia->vt->Lock(m_pDMO_Filter->m_pMedia, 0);
    if (size_read)
	*size_read = read;
    if (size_written)
	*size_written = written;

    return r;
}

uint_t DMO_AudioDecoder::GetMinSize() const
{
    unsigned long inputs, outputs;
    Setup_FS_Segment();
    m_pDMO_Filter->m_pMedia->vt->GetOutputSizeInfo(m_pDMO_Filter->m_pMedia, 0, &inputs, &outputs);
    if (m_pFormat->wFormatTag == 0x162)
	return 500000;
    //printf("MINSIZE %ld\n", inputs);
    return inputs;
    // rewrite time calculation routine to use smalles chunks
}

#ifdef NOAVIFILE_HEADERS
uint_t DMO_AudioDecoder::GetSrcSize(uint_t dest_size) const
{
#if 0
    double efficiency = (double) m_pFormat.nAvgBytesPerSec
	/ (m_pFormat.nSamplesPerSec*m_pFormat.nBlockAlign);
    int frames = int(dest_size*efficiency);
    if (frames < 1)
	frames = 1;
    return frames * m_pFormat.nBlockAlign;
#endif
    printf(" %d -> %d\n", m_pFormat->nBlockAlign, dest_size);
    return m_pFormat->nBlockAlign;
}
#endif

//
// ohh yes - DMO supports decoding to 6 channels & 24 bits
//
int DMO_AudioDecoder::SetOutputFormat(const WAVEFORMATEX* destfmt)
{
    int r;

    Setup_FS_Segment();
    m_sVhdr2.wBitsPerSample = destfmt->wBitsPerSample;
    m_sVhdr2.nChannels = destfmt->nChannels;
    m_sVhdr2.nBlockAlign = m_sVhdr2.nChannels * ((m_sVhdr2.wBitsPerSample + 7) / 8);
    m_sVhdr2.nAvgBytesPerSec = m_sVhdr2.nBlockAlign * m_sVhdr2.nSamplesPerSec;
    r = m_pDMO_Filter->m_pMedia->vt->SetOutputType(m_pDMO_Filter->m_pMedia, 0, &m_sDestType, DMO_SET_TYPEF_TEST_ONLY);
    if (r == 0)
	r = m_pDMO_Filter->m_pMedia->vt->SetOutputType(m_pDMO_Filter->m_pMedia, 0, &m_sDestType, 0);
    return r;
}

AVM_END_NAMESPACE;
