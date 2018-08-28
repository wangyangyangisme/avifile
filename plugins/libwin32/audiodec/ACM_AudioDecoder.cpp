/********************************************************

         Windows ACM-based audio decoder
	 Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)

*********************************************************/

#include "ACM_AudioDecoder.h"
#include "wineacm.h"
#include "avm_output.h"
#include <string.h>
#include <stdio.h>

//#define TIMING
AVM_BEGIN_NAMESPACE;

static int acmdrv = 0;

ACM_AudioDecoder::ACM_AudioDecoder(const CodecInfo& info, const WAVEFORMATEX* pwf)
    :IAudioDecoder(info, pwf), m_iOpened(0), first(true)
{
}

int ACM_AudioDecoder::init()
{
    if (m_pFormat->nSamplesPerSec == 0)
    {
	sprintf(m_Error, "bad argument");
        return -1;
    }

    MSACM_RegisterDriver((const char*)m_Info.dll, m_pFormat->wFormatTag, 0);
    acmdrv++;

    GetOutputFormat(&wf);

    int hr = acmStreamOpen(&srcstream, (HACMDRIVER)NULL,
			   m_pFormat, &wf, NULL, 0, 0,
			   0);
    //ACM_STREAMOPENF_NONREALTIME);
    if (hr != S_OK)
    {
#if 0
	char b[200];
	avm_wave_format(b, sizeof(b), m_pFormat);
	printf("ACM_AudioDecoder: error src %s\n", b);
	avm_wave_format(b, sizeof(b), &wf);
	printf("ACM_AudioDecoder: error dst %s\n", b);
#endif
	if (hr == ACMERR_NOTPOSSIBLE)
	    sprintf(m_Error, "inappropriate audio format");
        else
	    sprintf(m_Error, "acmStreamOpen error %d", (int) hr);
	return -1;
    }

    m_iOpened++;
    acmStreamSize(srcstream, m_pFormat->nBlockAlign, (DWORD*)&m_uiMinSize, ACM_STREAMSIZEF_SOURCE);
    //printf("ACM data block align %d   minimal buffer size: %d\n", m_pFormat->nBlockAlign, m_uiMinSize);
    return 0;
}

ACM_AudioDecoder::~ACM_AudioDecoder()
{
    if (m_iOpened)
	acmStreamClose(srcstream, 0);
    if (--acmdrv == 0)
	MSACM_UnregisterAllDrivers();
}

int ACM_AudioDecoder::Convert(const void* in_data, uint_t in_size,
			      void* out_data, uint_t out_size,
			      uint_t* size_read, uint_t* size_written)
{
    int hr;
    DWORD srcsize = 0;
    //printf("ACM_AudioDecoder: data starts with %X %X %X\n",
    //       *(int*)in_data, *(((int*)in_data)+1),  *(((int*)in_data)+1));

#ifdef TIMING
    printf("ACM_AudioDecoder: received request to convert %d bytes to %d bytes\n",
	    in_size, out_size);
#endif

    for (;;)
    {
	ACMSTREAMHEADER ash;

	acmStreamSize(srcstream, out_size, &srcsize, ACM_STREAMSIZEF_DESTINATION);
	if (srcsize > in_size)
	    srcsize = in_size;

	memset(&ash, 0, sizeof(ash));
	ash.cbStruct = sizeof(ash);
	ash.pbSrc = (LPBYTE) in_data;
	ash.cbSrcLength = srcsize;
	ash.pbDst = (LPBYTE)out_data;
	ash.cbDstLength = out_size;

	hr = acmStreamPrepareHeader(srcstream, &ash, 0);
	if (hr != S_OK)
	{
            in_size = out_size = 0;
	    break;
	}

	int flg = 0;//ACM_STREAMCONVERTF_BLOCKALIGN;
	if (first)
	{
	    // process first block twice - there are some internal
	    // buffers in codecs which might be good to be prefilled
	    // otherwice the first block is larger then expected
            // flags seems to not help here at all
	    ACMSTREAMHEADER ash2 = ash;
	    //flg |= ACM_STREAMCONVERTF_START;
	    hr = acmStreamConvert(srcstream, &ash2, flg);
	}
	hr = acmStreamConvert(srcstream, &ash, flg);
	//hr = acmStreamConvert(srcstream, &ash, 0);
#ifdef TIMING
	printf("ACM_AudioDecoder: srclen: %ld srclenu: %ld  dstlen: %ld  dstlenu: %ld\n", ash.cbSrcLength, ash.cbSrcLengthUsed, ash.cbDstLength, ash.cbDstLengthUsed);
#endif
	if (hr == S_OK)
	{
	    if (ash.cbSrcLengthUsed < in_size)
		in_size = ash.cbSrcLengthUsed;
	    out_size = ash.cbDstLengthUsed;
	    //printf("ACM_AudioDecoder: acmStreamConvert %d  %d\n", out_size, in_size);
	    m_iOpened = 1;
	    acmStreamUnprepareHeader(srcstream, &ash, 0);
	}
	else if (in_size > 0)
	{
	    acmStreamUnprepareHeader(srcstream, &ash, 0);

	    if (++m_iOpened < 3)
	    {
		AVM_WRITE("ACM_AudioDecoder", "acmStreamConvert error, reinitializing...\n");
		acmStreamClose(srcstream, 0);
		acmStreamOpen(&srcstream, (HACMDRIVER)NULL,
			      (WAVEFORMATEX*)m_pFormat,
			      (WAVEFORMATEX*)&wf,
			      NULL, 0, 0,
			      0); //ACM_STREAMOPENF_NONREALTIME );
		first = true;
		continue; // let's try again
	    }
	    out_size = 0;
	}
	break;
    }

    if (first)
    {
	first = false;
	//in_size = 0;
    }

    if (size_read)
	*size_read = in_size;
    if (size_written)
	*size_written = out_size;
#if 0
    unsigned short* xx = (unsigned short*) out_data;
    static unsigned short prev = 0;
    for (unsigned i = 0; i < out_size / 2; i++)
    {
	unsigned short d = xx[i];

	printf("0x%x\n", d - prev);
	prev = d;
	//d += 0x8000;
	//printf("0x%x\n", d);
	xx[i] = d;
    }
#endif
    return (hr == S_OK) ? (int)in_size : -1;
}

uint_t ACM_AudioDecoder::GetMinSize() const
{
    return m_uiMinSize;
}

uint_t ACM_AudioDecoder::GetSrcSize(uint_t dest_size) const
{
    DWORD srcsize = 0;

    acmStreamSize(srcstream, dest_size, &srcsize, ACM_STREAMSIZEF_DESTINATION);
    //printf("ACM %d  %d\n", srcsize, IAudioDecoder::GetSrcSize(dest_size));

    return srcsize;
}

AVM_END_NAMESPACE;
