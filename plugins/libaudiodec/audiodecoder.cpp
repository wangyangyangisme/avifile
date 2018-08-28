#include "audiodecoder.h"
#include "aulaw.h"
#include "adpcm_impl.h"
#include "xa_gsm_state.h"

#include "fillplugins.h"
#include "avm_output.h"
#include "utils.h"
#include "plugin.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(audiodec);

class ADPCM_Decoder : public IAudioDecoder
{
    adpcm_state state;
public:
    ADPCM_Decoder(const CodecInfo& info, const WAVEFORMATEX* wf)
	:IAudioDecoder(info, wf)
    {
	adpcm_init_table();
        Flush();
    }
    uint_t GetMinSize() const
    {
	return m_pFormat->nBlockAlign * m_pFormat->nChannels;
    }
    void Flush()
    {
	state.valprev = 0;
	state.index = 0;
    }
    int Convert(const void* in_data, uint_t in_size,
		void* out_data, uint_t out_size,
		uint_t* size_read, uint_t* size_written)
    {
	uint_t nblocks = in_size / m_pFormat->nBlockAlign;
	int smpl = 2 * m_pFormat->nBlockAlign / m_pFormat->nChannels
	    - 4 * m_pFormat->nChannels; // 4 bytes per channel

	int16_t* out_ptr = (int16_t*) out_data;

	uint_t minosmpl = out_size / (2 * (smpl + 1) * m_pFormat->nChannels);

	if (nblocks > minosmpl)
	    nblocks = minosmpl;
	const int* in_ptr = (const int*) in_data;

	//printf("in: %d    al: %d  ch: %d  ns %d   sampl %d\n",
	//       in_size, m_pFormat.nBlockAlign, m_pFormat.nChannels,  nblocks, smpl);
	for (unsigned i = 0; i < nblocks; i++)
	{
	    for (int c = 0; c < m_pFormat->nChannels; c++)
	    {
		const int32_t* inp = in_ptr + m_pFormat->nChannels + c;
		const uint8_t* hdr = (const uint8_t*) (in_ptr + c);
		state.valprev = (hdr[1] << 8) | hdr[0];
		state.index = hdr[2];
		if (hdr[3] != 0)
		{
		    AVM_WRITE("ADPCM_Decoder", "out of sync()\n");
		    //printf("Data  0x08%x   0x%08x   %d\n", in_ptr[c], state.valprev, state.index);
		}
                else
		    adpcm_decoder(out_ptr + c, inp, smpl, &state, m_pFormat->nChannels);
	    }
	    in_ptr += m_pFormat->nBlockAlign / sizeof(int);
	    out_ptr += (smpl + 0) * m_pFormat->nChannels;
	}

	if (size_read)
	    *size_read = nblocks * m_pFormat->nBlockAlign;
	if (size_written)
	    *size_written = nblocks * 2 * smpl * m_pFormat->nChannels;
	//printf("###  %d  %d\n", *size_read, *size_written);
	return 0;
    }
};

class AULAW_Decoder : public IAudioDecoder
{
    const short* table;
public:
    AULAW_Decoder(const CodecInfo& info, const WAVEFORMATEX* wf)
	:IAudioDecoder(info, wf)
    {
	table = (info.fourcc == 0x06) ? alaw2short : ulaw2short;
    }
    int Convert(const void* in_data, uint_t in_size,
		void* out_data, uint_t out_size,
		uint_t* size_read, uint_t* size_written)
    {
	uint_t csize = (in_size < out_size / 2) ? in_size : out_size / 2;
	const uint8_t* in = (const uint8_t*) in_data;
	short* o = (short*) out_data;
	short* end = o + csize;
	while (o < end)
	    *o++ = table[*in++];

	if (size_read)
	    *size_read = csize;
	if (size_written)
	    *size_written = 2 * csize;

	return 0;
    }
};

class MSGSM_Decoder : public IAudioDecoder
{
public:
    MSGSM_Decoder(const CodecInfo& info, const WAVEFORMATEX* wf)
	:IAudioDecoder(info, wf)
    {
	GSM_Init();
    }
    virtual uint_t GetMinSize() const
    {
	return 640;
    }
    virtual int Convert(const void* in_data, uint_t in_size,
			       void* out_data, uint_t out_size,
			       uint_t* size_read, uint_t* size_written)
    {
	unsigned num_samples=in_size/65;
	if(out_size<640*num_samples)
	    num_samples=out_size/640;
	if(num_samples==0)
	{
	    if(size_read)*size_read=0;
	    if(size_written)*size_written=0;
	    return -1;
	}
	int ocnt=XA_ADecode_GSMM_PCMxM(num_samples*65, num_samples,
				       (char*)in_data, // no const - interface
				       (uint8_t*)out_data, out_size);
	/*    if(format)
	 {
	 format->is_stereo=0;//mono
	 format->freq=local_wf.nSamplesPerSec;//unsure
	 //format->freq=44100;
	 format->valid=OUT_FORMAT_VALID;
	 }		*/
	//    if(ocnt<out_size)
	//    {
	if(size_read)*size_read=num_samples*65;
	if(size_written)*size_written=ocnt;
	return 0;
    }
};

class PCM_Decoder : public IAudioDecoder
{
public:
    PCM_Decoder(const CodecInfo& info, const WAVEFORMATEX* wf)
	:IAudioDecoder(info, wf)
    {
	//char buf[1000]; avm_wave_format(buf, 1000, wf); printf("%s\n", buf);
    }
    virtual int Convert(const void* in_data, uint_t in_size,
			void* out_data, uint_t out_size,
			uint_t* size_read, uint_t* size_written)
    {
	uint_t csize = (in_size < out_size) ? in_size : out_size;

	memcpy(out_data, in_data, csize);

	if (size_read)
	    *size_read = csize;
	if (size_written)
	    *size_written = csize;

	return 0;
    }
};


IAudioDecoder* CreateA52_Decoder(const CodecInfo& info, const WAVEFORMATEX* format);

static IAudioDecoder* audiodec_CreateAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* format)
{
    switch (info.fourcc)
    {
    case 0x01://PCM
	return new PCM_Decoder(info, format);
    case 0x06://ALaw
    case 0x07://uLaw (just different table)
	return new AULAW_Decoder(info, format);
    case 0x11://IMA ADPCM
	return new ADPCM_Decoder(info, format);
    case 0x31://MS GSM 6.10
    case 0x32://MSN Audio
	return new MSGSM_Decoder(info, format);
#ifdef HAVE_LIBA52
    case 0x2000://AC3
	return CreateA52_Decoder(info, format);
#endif
    default:
	audiodec_error_set("format unsupported");
        return 0;
    }
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_audiodec;

avm::codec_plugin_t avm_codec_plugin_audiodec =
{
    PLUGIN_API_VERSION,

    0, // err
    0, 0, 0, 0, 0, 0, // attrs
    avm::audiodec_FillPlugins,
    avm::audiodec_CreateAudioDecoder,
    0,
    0,
    0,
};
